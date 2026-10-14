#include "LootFilter.h"

#include <Windows.h>

#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>

#include "BH.h"
#include "Config.h"
#include "doctest/doctest.h"

extern BYTE LastConditionType;  // ItemDisplay.cpp

namespace support {
namespace {

DWORD nextTxtFileNo = 1;

std::string TempDir() {
	char path[MAX_PATH] = {};
	GetTempPathA(MAX_PATH, path);
	return std::string(path) + "bh-tests-" + std::to_string(GetCurrentProcessId()) + "\\";
}

}  // namespace

TestItem::TestItem(const char* code, DWORD quality)
	: item_(fake::NewItem(nextTxtFileNo, quality)), txt_(fake::AddItemTxt(nextTxtFileNo, code)), attrs_(), info_() {
	nextTxtFileNo++;
	attrs_.name = std::wstring(code, code + std::strlen(code));
	info_.item = &item_.unit;
	strncpy_s(info_.itemCode, code, _TRUNCATE);
	info_.attrs = &attrs_;
	attrs_.staffmodClass = 255;  // no staff mods (Item.cpp's value when ItemTypes.txt has none)
}

TestItem& TestItem::Stat(int stat, int value, int param) {
	fake::SetStat(&item_.unit, stat, value, param);
	return *this;
}

TestItem& TestItem::Flags(DWORD flags) {
	item_.data.dwFlags = flags;
	return *this;
}

TestItem& TestItem::ItemLevel(DWORD ilvl) {
	item_.data.dwItemLevel = ilvl;
	return *this;
}

Rule* ParseRule(const std::wstring& conditions, const std::wstring& action) {
	std::wstring buf;
	std::wstringstream ss(conditions);
	std::vector<std::wstring> tokens;
	while (ss >> buf) {
		tokens.push_back(buf);
	}
	LastConditionType = CT_None;
	std::vector<Condition*> raw;
	for (auto& token : tokens) {
		Condition::BuildConditions(raw, token);
	}
	std::wstring text = action;
	return new Rule(raw, &text);
}

bool Matches(const std::wstring& conditions, TestItem& item) {
	// Not freed: Rule's destructor needs ReplacementValue, which only ItemDisplay.cpp defines.
	return ParseRule(conditions)->Evaluate(item.info());
}

void LoadFilter(const std::string& filterText) {
	static Config filter("test.filter");
	BH::path = TempDir();
	CreateDirectoryA(BH::path.c_str(), nullptr);
	const std::string file = BH::path + "test.filter";
	{
		std::ofstream out(file, std::ios::binary | std::ios::trunc);
		out << filterText;
	}
	const bool parsed = filter.Parse();
	// Config keeps the parsed lines in memory, so the file is not needed past this point.
	DeleteFileA(file.c_str());
	RemoveDirectoryA(BH::path.c_str());
	REQUIRE_MESSAGE(parsed, "could not read the filter file back");
	BH::lootFilter = &filter;
	ItemDisplay::UninitializeItemRules();
	ItemDisplay::InitializeItemRules();
}

std::wstring NameOf(TestItem& item, const std::wstring& baseName) {
	std::wstring name = baseName;
	GetItemName(item.info(), name);
	return name;
}

std::wstring DescriptionOf(TestItem& item) {
	return item_desc_cache.Get(item.info());
}

}  // namespace support
