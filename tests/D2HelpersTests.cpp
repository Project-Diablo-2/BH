// D2Helpers.cpp, the helpers BH calls: item quality colours (drop notifications), percentage maths
// (requirements and elemental masteries), the party roster (auto-party), finding the player's
// mercenary (merc item checks in the loot filter), chat printing (only once the game is ready) and
// Skills.txt lookups (stats panel).
#include <climits>
#include <cstring>
#include <memory>
#include <string>

#include "doctest/doctest.h"

#include "Constants.h"
#include "D2DataTables.h"
#include "D2Helpers.h"
#include "D2Ptrs.h"
#include "FakeEngine.h"

namespace {

UnitAny Unit(DWORD type, DWORD id) {
	UnitAny unit;
	std::memset(&unit, 0, sizeof(unit));
	unit.dwType = type;
	unit.dwUnitId = id;
	return unit;
}

RosterUnit Roster(const char* name, DWORD id, WORD partyId) {
	RosterUnit roster;
	std::memset(&roster, 0, sizeof(roster));
	strcpy_s(roster.szName, name);
	roster.dwUnitId = id;
	roster.wPartyId = partyId;
	roster.wLevel = 1;
	return roster;
}

ClientPetData Pet(int petTypeId, int petUnitId, int ownerId, int ownerType) {
	ClientPetData pet;
	std::memset(&pet, 0, sizeof(pet));
	pet.nPetTypeId = petTypeId;
	pet.nPetUnitId = petUnitId;
	pet.nOwnerId = ownerId;
	pet.nOwnerType = ownerType;
	return pet;
}

// The player standing in a loaded level: what IsGameReady checks for.
struct LoadedGame {
	Path path{};
	Room1 room1{};
	Room2 room2{};
	Level level{};
	Act act{};
	char inventory[64] = {};

	explicit LoadedGame(DWORD levelNo = MAP_A1_ROGUE_ENCAMPMENT) {
		UnitAny& player = fake::Player();
		player.pPath = &path;
		path.pRoom1 = &room1;
		path.xPos = 5000;
		path.yPos = 6000;
		room1.pRoom2 = &room2;
		room2.pLevel = &level;
		level.dwLevelNo = levelNo;
		player.pAct = &act;
		act.pRoom1 = &room1;
		player.pInventory = reinterpret_cast<Inventory*>(inventory);
	}
};

}  // namespace

TEST_SUITE("D2Helpers") {
	TEST_CASE("item quality colours follow the game's item name colours") {
		CHECK(ItemColorFromQuality(ITEM_QUALITY_INFERIOR) == White);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_NORMAL) == White);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_SUPERIOR) == White);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_MAGIC) == Blue);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_SET) == Green);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_RARE) == Yellow);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_UNIQUE) == Gold);
		CHECK(ItemColorFromQuality(ITEM_QUALITY_CRAFT) == Orange);
	}

	TEST_CASE("UTILITY_CalcPercent takes a percentage of a value, truncating toward zero") {
		// -20% requirements on a 125 strength item: 25 less.
		CHECK(UTILITY_CalcPercent(125, -20, 100) == -25);
		// 51 * -20% = -10.2: the requirement drops by 10, not 11.
		CHECK(UTILITY_CalcPercent(51, -20, 100) == -10);
		// +37% fire mastery on 99 damage = 36.63: 36 more.
		CHECK(UTILITY_CalcPercent(99, 37, 100) == 36);
		CHECK(UTILITY_CalcPercent(0, 50, 100) == 0);
		CHECK(UTILITY_CalcPercent(200, 0, 100) == 0);
	}

	TEST_CASE("UTILITY_CalcPercent treats a zero base as 1 and caps at INT_MAX") {
		CHECK(UTILITY_CalcPercent(10, 50, 0) == 500);
		CHECK(UTILITY_CalcPercent(INT_MAX, 200, 100) == INT_MAX);
		CHECK(UTILITY_CalcPercent(2000000000, 2000000000.0, 1) == INT_MAX);
	}

	TEST_CASE("FindPlayerRoster finds a player in the roster by unit id") {
		RosterUnit me = Roster("Me", 1, INVALID_PARTY_ID);
		RosterUnit other = Roster("Other", 7, INVALID_PARTY_ID);
		RosterUnit last = Roster("Last", 9, INVALID_PARTY_ID);
		me.pNext = &other;
		other.pNext = &last;
		fake::Var(Var_D2CLIENT_PlayerUnitList) = &me;

		CHECK(FindPlayerRoster(1) == &me);
		CHECK(FindPlayerRoster(7) == &other);
		CHECK(FindPlayerRoster(9) == &last);
		CHECK(FindPlayerRoster(8) == nullptr);
	}

	TEST_CASE("FindPlayerRoster finds nobody when the roster is empty") {
		fake::Var(Var_D2CLIENT_PlayerUnitList) = nullptr;
		CHECK(FindPlayerRoster(1) == nullptr);
	}

	TEST_CASE("GetClientMercUnit finds the player's own hireling among the pets") {
		UnitAny merc = Unit(UNIT_MONSTER, 50);
		fake::AddServerUnit(&merc);
		UnitAny othersMerc = Unit(UNIT_MONSTER, 51);
		fake::AddServerUnit(&othersMerc);

		ClientPetData golem = Pet(3, 49, 1, UNIT_PLAYER);
		ClientPetData theirs = Pet(PETTYPE_HIREABLE, 51, 2, UNIT_PLAYER);
		ClientPetData mine = Pet(PETTYPE_HIREABLE, 50, 1, UNIT_PLAYER);
		golem.pNext = &theirs;
		theirs.pNext = &mine;
		fake::Var(Var_D2CLIENT_ClientPetData) = &golem;

		CHECK(GetClientMercUnit() == &merc);
	}

	TEST_CASE("GetClientMercUnit finds nothing without a hireling of the player's") {
		UnitAny othersMerc = Unit(UNIT_MONSTER, 51);
		fake::AddServerUnit(&othersMerc);

		SUBCASE("no pets") {
			fake::Var(Var_D2CLIENT_ClientPetData) = nullptr;
			CHECK(GetClientMercUnit() == nullptr);
		}
		SUBCASE("another player's hireling") {
			ClientPetData theirs = Pet(PETTYPE_HIREABLE, 51, 2, UNIT_PLAYER);
			fake::Var(Var_D2CLIENT_ClientPetData) = &theirs;
			CHECK(GetClientMercUnit() == nullptr);
		}
		SUBCASE("a hireling whose owner id is the player's but whose owner is not a player") {
			ClientPetData theirs = Pet(PETTYPE_HIREABLE, 51, 1, UNIT_MONSTER);
			fake::Var(Var_D2CLIENT_ClientPetData) = &theirs;
			CHECK(GetClientMercUnit() == nullptr);
		}
		SUBCASE("a dead hireling (no unit)") {
			ClientPetData mine = Pet(PETTYPE_HIREABLE, -1, 1, UNIT_PLAYER);
			fake::Var(Var_D2CLIENT_ClientPetData) = &mine;
			CHECK(GetClientMercUnit() == nullptr);
		}
		SUBCASE("no player") {
			ClientPetData mine = Pet(PETTYPE_HIREABLE, 51, 1, UNIT_PLAYER);
			fake::Var(Var_D2CLIENT_ClientPetData) = &mine;
			fake::Var(Var_D2CLIENT_PlayerUnit) = nullptr;
			CHECK(GetClientMercUnit() == nullptr);
		}
	}

	TEST_CASE("Print says nothing while any part of the player's position is missing") {
		LoadedGame game(MAP_A2_LUT_GHOLEIN);
		UnitAny& player = fake::Player();

		SUBCASE("no path") { player.pPath = nullptr; }
		SUBCASE("no room") { game.path.pRoom1 = nullptr; }
		SUBCASE("no room data") { game.room1.pRoom2 = nullptr; }
		SUBCASE("no level") { game.room2.pLevel = nullptr; }
		SUBCASE("level 0") { game.level.dwLevelNo = 0; }
		SUBCASE("no act") { player.pAct = nullptr; }
		SUBCASE("act without rooms") { game.act.pRoom1 = nullptr; }
		SUBCASE("no inventory") { player.pInventory = nullptr; }
		SUBCASE("x = 0") { game.path.xPos = 0; }
		SUBCASE("y = 0") { game.path.yPos = 0; }

		char format[] = "hello";
		Print(format);
		CHECK(fake::Printed().empty());
	}

	TEST_CASE("Print formats a message into the chat once the game is ready") {
		LoadedGame game;
		char format[] = "%d items for %s";
		Print(format, 3, "you");
		REQUIRE(fake::Printed().size() == 1);
		CHECK(fake::Printed()[0].first == L"3 items for you");
		CHECK(fake::Printed()[0].second == 0);
	}

	TEST_CASE("Print converts UTF-8 text, including colour codes") {
		LoadedGame game;
		char format[] = "ÿc1Red ÿc0Über";
		Print(format);
		REQUIRE(fake::Printed().size() == 1);
		CHECK(fake::Printed()[0].first == L"\u00FF" L"c1Red \u00FF" L"c0\u00DCber");
	}

	TEST_CASE("Print says nothing before the game is ready") {
		char format[] = "hello";
		Print(format);
		CHECK(fake::Printed().empty());
	}

	TEST_CASE("GetSkillRecord returns Skills.txt rows within the table only") {
		std::unique_ptr<sgptDataTable> table(new sgptDataTable());
		std::memset(table.get(), 0, sizeof(sgptDataTable));
		SkillsTxt skills[3];
		std::memset(skills, 0, sizeof(skills));
		table->pSkillsTxt = skills;
		table->dwSkillsRecs = 3;
		fake::Var(Var_D2COMMON_sgptDataTable) = table.get();

		CHECK(GetSkillRecord(0) == &skills[0]);
		CHECK(GetSkillRecord(2) == &skills[2]);
		CHECK(GetSkillRecord(3) == nullptr);
		CHECK(GetSkillRecord(-1) == nullptr);

		table->pSkillsTxt = nullptr;
		CHECK(GetSkillRecord(0) == nullptr);
	}
}
