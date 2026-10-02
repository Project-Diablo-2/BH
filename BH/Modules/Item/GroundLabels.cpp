#include "GroundLabels.h"
#include "ItemDisplay.h"
#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../D2Version.h"
#include "../../Patch.h"
#include <cstddef>

// Label pass (vanilla D2Client 0x58FB0; PD2 redirects GameDraw's call at 0xC3D5F to its rewrite in
// ProjectDiablo.dll), per visible ground item:
//   entry = &labels[count]; entry->unit = item; name -> entry->text (BH's item name hook renames it)
//   D2Win #10177 GetTextSize(entry->text, &w, &h)                      <- measure hook (font)
//   box = (x - (w+8)/2, y - h + 4) .. (x1 + w + 8, y); the engine moves it until it overlaps no
//   earlier box (up by its height, then sideways) and keeps it if it found a place
//   entry->bgColor/drawMode/textColor = black/TRANS50/quality colour, or hover blue/NORMAL/white
// and then for every kept entry:
//   D2Win #10013 DrawFramedText(entry->text, entry->x1, entry->y2, bgColor, drawMode, textColor)
//                                                                      <- draw hook (box, frame, font)
// #10013 measures the text again with the current font, draws DrawRectangle(x, bottom - h,
// x + w + 8, bottom, bgColor, drawMode) with bottom = y2 + 2 (both clamped to the screen) and the
// text inside. So a larger font set around both calls makes the engine size, stack and hit-test
// the box for that font.
//
// PD2's D2GL renderer (glide3x.dll, `-3dfx`: Project-Diablo-2/d2gl, a fork of bayaraa/d2gl 1.3.3)
// with "hd_text" on draws text at window resolution in a layer over the upscaled game frame and
// replaces three calls a label could use (d2gl/src/modules/hd_text.cpp):
//   D2Win #10013 -> drawRectangledText: its own box in that layer whatever the colour argument
//     (black 0xCC for mode 0, 0x99 for mode 1, its bordered popup box for mode 2, blue for mode 5)
//     and the text in its label font;
//   D2Gfx #10014 -> drawSolidRect: black (colour 0) becomes a box in that layer with D2GL's alpha
//     per mode (0x66 0x99 0xCC 0xDD 0xFF for modes 0 1 2 3 5); any other colour falls through to
//     the game frame (palette colour and draw mode as without D2GL), under everything in the layer;
//   D2Win #10150 DrawText -> the text alone in that layer.
// So under hd_text a restyled label does not call #10013: box and frame are drawn with #10014,
// the text with #10150.

namespace GroundLabels {
namespace {

struct LabelEntry {
	int x1, y1, x2, y2;
	UnitAny* unit;
	wchar_t text[0x80];
	DWORD bgColor;
	DWORD drawMode;
	DWORD textColor;
};
static_assert(sizeof(LabelEntry) == 0x120, "label entry layout of D2Client 0x58FB0");

const DWORD DRAWMODE_TRANS25 = 0;
const DWORD DRAWMODE_TRANS50 = 1;
const DWORD DRAWMODE_TRANS75 = 2;
const DWORD DRAWMODE_NORMAL = 5;

typedef DWORD(__fastcall* DrawFramedText_t)(const wchar_t* text, int x, int y, DWORD bgColor, DWORD drawMode, DWORD textColor);
typedef DWORD(__fastcall* GetTextSize_t)(const wchar_t* text, DWORD* width, DWORD* height);
typedef void(__stdcall* GetScreenSize_t)(int* width, int* height);

DrawFramedText_t origDrawFramedText = nullptr; // trampolines
GetTextSize_t origGetTextSize = nullptr;
GetScreenSize_t pGetScreenSize = nullptr; // D2Gfx #10080

typedef DWORD(__fastcall* GetTextWidth_t)(const wchar_t* text);
typedef WORD(__fastcall* GetFontHeight_t)();
typedef int(__cdecl* D2GLIsReady_t)();
typedef BOOL(__stdcall* D2GLConfigQuery_t)(D2GLConfigId configId);
GetTextWidth_t pGetTextWidth = nullptr;     // D2Win #10028
GetFontHeight_t pGetFontHeight = nullptr;   // D2Win #10083
D2GLIsReady_t pD2GLIsReady = nullptr;       // D2GL's exports
D2GLConfigQuery_t pD2GLConfigQuery = nullptr;

// D2GL renders this frame and draws its text itself (the option can change in game). PD2 loads
// glide3x.dll under -ddraw too, where it hooks nothing and is never ready but still reports
// hd_text on.
bool HdText() {
	return pD2GLConfigQuery && pD2GLIsReady() && pD2GLConfigQuery(D2GL_CONFIG_HD_TEXT);
}

void (*preLabelCallback)() = nullptr;

// Images that hold a label array: D2Client (vanilla, 0x11FEF8) and ProjectDiablo.dll (PD2).
struct Image {
	DWORD lo, hi;
	LabelEntry* known; // an entry the draw hook has seen, so the array's position is known
};
Image images[2];
int imageCount = 0;

bool AddImage(const char* module) {
	HMODULE h = GetModuleHandleA(module);
	if (!h)
		return false;
	const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)h;
	const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)((const BYTE*)h + dos->e_lfanew);
	images[imageCount++] = { (DWORD)h, (DWORD)h + nt->OptionalHeader.SizeOfImage, nullptr };
	return true;
}

// The entry whose name `text` would be, when that entry lies inside one of the images (so reading
// it is safe); nullptr otherwise.
LabelEntry* EntryAt(const wchar_t* text, Image** image) {
	DWORD e = (DWORD)text - offsetof(LabelEntry, text);
	if (e & 3)
		return nullptr;
	for (int i = 0; i < imageCount; ++i) {
		if (e >= images[i].lo && e + sizeof(LabelEntry) <= images[i].hi) {
			*image = &images[i];
			return (LabelEntry*)e;
		}
	}
	return nullptr;
}

bool IsItem(const LabelEntry* e) {
	return e->unit && e->unit->dwType == UNIT_ITEM;
}

// The item's style, when it changes the label (a rule may style only the beam or the map marker).
bool StyleOf(UnitAny* item, GroundStyle* style) {
	return App.lootfilter.enableFilter.value && GetGroundStyle(item, style) &&
		(style->bgColor != UNDEFINED_COLOR || style->bgOpacity >= 0 || style->frameColor != UNDEFINED_COLOR ||
			style->labelFont >= 0);
}

DWORD ModeForOpacity(int opacity) {
	switch (opacity) {
	case 25: return DRAWMODE_TRANS25;
	case 75: return DRAWMODE_TRANS75;
	case 100: return DRAWMODE_NORMAL;
	default: return DRAWMODE_TRANS50;
	}
}

// Font switch around an engine call; labelFont -1 keeps the engine's font.
struct FontScope {
	bool set;
	DWORD old;
	explicit FontScope(int labelFont) : set(labelFont >= 0), old(0) {
		if (set)
			old = D2WIN_SetTextSize((DWORD)labelFont);
	}
	~FontScope() {
		if (set)
			D2WIN_SetTextSize(old);
	}
};

// Under hd_text a restyled label is always drawn with an explicit font: its %SIZE%, else the
// engine's label font 1.
int HdLabelFont(const GroundStyle& style) {
	return style.labelFont >= 0 ? style.labelFont : 1;
}

// The label size #10177 reports with the current font. D2GL's hd_text gives font 1 its label
// margins (width + 10, 18 rows a line + 2) and every other font the bare text block, which gets
// the same width margin and 2 rows above and below here.
DWORD MeasureLabel(const wchar_t* text, DWORD* width, DWORD* height, bool hdBare) {
	DWORD ret = origGetTextSize(text, width, height);
	if (hdBare) {
		*width += 10;
		ret = *height += 4;
	}
	return ret;
}

// The rectangle D2Win #10013 fills for (text, x, y) with the current font.
void FramedTextBox(const wchar_t* text, int x, int y, bool hdBare, int* left, int* top, int* right, int* bottom) {
	DWORD w = 0, h = 0;
	MeasureLabel(text, &w, &h, hdBare);
	int width = (int)w + 8;
	int screenW = 0, screenH = 0;
	pGetScreenSize(&screenW, &screenH);
	int l = x > 0 ? x : 0;
	if (l >= screenW - width)
		l = screenW - width;
	int b = y + 2;
	if (b <= (int)h)
		b = (int)h;
	if (b >= screenH - 0x1F)
		b = screenH - 0x1F;
	*left = l;
	*top = b - (int)h;
	*right = l + width;
	*bottom = b;
}

void DrawFrame(int left, int top, int right, int bottom, DWORD color) {
	D2GFX_DrawRectangle(left, top, right, top + 1, color, DRAWMODE_NORMAL);
	D2GFX_DrawRectangle(left, bottom - 1, right, bottom, color, DRAWMODE_NORMAL);
	D2GFX_DrawRectangle(left, top + 1, left + 1, bottom - 1, color, DRAWMODE_NORMAL);
	D2GFX_DrawRectangle(right - 1, top + 1, right, bottom - 1, color, DRAWMODE_NORMAL);
}

// A restyled label under D2GL's hd_text, laid out like #10013 with the current font: the box with
// #10014 (a coloured box goes to the game frame, a black one to D2GL's text layer, so under a
// frame it leaves the edge rows to the frame), then the frame, then the text centred with #10150.
void DrawLabelHd(const wchar_t* text, int x, int y, DWORD bgColor, DWORD drawMode, DWORD textColor, int frameColor,
	bool bare) {
	int left, top, right, bottom;
	FramedTextBox(text, x, y, bare, &left, &top, &right, &bottom);
	bool framed = frameColor != UNDEFINED_COLOR;
	int inset = framed && bgColor == 0 ? 1 : 0;
	D2GFX_DrawRectangle(left + inset, top + inset, right - inset, bottom - inset, bgColor, drawMode);
	if (framed)
		DrawFrame(left, top, right, bottom, (DWORD)frameColor);

	// #10150 puts the first line's baseline at y and every further line one line higher. Text
	// block height: the bare measurement, or for font 1 the font height and 18 rows a further line.
	int lines = 1;
	for (const wchar_t* p = text; *p; ++p)
		if (*p == L'\n' && p[1])
			++lines;
	int block = (int)pGetFontHeight() + (lines - 1) * 18;
	if (bare) {
		DWORD w = 0, h = 0;
		origGetTextSize(text, &w, &h);
		block = (int)h;
	}
	int textX = left + (right - left - (int)pGetTextWidth(text)) / 2;
	int textY = bottom - (bottom - top - block) / 2;
	D2WIN_DrawText(text, textX, textY, textColor, 0);
}

bool passPending = true; // a label pass has measured labels that are not drawn yet
LabelEntry* lastDrawn = nullptr;

// The entries measured in the current label pass (each slot once; an item the engine could not place
// leaves its slot to the next item), for LabelBoxOf while the pre-label callback runs.
const int kMaxMeasured = 64;
LabelEntry* measured[kMaxMeasured];
int measuredCount = 0;
bool inPreLabelCallback = false;

DWORD __fastcall DrawFramedTextHook(const wchar_t* text, int x, int y, DWORD bgColor, DWORD drawMode, DWORD textColor) {
	Image* image = nullptr;
	LabelEntry* e = EntryAt(text, &image);
	// A ground label passes its own entry's fields; anything else drawn with #10013 does not.
	if (!e || e->x1 != x || e->y2 != y || e->bgColor != bgColor || e->drawMode != drawMode ||
		e->textColor != textColor || !IsItem(e))
		return origDrawFramedText(text, x, y, bgColor, drawMode, textColor);

	image->known = e;
	if (passPending || !lastDrawn || e <= lastDrawn) {
		passPending = false;
		if (preLabelCallback) {
			inPreLabelCallback = true;
			preLabelCallback();
			inPreLabelCallback = false;
		}
	}
	lastDrawn = e;

	GroundStyle style;
	if (!StyleOf(e->unit, &style))
		return origDrawFramedText(text, x, y, bgColor, drawMode, textColor);

	// The engine marks the hovered label with an opaque highlight box: keep that feedback.
	bool hovered = drawMode == DRAWMODE_NORMAL;
	if (!hovered) {
		if (style.bgColor != UNDEFINED_COLOR)
			bgColor = (DWORD)style.bgColor;
		if (style.bgOpacity >= 0)
			drawMode = ModeForOpacity(style.bgOpacity);
	}
	if (HdText()) {
		int labelFont = HdLabelFont(style);
		FontScope font(labelFont);
		DrawLabelHd(text, x, y, bgColor, drawMode, textColor, style.frameColor, labelFont != 1);
		return 0;
	}
	FontScope font(style.labelFont);
	DWORD ret = origDrawFramedText(text, x, y, bgColor, drawMode, textColor);
	if (style.frameColor != UNDEFINED_COLOR) {
		int left, top, right, bottom;
		FramedTextBox(text, x, y, false, &left, &top, &right, &bottom);
		DrawFrame(left, top, right, bottom, (DWORD)style.frameColor);
	}
	return ret;
}

DWORD __fastcall GetTextSizeHook(const wchar_t* text, DWORD* width, DWORD* height) {
	Image* image = nullptr;
	LabelEntry* e = EntryAt(text, &image);
	// Only entries of an array the draw hook has seen: (e - known) is a whole number of entries.
	if (!e || !image->known)
		return origGetTextSize(text, width, height);
	int delta = (int)((DWORD)e - (DWORD)image->known);
	if (delta % (int)sizeof(LabelEntry) != 0 || delta / (int)sizeof(LabelEntry) < -64 ||
		delta / (int)sizeof(LabelEntry) > 64 || !IsItem(e))
		return origGetTextSize(text, width, height);

	if (!passPending)
		measuredCount = 0;  // a new label pass
	passPending = true;
	int slot = 0;
	while (slot < measuredCount && measured[slot] != e)
		slot++;
	if (slot == measuredCount && measuredCount < kMaxMeasured)
		measured[measuredCount++] = e;
	GroundStyle style;
	if (!StyleOf(e->unit, &style))
		return origGetTextSize(text, width, height);
	DWORD ret;
	if (HdText()) {
		int labelFont = HdLabelFont(style);
		FontScope font(labelFont);
		ret = MeasureLabel(text, width, height, labelFont != 1);
	} else {
		FontScope font(style.labelFont);
		ret = origGetTextSize(text, width, height);
	}
	// The engine's stacked boxes overlap by one row (the box drawn later covers the top row of the
	// one below it). One more reserved row keeps a framed label's top edge visible.
	if (style.frameColor != UNDEFINED_COLOR)
		ret = ++*height;
	return ret;
}

struct Detour {
	int ordinal;
	const char* name;
	BYTE expected[20];
	int expectedLen;
	int prologue;
	void* hook;
	BYTE* entry;
	DetourPlan plan;
	ULONGLONG saved; // the 8 entry bytes before the hook
	bool installed;
};

Detour drawDetour = { 10013, "D2Win #10013 DrawFramedText",
	{ 0x83, 0xEC, 0x0C, 0x53, 0x55, 0x56, 0x8B, 0xF1, 0x57, 0x8B, 0xC6, 0x8B, 0xFA, 0xE8, 0x0E, 0xF1, 0xFF, 0xFF }, 18, 5,
	(void*)DrawFramedTextHook };
Detour sizeDetour = { 10177, "D2Win #10177 GetTextSize",
	{ 0x56, 0x8B, 0xF1, 0x57, 0x8B, 0xC6, 0x8B, 0xFA, 0xE8, 0xA3, 0xF8, 0xFF, 0xFF }, 13, 6,
	(void*)GetTextSizeHook };

BYTE* trampolines = nullptr;

// The jmp is written as one locked 8-byte swap, so a thread running the function sees either the
// old or the new entry, never half of each.
bool Swap(BYTE* at, ULONGLONG from, ULONGLONG to) {
	DWORD old;
	if (!VirtualProtect(at, 8, PAGE_EXECUTE_READWRITE, &old))
		return false;
	bool ok = (ULONGLONG)InterlockedCompareExchange64((volatile LONGLONG*)at, (LONGLONG)to, (LONGLONG)from) == from;
	VirtualProtect(at, 8, old, &old);
	FlushInstructionCache(GetCurrentProcess(), at, 8);
	return ok;
}

bool Prepare(Detour& d, BYTE* trampoline) {
	d.entry = (BYTE*)Patch::GetDllOffset(D2WIN, -d.ordinal);
	if (!d.entry || ((DWORD)d.entry & 7))
		return false;
	if (!PlanDetour(d.entry, (DWORD)d.entry, d.expected, d.expectedLen, d.prologue, (DWORD)trampoline, (DWORD)d.hook, &d.plan))
		return false;
	memcpy(trampoline, d.plan.trampoline, d.plan.trampolineLen);
	FlushInstructionCache(GetCurrentProcess(), trampoline, d.plan.trampolineLen);
	d.saved = *(const ULONGLONG*)d.entry;
	return true;
}

}  // namespace

bool PlanDetour(const BYTE* code, DWORD entry, const BYTE* expected, int expectedLen, int prologue, DWORD trampoline,
	DWORD hook, DetourPlan* plan) {
	int i = 0;
	int stolen = prologue;
	bool detoured = code[0] == 0xE9 && expected[0] != 0xE9;
	if (detoured) {
		i = 5;
		while (i < expectedLen && code[i] == 0xCC && expected[i] != 0xCC)
			++i;
		if (expectedLen - i < 4)
			return false; // too little left to recognise the function
		stolen = i;
	}
	for (; i < expectedLen; ++i)
		if (code[i] != expected[i])
			return false;
	if (stolen < 5 || stolen > 8)
		return false;

	BYTE* t = plan->trampoline;
	if (detoured) {
		DWORD target = entry + 5 + *(const int*)(code + 1);
		t[0] = 0xE9;
		*(int*)(t + 1) = (int)(target - (trampoline + 5));
		plan->trampolineLen = 5;
	} else {
		memcpy(t, code, stolen);
		t[stolen] = 0xE9;
		*(int*)(t + stolen + 1) = (int)((entry + stolen) - (trampoline + stolen + 5));
		plan->trampolineLen = stolen + 5;
	}
	BYTE head[8];
	memcpy(head, code, 8);
	head[0] = 0xE9;
	*(int*)(head + 1) = (int)(hook - (entry + 5));
	for (int k = 5; k < stolen; ++k)
		head[k] = 0xCC;
	memcpy(&plan->hookCode, head, 8);
	plan->stolen = stolen;
	return true;
}

bool Install(const char** why) {
	*why = "";
	if (drawDetour.installed && sizeDetour.installed)
		return true;
	if (D2Version::GetGameVersionID() != VERSION_113c) {
		*why = "game version is not 1.13c";
		return false;
	}
	imageCount = 0;
	if (!AddImage("D2Client.dll")) {
		*why = "D2Client.dll not loaded";
		return false;
	}
	AddImage("ProjectDiablo.dll");
	pGetScreenSize = (GetScreenSize_t)Patch::GetDllOffset(D2GFX, -10080);
	if (!pGetScreenSize) {
		*why = "D2Gfx #10080 missing";
		return false;
	}
	// D2GL (glide3x.dll, or ddraw.dll in its DirectDraw build; looked up as BH::CheckForD2GL does)
	// loads and hooks before BH.
	HMODULE d2gl = GetModuleHandleA("glide3x.dll");
	if (!d2gl)
		d2gl = GetModuleHandleA("ddraw.dll");
	pD2GLIsReady = d2gl ? (D2GLIsReady_t)GetProcAddress(d2gl, "d2glIsReady") : nullptr;
	pD2GLConfigQuery = d2gl ? (D2GLConfigQuery_t)GetProcAddress(d2gl, "_d2glConfigQueryImpl@4") : nullptr;
	pGetTextWidth = (GetTextWidth_t)Patch::GetDllOffset(D2WIN, -10028);
	pGetFontHeight = (GetFontHeight_t)Patch::GetDllOffset(D2WIN, -10083);
	if (!pD2GLIsReady || !pGetTextWidth || !pGetFontHeight)
		pD2GLConfigQuery = nullptr; // no hd_text path: D2GL draws the labels its way
	if (!trampolines)
		trampolines = (BYTE*)VirtualAlloc(NULL, 64, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!trampolines) {
		*why = "no memory for trampolines";
		return false;
	}
	Detour* detours[] = { &sizeDetour, &drawDetour };
	for (int i = 0; i < 2; ++i) {
		if (!Prepare(*detours[i], trampolines + 32 * i)) {
			*why = detours[i]->name;
			return false;
		}
	}
	origGetTextSize = (GetTextSize_t)(trampolines + 0);
	origDrawFramedText = (DrawFramedText_t)(trampolines + 32);
	for (Detour* d : detours) {
		if (!Swap(d->entry, d->saved, d->plan.hookCode)) {
			Uninstall();
			*why = d->name;
			return false;
		}
		d->installed = true;
	}
	return true;
}

void Uninstall() {
	Detour* detours[] = { &drawDetour, &sizeDetour };
	for (Detour* d : detours) {
		// Leave the entry alone if someone hooked it over ours since.
		if (d->installed)
			Swap(d->entry, d->plan.hookCode, d->saved);
		d->installed = false;
	}
}

void SetPreLabelCallback(void (*callback)()) {
	preLabelCallback = callback;
}

bool LabelBoxOf(const UnitAny* item, Box* box) {
	if (!inPreLabelCallback || !item)
		return false;
	for (int i = 0; i < measuredCount; ++i) {
		const LabelEntry* e = measured[i];
		if (e->unit == item) {
			*box = Box{ e->x1, e->y1 - 2, e->x2, e->y2 + 2 };
			return true;
		}
	}
	return false;
}

}  // namespace GroundLabels
