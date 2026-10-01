// Modules/MapNotify: drop notifications. Each frame BH looks at the items lying in the act; the first
// time it sees an item that matches a loot filter rule with a map/notification keyword (%MAP-xx%,
// %BORDER-xx%, %SOUNDID-n%...), it prints the item's name in the chat in the item's quality colour
// (subject to "detailed notifications": 0 off, 1 all, 2 only new drops, and to %TIER-n% against the
// filter level), and plays the rule's sound.
#include <cstring>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "BH.h"
#include "D2DataTables.h"
#include "D2Ptrs.h"
#include "FakeEngine.h"
#include "LootFilter.h"
#include "Modules/Item/Item.h"
#include "Modules/MapNotify/MapNotify.h"

namespace {

using support::TestItem;

// Items lying in the player's current act. Every item's code is registered in ItemAttributeMap
// (as Item.cpp does for every Weapons/Armor/Misc.txt row at game join).
struct Ground {
	Path path{};
	Room1 room1{};
	Room2 room2{};
	Level level{};
	Act act{};
	std::vector<TestItem*> items;
	MapNotify module;

	Ground() {
		UnitAny& player = fake::Player();
		player.pPath = &path;
		path.pRoom1 = &room1;
		room1.pRoom2 = &room2;
		room2.pLevel = &level;
		level.dwLevelNo = 4;
		player.pAct = &act;
		act.pRoom1 = &room1;
	}
	~Ground() {
		for (TestItem* item : items) {
			delete item;
		}
		ItemAttributeMap.clear();
	}

	// Drops an item whose in-game name (D2CLIENT_GetItemName) is `name`.
	TestItem& Drop(const char* code, DWORD quality, const std::wstring& name, DWORD flags = ITEM_IDENTIFIED) {
		TestItem* item = new TestItem(code, quality);
		item->Flags(flags);
		fake::SetItemName(item->unit(), name);
		ItemAttributeMap[code] = &item->attrs();
		item->unit()->pListNext = room1.pUnitFirst;
		room1.pUnitFirst = item->unit();
		items.push_back(item);
		return *item;
	}

	// Runs one frame; returns the chat lines it printed.
	std::vector<std::pair<std::wstring, int>> Frame() {
		size_t before = fake::Printed().size();
		module.OnDraw();
		return std::vector<std::pair<std::wstring, int>>(fake::Printed().begin() + before, fake::Printed().end());
	}
};

using Lines = std::vector<std::pair<std::wstring, int>>;

Lines Line(const std::wstring& text, int color) {
	return { std::make_pair(text, color) };
}

struct PlayedSound {
	int sound;
	int volume;
};
std::vector<PlayedSound> playedSounds;

BOOL __stdcall RecordSound(UnitAny* pUnit, int nSound, int nVolume, int nPriority, BOOL bDropSound) {
	playedSounds.push_back(PlayedSound{ nSound, nVolume });
	return TRUE;
}

// sounds.txt with 10 records; record `id` is a one-shot sound at volume 100 unless changed.
struct SoundTable {
	std::vector<SoundsTxt> sounds;
	SoundTable() : sounds(10) {
		std::memset(sounds.data(), 0, sounds.size() * sizeof(SoundsTxt));
		for (auto& sound : sounds) {
			sound.volume = 100;
		}
		fake::Var(Var_D2CLIENT_SoundsTxt) = sounds.data();
		fake::Var(Var_D2CLIENT_SoundRecords) = static_cast<DWORD>(sounds.size());
		App.pd2.pd2PlaySoundImpl = &RecordSound;
		playedSounds.clear();
	}
	~SoundTable() {
		playedSounds.clear();
	}
};

}  // namespace

TEST_SUITE("MapNotify") {
	TEST_CASE("an item matching a map rule is announced once, in its quality colour") {
		support::LoadFilter("ItemDisplay[uap]: %NAME%%MAP-97%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
		CHECK(ground.Frame() == Line(L"Shako", Gold));
		CHECK(ground.Frame().empty());
	}

	TEST_CASE("each announced item gets the colour of its own quality") {
		support::LoadFilter("ItemDisplay[]: %NAME%%BORDER-62%\n");
		Ground ground;
		ground.Drop("rin", ITEM_QUALITY_RARE, L"Doom Loop");
		ground.Drop("amu", ITEM_QUALITY_SET, L"Tal Rasha's Adjudication");
		ground.Drop("jew", ITEM_QUALITY_MAGIC, L"Ruby Jewel");
		ground.Drop("r33", ITEM_QUALITY_NORMAL, L"Zod Rune");
		Lines lines = ground.Frame();
		REQUIRE(lines.size() == 4);
		// The room's unit list holds the last drop first.
		CHECK(lines[0] == std::make_pair(std::wstring(L"Zod Rune"), static_cast<int>(White)));
		CHECK(lines[1] == std::make_pair(std::wstring(L"Ruby Jewel"), static_cast<int>(Blue)));
		CHECK(lines[2] == std::make_pair(std::wstring(L"Tal Rasha's Adjudication"), static_cast<int>(Green)));
		CHECK(lines[3] == std::make_pair(std::wstring(L"Doom Loop"), static_cast<int>(Yellow)));
	}

	TEST_CASE("items matching no map rule are not announced") {
		support::LoadFilter(
			"ItemDisplay[uap]: %NAME%%MAP-97%\n"
			"ItemDisplay[hax]: %RED%%NAME%\n");
		Ground ground;
		ground.Drop("hax", ITEM_QUALITY_NORMAL, L"Hand Axe");
		CHECK(ground.Frame().empty());
	}

	TEST_CASE("an item that appears later is announced when it appears") {
		support::LoadFilter("ItemDisplay[uap]: %NAME%%MAP-97%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
		ground.Frame();
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Peasant Crown");
		CHECK(ground.Frame() == Line(L"Peasant Crown", Gold));
	}

	TEST_CASE("detailed notifications set to 2 announce only new drops") {
		support::LoadFilter("ItemDisplay[uap]: %NAME%%MAP-97%\n");
		App.lootfilter.detailedNotifications.value = 2;
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Old Shako");
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"New Shako", ITEM_IDENTIFIED | ITEM_NEW);
		CHECK(ground.Frame() == Line(L"New Shako", Gold));
	}

	TEST_CASE("no announcements with detailed notifications off or the loot filter disabled") {
		support::LoadFilter("ItemDisplay[uap]: %NAME%%MAP-97%\n");
		SUBCASE("notifications off") {
			App.lootfilter.detailedNotifications.value = 0;
		}
		SUBCASE("filter disabled") {
			App.lootfilter.enableFilter.value = false;
		}
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako", ITEM_IDENTIFIED | ITEM_NEW);
		CHECK(ground.Frame().empty());
	}

	TEST_CASE("%TIER-n% rules are announced up to filter level n, and always at filter level 0") {
		support::LoadFilter("ItemDisplay[uap]: %NAME%%MAP-97%%TIER-2%\n");
		SUBCASE("level 2") {
			App.lootfilter.filterLevel.value = 2;
			Ground ground;
			ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
			CHECK(ground.Frame() == Line(L"Shako", Gold));
		}
		SUBCASE("level 3") {
			App.lootfilter.filterLevel.value = 3;
			Ground ground;
			ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
			CHECK(ground.Frame().empty());
		}
		SUBCASE("level 0") {
			App.lootfilter.filterLevel.value = 0;
			Ground ground;
			ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
			CHECK(ground.Frame() == Line(L"Shako", Gold));
		}
	}

	TEST_CASE("a later map rule still announces an item whose first map rule is above the filter level") {
		support::LoadFilter(
			"ItemDisplay[uap]: %NAME%%MAP-97%%TIER-1%%CONTINUE%\n"
			"ItemDisplay[uap]: %NAME%%BORDER-62%\n");
		App.lootfilter.filterLevel.value = 3;
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
		CHECK(ground.Frame() == Line(L"Shako", Gold));
	}

	TEST_CASE("announced names lose their padding but keep their leading colour") {
		support::LoadFilter("ItemDisplay[]: %NAME%%MAP-97%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"   Shako   ");
		CHECK(ground.Frame() == Line(L"Shako", Gold));
		ground.Drop("rin", ITEM_QUALITY_RARE, L"  \u00FF" L"c9 Doom Loop \u00FF" L"c0 ");
		CHECK(ground.Frame() == Line(L"\u00FF" L"c9Doom Loop", Yellow));
	}

	TEST_CASE("multi-line names are announced on one line, joined by ' - '") {
		support::LoadFilter("ItemDisplay[]: %NAME%%MAP-97%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Harlequin Crest\nShako");
		CHECK(ground.Frame() == Line(L"Harlequin Crest - Shako", Gold));
		ground.Drop("r33", ITEM_QUALITY_NORMAL, L"a\nb\nc");
		CHECK(ground.Frame() == Line(L"a - b - c", White));
	}

	TEST_CASE("multi-line names lose their padding too") {
		support::LoadFilter("ItemDisplay[]: %NAME%%MAP-97%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"  Harlequin Crest\nShako  ");
		CHECK(ground.Frame() == Line(L"Harlequin Crest - Shako", Gold));
	}

	TEST_CASE("an announced item plays its rule's sound at the sound's volume") {
		SoundTable table;
		table.sounds[5].volume = 80;
		support::LoadFilter("ItemDisplay[uap]: %NAME%%SOUNDID-5%\n");
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
		CHECK(ground.Frame() == Line(L"Shako", Gold));
		REQUIRE(playedSounds.size() == 1);
		CHECK(playedSounds[0].sound == 5);
		CHECK(playedSounds[0].volume == 80);

		ground.Frame();
		CHECK(playedSounds.size() == 1);
	}

	TEST_CASE("looping sounds, sound group 2 and drop sounds turned off play nothing") {
		SoundTable table;
		table.sounds[5].loop = 1;
		table.sounds[6].nSoundGroup = 2;
		SUBCASE("looping") {
			support::LoadFilter("ItemDisplay[uap]: %NAME%%SOUNDID-5%\n");
		}
		SUBCASE("group 2") {
			support::LoadFilter("ItemDisplay[uap]: %NAME%%SOUNDID-6%\n");
		}
		SUBCASE("drop sounds off") {
			support::LoadFilter("ItemDisplay[uap]: %NAME%%SOUNDID-7%\n");
			App.lootfilter.dropSounds.value = false;
		}
		Ground ground;
		ground.Drop("uap", ITEM_QUALITY_UNIQUE, L"Shako");
		CHECK(ground.Frame() == Line(L"Shako", Gold));
		CHECK(playedSounds.empty());
	}
}
