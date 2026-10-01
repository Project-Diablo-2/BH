#include "FakeEngine.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <map>
#include <memory>

#include "Constants.h"
#include "ResetEachTest.h"

namespace fake {
namespace {

// A unit's stats, kept as the game keeps them: one list per unit, plus the runeword state list.
// The StatList objects are only handles; the values live in the maps.
struct Stats {
	StatList list{};
	StatList runewordList{};
	std::map<std::pair<int, int>, int> values;          // (stat, param) -> value
	std::map<std::pair<int, int>, int> runewordValues;  // same, runeword state list
};

struct World {
	std::deque<ItemUnit> items;
	UnitAny player{};
	PlayerData playerData{};
	std::map<const UnitAny*, std::unique_ptr<Stats>> stats;
	std::map<DWORD, std::unique_ptr<ItemsTxt>> itemTxt;
	std::map<WORD, std::wstring> localeText;
	std::map<std::pair<const UnitAny*, int>, DWORD> levelRequirement;  // (item, class or -1)
	std::map<const UnitAny*, BYTE> maxSockets;
	std::map<std::pair<const UnitAny*, int>, DWORD> price;  // (item, transaction type)
	std::map<DWORD, DWORD> uiVars;
	std::vector<std::pair<std::wstring, int>> printed;
	UnitAny* interactingNpc = nullptr;
	BYTE difficulty = 0;
	int areaId = 0;
	std::vector<DrawnText> drawn;
	DWORD textSize = 0;
	std::map<std::pair<DWORD, DWORD>, UnitAny*> serverUnits;  // (type, id) -> unit
	std::map<DWORD, std::wstring> levelNames;
	std::map<const UnitAny*, std::wstring> itemNames;
	void* questInfo = nullptr;
	std::vector<std::vector<BYTE>> sentPackets;
	std::vector<std::pair<RosterUnit*, DWORD>> partyClicks;
	int partyLeaves = 0;
};

World& W() {
	static World world;
	return world;
}

// Game variables and asm addresses: one zeroed block per (dll, offset), handed out the first
// time BH resolves it (at static initialisation, for most of them) and cleared on Reset().
struct SlotStore {
	static const size_t kSlotSize = 256;
	std::map<std::pair<int, int>, std::unique_ptr<char[]>> slots;
};

SlotStore& Slots() {
	static SlotStore store;
	return store;
}

Stats& StatsOf(const UnitAny* unit) {
	auto& entry = W().stats[unit];
	if (!entry) {
		entry.reset(new Stats());
	}
	return *entry;
}

const UnitAny* OwnerOf(const StatList* list, bool& runeword) {
	for (auto& entry : W().stats) {
		if (&entry.second->list == list) {
			runeword = false;
			return entry.first;
		}
		if (&entry.second->runewordList == list) {
			runeword = true;
			return entry.first;
		}
	}
	return nullptr;
}

}  // namespace

REGISTER_LISTENER("fake-engine-reset", 1, ResetEachTest<&Reset>);

void Reset() {
	W() = World();
	for (auto& slot : Slots().slots) {
		std::memset(slot.second.get(), 0, SlotStore::kSlotSize);
	}
	W().player.dwType = UNIT_PLAYER;
	W().player.dwUnitId = 1;
	W().player.pPlayerData = &W().playerData;
	Var(Var_D2CLIENT_PlayerUnit) = &W().player;
}

void* Slot(int dll, int offset) {
	auto& slot = Slots().slots[std::make_pair(dll, offset)];
	if (!slot) {
		slot.reset(new char[SlotStore::kSlotSize]());
	}
	return slot.get();
}

ItemUnit& NewItem(DWORD txtFileNo, DWORD quality) {
	W().items.emplace_back();
	ItemUnit& item = W().items.back();
	std::memset(&item, 0, sizeof(item));
	item.unit.dwType = UNIT_ITEM;
	item.unit.dwTxtFileNo = txtFileNo;
	item.unit.dwUnitId = 1000 + static_cast<DWORD>(W().items.size());
	item.unit.pItemData = &item.data;
	item.data.dwQuality = quality;
	item.data.dwFlags = ITEM_IDENTIFIED;
	item.data.dwItemLevel = 1;
	return item;
}

UnitAny& Player() {
	return W().player;
}

void SetStat(UnitAny* unit, int stat, int value, int param) {
	StatsOf(unit).values[std::make_pair(stat, param)] = value;
}

void SetRunewordStat(UnitAny* unit, int stat, int value, int param) {
	StatsOf(unit).runewordValues[std::make_pair(stat, param)] = value;
}

ItemsTxt& AddItemTxt(DWORD txtFileNo, const char* code) {
	auto& txt = W().itemTxt[txtFileNo];
	txt.reset(new ItemsTxt());
	std::memset(txt.get(), 0, sizeof(ItemsTxt));
	std::memset(txt->szCode, ' ', sizeof(txt->szCode));
	std::memcpy(txt->szCode, code, (std::min)(std::strlen(code), sizeof(txt->szCode)));
	return *txt;
}

void SetLocaleText(WORD id, const std::wstring& text) {
	W().localeText[id] = text;
}

void SetDifficulty(BYTE difficulty) {
	W().difficulty = difficulty;
}

void SetAreaId(int levelId) {
	W().areaId = levelId;
}

void SetLevelRequirement(UnitAny* item, DWORD level, int classId) {
	W().levelRequirement[std::make_pair(static_cast<const UnitAny*>(item), classId)] = level;
}

void SetMaxSockets(UnitAny* item, BYTE sockets) {
	W().maxSockets[item] = sockets;
}

void SetPrice(UnitAny* item, int transactionType, DWORD price) {
	W().price[std::make_pair(static_cast<const UnitAny*>(item), transactionType)] = price;
}

void SetInteractingNpc(UnitAny* npc) {
	W().interactingNpc = npc;
}

void SetUIVar(DWORD varno, DWORD value) {
	W().uiVars[varno] = value;
}

const std::vector<std::pair<std::wstring, int>>& Printed() {
	return W().printed;
}

// ---- Engine function fakes -----------------------------------------------------------------

wchar_t* __fastcall GetLocaleText(WORD nLocaleTxtNo) {
	static wchar_t empty[1] = { 0 };
	auto it = W().localeText.find(nLocaleTxtNo);
	return it == W().localeText.end() ? empty : &it->second[0];
}

void __stdcall PrintGameString(wchar_t* wMessage, int nColor) {
	W().printed.emplace_back(wMessage, nColor);
}

UnitAny* __stdcall GetPlayerUnit() {
	return &W().player;
}

BYTE __stdcall GetDifficulty() {
	return W().difficulty;
}

UnitAny* __fastcall GetCurrentInteractingNPC() {
	return W().interactingNpc;
}

void* __stdcall GetQuestInfo() {
	return W().questInfo;
}

DWORD __stdcall GetUnitStat(UnitAny* pUnit, DWORD dwStat, DWORD dwStat2) {
	auto it = W().stats.find(pUnit);
	if (it == W().stats.end()) {
		return 0;
	}
	auto value = it->second->values.find(std::make_pair(static_cast<int>(dwStat), static_cast<int>(dwStat2)));
	return value == it->second->values.end() ? 0 : static_cast<DWORD>(value->second);
}

StatList* __stdcall GetStatList(UnitAny* pUnit, DWORD dwUnk, DWORD dwMaxEntries) {
	return pUnit ? &StatsOf(pUnit).list : nullptr;
}

DWORD __stdcall CopyStatList(StatList* pStatList, Stat* pStatArray, DWORD dwMaxEntries) {
	bool runeword = false;
	const UnitAny* owner = OwnerOf(pStatList, runeword);
	if (!owner) {
		return 0;
	}
	const auto& values = runeword ? W().stats[owner]->runewordValues : W().stats[owner]->values;
	DWORD count = 0;
	for (const auto& entry : values) {
		if (count == dwMaxEntries) {
			break;
		}
		pStatArray[count].wStatIndex = static_cast<WORD>(entry.first.first);
		pStatArray[count].wSubIndex = static_cast<WORD>(entry.first.second);
		pStatArray[count].dwStatValue = static_cast<DWORD>(entry.second);
		count++;
	}
	return count;
}

int __stdcall GetStatValueFromStatList(StatList* pStatList, int statId, WORD nLayer) {
	bool runeword = false;
	const UnitAny* owner = OwnerOf(pStatList, runeword);
	if (!owner) {
		return 0;
	}
	const auto& values = runeword ? W().stats[owner]->runewordValues : W().stats[owner]->values;
	auto it = values.find(std::make_pair(statId, static_cast<int>(nLayer)));
	return it == values.end() ? 0 : it->second;
}

StatList* __stdcall GetStateStatList(UnitAny* pUnit, DWORD dwStateNo) {
	if (!pUnit || dwStateNo != STATE_RUNEWORD) {
		return nullptr;
	}
	auto it = W().stats.find(pUnit);
	if (it == W().stats.end() || it->second->runewordValues.empty()) {
		return nullptr;
	}
	return &it->second->runewordList;
}

ItemsTxt* __stdcall GetItemText(DWORD dwItemNo) {
	auto it = W().itemTxt.find(dwItemNo);
	return it == W().itemTxt.end() ? nullptr : it->second.get();
}

ItemsTxt* __stdcall GetItemTextFromItemCode(DWORD dwCode, int* pItemId) {
	for (auto& entry : W().itemTxt) {
		if (entry.second->dwcode == dwCode) {
			if (pItemId) {
				*pItemId = static_cast<int>(entry.first);
			}
			return entry.second.get();
		}
	}
	return nullptr;
}

DWORD __stdcall GetItemLevelRequirement(UnitAny* pItem, UnitAny* pPlayer) {
	const auto& reqs = W().levelRequirement;
	auto it = pPlayer ? reqs.find(std::make_pair(static_cast<const UnitAny*>(pItem), static_cast<int>(pPlayer->dwTxtFileNo)))
		: reqs.end();
	if (it == reqs.end()) {
		it = reqs.find(std::make_pair(static_cast<const UnitAny*>(pItem), -1));
	}
	return it == reqs.end() ? 0 : it->second;
}

BYTE __stdcall GetMaxSockets(UnitAny* pItem) {
	auto it = W().maxSockets.find(pItem);
	return it == W().maxSockets.end() ? 0 : it->second;
}

DWORD __stdcall GetItemPrice(UnitAny* pPlayer, UnitAny* pItem, DWORD nDifficulty, DWORD pQuestInfo, DWORD nVendorId,
	DWORD nTransactionType) {
	auto it = W().price.find(std::make_pair(static_cast<const UnitAny*>(pItem), static_cast<int>(nTransactionType)));
	return it == W().price.end() ? 0 : it->second;
}

Room1* __stdcall GetRoomFromUnit(UnitAny* ptUnit) {
	// Any non-null room; GetLevelIdFromRoom ignores it.
	static char room[64];
	return ptUnit ? reinterpret_cast<Room1*>(room) : nullptr;
}

int __stdcall GetLevelIdFromRoom(Room1* pRoom) {
	return W().areaId;
}


const std::vector<DrawnText>& Drawn() {
	return W().drawn;
}

void AddServerUnit(UnitAny* unit) {
	W().serverUnits[std::make_pair(unit->dwType, unit->dwUnitId)] = unit;
}

void SetLevelName(DWORD levelId, const std::wstring& name) {
	W().levelNames[levelId] = name;
}

void SetItemName(UnitAny* item, const std::wstring& name) {
	W().itemNames[item] = name;
}

void SetQuestInfo(void* quests) {
	W().questInfo = quests;
}

const std::vector<std::vector<BYTE>>& SentPackets() {
	return W().sentPackets;
}

const std::vector<std::pair<RosterUnit*, DWORD>>& PartyClicks() {
	return W().partyClicks;
}

int PartyLeaves() {
	return W().partyLeaves;
}

void __fastcall WinDrawText(const wchar_t* wStr, int xPos, int yPos, DWORD dwColor, DWORD dwUnk) {
	W().drawn.push_back(DrawnText{ wStr, xPos, yPos, dwColor, W().textSize });
}

DWORD __fastcall SetTextSize(DWORD dwSize) {
	DWORD old = W().textSize;
	W().textSize = dwSize;
	return old;
}

DWORD __fastcall GetTextWidthFileNo(wchar_t* wStr, DWORD* dwWidth, DWORD* dwFileNo) {
	*dwWidth = static_cast<DWORD>(wcslen(wStr)) * kCharWidth;
	*dwFileNo = 0;
	return *dwWidth;
}

UnitAny* __fastcall FindServerSideUnit(DWORD dwId, DWORD dwType) {
	auto it = W().serverUnits.find(std::make_pair(dwType, dwId));
	return it == W().serverUnits.end() ? nullptr : it->second;
}

BOOL __stdcall ClientGetItemName(UnitAny* pItem, wchar_t* wBuffer, DWORD dwSize) {
	auto it = W().itemNames.find(pItem);
	std::wstring name = it == W().itemNames.end() ? L"" : it->second;
	wcsncpy_s(wBuffer, dwSize, name.c_str(), _TRUNCATE);
	return TRUE;
}

void __fastcall LeaveParty(void) {
	W().partyLeaves++;
}

void __stdcall SendPacket(size_t aLen, DWORD arg1, BYTE* aPacket) {
	W().sentPackets.emplace_back(aPacket, aPacket + aLen);
}

}  // namespace fake

// D2Stubs.cpp wraps a few engine functions with register-convention asm thunks; the test build
// does not compile D2Stubs.cpp, so these are the fakes for the ones the code under test calls.
DWORD __fastcall D2CLIENT_GetUIVar_STUB(DWORD varno) {
	auto it = fake::W().uiVars.find(varno);
	return it == fake::W().uiVars.end() ? 0 : it->second;
}

DWORD __fastcall D2CLIENT_GetUnitName_STUB(DWORD unit) {
	static wchar_t empty[1] = { 0 };
	return reinterpret_cast<DWORD>(empty);
}

DWORD __fastcall TestPvpFlag_STUB(DWORD planum1, DWORD planum2, DWORD flagmask) {
	return 0;
}

int __stdcall D2COMMON_GetSequenceIndex_STUB(UnitAny* pUnit) {
	return 0;
}

int __stdcall D2COMMON_GetFrameMinAccr_STUB(int nIndex, UnitAny* pUnit) {
	return 0;
}

// Gamefilter.cpp loads the join-game screen's button/box images; tests have no UI images.
CellFile* __fastcall D2CLIENT_LoadUiImage(CHAR* szPath) {
	return NULL;
}

DWORD __fastcall D2CLIENT_GetLevelName_STUB(DWORD levelId) {
	static wchar_t empty[1] = { 0 };
	auto it = fake::W().levelNames.find(levelId);
	return reinterpret_cast<DWORD>(it == fake::W().levelNames.end() ? empty : &it->second[0]);
}

DWORD __fastcall D2CLIENT_ClickParty_ASM(RosterUnit* RosterUnit, DWORD Mode) {
	fake::W().partyClicks.emplace_back(RosterUnit, Mode);
	return 0;
}
