// A small model of the game functions BH's logic calls, with state the tests set up.
// EnginePtrs.cpp routes the matching D2Ptrs.h entries here. Everything is reset before each
// test case, so tests never see each other's units, strings or settings.
#pragma once
#include <Windows.h>

#include <string>
#include <utility>
#include <vector>

#include "D2Ptrs.h"

namespace fake {

void Reset();

// ---- Units -------------------------------------------------------------------------------

// An item unit. `unit.pItemData` points at `data`. Lives until the next Reset().
struct ItemUnit {
	UnitAny unit;
	ItemData data;
};

// A new item: dwType UNIT_ITEM, dwTxtFileNo = txtFileNo (see AddItemTxt), the given quality,
// identified, item level 1.
ItemUnit& NewItem(DWORD txtFileNo, DWORD quality);

// The player unit D2CLIENT_GetPlayerUnit returns (and *p_D2CLIENT_PlayerUnit points at).
// Class is dwTxtFileNo (0 Amazon .. 6 Assassin, charstats.txt order); default Amazon.
UnitAny& Player();

// A unit stat as D2COMMON_GetUnitStat reports it, and as the unit's stat list
// (D2COMMON_GetStatList + CopyStatList / GetStatValueFromStatList) contains it.
void SetStat(UnitAny* unit, int stat, int value, int param = 0);

// A stat on the unit's runeword state list (D2COMMON_GetStateStatList(unit, STATE_RUNEWORD)).
void SetRunewordStat(UnitAny* unit, int stat, int value, int param = 0);

// ---- Data tables -------------------------------------------------------------------------

// Registers a Weapons/Armor/Misc.txt record for D2COMMON_GetItemText(txtFileNo) and
// D2COMMON_GetItemTextFromItemCode. `code` is the 3-4 letter item code. Everything else is zero.
ItemsTxt& AddItemTxt(DWORD txtFileNo, const char* code);

// D2LANG_GetLocaleText: the string table. Unknown ids return an empty string.
void SetLocaleText(WORD id, const std::wstring& text);

// ---- Game state --------------------------------------------------------------------------

void SetDifficulty(BYTE difficulty);  // 0 normal, 1 nightmare, 2 hell
void SetAreaId(int levelId);          // level the player is in (Levels.txt id)
// D2COMMON_GetItemLevelRequirement: the item's level requirement for a character of class
// `classId` (charstats.txt order), or for every class when classId is -1. A class-specific value
// wins over the every-class one (charged skills of another class raise the requirement).
void SetLevelRequirement(UnitAny* item, DWORD level, int classId = -1);
void SetMaxSockets(UnitAny* item, BYTE sockets);       // D2COMMON_GetMaxSockets
// D2COMMON_GetItemPrice for a transaction type (TRANSACTIONTYPE_BUY/SELL/...), any vendor.
void SetPrice(UnitAny* item, int transactionType, DWORD price);
void SetInteractingNpc(UnitAny* npc);                  // D2CLIENT_GetCurrentInteractingNPC
void SetUIVar(DWORD varno, DWORD value);               // D2CLIENT_GetUIState

// Lines printed to the in-game chat with D2CLIENT_PrintGameString (text, colour).
const std::vector<std::pair<std::wstring, int>>& Printed();

// A game variable (VARPTR in D2Ptrs.h), e.g. Var(Var_D2CLIENT_ScreenSizeX) = 800.
// Every variable starts zeroed in each test.
template <class T>
T& Var(T** (*accessor)(void)) {
	return **accessor();
}

// Storage for a game variable or asm address no test has set; used by EnginePtrs.cpp.
void* Slot(int dll, int offset);

// ---- Engine function fakes (signatures match D2Ptrs.h) ------------------------------------

wchar_t* __fastcall GetLocaleText(WORD nLocaleTxtNo);
void __stdcall PrintGameString(wchar_t* wMessage, int nColor);
UnitAny* __stdcall GetPlayerUnit();
BYTE __stdcall GetDifficulty();
UnitAny* __fastcall GetCurrentInteractingNPC();
void* __stdcall GetQuestInfo();
DWORD __stdcall GetUnitStat(UnitAny* pUnit, DWORD dwStat, DWORD dwStat2);
StatList* __stdcall GetStatList(UnitAny* pUnit, DWORD dwUnk, DWORD dwMaxEntries);
DWORD __stdcall CopyStatList(StatList* pStatList, Stat* pStatArray, DWORD dwMaxEntries);
int __stdcall GetStatValueFromStatList(StatList* pStatList, int statId, WORD nLayer);
StatList* __stdcall GetStateStatList(UnitAny* pUnit, DWORD dwStateNo);
ItemsTxt* __stdcall GetItemText(DWORD dwItemNo);
ItemsTxt* __stdcall GetItemTextFromItemCode(DWORD dwCode, int* pItemId);
DWORD __stdcall GetItemLevelRequirement(UnitAny* pItem, UnitAny* pPlayer);
BYTE __stdcall GetMaxSockets(UnitAny* pItem);
DWORD __stdcall GetItemPrice(UnitAny* pPlayer, UnitAny* pItem, DWORD nDifficulty, DWORD pQuestInfo, DWORD nVendorId,
	DWORD nTransactionType);
Room1* __stdcall GetRoomFromUnit(UnitAny* ptUnit);
int __stdcall GetLevelIdFromRoom(Room1* pRoom);

// ---- Text drawing, server units, names, party and network --------------------------------

// A string drawn with D2WIN_DrawText (Texthook::Draw, hooks): text, position, colour, and the
// D2WIN_SetTextSize font in effect.
struct DrawnText {
	std::wstring text;
	int x;
	int y;
	DWORD color;
	DWORD font;
};
const std::vector<DrawnText>& Drawn();

// D2WIN_GetTextWidthFileNo measures every character as kCharWidth pixels.
const DWORD kCharWidth = 8;

void AddServerUnit(UnitAny* unit);                               // D2CLIENT_FindServerSideUnit(id, type)
void SetLevelName(DWORD levelId, const std::wstring& name);      // D2CLIENT_GetLevelName (default "")
void SetItemName(UnitAny* item, const std::wstring& name);       // D2CLIENT_GetItemName (default "")
void SetQuestInfo(void* quests);                                 // D2CLIENT_GetQuestInfo (default null)

// Packets sent with D2NET_SendPacket, in order.
const std::vector<std::vector<BYTE>>& SentPackets();
// D2CLIENT_ClickParty calls (roster entry, mode), in order, and the D2CLIENT_LeaveParty count.
const std::vector<std::pair<RosterUnit*, DWORD>>& PartyClicks();
int PartyLeaves();

void __fastcall WinDrawText(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD dwUnk);
DWORD __fastcall SetTextSize(DWORD dwSize);
DWORD __fastcall GetTextWidthFileNo(wchar_t* wStr, DWORD* dwWidth, DWORD* dwFileNo);
UnitAny* __fastcall FindServerSideUnit(DWORD dwId, DWORD dwType);
BOOL __stdcall ClientGetItemName(UnitAny* pItem, wchar_t* wBuffer, DWORD dwSize);
void __fastcall LeaveParty(void);
void __stdcall SendPacket(size_t aLen, DWORD arg1, BYTE* aPacket);

}  // namespace fake
