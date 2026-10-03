#include "ChatOverhaul.h"
#include "ChatModel.h"
#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../D2Stubs.h"
#include "../../D2Helpers.h"
#include "../../Constants.h"
#include "../../D2Version.h"
#include "../../Patch.h"
#include "../ItemLinks/ChatLinkApi.h"

#include <string.h>
#include <string>
#include <vector>

using namespace ChatModel;

namespace {

// ---- 1.13c D2Client addresses (module-relative) -------------------------------------------------
const int kPrintPartyString = 0x7D610; // void __stdcall (wchar_t* msg, int color): the party-message area
const int kPacketTable = 0xDDE60;      // S->C handlers: { handler, size, 0 } per opcode, 12 bytes
const int kCallDrawChat = 0x7ECDA, kDrawChatLines = 0x7B730;   // call sites in the per-frame UI draw
const int kCallDrawParty = 0x7ECDF, kDrawPartyLines = 0x7B580;
const int kChatMsg = 0x11EC80;    // wchar_t[257] chat input buffer
const int kChatMsgLen = 0x11C028; // DWORD, chars in the input (append position)
const BYTE kPpsPrologue[] = { 0x81, 0xEC, 0x08, 0x04, 0x00, 0x00 }; // sub esp,408

const int kFont = 13;      // the engine's chat font
const int kRowH = 15;      // the engine's chat row height (text baseline to baseline)
const int kTabH = 18;
const int kMaxInput = 255; // engine cap on the input line

// D2 text colours
const int kWhite = 0, kGold = 4, kGrey = 5, kYellow = 9;
// Palette indices for boxes/lines (D2GFX DrawRectangle/DrawLine) and their transparency modes
const int kBlack = 0, kLineGrey = 0x1D, kTabFill = 0x37, kMentionFill = 0x0A, kMentionBar = 0x6D;
const int kTransLight = 0, kTransHalf = 1, kOpaque = 5;

BYTE* D2C(int off) { return (BYTE*)Patch::GetDllOffset(D2CLIENT, off); }

bool WriteCode(BYTE* at, const BYTE* bytes, size_t n) {
	DWORD old;
	if (!VirtualProtect(at, n, PAGE_EXECUTE_READWRITE, &old)) return false;
	memcpy(at, bytes, n);
	VirtualProtect(at, n, old, &old);
	FlushInstructionCache(GetCurrentProcess(), at, n);
	return true;
}

void Rel32(BYTE op, BYTE* at, const void* to, BYTE* out5) {
	out5[0] = op;
	*(int*)(out5 + 1) = (int)((BYTE*)to - (at + 5));
}

// Entry detour: jmp at the function entry to `fn`; the trampoline runs the stolen prologue (or the
// previous owner's jmp) and continues. Refuses anything but the expected bytes.
struct Detour {
	BYTE* at = nullptr;
	BYTE saved[8] = {};
	size_t len = 0;
	BYTE* tramp = nullptr;
	bool on = false;

	bool Install(int off, const BYTE* expect, size_t expectLen, void* fn) {
		if (on) return true;
		at = D2C(off);
		if (!tramp) tramp = (BYTE*)VirtualAlloc(NULL, 32, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!at || !tramp) return false;
		if (at[0] == 0xE9) { // someone else detoured it: chain through their jmp
			len = 5;
			BYTE* prevTarget = at + 5 + *(int*)(at + 1);
			Rel32(0xE9, tramp, prevTarget, tramp);
		} else if (memcmp(at, expect, expectLen) == 0) {
			len = expectLen;
			memcpy(tramp, at, len);
			Rel32(0xE9, tramp + len, at + len, tramp + len);
		} else {
			return false;
		}
		FlushInstructionCache(GetCurrentProcess(), tramp, 32);
		memcpy(saved, at, len);
		BYTE code[8];
		memset(code, 0x90, sizeof(code));
		Rel32(0xE9, at, fn, code);
		if (!WriteCode(at, code, len)) return false;
		on = true;
		return true;
	}
	void Remove(void* fn) {
		if (!on) return;
		BYTE mine[5];
		Rel32(0xE9, at, fn, mine);
		if (memcmp(at, mine, 5) == 0) WriteCode(at, saved, len); // only if still ours
		on = false;
	}
};

// A verified `call rel32` whose target we replace.
struct CallSite {
	BYTE* at = nullptr;
	BYTE saved[5] = {};
	bool on = false;

	bool Install(int off, int expectTarget, void* fn) {
		if (on) return true;
		at = D2C(off);
		if (!at || at[0] != 0xE8 || at + 5 + *(int*)(at + 1) != D2C(expectTarget)) return false;
		memcpy(saved, at, 5);
		BYTE code[5];
		Rel32(0xE8, at, fn, code);
		if (!WriteCode(at, code, 5)) return false;
		on = true;
		return true;
	}
	void Remove(void* fn) {
		if (!on) return;
		BYTE mine[5];
		Rel32(0xE8, at, fn, mine);
		if (memcmp(at, mine, 5) == 0) WriteCode(at, saved, 5);
		on = false;
	}
};

// One S->C packet handler slot; the previous handler (whoever owns it) is chained.
typedef void(__fastcall* PacketHandler)(BYTE* packet);
struct TableSlot {
	DWORD* slot = nullptr;
	PacketHandler prev = nullptr;
	bool on = false;

	bool Install(BYTE opcode, PacketHandler fn) {
		if (on) return true;
		slot = (DWORD*)(D2C(kPacketTable) + opcode * 12);
		if (!slot || !*slot) return false;
		prev = (PacketHandler)*slot;
		DWORD v = (DWORD)fn;
		if (!WriteCode((BYTE*)slot, (BYTE*)&v, 4)) return false;
		on = true;
		return true;
	}
	void Remove(PacketHandler fn) {
		if (!on) return;
		if (*slot == (DWORD)fn) {
			DWORD v = (DWORD)prev;
			WriteCode((BYTE*)slot, (BYTE*)&v, 4);
		}
		on = false;
	}
};

// ---- state ---------------------------------------------------------------------------------------
struct Line {
	uint64_t id = 0;
	std::wstring text;
	int color = 0;
	unsigned tabs = 0;
	Kind kind = KIND_SYSTEM;
	bool mention = false;
	int hour = 0, minute = 0;
	DWORD tick = 0;
	std::vector<Seg> segs;
	int wrapW = -1;
	std::vector<Row> rows;
};

// The chat packet whose handler is running (set around the engine's 0x26 handler).
struct PacketCtx {
	bool active = false;
	BYTE type = 0, unitType = 0;
	char name[16] = {};
	char msg[256] = {};
};

// A Battle.net whisper reported by the chat server, matched to the line the client prints next.
struct BnetWhisper {
	bool pending = false;
	bool outgoing = false;
	DWORD tick = 0;
	std::wstring name, text;
};

struct Layout {
	int x0, top, w, tabBottom, bodyTop, rows, textX, tsW, wrapW, bottom;
};

CRITICAL_SECTION g_cs;
bool g_csInit = false;
struct Lock {
	Lock() { EnterCriticalSection(&g_cs); }
	~Lock() { LeaveCriticalSection(&g_cs); }
};

Ring<Line>* g_lines = nullptr;
ScrollState g_scroll;
SentHistory g_sent(50);
uint64_t g_nextId = 1;
std::wstring g_lastWhisperFrom, g_lastWhisperTo;
std::vector<std::wstring> g_keywords;
PacketCtx g_ctx;
BnetWhisper g_bnet;
uint64_t g_captured = 0, g_mentions = 0, g_sounds = 0;

bool g_hooksOn = false;
bool g_hookError = false;
Detour g_pps;
CallSite g_drawChat, g_drawParty;
TableSlot g_slot26;
WNDPROC g_prevWndProc = nullptr;
HWND g_hwnd = NULL;
WPARAM g_swallowChar = 0;

// Hit areas of the last drawn panel (game thread only)
bool g_panelShown = false, g_panelOpen = false;
RECT g_panelRect = {}, g_jumpRect = {}, g_tabRect[TAB_COUNT] = {};
int g_lastVisibleRows = 1, g_lastWrapW = 100;

typedef void(__stdcall* PrintFn)(wchar_t*, int);

int TextWidth(const std::wstring& s) {
	DWORD w = 0, file = 0;
	D2WIN_GetTextWidthFileNo((wchar_t*)s.c_str(), &w, &file);
	return (int)w;
}

const std::vector<Row>& RowsOf(Line& l, int wrapW) {
	if (l.wrapW != wrapW) {
		l.rows = Wrap(l.segs, wrapW, TextWidth);
		l.wrapW = wrapW;
	}
	return l.rows;
}

std::wstring Widen(const char* s) {
	wchar_t buf[300];
	int n = MultiByteToWideChar(CP_ACP, 0, s, -1, buf, 300);
	return n > 0 ? std::wstring(buf) : std::wstring();
}

std::wstring MyName() {
	UnitAny* me = D2CLIENT_GetPlayerUnit();
	if (!me || !me->pPlayerData) return L"";
	return Widen(me->pPlayerData->szName);
}

void ReleaseLine(Line& l) {
	for (const Seg& s : l.segs)
		if (s.link) ItemLinks::Release(s.link);
	l.segs.clear();
	l.rows.clear();
}

std::wstring InputText() {
	const wchar_t* buf = (const wchar_t*)D2C(kChatMsg);
	return std::wstring(buf, wcsnlen(buf, kMaxInput));
}

void SetInputText(const std::wstring& s) {
	wchar_t* buf = (wchar_t*)D2C(kChatMsg);
	size_t n = s.size() < (size_t)kMaxInput ? s.size() : (size_t)kMaxInput;
	wmemcpy(buf, s.c_str(), n);
	wmemset(buf + n, 0, kMaxInput + 2 - n); // the engine appends at len without terminating
	*(DWORD*)D2C(kChatMsgLen) = (DWORD)n;
}

void PlayMentionSound() {
	int sound = App.chat.mentionSound.value;
	SoundsTxt* txt = *p_D2CLIENT_SoundsTxt;
	if (sound <= 0 || !App.pd2.pd2PlaySoundImpl || !txt || sound >= (int)*p_D2CLIENT_SoundRecords) return;
	App.pd2.pd2PlaySoundImpl(NULL, sound, txt[sound].volume, 255, FALSE);
	g_sounds++;
}

// Every line the engine prints (and every line BH prints through the engine) arrives here first.
void Capture(const wchar_t* msg, int color) {
	if (!msg || !g_lines) return;
	bool sound = false;
	{
		Lock lock;
		Source src;
		std::wstring body, sender;
		if (g_ctx.active) {
			src.origin = ORIGIN_CHAT_PACKET;
			src.chatType = g_ctx.type;
			src.unitType = g_ctx.unitType;
			sender = Widen(g_ctx.name);
			body = Widen(g_ctx.msg);
		} else if (g_bnet.pending && GetTickCount() - g_bnet.tick < 3000 &&
			StripColorCodes(msg).find(g_bnet.text) != std::wstring::npos) {
			src.origin = g_bnet.outgoing ? ORIGIN_BNET_WHISPER_OUT : ORIGIN_BNET_WHISPER_IN;
			sender = g_bnet.name;
			body = g_bnet.text;
			g_bnet.pending = false;
		}
		std::wstring me = MyName();
		bool senderIsMe = !sender.empty() && _wcsicmp(sender.c_str(), me.c_str()) == 0;
		Line l;
		l.id = g_nextId++;
		l.kind = Classify(src);
		l.tabs = TabsFor(l.kind);
		l.text.assign(msg, wcsnlen(msg, 1024));
		l.color = color & 0xFF;
		l.mention = App.chat.mentionHighlight.value && !senderIsMe &&
			(l.kind == KIND_CHAT || l.kind == KIND_WHISPER_IN) &&
			IsMention(body, me, g_keywords);
		if (l.kind == KIND_WHISPER_IN && !sender.empty()) g_lastWhisperFrom = sender;
		if (l.kind == KIND_WHISPER_OUT && !sender.empty()) g_lastWhisperTo = sender;
		SYSTEMTIME t;
		GetLocalTime(&t);
		l.hour = t.wHour;
		l.minute = t.wMinute;
		l.tick = GetTickCount();
		std::vector<ChatRun> runs; // item links: "[Name]" runs in quality colour, each holds a reference
		ItemLinks::FormatMessage(l.text.c_str(), runs);
		for (const ChatRun& r : runs) {
			Seg s;
			s.text = r.text;
			s.color = r.color;
			s.link = r.link;
			l.segs.push_back(s);
		}

		DWORD oldFont = D2WIN_SetTextSize(kFont);
		int rows = (int)RowsOf(l, g_lastWrapW).size();
		D2WIN_SetTextSize(oldFont);

		unsigned tabs = l.tabs;
		sound = l.mention;
		Line evicted;
		if (g_lines->Push(std::move(l), &evicted)) ReleaseLine(evicted);
		g_scroll.OnLine(tabs, rows);
		g_captured++;
		if (sound) g_mentions++;
	}
	if (sound) PlayMentionSound();
}

// Every PrintGameString line, as the engine stores it (after the item-link rewrite).
void __stdcall ObserveLine(const wchar_t* msg, int color) {
	Capture(msg, color);
}

void __stdcall PrintPartyString_Hook(wchar_t* msg, int color) {
	Capture(msg, color);
	((PrintFn)g_pps.tramp)(msg, color);
}

void CopyStr(char* dst, const char* src, size_t cap) {
	size_t n = 0;
	while (n + 1 < cap && src[n]) {
		dst[n] = src[n];
		n++;
	}
	dst[n] = 0;
}

// S->C 0x26: +1 type, +3 unitType, +10 name\0 msg\0
void __fastcall Packet26_Hook(BYTE* p) {
	PacketCtx saved = g_ctx;
	g_ctx = PacketCtx();
	g_ctx.active = true;
	g_ctx.type = p[1];
	g_ctx.unitType = p[3];
	CopyStr(g_ctx.name, (const char*)p + 10, sizeof(g_ctx.name));
	CopyStr(g_ctx.msg, (const char*)p + 10 + strnlen((const char*)p + 10, 16) + 1, sizeof(g_ctx.msg));
	g_slot26.prev(p);
	g_ctx = saved;
}

// The vanilla overlay is replaced by the panel, drawn at the same point of the frame (before BH's
// overlays, so tooltips and BH panels stay on top). The party-message area's lines are in the panel too.
void DrawChatArea();
void __cdecl DrawChatHook() { DrawChatArea(); }
void __cdecl SkipDraw() {}

void RemoveHooks() {
	g_drawChat.Remove((void*)DrawChatHook);
	g_drawParty.Remove((void*)SkipDraw);
	g_slot26.Remove(Packet26_Hook);
	ItemLinks::SetLineObserver(nullptr);
	g_pps.Remove((void*)PrintPartyString_Hook);
	g_hooksOn = false;
}

bool InstallHooks() {
	if (D2Version::GetGameVersionID() != VERSION_113c) return false;
	bool ok = ItemLinks::SetLineObserver(ObserveLine) &&
		g_pps.Install(kPrintPartyString, kPpsPrologue, sizeof(kPpsPrologue), (void*)PrintPartyString_Hook) &&
		g_slot26.Install(0x26, Packet26_Hook) &&
		g_drawChat.Install(kCallDrawChat, kDrawChatLines, (void*)DrawChatHook) &&
		g_drawParty.Install(kCallDrawParty, kDrawPartyLines, (void*)SkipDraw);
	if (!ok) {
		RemoveHooks();
		return false;
	}
	g_hooksOn = true;
	return true;
}

// Called on the game thread: hooks follow the config switch.
void ApplyEnabled() {
	if (App.chat.enabled.value && !g_hooksOn && !g_hookError) {
		if (!InstallHooks()) g_hookError = true; // engine bytes not as expected: stay vanilla
	} else if (!App.chat.enabled.value && g_hooksOn) {
		RemoveHooks();
	}
}

Layout ComputeLayout() {
	Layout L;
	int W = (int)*p_D2CLIENT_ScreenSizeX, H = (int)*p_D2CLIENT_ScreenSizeY;
	DWORD covered = *p_D2CLIENT_ScreenCovered;
	int left = covered == 2 ? W / 2 : 0;
	int avail = (covered == 1 || covered == 2) ? W / 2 : W;
	L.w = App.chat.width.value;
	if (L.w > avail - 20) L.w = avail - 20;
	L.x0 = left + 10;
	L.top = D2CLIENT_GetUIState(UI_ESCMENU_EX) ? 81 : 6; // the engine's own chat starts at y 20 / 95
	L.tabBottom = L.top + kTabH;
	L.bodyTop = L.tabBottom + 3;
	int maxRows = (H - 150 - L.bodyTop) / kRowH;
	L.rows = App.chat.visibleRows.value < maxRows ? App.chat.visibleRows.value : maxRows;
	if (L.rows < 1) L.rows = 1;
	L.bottom = L.bodyTop + L.rows * kRowH + 4;
	L.textX = L.x0 + 6;
	L.tsW = App.chat.timestamps.value ? TextWidth(L"[00:00] ") : 0;
	L.wrapW = L.w - 12 - L.tsW;
	if (L.wrapW < 40) L.wrapW = 40;
	return L;
}

bool InRect(const RECT& r, int x, int y) { return x >= r.left && x < r.right && y >= r.top && y < r.bottom; }

// Item-link runs drawn this frame (screen boxes) and in the last finished frame (click targets).
struct LinkHit {
	RECT r;
	int link;
};
std::vector<LinkHit> g_hitsCur, g_hitsLast;

void DrawRow(const Line& l, const Row& row, bool first, int x, int y, int tsW) {
	if (tsW && first) D2WIN_DrawText(Timestamp(l.hour, l.minute).c_str(), x, y, kGrey, 0);
	x += tsW;
	for (const Seg& s : row) {
		D2WIN_DrawText(s.text.c_str(), x, y, s.color < 0 ? l.color : s.color, 0);
		int w = TextWidth(s.text);
		if (s.link && g_hitsCur.size() < 256) g_hitsCur.push_back({ { x, y - 14, x + w, y + 2 }, s.link });
		x += w;
	}
}

int TotalRows(int tab, int wrapW) {
	int n = 0;
	for (size_t i = 0; i < g_lines->Size(); i++) {
		Line& l = g_lines->At(i);
		if (l.tabs & TabBit(tab)) n += (int)RowsOf(l, wrapW).size();
	}
	return n;
}

struct VisRow {
	Line* line;
	size_t row;
};

void DrawPanel(const Layout& L) {
	int right = L.x0 + L.w;
	D2GFX_DrawRectangle(L.x0, L.top, right, L.bottom, kBlack, kTransHalf);
	D2GFX_DrawLine(L.x0, L.tabBottom, right, L.tabBottom, kLineGrey, 0xFF);

	// Tabs
	int x = L.x0 + 2;
	for (int t = 0; t < TAB_COUNT; t++) {
		std::wstring label = TabName(t);
		int n = g_scroll.Unread(t);
		if (n > 0) label += L" \xFF" L"c8(" + std::to_wstring(n > 99 ? 99 : n) + L")";
		int w = TextWidth(label) + 10;
		bool active = g_scroll.Active() == t;
		if (active) D2GFX_DrawRectangle(x, L.top + 1, x + w, L.tabBottom, kTabFill, kTransHalf);
		D2WIN_DrawText(label.c_str(), x + 5, L.top + 15, active ? kGold : (t == TAB_GLOBAL ? kGrey : kWhite), 0);
		g_tabRect[t] = { x, L.top, x + w, L.tabBottom };
		x += w + 2;
	}

	g_jumpRect = {};
	int active = g_scroll.Active();
	if (active == TAB_GLOBAL) {
		D2WIN_DrawText(L"Global chat: coming soon", L.textX, L.bodyTop + 14, kGold, 0);
		D2WIN_DrawText(L"Realm-wide chat needs PD2 server support.", L.textX, L.bodyTop + 14 + kRowH, kGrey, 0);
		return;
	}

	// Rows of the active tab, newest first, skipping the scroll offset.
	std::vector<VisRow> vis;
	int skip = g_scroll.Offset(active);
	for (size_t i = 0; i < g_lines->Size() && (int)vis.size() < L.rows; i++) {
		Line& l = g_lines->FromNewest(i);
		if (!(l.tabs & TabBit(active))) continue;
		const std::vector<Row>& rows = RowsOf(l, L.wrapW);
		for (size_t r = rows.size(); r-- > 0 && (int)vis.size() < L.rows;) {
			if (skip > 0) {
				skip--;
				continue;
			}
			vis.push_back({ &l, r });
		}
	}
	if (vis.empty()) {
		D2WIN_DrawText(L"No messages yet.", L.textX, L.bodyTop + 14, kGrey, 0);
		return;
	}
	for (size_t i = 0; i < vis.size(); i++) {
		int y = L.bodyTop + 14 + (L.rows - 1 - (int)i) * kRowH;
		Line& l = *vis[i].line;
		if (l.mention) {
			D2GFX_DrawRectangle(L.x0 + 2, y - 13, right - 2, y + 2, kMentionFill, kTransLight);
			D2GFX_DrawRectangle(L.x0 + 2, y - 13, L.x0 + 4, y + 2, kMentionBar, kOpaque);
		}
		DrawRow(l, l.rows[vis[i].row], vis[i].row == 0, L.textX, y, L.tsW);
	}

	if (g_scroll.Offset(active) > 0) {
		int nb = g_scroll.NewBelow(active);
		std::wstring label = nb > 0 ? L"Jump to latest (" + std::to_wstring(nb) + L" new)" : L"Jump to latest";
		int w = TextWidth(label) + 10;
		int y = L.bottom - 4;
		g_jumpRect = { right - w - 4, y - 14, right - 4, y + 2 };
		D2GFX_DrawRectangle(g_jumpRect.left, g_jumpRect.top, g_jumpRect.right, g_jumpRect.bottom, kBlack, kOpaque);
		D2WIN_DrawText(label.c_str(), g_jumpRect.left + 5, y, kYellow, 0);
	}
}

// Closed panel: the recent lines of the active tab, like the engine's own fading chat.
void DrawFaded(const Layout& L) {
	int active = g_scroll.Active();
	if (active == TAB_GLOBAL || App.chat.fadeRows.value <= 0) return;
	DWORD now = GetTickCount(), life = (DWORD)App.chat.fadeSeconds.value * 1000;
	std::vector<VisRow> vis;
	for (size_t i = 0; i < g_lines->Size() && (int)vis.size() < App.chat.fadeRows.value; i++) {
		Line& l = g_lines->FromNewest(i);
		if (now - l.tick > life) break;
		if (!(l.tabs & TabBit(active))) continue;
		const std::vector<Row>& rows = RowsOf(l, L.wrapW);
		for (size_t r = rows.size(); r-- > 0 && (int)vis.size() < App.chat.fadeRows.value;) vis.push_back({ &l, r });
	}
	int y = L.top + 14;
	for (size_t i = vis.size(); i-- > 0; y += kRowH) {
		Line& l = *vis[i].line;
		const Row& row = l.rows[vis[i].row];
		int w = L.tsW;
		for (const Seg& s : row) w += TextWidth(s.text);
		D2GFX_DrawRectangle(L.textX - 4, y - 14, L.textX + w + 4, y + 2, l.mention ? kMentionFill : kBlack, kTransHalf);
		DrawRow(l, row, vis[i].row == 0, L.textX, y, L.tsW);
	}
}

void ScrollBy(int delta) {
	int active = g_scroll.Active();
	if (active == TAB_GLOBAL) return;
	DWORD oldFont = D2WIN_SetTextSize(kFont);
	g_scroll.Scroll(delta, TotalRows(active, g_lastWrapW), g_lastVisibleRows);
	D2WIN_SetTextSize(oldFont);
}

void Reply() {
	std::wstring target = !g_lastWhisperFrom.empty() ? g_lastWhisperFrom : g_lastWhisperTo;
	if (target.empty()) return;
	if (!D2CLIENT_GetUIState(UI_CHAT_CONSOLE)) D2CLIENT_SetUIVar(UI_CHAT_CONSOLE, 0, 0); // opening clears the input
	std::wstring rest = InputText();
	if (rest.compare(0, 3, L"/w ") == 0 || rest.compare(0, 3, L"/r ") == 0) { // replace an earlier target
		size_t sp = rest.find(L' ', 3);
		rest = sp == std::wstring::npos ? L"" : rest.substr(sp + 1);
	}
	SetInputText(L"/w " + target + L" " + rest);
}

// Window messages the panel owns. Returns true when consumed.
bool HandleMessage(UINT m, WPARAM wp) {
	bool chatOpen = D2CLIENT_GetUIState(UI_CHAT_CONSOLE) != 0;
	if (m == WM_CHAR) {
		WPARAM vk = g_swallowChar; // the key we handled: drop the character it produces
		g_swallowChar = 0;
		if (vk && (wp == vk || (vk >= 'A' && vk <= 'Z' && (wp == vk - 'A' + 1 || wp == vk + 32)))) return true;
		if (chatOpen && wp == VK_RETURN) {
			Lock lock;
			g_sent.Add(InputText());
		} else if (chatOpen && wp == VK_ESCAPE) {
			Lock lock;
			g_sent.ResetBrowse();
		}
		return false;
	}
	if (m == WM_KEYDOWN) {
		bool ctrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
		bool shift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
		if (ctrl && App.chat.replyHotkey.hotkey && wp == App.chat.replyHotkey.hotkey) {
			Lock lock;
			Reply();
			g_swallowChar = wp;
			return true;
		}
		if (!chatOpen) return false;
		Lock lock;
		std::wstring out;
		switch (wp) {
		case VK_UP:
			if (g_sent.Up(InputText(), &out)) SetInputText(out);
			return true;
		case VK_DOWN:
			if (g_sent.Down(&out)) SetInputText(out);
			return true;
		case VK_PRIOR:
			ScrollBy(g_lastVisibleRows - 1);
			return true;
		case VK_NEXT:
			ScrollBy(-(g_lastVisibleRows - 1));
			return true;
		case VK_HOME:
			ScrollBy(1 << 20);
			return true;
		case VK_END:
			g_scroll.JumpToLatest();
			return true;
		case VK_TAB:
			if (!ctrl) return false;
			g_scroll.Select((g_scroll.Active() + (shift ? TAB_COUNT - 1 : 1)) % TAB_COUNT);
			g_swallowChar = '\t';
			return true;
		}
		return false;
	}
	if (m == WM_MOUSEWHEEL) {
		int x = (int)*p_D2CLIENT_MouseX, y = (int)*p_D2CLIENT_MouseY;
		if (!g_panelShown || !g_panelOpen || !InRect(g_panelRect, x, y)) return false;
		int notches = GET_WHEEL_DELTA_WPARAM(wp) / WHEEL_DELTA;
		if (notches == 0) notches = GET_WHEEL_DELTA_WPARAM(wp) > 0 ? 1 : -1;
		Lock lock;
		ScrollBy(notches * 3);
		return true;
	}
	return false;
}

void Say(const std::wstring& s) { D2CLIENT_PrintGameString((wchar_t*)s.c_str(), kWhite); }

// Panel draw cost since the last ".chat status" (QueryPerformanceCounter ticks).
LONGLONG g_qpf = 0, g_drawTicks = 0, g_drawMax = 0;
unsigned g_draws = 0;

// .chat on | off | ts | clear | status | tab <name>
void RunCommand(const std::wstring& arg) {
	const std::wstring tag = L"\xFF" L"c4Chat:\xFF" L"c0 ";
	if (arg == L"on" || arg == L"off") {
		App.chat.enabled.value = arg == L"on";
		g_hookError = false;
		ApplyEnabled();
		Say(tag + (App.chat.enabled.value ? (g_hooksOn ? L"panel on" : L"\xFF" L"c1could not hook the engine, vanilla chat kept") : L"vanilla chat"));
		return;
	}
	if (arg == L"ts") {
		App.chat.timestamps.value = !App.chat.timestamps.value;
		Say(tag + L"timestamps " + (App.chat.timestamps.value ? L"on" : L"off"));
		return;
	}
	if (arg == L"clear") {
		Lock lock;
		g_lines->Clear(ReleaseLine);
		g_scroll.Reset();
		return;
	}
	if (arg.compare(0, 4, L"tab ") == 0) {
		Lock lock;
		for (int t = 0; t < TAB_COUNT; t++)
			if (_wcsicmp(arg.c_str() + 4, TabName(t)) == 0) g_scroll.Select(t);
		return;
	}
	if (arg == L"status") {
		wchar_t b[300];
		{
			Lock lock;
			double us = g_qpf ? 1e6 / (double)g_qpf : 0;
			swprintf_s(b, L"%lshooks %ls, mentions %llu, sounds %llu, lines %u/%u, captured %llu, draw avg %.0fus max %.0fus over %u frames",
				tag.c_str(), g_hooksOn ? L"on" : (g_hookError ? L"FAILED" : L"off"),
				(unsigned long long)g_mentions, (unsigned long long)g_sounds, (unsigned)g_lines->Size(), (unsigned)g_lines->Capacity(),
				(unsigned long long)g_captured, g_draws ? (double)g_drawTicks * us / g_draws : 0.0, (double)g_drawMax * us, g_draws);
			g_drawTicks = g_drawMax = 0;
			g_draws = 0;
		}
		Say(b);
		return;
	}
	Say(tag + L".chat on | off | ts | clear | status | tab <All|Game|Whispers|System|Global>");
}

// ".chat ..." typed in the chat box is handled here on Enter, so it works whoever owns the engine's
// input path (in PD2 BH's own dot-commands never reach BH).
bool TryChatCommand() {
	std::wstring in = InputText();
	size_t b = in.find_first_not_of(L' ');
	if (b == std::wstring::npos) return false;
	std::wstring cmd = in.substr(b);
	if (_wcsnicmp(cmd.c_str(), L".chat", 5) != 0 || (cmd.size() > 5 && cmd[5] != L' ')) return false;
	size_t a = cmd.find_first_not_of(L' ', 5);
	std::wstring arg = a == std::wstring::npos ? L"" : cmd.substr(a);
	while (!arg.empty() && arg.back() == L' ') arg.pop_back();
	{
		Lock lock;
		g_sent.Add(in);
	}
	SetInputText(L"");
	D2CLIENT_SetUIVar(UI_CHAT_CONSOLE, 1, 0);
	RunCommand(arg);
	return true;
}

LRESULT CALLBACK ChatWndProc(HWND h, UINT m, WPARAM wp, LPARAM lp) {
	if (D2CLIENT_GetPlayerUnit()) {
		if (m == WM_CHAR && wp == VK_RETURN && D2CLIENT_GetUIState(UI_CHAT_CONSOLE) && TryChatCommand()) return 0;
		if (g_hooksOn && HandleMessage(m, wp)) return 0;
	}
	return CallWindowProcA(g_prevWndProc, h, m, wp, lp);
}

} // namespace

void ChatOverhaul::OnLoad() {
	if (!g_csInit) {
		InitializeCriticalSection(&g_cs);
		g_csInit = true;
	}
	LoadConfig();
}

void ChatOverhaul::OnUnload() {
	if (g_hooksOn) RemoveHooks();
	if (g_prevWndProc && g_hwnd && (WNDPROC)GetWindowLong(g_hwnd, GWL_WNDPROC) == ChatWndProc)
		SetWindowLong(g_hwnd, GWL_WNDPROC, (LONG)g_prevWndProc);
}

void ChatOverhaul::LoadConfig() {
	Lock lock;
	g_keywords.clear();
	for (const std::string& k : App.chat.mentionKeywords.values) g_keywords.push_back(Widen(k.c_str()));
	size_t cap = (size_t)App.chat.historyLines.value;
	if (!g_lines || g_lines->Capacity() != cap) {
		Ring<Line>* fresh = new Ring<Line>(cap);
		if (g_lines) {
			size_t n = g_lines->Size(), keep = n < cap ? n : cap;
			for (size_t i = 0; i < n; i++) {
				if (i < n - keep) ReleaseLine(g_lines->At(i));
				else fresh->Push(std::move(g_lines->At(i)));
			}
			delete g_lines;
		}
		g_lines = fresh;
	}
	g_hookError = false; // a reload retries a failed install
}

void ChatOverhaul::OnLoop() {
	HWND h = D2GFX_GetHwnd();
	if (h && !g_prevWndProc) {
		g_hwnd = h;
		g_prevWndProc = (WNDPROC)SetWindowLong(h, GWL_WNDPROC, (LONG)ChatWndProc);
	}
	ApplyEnabled();
}

void ChatOverhaul::OnGameJoin() {}

void ChatOverhaul::OnGameExit() {
	Lock lock;
	g_scroll.Reset();
	g_sent.ResetBrowse();
}

namespace {
bool g_drewThisFrame = false;

// Game thread, from the engine's UI draw (where DrawChatLines ran; skipped while the message log is open).
void DrawChatArea() {
	if (!g_lines) return;
	bool chatOpen = D2CLIENT_GetUIState(UI_CHAT_CONSOLE) != 0;
	Lock lock;
	LARGE_INTEGER t0, t1;
	QueryPerformanceCounter(&t0);
	if (!chatOpen) g_sent.ResetBrowse();
	DWORD oldFont = D2WIN_SetTextSize(kFont);
	Layout L = ComputeLayout();
	g_lastWrapW = L.wrapW;
	g_lastVisibleRows = L.rows;
	g_panelOpen = chatOpen || App.chat.alwaysOpen.value;
	if (g_panelOpen) {
		g_scroll.Clamp(g_scroll.Active(), TotalRows(g_scroll.Active(), L.wrapW), L.rows);
		DrawPanel(L);
		g_panelRect = { L.x0, L.top, L.x0 + L.w, L.bottom };
	} else {
		DrawFaded(L);
		g_panelRect = {};
	}
	g_drewThisFrame = true;
	D2WIN_SetTextSize(oldFont);
	QueryPerformanceCounter(&t1);
	if (!g_qpf) {
		LARGE_INTEGER f;
		QueryPerformanceFrequency(&f);
		g_qpf = f.QuadPart;
	}
	LONGLONG dt = t1.QuadPart - t0.QuadPart;
	g_drawTicks += dt;
	if (dt > g_drawMax) g_drawMax = dt;
	g_draws++;
}

} // namespace

void ChatOverhaul::OnDraw() {
	ApplyEnabled();
	// BH draws after the engine's UI: publish what the panel drew this frame for clicks/wheel.
	g_panelShown = g_hooksOn && g_drewThisFrame;
	g_drewThisFrame = false;
	g_hitsLast.swap(g_hitsCur);
	g_hitsCur.clear();
}

void ChatOverhaul::OnLeftClick(bool up, int x, int y, bool* block) {
	if (!g_hooksOn || !g_panelShown) return;
	for (const LinkHit& h : g_hitsLast) // item links, open panel or recent lines
		if (InRect(h.r, x, y)) {
			*block = true;
			if (!up) ItemLinks::OnClick(h.link, (h.r.left + h.r.right) / 2, h.r.top, h.r.bottom);
			return;
		}
	if (!g_panelOpen || !InRect(g_panelRect, x, y)) return;
	*block = true;
	if (up) return;
	Lock lock;
	if (InRect(g_jumpRect, x, y)) {
		g_scroll.JumpToLatest();
		return;
	}
	for (int t = 0; t < TAB_COUNT; t++)
		if (InRect(g_tabRect[t], x, y)) g_scroll.Select(t);
}

// Battle.net SID_CHATEVENT (0x0F): event 4 = whisper from, 0x0A = whisper to; name at 28, text after.
void ChatOverhaul::OnChatPacketRecv(BYTE* packet, bool* block) {
	if (!g_hooksOn || packet[1] != 0x0F) return;
	DWORD ev = *(DWORD*)&packet[4];
	if (ev != 4 && ev != 0x0A) return;
	const char* name = (const char*)&packet[28];
	const char* text = name + strlen(name) + 1;
	Lock lock;
	g_bnet.pending = true;
	g_bnet.outgoing = ev == 0x0A;
	g_bnet.tick = GetTickCount();
	g_bnet.name = Widen(name);
	g_bnet.text = Widen(text);
	if (!g_bnet.outgoing) g_lastWhisperFrom = g_bnet.name;
	else g_lastWhisperTo = g_bnet.name;
}

void ChatOverhaul::OnUserInput(const wchar_t* msg, bool fromGame, bool* block) {
	*block = true;
	RunCommand(msg ? msg : L"");
}
