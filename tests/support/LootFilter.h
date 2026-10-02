// Helpers for loot filter tests: build an item the rule engine can look at, load a filter, and
// read back what BH would show for the item.
#pragma once
#include <string>

#include "FakeEngine.h"
#include "Modules/Item/Item.h"
#include "Modules/Item/ItemDisplay.h"

namespace support {

// An item as BH's loot filter sees it: the unit, its Weapons/Armor/Misc.txt record, the
// ItemTypes-derived attributes Item.cpp would have built, and the UnitItemInfo tying them together.
// Defaults: identified, item level 1, no stats, ItemAttributes zeroed except name = code and
// staffmodClass = 255 (no staff mods).
class TestItem {
public:
	TestItem(const char* code, DWORD quality);
	TestItem(const TestItem&) = delete;
	TestItem& operator=(const TestItem&) = delete;

	UnitAny* unit() { return &item_.unit; }
	ItemData& data() { return item_.data; }
	ItemsTxt& txt() { return txt_; }
	ItemAttributes& attrs() { return attrs_; }
	UnitItemInfo* info() { return &info_; }

	TestItem& Stat(int stat, int value, int param = 0);
	TestItem& Flags(DWORD flags);  // replaces ItemData::dwFlags
	TestItem& ItemLevel(DWORD ilvl);

private:
	fake::ItemUnit& item_;
	ItemsTxt& txt_;
	ItemAttributes attrs_;
	UnitItemInfo info_;
};

// Parses one rule the way InitializeItemRules does for an `ItemDisplay[conditions]: action` line.
// The rule is not added to RuleList, and is never freed (Rule's destructor needs
// ReplacementValue, which only ItemDisplay.cpp defines; a test process can afford the leak).
Rule* ParseRule(const std::wstring& conditions, const std::wstring& action = L"%NAME%");

// True when `conditions` match the item.
bool Matches(const std::wstring& conditions, TestItem& item);

// Writes `filterText` as the loot filter file, parses it with BH's Config and runs
// ItemDisplay::InitializeItemRules, exactly as BH does when the filter is (re)loaded.
void LoadFilter(const std::string& filterText);

// The name BH shows for the item, starting from the game's name `baseName` (GetItemName).
std::wstring NameOf(TestItem& item, const std::wstring& baseName);

// The description (the text added above the item's stats).
std::wstring DescriptionOf(TestItem& item);

}  // namespace support
