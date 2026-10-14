#include "ItemLinkGame.h"

#include <string.h>
#include <wchar.h>

#include "../../D2Ptrs.h"
#include "../../D2Structs.h"
#include "../../D2DataTables.h"
#include "../../D2Version.h"
#include "../../Constants.h"
#include "../Item/Item.h"

// Addresses are Diablo II 1.13c (D2Client/D2Common/D2Win RVAs and D2Common ordinals). PD2's
// D2Client/D2Common are byte-identical to retail 1.13c; Init() checks the code bytes anyway.

namespace ItemLinkGame {

namespace {

// ---- engine entry points -------------------------------------------------------------------------

typedef DWORD(__stdcall* DecodeItem_t)(UnitAny* item, BYTE* bits, DWORD size, BOOL saveForm, int* socketed, DWORD version, BOOL* fail);
typedef DWORD(__stdcall* SaveItem_t)(UnitAny* item, BYTE* buf, DWORD size, BOOL saveForm, BOOL withChildren, BOOL gamble);
typedef UnitAny*(__stdcall* AllocUnit_t)(void* pool, DWORD type);
typedef void(__fastcall* InitSeed_t)(void* seed);
typedef void(__stdcall* AllocItemData_t)(void* pool, UnitAny* item);
typedef void(__stdcall* UnitOnly_t)(UnitAny* item);
typedef BOOL(__stdcall* PlaceInSocket_t)(Inventory* inv, UnitAny* item, int unused);
typedef UnitAny*(__stdcall* InvRemove_t)(Inventory* inv, UnitAny* item);
typedef void(__stdcall* Unmerge_t)(UnitAny* owner, UnitAny* item);
typedef UnitAny*(__stdcall* InvFirst_t)(Inventory* inv);
typedef UnitAny*(__stdcall* CursorItem_t)(Inventory* inv);
typedef BOOL(__stdcall* GetItemName_t)(UnitAny* item, wchar_t* buf, DWORD size);
typedef void(__fastcall* ClearHover_t)(UnitAny* item);
typedef void(__stdcall* DrawHoverPopup_t)(UnitAny* owner, BOOL reqVsPlayer);
typedef void(__fastcall* SetHoverPopup_t)(const wchar_t* text, int x, int y, DWORD color, BOOL center);
typedef void(__stdcall* RenderHoverPopup_t)(void);
typedef DWORD(__fastcall* TextNWidth_t)(const wchar_t* s, int n);
typedef DWORD(__fastcall* SetFont_t)(DWORD font);

DecodeItem_t pDecodeItem;
SaveItem_t pSaveItem;
AllocUnit_t pAllocUnit;
InitSeed_t pInitSeed;
AllocItemData_t pAllocItemData;
UnitOnly_t pInitItemDataSeed, pAllocStaticPath, pFreeClientUnit;
PlaceInSocket_t pPlaceInSocket;
InvRemove_t pInvRemove;
Unmerge_t pUnmerge;
InvFirst_t pInvFirst;
CursorItem_t pCursorItem;
GetItemName_t pGetItemName;
ClearHover_t pClearHover;
DrawHoverPopup_t pDrawHoverPopup;
SetHoverPopup_t pSetHoverPopup;
RenderHoverPopup_t pRenderHoverPopup;
TextNWidth_t pTextNWidth;
SetFont_t pSetFont;
DWORD aItemInitMode, aAllocUnitGfx, aLinkItemStats; // D2Client usercall helpers

// D2Client globals
UnitAny** gSelectedInvItem;
BOOL *gHoverGrid, *gHoverBody;
int *gAboveX, *gAboveY, *gBelowX, *gBelowY;
DWORD* gShopCursorType;
DWORD* gExpCharFlag;
wchar_t* gChatMsg;
DWORD* gChatMsgLen;
wchar_t* gHoverPopupText; // D2Win, wchar_t[0x400]: non-empty = something staged a popup this frame
DWORD* gHoverPopup;       // D2Win, after the text: the staged x, y, colour, centre flag

bool ready = false;
DWORD nextId = 0;

const DWORD D2CLIENT_ITEM_INIT_MODE = 0x82820;   // eax=mode, esi=item, edi=room; push x, y, bInit; ret 0xC
const DWORD D2CLIENT_ALLOC_UNIT_GFX = 0x66950;   // esi=unit
const DWORD D2CLIENT_LINK_ITEM_STATS = 0x81FF0;  // edi=owner, esi=item; push bForce; ret 4
const DWORD ITEM_VERSION_ARG = 0x60;             // what the client passes to the decoder
const DWORD TRANSIENT_ID_BASE = 0xFFF00000;      // ids the server never hands out
const int MAX_SAVE = 0x400;

struct Site {
	Dll dll;
	int offset; // RVA, or -ordinal
	const char* bytes;
	const char* name;
};

// Code we call; the first bytes of each, from the 1.13c binaries (PD2's are byte-identical).
const Site kSites[] = {
	{ D2COMMON, -11145, "8b 44 24 0c 8b 4c 24 08 83 ec 14 53", "D2Common#11145 DecodeItem" },
	{ D2COMMON, -10987, "8b 44 24 0c 8b 4c 24 08 83 ec 14 50", "D2Common#10987 SaveItem" },
	{ D2COMMON, -11153, "53 8b 5c 24 08 56 57 6a 00 68 6e 06", "D2Common#11153 AllocUnit" },
	{ D2COMMON, -10936, "c7 01 01 00 00 00 c7 41 04 9a 02 00", "D2Common#10936 InitSeed" },
	{ D2COMMON, -10874, "56 8b 74 24 0c 85 f6 74 5b 83 3e 04", "D2Common#10874 AllocItemData" },
	{ D2COMMON, -10679, "8b 44 24 04 85 c0 74 1a 83 38 04 75", "D2Common#10679 InitItemDataSeed" },
	{ D2COMMON, -10445, "56 8b 74 24 08 8b 46 2c 85 c0 75 35", "D2Common#10445 AllocStaticPath" },
	{ D2COMMON, -10405, "56 8b 74 24 08 85 f6 74 08 81 3e 04", "D2Common#10405 PlaceItemInSocket" },
	{ D2COMMON, -10646, "8b 44 24 04 85 c0 75 03 c2 08 00 56", "D2Common#10646 InvRemoveItem" },
	{ D2COMMON, -11127, "8b 44 24 08 85 c0 74 0d 8b 40 5c 85", "D2Common#11127 UnmergeItemStats" },
	{ D2COMMON, -10460, "8b 44 24 04 85 c0 74 08 81 38 04 03", "D2Common#10460 FirstInventoryItem" },
	{ D2CLIENT, 0x82820, "53 8b d8 83 fb 07 55 8b 6c 24 10 0f", "D2Client ItemInitMode" },
	{ D2CLIENT, 0x66950, "e8 3b 20 00 00 85 c0 89 46 54 75 04", "D2Client AllocUnitGfx" },
	{ D2CLIENT, 0x81FF0, "85 ff 0f 84 50 01 00 00 53 6a 14 56", "D2Client LinkItemStats" },
	{ D2CLIENT, 0xA6D60, "55 8b 6c 24 08 56 8b 75 64 57 33 ff", "D2Client FreeClientUnit" },
	{ D2CLIENT, 0x8D370, "a1 38 bc bc 6f 3b c1 75 37 83 c8 ff", "D2Client ClearHoverItem" },
	{ D2CLIENT, 0x93450, "b8 50 66 00 00 e8 06 4a f7 ff 53 55", "D2Client DrawItemHoverPopup" },
	{ D2WIN, -10085, "53 56 8b f1 85 f6 8b da 74 1c ff 15", "D2Win#10085 SetHoverPopup" },
	{ D2WIN, -10110, "83 ec 1c a1 34 fd 8f 6f b9 58 9e 9a", "D2Win#10110 RenderHoverPopup" },
	{ D2WIN, -10171, "51 55 56 57 33 ff 33 ed 66 39 39 89", "D2Win#10171 TextNWidth" },
	{ D2WIN, -10177, "56 8b f1 57 8b c6 8b fa e8 a3 f8 ff", "D2Win#10177 GetTextSize" },
};

int HexByte(const char* p) {
	int v = 0;
	for (int i = 0; i < 2; ++i) {
		char c = p[i];
		v = v * 16 + (c >= 'a' ? c - 'a' + 10 : c - '0');
	}
	return v;
}

// Compare code bytes. A few prologues embed absolute addresses of D2Client/D2Win globals at the
// modules' preferred bases; a relocated module fails the check and the feature stays off.
// A function another module has detoured (`jmp rel32`, int3 padding up to the next whole
// instruction, the rest untouched: D2GL does this to SetHoverPopup and GetTextSize for its HD
// text and tooltips) still matches when everything after the padding does; we only call these,
// so going through the detour is what the game itself does.
bool CodeMatches(DWORD addr, const char* hex) {
	if (!addr)
		return false;
	BYTE want[32];
	int n = 0;
	while (*hex && n < (int)sizeof want) {
		want[n++] = (BYTE)HexByte(hex);
		hex += 2;
		while (*hex == ' ')
			++hex;
	}
	const BYTE* p = (const BYTE*)addr;
	int i = 0;
	if (p[0] == 0xE9 && want[0] != 0xE9) {
		i = 5;
		while (i < n && p[i] == 0xCC && want[i] != 0xCC)
			++i;
		if (n - i < 4)
			return false; // too little left to recognise the function
	}
	for (; i < n; ++i)
		if (p[i] != want[i])
			return false;
	return true;
}

DWORD Addr(Dll dll, int offset) {
	return (DWORD)Patch::GetDllOffset(dll, offset);
}

template <typename T>
T Fn(Dll dll, int offset) {
	return (T)Addr(dll, offset);
}

// Register-convention D2Client helpers. Naked __fastcall stubs: the caller then treats eax/ecx/edx
// as clobbered by the call, which an inline __asm block calling the engine does not guarantee
// (clang-cl kept a live pointer in edx across such a block).
__declspec(naked) void __fastcall ItemInitMode(UnitAny* item) {
	__asm {
		push esi
		push edi
		push 1 // bInit
		push 0 // y
		push 0 // x
		mov esi, ecx
		xor edi, edi // no room
		xor eax, eax // mode 0
		call aItemInitMode
		pop edi
		pop esi
		ret
	}
}

__declspec(naked) void __fastcall AllocUnitGfx(UnitAny* item) {
	__asm {
		push esi
		mov esi, ecx
		call aAllocUnitGfx
		pop esi
		ret
	}
}

__declspec(naked) void __fastcall LinkItemStats(UnitAny* owner, UnitAny* item) {
	__asm {
		push esi
		push edi
		push 0 // bForce
		mov edi, ecx
		mov esi, edx
		call aLinkItemStats
		pop edi
		pop esi
		ret
	}
}

// One record (save form, "JM" first) -> a bare item unit; nothing links it anywhere.
UnitAny* BuildOne(const ItemLinkCodec::Item& it, BYTE* rec, DWORD len, int* socketed) {
	UnitAny* u = pAllocUnit(NULL, UNIT_ITEM);
	if (!u)
		return NULL;
	u->dwTxtFileNo = it.itemIndex;
	u->dwUnitId = TRANSIENT_ID_BASE | (nextId++ & 0xFFFFF);
	if (*gExpCharFlag)
		u->dwFlags2 |= 0x2000000;
	else
		u->dwFlags2 &= ~0x2000000;
	pInitSeed(&u->dwSeed[0]);
	pAllocItemData(NULL, u);
	pInitItemDataSeed(u);
	pAllocStaticPath(u);
	ItemInitMode(u);
	AllocUnitGfx(u);
	BOOL fail = TRUE;
	int nsock = 0;
	DWORD used = pDecodeItem(u, rec, len, TRUE, &nsock, ITEM_VERSION_ARG, &fail);
	*socketed = nsock;
	if (fail || used != len) {
		FreeItem(u);
		return NULL;
	}
	return u;
}

// PD2 draws the inventory hover with its own popup builder, which leaves the "Quantity: N" line
// out for stacks of an equippable item type (ItemTypes Body set: javelins, throwing knives and
// axes); keys and other non-equippable stacks keep it. The link tooltip runs the game's builder,
// so drop that line the same way, then stage the popup where the builder would have put the
// shorter text (above the anchor if it fits, else below).
void DropEquippableQuantity(UnitAny* item, int yTop, int yBottom) {
	sgptDataTable* g = *p_D2COMMON_sgptDataTable;
	int type = D2COMMON_GetItemType(item);
	if (!g || !g->pItemsTypeTxt || type < 0 || (DWORD)type >= g->dwItemsTypeRecs || !g->pItemsTypeTxt[type].nBody)
		return;
	// The builder's line: GetString(3462) GetString(0xF9B) "%ld" GetString(0xF9E), colour codes first.
	wchar_t want[64];
	const wchar_t* label = D2LANG_GetLocaleText(3462);
	const wchar_t* sep = D2LANG_GetLocaleText(0xF9B);
	if (!label || !sep || swprintf_s(want, 64, L"%s%s%ld", label, sep, (long)D2COMMON_GetUnitStat(item, STAT_AMMOQUANTITY, 0)) < 0)
		return;
	size_t wantLen = wcslen(want);
	wchar_t* text = gHoverPopupText;
	for (wchar_t* line = text; *line;) {
		wchar_t* end = wcschr(line, L'\n');
		size_t len = end ? (size_t)(end - line) : wcslen(line);
		const wchar_t* s = line;
		while (s + 3 <= line + len && s[0] == 0xFF && s[1] == L'c')
			s += 3;
		if ((size_t)(line + len - s) == wantLen && !wcsncmp(s, want, wantLen)) {
			wchar_t* from = end ? end + 1 : line + len;
			if (!end && line > text)
				--line; // last line: drop the newline before it instead
			memmove(line, from, (wcslen(from) + 1) * sizeof(wchar_t));
			wchar_t restage[0x400];
			wcscpy_s(restage, 0x400, text);
			DWORD w = 0, h = 0;
			D2WIN_GetTextSize(restage, &w, &h);
			bool above = yTop - (int)h > 0;
			pSetHoverPopup(restage, above ? *gAboveX : *gBelowX, above ? yTop : yBottom + (int)h, gHoverPopup[2], gHoverPopup[3]);
			return;
		}
		if (!end)
			break;
		line = end + 1;
	}
}

} // namespace

bool Init(const char** why) {
	*why = "";
	if (D2Version::GetGameVersionID() != VERSION_113c) {
		*why = "game version is not 1.13c";
		return false;
	}
	for (const Site& s : kSites) {
		if (!CodeMatches(Addr(s.dll, s.offset), s.bytes)) {
			*why = s.name;
			return false;
		}
	}
	pDecodeItem = Fn<DecodeItem_t>(D2COMMON, -11145);
	pSaveItem = Fn<SaveItem_t>(D2COMMON, -10987);
	pAllocUnit = Fn<AllocUnit_t>(D2COMMON, -11153);
	pInitSeed = Fn<InitSeed_t>(D2COMMON, -10936);
	pAllocItemData = Fn<AllocItemData_t>(D2COMMON, -10874);
	pInitItemDataSeed = Fn<UnitOnly_t>(D2COMMON, -10679);
	pAllocStaticPath = Fn<UnitOnly_t>(D2COMMON, -10445);
	pPlaceInSocket = Fn<PlaceInSocket_t>(D2COMMON, -10405);
	pInvRemove = Fn<InvRemove_t>(D2COMMON, -10646);
	pUnmerge = Fn<Unmerge_t>(D2COMMON, -11127);
	pInvFirst = Fn<InvFirst_t>(D2COMMON, -10460);
	pCursorItem = Fn<CursorItem_t>(D2COMMON, -11017);
	pFreeClientUnit = Fn<UnitOnly_t>(D2CLIENT, 0xA6D60);
	pClearHover = Fn<ClearHover_t>(D2CLIENT, 0x8D370);
	pGetItemName = Fn<GetItemName_t>(D2CLIENT, 0x914F0);
	pDrawHoverPopup = Fn<DrawHoverPopup_t>(D2CLIENT, 0x93450);
	pSetHoverPopup = Fn<SetHoverPopup_t>(D2WIN, -10085);
	pRenderHoverPopup = Fn<RenderHoverPopup_t>(D2WIN, -10110);
	pTextNWidth = Fn<TextNWidth_t>(D2WIN, -10171);
	pSetFont = Fn<SetFont_t>(D2WIN, -10184);
	aItemInitMode = Addr(D2CLIENT, D2CLIENT_ITEM_INIT_MODE);
	aAllocUnitGfx = Addr(D2CLIENT, D2CLIENT_ALLOC_UNIT_GFX);
	aLinkItemStats = Addr(D2CLIENT, D2CLIENT_LINK_ITEM_STATS);
	gSelectedInvItem = (UnitAny**)Addr(D2CLIENT, 0x11BC38);
	gHoverGrid = (BOOL*)Addr(D2CLIENT, 0x11BC28);
	gHoverBody = (BOOL*)Addr(D2CLIENT, 0x11BC2C);
	gAboveX = (int*)Addr(D2CLIENT, 0xE0EA8);
	gAboveY = (int*)Addr(D2CLIENT, 0xE0EAC);
	gBelowX = (int*)Addr(D2CLIENT, 0xE0EB0);
	gBelowY = (int*)Addr(D2CLIENT, 0xE0EB4);
	gShopCursorType = (DWORD*)Addr(D2CLIENT, 0x11BC34);
	gExpCharFlag = (DWORD*)Addr(D2CLIENT, 0x119854);
	gChatMsg = (wchar_t*)Addr(D2CLIENT, 0x11EC80);
	gChatMsgLen = (DWORD*)Addr(D2CLIENT, 0x11C028);
	gHoverPopupText = (wchar_t*)Addr(D2WIN, 0xC9E58);
	gHoverPopup = (DWORD*)Addr(D2WIN, 0xCA658);
	nextId = GetTickCount() & 0xFFFFF;
	ready = true;
	return true;
}

bool Ready() {
	return ready;
}

bool Tables(ItemLinkCodec::Tables* t) {
	memset(t, 0, sizeof *t);
	sgptDataTable* g = *p_D2COMMON_sgptDataTable;
	if (!ready || !g)
		return false;
	D2ItemDataTbl* items = D2COMMON_10535_DATATBLS_GetItemDataTables();
	D2MagicAffixDataTbl* magic = D2COMMON_10492_DATATBLS_GetMagicAffixDataTables();
	if (!items || !magic || !items->pItemsTxt || !magic->pMagicAffixTxt || !g->pItemStatCostTxt || !g->pItemsTypeTxt)
		return false;
	// The rare-affix block {count, all, suffixes, prefixes} lives in the data tables between the item
	// and magic-affix blocks; no export returns it, so find it by its shape (suffixes first, both
	// pointers inside the one array of 0x48-byte records).
	const RareAffixDataTbl* rare = NULL;
	for (const BYTE* p = (const BYTE*)items; p + sizeof(RareAffixDataTbl) <= (const BYTE*)magic + 0x200 && !rare; p += 4) {
		const RareAffixDataTbl* r = (const RareAffixDataTbl*)p;
		if (r->nRareAffixTxtRecordCount < 16 || r->nRareAffixTxtRecordCount > 1024 || !r->pRareAffixTxt ||
		    r->pRareSuffix != r->pRareAffixTxt || r->pRarePrefix <= r->pRareSuffix ||
		    r->pRarePrefix >= r->pRareAffixTxt + r->nRareAffixTxtRecordCount)
			continue;
		rare = r;
	}
	if (!rare)
		return false;
	t->isc = g->pItemStatCostTxt;
	t->iscCount = g->dwItemStatCostRecs;
	t->iscStride = sizeof(ItemStatCostTxt);
	t->items = items->pItemsTxt;
	t->itemsCount = items->nItemsTxtRecordCount;
	t->itemsStride = sizeof(ItemsTxt);
	t->weaponsCount = items->nWeaponsTxtRecordCount;
	t->armorCount = items->nArmorTxtRecordCount;
	t->itemTypes = g->pItemsTypeTxt;
	t->itemTypesCount = g->dwItemsTypeRecs;
	t->itemTypesStride = sizeof(D2ItemTypesTxt);
	t->skillsCount = g->dwSkillsRecs;
	t->magicSuffixCount = (unsigned)(magic->pMagicPrefix - magic->pMagicSuffix);
	t->magicPrefixCount = (unsigned)(magic->pAutoMagic - magic->pMagicPrefix);
	t->autoMagicCount = (unsigned)(magic->nMagicAffixTxtRecordCount - (magic->pAutoMagic - magic->pMagicAffixTxt));
	t->rareSuffixCount = (unsigned)(rare->pRarePrefix - rare->pRareSuffix);
	t->rarePrefixCount = (unsigned)(rare->nRareAffixTxtRecordCount - (rare->pRarePrefix - rare->pRareAffixTxt));
	t->uniqueCount = g->dwUniqItemsRecs;
	t->setItemCount = g->dwSetItemsRecs;
	int* runes = D2COMMON_GetRunesTxtRecords();
	t->runesCount = runes ? (unsigned)*runes : 0;
	// 3-bit fields: LowQualityItems has 4 rows, QualityItems 8 (PD2 S13 tables).
	t->lowQualityCount = 4;
	t->qualityItemsCount = 8;
	t->classCount = 7;
	t->monStatsCount = g->dwMonStatsRecs;
	t->monTypeCount = g->dwMonTypeRecs;
	t->statesCount = g->dwStatesTxtRecs;
	return t->iscCount > 0 && t->itemsCount > 0 && t->magicPrefixCount < 0x800 && t->magicSuffixCount < 0x800 &&
	       t->rareSuffixCount > 0 && t->rareSuffixCount < 256 && t->rarePrefixCount < 256;
}

size_t SerializeItem(UnitAny* item, uint8_t* out, size_t cap) {
	if (!ready || !item || item->dwType != UNIT_ITEM || !item->pItemData)
		return 0;
	return pSaveItem(item, out, (DWORD)cap, TRUE, TRUE, FALSE);
}

UnitAny* CreateItem(const ItemLinkCodec::Tables& t, const ItemLinkCodec::Link& link) {
	if (!ready || link.count < 1)
		return NULL;
	BYTE rec[ItemLinkCodec::kMaxPayload + 2];
	int socketed = 0;
	DWORD n = (DWORD)ItemLinkCodec::EncodeRecord(t, link.items[0], true, rec, sizeof rec);
	if (!n)
		return NULL;
	UnitAny* parent = BuildOne(link.items[0], rec, n, &socketed);
	if (!parent)
		return NULL;
	if (socketed != link.count - 1) {
		FreeItem(parent);
		return NULL;
	}
	for (int i = 1; i < link.count; ++i) {
		int s = 0;
		n = (DWORD)ItemLinkCodec::EncodeRecord(t, link.items[i], true, rec, sizeof rec);
		UnitAny* child = n ? BuildOne(link.items[i], rec, n, &s) : NULL;
		if (!child) {
			FreeItem(parent);
			return NULL;
		}
		if (!parent->pInventory || !pPlaceInSocket(parent->pInventory, child, 1)) {
			FreeItem(child);
			FreeItem(parent);
			return NULL;
		}
		LinkItemStats(parent, child);
	}
	return parent;
}

void FreeItem(UnitAny* item) {
	if (!item)
		return;
	// Detach and free the socket fillers first: the engine's own child cleanup looks children up in
	// the client unit table by id, which transient children are never in.
	if (item->pInventory) {
		for (int guard = 0; guard < 16; ++guard) {
			UnitAny* c = pInvFirst(item->pInventory);
			if (!c)
				break;
			if (!(c->pItemData && (c->pItemData->dwFlags & 0x100)))
				pUnmerge(item, c);
			pInvRemove(item->pInventory, c);
			pClearHover(c);
			pFreeClientUnit(c);
		}
	}
	pClearHover(item);
	pFreeClientUnit(item);
}

bool ItemName(UnitAny* item, wchar_t* out, size_t cap) {
	if (!ready || !item || cap < 2)
		return false;
	// Size 191 is the NPC-buy popup's: BH's loot-filter name hook leaves that name as the game made it.
	wchar_t buf[192];
	buf[0] = 0;
	if (!pGetItemName(item, buf, 191))
		return false;
	buf[191] = 0;
	// Multi-line names (unique/set/runeword + base type): the game draws the last line on top.
	wchar_t* line = buf;
	for (wchar_t* p = buf; *p; ++p)
		if (*p == L'\n' && p[1])
			line = p + 1;
	size_t n = 0;
	for (wchar_t* p = line; *p && *p != L'\n' && n + 1 < cap;) {
		if (p[0] == 0xFF && p[1] == L'c' && p[2]) {
			p += 3;
			continue;
		}
		out[n++] = *p++;
	}
	while (n && out[n - 1] == L' ')
		--n;
	out[n] = 0;
	return n > 0;
}

int NameColor(const ItemLinkCodec::Item& it) {
	if (it.flags & 0x04000000) // runeword
		return 4;
	switch (it.quality) {
	case 4: return 3; // magic: blue
	case 5: return 2; // set: green
	case 6: return 9; // rare: yellow
	case 7: return 4; // unique: gold
	case 8: return 8; // crafted: orange
	default: return (it.flags & (0x00400000 | 0x00000800)) ? 5 : 0; // ethereal/socketed grey, else white
	}
}

void DrawTooltip(UnitAny* item, int x, int yTop, int yBottom) {
	if (!ready || !item || !item->pItemData)
		return;
	UnitAny* me = D2CLIENT_GetPlayerUnit();
	if (!me || !me->pInventory)
		return;
	if (gHoverPopupText[0])
		return; // the game drew its own popup this frame (a real hover wins)
	if (pCursorItem(me->pInventory))
		return; // like the game: no item tooltips while holding an item
	UnitAny* view = Item::GetViewUnit();
	Inventory* checked = (view && view->pInventory) ? view->pInventory : me->pInventory;

	UnitAny* sSel = *gSelectedInvItem;
	BOOL sGrid = *gHoverGrid, sBody = *gHoverBody;
	int sAX = *gAboveX, sAY = *gAboveY, sBX = *gBelowX, sBY = *gBelowY;
	DWORD sShop = *gShopCursorType;
	Inventory* sOwn = item->pItemData->pOwnerInventory;

	*gSelectedInvItem = item;
	*gHoverGrid = FALSE; // grid flag off: no vendor price path
	*gHoverBody = TRUE;
	*gAboveX = x;
	*gAboveY = yTop;
	*gBelowX = x;
	*gBelowY = yBottom;
	*gShopCursorType = 0;
	item->pItemData->pOwnerInventory = checked; // the builder's "is this the hovered inventory's item" check

	DWORD oldFont = pSetFont(1); // the inventory panel's font for the popup
	pDrawHoverPopup(me, FALSE);
	if (gHoverPopupText[0]) {
		DropEquippableQuantity(item, yTop, yBottom);
		pRenderHoverPopup();
	}
	pSetHoverPopup(NULL, 0, 0, 0, 0);
	pSetFont(oldFont);

	item->pItemData->pOwnerInventory = sOwn;
	*gShopCursorType = sShop;
	*gAboveX = sAX;
	*gAboveY = sAY;
	*gBelowX = sBX;
	*gBelowY = sBY;
	*gHoverGrid = sGrid;
	*gHoverBody = sBody;
	*gSelectedInvItem = sSel;
}

int TextWidth(const wchar_t* s, int n) {
	// D2Win's TextNWidth skips colour codes, except one that ends the measured span: that one is
	// measured as three glyphs. Colour codes have no width, so leave trailing ones out.
	while (n >= 3 && s[n - 3] == 0xFF && s[n - 2] == L'c')
		n -= 3;
	if (!ready || n <= 0)
		return 0;
	return (int)pTextNWidth(s, n);
}

bool AppendChatInput(const wchar_t* text) {
	if (!ready)
		return false;
	if (!D2CLIENT_GetUIState(UI_CHAT_CONSOLE))
		D2CLIENT_SetUIVar(UI_CHAT_CONSOLE, 0, 0); // opening clears the box, so open first
	DWORD len = *gChatMsgLen;
	if (len > 255)
		return false;
	bool space = len > 0 && gChatMsg[len - 1] != L' ';
	size_t add = wcslen(text) + (space ? 1 : 0);
	if (len + add > 255)
		return false;
	if (space)
		gChatMsg[len++] = L' ';
	wmemcpy(gChatMsg + len, text, wcslen(text));
	len += (DWORD)wcslen(text);
	gChatMsg[len] = 0;
	*gChatMsgLen = len;
	return true;
}

} // namespace ItemLinkGame
