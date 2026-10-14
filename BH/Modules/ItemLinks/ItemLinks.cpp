#include "ItemLinks.h"

#include <map>
#include <string>
#include <vector>

#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../Constants.h"
#include "ChatLinkApi.h"
#include "ItemLinkCodec.h"
#include "ItemLinkGame.h"

using namespace ItemLinkCodec;

// ---------------------------------------------------------------------------------------------------
// How links appear in the game's own chat (1.13c D2Client):
//   * Receive: every chat/system line the client stores goes through PrintGameString (0x7D850),
//     which word-wraps and stores it; its entry is hooked (PrintGameStringHook, the one place a
//     line is rewritten): every "#i<token>" word that validates is replaced, together with an
//     optional "[typed name] " just before it, by a zero-width marker and "[Item Name]" in the
//     item's quality colour. The marker is six colour codes, "ÿc0ÿc0ÿcDÿcDÿcDÿcQ": a fixed lead-in,
//     three digits of the link handle and the name colour; colour codes have no width, so the
//     line wraps and measures exactly as it reads. A token that doesn't validate stays plain text.
//   * Draw: the DrawText calls that draw chat lines (overlay 0x7B8FE/0x7B872, message log
//     0x67B69/0x67BD4) go through ChatDrawTextHook, which records the screen box of every link
//     name drawn this frame (a name cut by the wrap continues, coloured, on the next row).
//   * Click: a left click inside a recorded box opens the tooltip (the engine's own hover popup
//     for a transient copy of the item); Esc, right-click or any other click closes it.
// ---------------------------------------------------------------------------------------------------

namespace {

const int kMaxEntries = 256;     // live link handles
const int kVanillaRefs = 64;     // the game chat keeps references to its most recent links
const int kMaxHandle = 999;      // three digits in the marker
const int kMaxHits = 64;
const int kMaxNameChars = 64;
const int kLineBufChars = 1024;
const int kBoxAscent = 14, kBoxDescent = 2; // chat rows: text box [y-14, y+2] around the baseline

struct Entry {
	int refs;
	uint8_t payload[kMaxPayload];
	size_t len;
	wchar_t name[kMaxNameChars];
	int color;
};
std::map<int, Entry> entries;
int lastHandle = 0;
int vanillaRing[kVanillaRefs];
int vanillaPos = 0;

struct Hit {
	int x0, x1, y0, y1, handle;
};
Hit hitsCur[kMaxHits], hitsLast[kMaxHits];
int nHitsCur = 0, nHitsLast = 0;
int pendingHandle = 0, pendingColor = 0; // a link name wrapped onto the next drawn row

struct Popup {
	bool open;
	int handle;
	UnitAny* item;
	int x, yTop, yBottom;
} popup = {};

bool installed = false;
bool swallowUp = false;
std::vector<Patch*> patches;
const char* offReason = NULL; // why the feature couldn't start (told once, in game)
bool offTold = false;

bool Enabled() {
	return installed && App.itemLinks.enabled.value && ItemLinkGame::Ready();
}

// ---- registry --------------------------------------------------------------------------------------

int FindPayload(const uint8_t* p, size_t n) {
	for (auto& e : entries)
		if (e.second.len == n && memcmp(e.second.payload, p, n) == 0)
			return e.first;
	return 0;
}

int NewHandle() {
	for (int i = 0; i < kMaxHandle; ++i) {
		lastHandle = lastHandle % kMaxHandle + 1;
		if (!entries.count(lastHandle))
			return lastHandle;
	}
	return 0;
}

void AddRef(int h) {
	auto it = entries.find(h);
	if (it != entries.end())
		++it->second.refs;
}

// Validate a token (the base64 text after "#i") and register it. Returns a handle holding one
// reference, or 0 (invalid, the engine disagreed, or the registry is full).
int Register(const wchar_t* b64, size_t len) {
	if (len < kMinTokenChars || len > kMaxTokenChars)
		return 0;
	char text[kMaxTokenChars];
	for (size_t i = 0; i < len; ++i) {
		if (b64[i] > 0x7F)
			return 0;
		text[i] = (char)b64[i];
	}
	ItemLinkCodec::Tables t;
	if (!ItemLinkGame::Tables(&t))
		return 0;
	static Link link; // large; the game thread is the only caller
	if (ParseToken(t, text, len, &link) != OK)
		return 0;
	Canonicalize(&link);
	uint8_t payload[kMaxPayload];
	size_t n = EncodePayload(t, link, payload, sizeof payload);
	if (!n)
		return 0;
	int h = FindPayload(payload, n);
	if (h) {
		AddRef(h);
		return h;
	}
	if ((int)entries.size() >= kMaxEntries || !(h = NewHandle()))
		return 0;
	UnitAny* item = ItemLinkGame::CreateItem(t, link);
	if (!item)
		return 0;
	Entry e;
	e.refs = 1;
	memcpy(e.payload, payload, n);
	e.len = n;
	bool named = ItemLinkGame::ItemName(item, e.name, kMaxNameChars);
	ItemLinkGame::FreeItem(item);
	if (!named)
		return 0;
	e.color = ItemLinkGame::NameColor(link.items[0]);
	entries[h] = e;
	return h;
}

void ClosePopup() {
	if (popup.item)
		ItemLinkGame::FreeItem(popup.item);
	popup = Popup();
}

void Unref(int h) {
	auto it = entries.find(h);
	if (it == entries.end())
		return;
	if (--it->second.refs <= 0) {
		if (popup.open && popup.handle == h)
			ClosePopup();
		entries.erase(it);
	}
}

void HoldForGameChat(int h) {
	int old = vanillaRing[vanillaPos];
	vanillaRing[vanillaPos] = h;
	vanillaPos = (vanillaPos + 1) % kVanillaRefs;
	if (old)
		Unref(old);
}

// ---- markers ---------------------------------------------------------------------------------------

const wchar_t kC = 0xFF;

wchar_t ColorChar(int c) {
	return (wchar_t)(L'0' + (c < 0 ? 0 : c > 12 ? 0 : c));
}

// Append the marker + "[name]" + the colour to resume with.
void AppendLink(std::wstring& out, int h, const Entry& e, wchar_t resume) {
	const wchar_t lead[] = { kC, L'c', L'0', kC, L'c', L'0', 0 };
	out += lead;
	int digits[3] = { h / 100 % 10, h / 10 % 10, h % 10 };
	for (int d : digits) {
		out += kC;
		out += L'c';
		out += (wchar_t)(L'0' + d);
	}
	out += kC;
	out += L'c';
	out += ColorChar(e.color);
	out += L'[';
	out += e.name;
	out += L']';
	out += kC;
	out += L'c';
	out += resume;
}

// A marker at s[i]? Returns the handle, *nameAt = index of '[', *color = the name colour.
int MarkerAt(const wchar_t* s, size_t len, size_t i, size_t* nameAt, int* color) {
	if (i + 19 > len)
		return 0;
	for (int k = 0; k < 6; ++k)
		if (s[i + 3 * k] != kC || s[i + 3 * k + 1] != L'c')
			return 0;
	if (s[i + 2] != L'0' || s[i + 5] != L'0' || s[i + 18] != L'[')
		return 0;
	int h = 0;
	for (int k = 2; k < 5; ++k) {
		wchar_t d = s[i + 3 * k + 2];
		if (d < L'0' || d > L'9')
			return 0;
		h = h * 10 + (d - L'0');
	}
	auto it = entries.find(h);
	if (it == entries.end())
		return 0;
	// The name must be the registered one (a hand-typed marker can't relabel a link).
	size_t nl = wcslen(it->second.name);
	if (i + 19 + nl < len && s[i + 19 + nl] != L']')
		return 0;
	if (wcsncmp(s + i + 19, it->second.name, (std::min)(nl, len - (i + 19))) != 0)
		return 0;
	*nameAt = i + 18;
	*color = it->second.color;
	return h;
}

// The colour in effect just before s[pos] (the line's own colour unless a code changed it).
wchar_t ColorBefore(const wchar_t* s, size_t pos, int lineColor) {
	wchar_t c = ColorChar(lineColor);
	for (size_t i = 0; i + 2 < pos; ++i)
		if (s[i] == kC && s[i + 1] == L'c')
			c = s[i + 2];
	return c;
}

// Where a "[typed name] " right before the token at `tok` starts, else `tok`.
size_t TypedNameStart(const wchar_t* s, size_t tok) {
	if (tok < 3 || s[tok - 1] != L' ' || s[tok - 2] != L']')
		return tok;
	for (size_t i = tok - 2; i-- > 0 && tok - i <= 2 + kMaxNameChars;) {
		if (s[i] == L']')
			return tok;
		if (s[i] == L'[')
			return i;
	}
	return tok;
}

// Rewrite a whole line for the game chat; the game chat holds a reference on each link (its most
// recent kVanillaRefs links stay clickable). Returns false if the line has no valid link.
bool RewriteLine(const wchar_t* in, int lineColor, std::wstring& out) {
	size_t len = wcslen(in);
	size_t a = 0, b = 0, pos = 0, copied = 0;
	bool any = false;
	out.clear();
	while (FindToken(in, len, pos, &a, &b)) {
		int h = Register(in + a + 2, b - a - 2);
		pos = b;
		if (!h)
			continue;
		size_t start = TypedNameStart(in, a);
		out.append(in + copied, start - copied);
		AppendLink(out, h, entries[h], ColorBefore(in, start, lineColor));
		copied = b;
		any = true;
		HoldForGameChat(h);
	}
	if (!any)
		return false;
	out.append(in + copied);
	return true;
}

// ---- hooks -----------------------------------------------------------------------------------------

typedef void(__stdcall* PrintGameString_t)(wchar_t* msg, int color);
PrintGameString_t origPrintGameString; // trampoline: the relocated first instructions + jmp back
ItemLinks::ChatLineObserver lineObserver = NULL; // ChatLinkApi SetLineObserver
bool printHooked = false;

// Every chat/system line the client stores goes through PrintGameString (the game's chat
// formatter, PD2's own chat code and BH's messages all call it), so its entry is hooked.
void __stdcall PrintGameStringHook(wchar_t* msg, int color) {
	if (msg && Enabled()) {
		std::wstring out;
		if (RewriteLine(msg, color, out) && out.size() < kLineBufChars) {
			static wchar_t buf[kLineBufChars];
			wcsncpy_s(buf, out.c_str(), _TRUNCATE);
			if (lineObserver)
				lineObserver(buf, color);
			origPrintGameString(buf, color);
			return;
		}
	}
	if (msg && lineObserver)
		lineObserver(msg, color);
	origPrintGameString(msg, color);
}

// PrintGameString 0x7D850 starts `sub esp,0x14; push ebx; push ebp; push esi; push edi; xor ebp,ebp`:
// its first 5 bytes are whole, position-independent instructions.
const DWORD PRINT_GAME_STRING = 0x7D850;
const BYTE kPrintPrologue[] = { 0x83, 0xEC, 0x14, 0x53, 0x55, 0x56, 0x57, 0x33, 0xED };
BYTE* printTrampoline = NULL;

bool HookPrintGameString() {
	if (printHooked)
		return true;
	BYTE* fn = (BYTE*)Patch::GetDllOffset(D2CLIENT, PRINT_GAME_STRING);
	if (!fn || memcmp(fn, kPrintPrologue, sizeof kPrintPrologue) != 0)
		return false;
	if (!printTrampoline) {
		printTrampoline = (BYTE*)VirtualAlloc(NULL, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
		if (!printTrampoline)
			return false;
		memcpy(printTrampoline, fn, 5);
		printTrampoline[5] = 0xE9;
		*(int*)(printTrampoline + 6) = (int)(fn + 5) - (int)(printTrampoline + 10);
		origPrintGameString = (PrintGameString_t)printTrampoline;
	}
	BYTE jmp[5] = { 0xE9 };
	*(int*)(jmp + 1) = (int)PrintGameStringHook - (int)(fn + 5);
	printHooked = Patch::WriteBytes((int)fn, 5, jmp);
	return printHooked;
}

void UnhookPrintGameString() {
	BYTE* fn = (BYTE*)Patch::GetDllOffset(D2CLIENT, PRINT_GAME_STRING);
	if (fn && fn[0] == 0xE9 && printTrampoline)
		Patch::WriteBytes((int)fn, 5, (BYTE*)kPrintPrologue);
	printHooked = false;
}

void AddHit(int x0, int x1, int y, int h) {
	if (nHitsCur < kMaxHits && x1 > x0)
		hitsCur[nHitsCur++] = { x0, x1, y - kBoxAscent, y + kBoxDescent, h };
}

void __fastcall ChatDrawTextHook(const wchar_t* s, int x, int y, DWORD color, DWORD centered) {
	if (!s || !Enabled() || centered) {
		D2WIN_DrawText(s, x, y, color, centered);
		return;
	}
	size_t len = wcslen(s);
	const wchar_t* draw = s;
	static wchar_t cont[kLineBufChars];
	size_t from = 0;
	if (pendingHandle) { // the rest of a wrapped link name starts this row
		size_t close = 0;
		while (close < len && s[close] != L']')
			++close;
		if (len + 4 < kLineBufChars) {
			cont[0] = kC;
			cont[1] = L'c';
			cont[2] = ColorChar(pendingColor);
			wmemcpy(cont + 3, s, len + 1);
			draw = cont;
		}
		AddHit(x, x + ItemLinkGame::TextWidth(s, (int)(close < len ? close + 1 : len)), y, pendingHandle);
		from = close < len ? close + 1 : len;
		if (close < len)
			pendingHandle = 0;
	}
	for (size_t i = from; i < len; ++i) {
		size_t nameAt = 0;
		int col = 0;
		int h = MarkerAt(s, len, i, &nameAt, &col);
		if (!h)
			continue;
		size_t close = nameAt;
		while (close < len && s[close] != L']')
			++close;
		int x0 = x + ItemLinkGame::TextWidth(s, (int)nameAt);
		int x1 = x + ItemLinkGame::TextWidth(s, (int)(close < len ? close + 1 : len));
		AddHit(x0, x1, y, h);
		if (close >= len) {
			pendingHandle = h;
			pendingColor = col;
		}
		i = close;
	}
	D2WIN_DrawText(draw, x, y, color, centered);
}

struct SiteHook {
	int offset;
	DWORD expectTarget; // RVA the original call goes to
	int fn;
};

bool CallTargets(DWORD site, DWORD target) {
	const BYTE* p = (const BYTE*)site;
	return p[0] == 0xE8 && site + 5 + *(const int*)(p + 1) == target;
}

bool InstallHooks() {
	if (installed)
		return true;
	const SiteHook sites[] = {
		// DrawText (import thunk 0xD37E) calls that draw chat rows: overlay, overlay re-wrapped, log
		{ 0x7B8FE, 0xD37E, (int)ChatDrawTextHook }, { 0x7B872, 0xD37E, (int)ChatDrawTextHook },
		{ 0x67B69, 0xD37E, (int)ChatDrawTextHook }, { 0x67BD4, 0xD37E, (int)ChatDrawTextHook },
	};
	for (const SiteHook& s : sites)
		if (!CallTargets(Patch::GetDllOffset(D2CLIENT, s.offset), Patch::GetDllOffset(D2CLIENT, s.expectTarget)))
			return false; // someone else owns a site: stay off rather than chain blindly
	if (!HookPrintGameString())
		return false;
	if (patches.empty())
		for (const SiteHook& s : sites)
			patches.push_back(new Patch(Call, D2CLIENT, { s.offset, 0 }, s.fn, 5)); // BH keeps every Patch for life
	for (Patch* p : patches)
		p->Install();
	installed = true;
	return true;
}

void RemoveHooks() {
	for (Patch* p : patches)
		p->Remove();
	if (!lineObserver)
		UnhookPrintGameString();
	installed = false;
}

// ---- sender ----------------------------------------------------------------------------------------

void Say(const wchar_t* msg) {
	std::wstring s = L"\xFF" L"c4Item links:\xFF" L"c0 ";
	s += msg;
	static wchar_t buf[256];
	wcsncpy_s(buf, s.c_str(), _TRUNCATE);
	D2CLIENT_PrintGameString(buf, 0);
}

bool LinkHoveredItem() {
	UnitAny* item = *p_D2CLIENT_SelectedInvItem;
	if (!item || item->dwType != UNIT_ITEM || !item->pItemData)
		return false;
	if (!(item->pItemData->dwFlags & ITEM_IDENTIFIED)) {
		Say(L"identify the item first.");
		return true;
	}
	ItemLinkCodec::Tables t;
	uint8_t save[0x400];
	size_t n = ItemLinkGame::Tables(&t) ? ItemLinkGame::SerializeItem(item, save, sizeof save) : 0;
	static Link link;
	Result r = n ? ParseSaveRecords(t, save, n, &link) : ERR_TABLES;
	char token[kMaxTokenChars + 1];
	size_t tn = 0;
	if (r == OK) {
		Canonicalize(&link);
		tn = EncodeToken(t, link, token, sizeof token);
	}
	wchar_t name[kMaxNameChars];
	if (!tn || !ItemLinkGame::ItemName(item, name, kMaxNameChars)) {
		std::string why = r == OK ? "too large" : ResultName(r);
		Say((L"can't link this item (" + std::wstring(why.begin(), why.end()) + L").").c_str());
		return true;
	}
	std::wstring text = L"[";
	text += name;
	text += L"] #i";
	text.append(token, token + tn);
	if (!ItemLinkGame::AppendChatInput(text.c_str()))
		Say(L"the link doesn't fit in the chat box (255 characters).");
	return true;
}

bool KeyDown(int vk) {
	return (GetKeyState(vk) & 0x8000) != 0;
}

bool OpenLink(int h, int x, int yTop, int yBottom) {
	auto it = entries.find(h);
	if (it == entries.end())
		return false;
	ClosePopup();
	ItemLinkCodec::Tables t;
	static Link link;
	if (!ItemLinkGame::Tables(&t) || ParsePayload(t, it->second.payload, it->second.len, &link) != OK)
		return false;
	UnitAny* item = ItemLinkGame::CreateItem(t, link);
	if (!item)
		return false;
	popup.open = true;
	popup.handle = h;
	popup.item = item;
	popup.x = x;
	popup.yTop = yTop;
	popup.yBottom = yBottom;
	AddRef(h); // the popup keeps the entry alive while open
	return true;
}

void ClosePopupAndUnref() {
	if (!popup.open)
		return;
	int h = popup.handle;
	ClosePopup();
	Unref(h);
}

} // namespace

// ---- ChatLinkApi -----------------------------------------------------------------------------------

namespace ItemLinks {

void FormatMessage(const wchar_t* msg, std::vector<ChatRun>& out) {
	out.clear();
	if (!msg)
		return;
	size_t len = wcslen(msg);
	std::wstring plain;
	auto flush = [&]() {
		if (!plain.empty()) {
			out.push_back({ plain, -1, 0 });
			plain.clear();
		}
	};
	for (size_t i = 0; i < len;) {
		size_t nameAt = 0, a = 0, b = 0;
		int col = 0;
		int h = Enabled() ? MarkerAt(msg, len, i, &nameAt, &col) : 0;
		if (h) {
			size_t close = nameAt;
			while (close < len && msg[close] != L']')
				++close;
			flush();
			AddRef(h);
			out.push_back({ L"[" + std::wstring(entries[h].name) + L"]", col, h });
			i = close < len ? close + 1 : len;
			if (i + 2 < len && msg[i] == kC && msg[i + 1] == L'c')
				i += 3; // the colour the game-chat rewrite resumes with
			continue;
		}
		if (Enabled() && msg[i] == L'#' && FindToken(msg, len, i, &a, &b) && a == i) {
			int t = Register(msg + a + 2, b - a - 2);
			if (t) {
				size_t start = TypedNameStart(msg, a);
				if (start < a && plain.size() >= a - start)
					plain.resize(plain.size() - (a - start));
				flush();
				out.push_back({ L"[" + std::wstring(entries[t].name) + L"]", entries[t].color, t });
				i = b;
				continue;
			}
		}
		plain += msg[i++];
	}
	flush();
}

void Release(int link) {
	if (link > 0)
		Unref(link);
}

bool OnClick(int link, int x, int yTop, int yBottom) {
	return Enabled() && OpenLink(link, x, yTop, yBottom);
}

bool SetLineObserver(ChatLineObserver cb) {
	lineObserver = cb;
	if (cb)
		return HookPrintGameString();
	if (!installed)
		UnhookPrintGameString();
	return true;
}

} // namespace ItemLinks

// ---- module ----------------------------------------------------------------------------------------

void ChatItemLinks::OnLoad() {
	LoadConfig();
	if (!ItemLinkGame::Init(&offReason))
		return; // not 1.13c, or unexpected code at a site we call: the feature stays off
	if (!InstallHooks())
		offReason = "a chat call site is already patched by another module";
}

void ChatItemLinks::OnUnload() {
	ClosePopupAndUnref();
	RemoveHooks();
}

void ChatItemLinks::LoadConfig() {}

void ChatItemLinks::OnGameJoin() {
	nHitsCur = nHitsLast = 0;
	pendingHandle = 0;
}

void ChatItemLinks::OnGameExit() {
	ClosePopupAndUnref();
	for (int& h : vanillaRing) {
		if (h)
			Unref(h);
		h = 0;
	}
	nHitsCur = nHitsLast = 0;
	pendingHandle = 0;
}

void ChatItemLinks::OnDraw() {
	if (offReason && *offReason && !offTold && App.itemLinks.enabled.value) {
		offTold = true;
		std::string r(offReason);
		Say((L"off (" + std::wstring(r.begin(), r.end()) + L").").c_str());
	}
	// The chat rows of this frame were drawn before BH's draw hook: publish their link boxes.
	memcpy(hitsLast, hitsCur, sizeof(Hit) * nHitsCur);
	nHitsLast = nHitsCur;
	nHitsCur = 0;
	pendingHandle = 0;
	if (popup.open) {
		if (!Enabled())
			ClosePopupAndUnref();
		else
			ItemLinkGame::DrawTooltip(popup.item, popup.x, popup.yTop, popup.yBottom);
	}
}

void ChatItemLinks::OnLeftClick(bool up, int x, int y, bool* block) {
	if (up) {
		if (swallowUp) {
			swallowUp = false;
			*block = true;
		}
		return;
	}
	if (!Enabled() || !D2CLIENT_GetPlayerUnit())
		return;
	if (KeyDown(VK_CONTROL) && KeyDown(VK_SHIFT) && LinkHoveredItem()) {
		*block = true;
		swallowUp = true;
		return;
	}
	for (int i = 0; i < nHitsLast; ++i) {
		const Hit& h = hitsLast[i];
		if (x >= h.x0 && x < h.x1 && y >= h.y0 && y <= h.y1) {
			bool same = popup.open && popup.handle == h.handle;
			ClosePopupAndUnref();
			if (!same)
				OpenLink(h.handle, (h.x0 + h.x1) / 2, h.y0, h.y1);
			*block = true;
			swallowUp = true;
			return;
		}
	}
	if (*block)
		return; // another chat UI consumed the click (e.g. opened a link through OnClick)
	ClosePopupAndUnref(); // a click anywhere else closes it and goes on to the game
}

void ChatItemLinks::OnRightClick(bool up, int x, int y, bool* block) {
	if (!up)
		ClosePopupAndUnref();
}

void ChatItemLinks::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	if (key == VK_ESCAPE && popup.open) {
		if (!up)
			ClosePopupAndUnref();
		*block = true;
	}
}
