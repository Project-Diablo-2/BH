// Modules/ScreenInfo: the lines shown under the automap ("automap info" in BH.json, with %GAMENAME%,
// %GAMEPASS%, %LEVEL%, %AREALEVEL%, %PING%... filled in, and lines whose value is empty left out),
// and the experience meter (progress through the current level and gained since joining).
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "BH.h"
#include "Constants.h"
#include "D2DataTables.h"
#include "D2Ptrs.h"
#include "FakeEngine.h"
#include "Modules/ScreenInfo/ScreenInfo.h"

namespace {

const DWORD kWorldstoneKeep2 = 129;
const DWORD kBloodMoor = 2;
const DWORD kColdPlains = 3;

// A player standing in `levelNo` of a Battle.net game, with the automap info lines set to `lines`.
struct Automap {
	GameStructInfo gameInfo{};
	BnetData bnet;
	Path path{};
	Room1 room1{};
	Room2 room2{};
	Level level{};
	std::unique_ptr<sgptDataTable> tables;
	std::vector<LevelsTxt> levels;
	ScreenInfo module;

	Automap(DWORD levelNo, std::vector<std::string> lines) : tables(new sgptDataTable()), levels(200) {
		std::memset(&bnet, 0, sizeof(bnet));
		strcpy_s(bnet.szGameName, "Baal-7");
		strcpy_s(bnet.szGamePass, "pw");
		strcpy_s(bnet.szGameIP, "10.0.0.5");
		strcpy_s(bnet.szAccountName, "acct");
		bnet.nCharFlags = PLAYER_TYPE_EXPANSION;
		fake::Var(Var_D2CLIENT_GameInfo) = &gameInfo;
		fake::Var(Var_D2LAUNCH_BnData) = &bnet;

		UnitAny& player = fake::Player();
		strcpy_s(player.pPlayerData->szName, "Hero");
		player.pPath = &path;
		path.pRoom1 = &room1;
		room1.pRoom2 = &room2;
		room2.pLevel = &level;
		level.dwLevelNo = levelNo;

		std::memset(tables.get(), 0, sizeof(sgptDataTable));
		std::memset(levels.data(), 0, levels.size() * sizeof(LevelsTxt));
		tables->pLevelsTxt = levels.data();
		fake::Var(Var_D2COMMON_sgptDataTable) = tables.get();

		fake::Var(Var_D2CLIENT_Ping) = 42;
		fake::Var(Var_D2CLIENT_ScreenSizeX) = 800;
		fake::Var(Var_D2CLIENT_ScreenSizeY) = 600;
		fake::SetLevelName(kWorldstoneKeep2, L"Worldstone Keep Level 2");
		fake::SetLevelName(kBloodMoor, L"Blood Moor");
		fake::SetLevelName(kColdPlains, L"Cold Plains");
		fake::SetLevelName(MAP_A5_HARROGATH, L"Harrogath");
		fake::SetDifficulty(2);

		App.screen.automapInfo.values = lines;
		module.OnGameJoin();
	}

	// Monster level of a level in each difficulty: classic and expansion characters.
	void SetMonsterLevel(DWORD levelNo, WORD classic, WORD expansion) {
		for (int difficulty = 0; difficulty < 3; difficulty++) {
			levels[levelNo].wMonLvl[difficulty] = static_cast<WORD>(classic + difficulty);
			levels[levelNo].wMonLvlEx[difficulty] = static_cast<WORD>(expansion + difficulty);
		}
	}

	std::vector<std::wstring> Draw() {
		size_t before = fake::Drawn().size();
		module.OnAutomapDraw();
		std::vector<std::wstring> lines;
		for (size_t i = before; i < fake::Drawn().size(); i++) {
			lines.push_back(fake::Drawn()[i].text);
		}
		return lines;
	}
};

std::vector<std::wstring> W(std::initializer_list<const wchar_t*> values) {
	return std::vector<std::wstring>(values.begin(), values.end());
}

// A player who joined the game at `level` with `exp` experience.
struct ExperienceMeter {
	BnetData bnet;
	ScreenInfo module;
	int quests = 0;

	ExperienceMeter(int level, DWORD exp) {
		std::memset(&bnet, 0, sizeof(bnet));
		fake::Var(Var_D2LAUNCH_BnData) = &bnet;
		fake::SetQuestInfo(&quests);
		fake::Var(Var_D2CLIENT_ScreenSizeX) = 800;
		fake::Var(Var_D2CLIENT_ScreenSizeY) = 600;
		App.game.experienceMeter.value = true;
		fake::SetStat(&fake::Player(), STAT_LEVEL, level);
		fake::SetStat(&fake::Player(), STAT_EXP, static_cast<int>(exp));
		module.OnGameJoin();
		// The quest state is known (packet 0x52), so the meter does not ask the server for it.
		std::vector<BYTE> questPacket(64, 1);
		questPacket[0] = 0x52;
		bool block = false;
		module.OnGamePacketRecv(questPacket.data(), &block);
	}

	// The meter text after reaching `level` with `exp`, up to the experience rate.
	std::wstring Show(int level, DWORD exp) {
		fake::SetStat(&fake::Player(), STAT_LEVEL, level);
		fake::SetStat(&fake::Player(), STAT_EXP, static_cast<int>(exp));
		size_t before = fake::Drawn().size();
		module.OnDraw();
		if (fake::Drawn().size() != before + 1) {
			return L"<nothing drawn>";
		}
		std::wstring text = fake::Drawn().back().text;
		return text.substr(0, text.find(L" ["));
	}
};

}  // namespace

TEST_SUITE("ScreenInfo") {
	TEST_CASE("automap info fills in the game, character and connection details") {
		Automap automap(kWorldstoneKeep2, {
			"Name: %GAMENAME%",
			"Password: %GAMEPASS%",
			"%GAMEDIFF%",
			"Ping: %PING%",
			"%CHARNAME%@%ACCOUNTNAME% on %GAMEIP%",
		});
		CHECK(automap.Draw() == W({
			L"Name: Baal-7",
			L"Password: pw",
			L"Hell",
			L"Ping: 42",
			L"Hero@acct on 10.0.0.5",
		}));
	}

	TEST_CASE("automap info names each difficulty") {
		Automap automap(kWorldstoneKeep2, { "%GAMEDIFF%" });
		fake::SetDifficulty(0);
		CHECK(automap.Draw() == W({ L"Normal" }));
		fake::SetDifficulty(1);
		CHECK(automap.Draw() == W({ L"Nightmare" }));
	}

	TEST_CASE("lines without tokens are shown as written") {
		Automap automap(kWorldstoneKeep2, { "gl hf", "100% magic find" });
		CHECK(automap.Draw() == W({ L"gl hf", L"100% magic find" }));
	}

	TEST_CASE("a line whose token has no value is left out, without leaving a gap") {
		Automap automap(kWorldstoneKeep2, { "Name: %GAMENAME%", "Password: %GAMEPASS%", "Ping: %PING%" });
		automap.bnet.szGamePass[0] = 0;
		CHECK(automap.Draw() == W({ L"Name: Baal-7", L"Ping: 42" }));
		REQUIRE(fake::Drawn().size() == 2);
		CHECK(fake::Drawn()[1].y - fake::Drawn()[0].y == 16);
	}

	TEST_CASE("the game password is masked when hideGamePassword is on") {
		Automap automap(kWorldstoneKeep2, { "Password: %GAMEPASS%" });
		App.screen.hideGamePassword.value = true;
		CHECK(automap.Draw() == W({ L"Password: **" }));

		// No password: still nothing to show.
		automap.bnet.szGamePass[0] = 0;
		CHECK(automap.Draw().empty());
	}

	TEST_CASE("the level line shows the area name and its monster level for an expansion character") {
		Automap automap(kWorldstoneKeep2, { "%LEVEL%", "Area level: %AREALEVEL%" });
		automap.SetMonsterLevel(kWorldstoneKeep2, 40, 83);  // hell: 42 classic, 85 expansion
		CHECK(automap.Draw() == W({ L"Worldstone Keep Level 2 (85)", L"Area level: 85" }));
	}

	TEST_CASE("the level line uses the classic monster level for a classic character") {
		Automap automap(kBloodMoor, { "%LEVEL%" });
		automap.bnet.nCharFlags = 0;
		automap.SetMonsterLevel(kBloodMoor, 1, 60);
		fake::SetDifficulty(1);
		CHECK(automap.Draw() == W({ L"Blood Moor (2)" }));
	}

	TEST_CASE("towns have no monster level: the level line is the name alone and the area level line is left out") {
		Automap automap(MAP_A5_HARROGATH, { "%LEVEL%", "Area level: %AREALEVEL%" });
		CHECK(automap.Draw() == W({ L"Harrogath" }));
	}

	TEST_CASE("a corrupted zone shows in purple at monster level 85") {
		Automap automap(kBloodMoor, { "%LEVEL%", "%AREALEVEL%" });
		automap.SetMonsterLevel(kBloodMoor, 60, 65);
		fake::Player().pPlayerData->nCorruptZone = 1;  // Blood Moor + Den of Evil
		CHECK(automap.Draw() == W({ L"\u00FF" L"c;Blood Moor (85)", L"85" }));
	}

	TEST_CASE("areas outside the corrupted zone keep their own monster level") {
		Automap automap(kColdPlains, { "%LEVEL%" });
		automap.SetMonsterLevel(kColdPlains, 60, 65);
		fake::Player().pPlayerData->nCorruptZone = 1;  // Blood Moor + Den of Evil
		CHECK(automap.Draw() == W({ L"Cold Plains (67)" }));
	}

	TEST_CASE("Allocated Loot is shown under the info lines when loot allocation is on") {
		Automap automap(kWorldstoneKeep2, { "%GAMENAME%" });
		fake::Player().pPlayerData->nItemAllocation = 1;
		CHECK(automap.Draw() == W({ L"Baal-7", L"Allocated Loot" }));
		fake::Player().pPlayerData->nItemAllocation = 0;
		CHECK(automap.Draw() == W({ L"Baal-7" }));
	}

	TEST_CASE("automap info is right-aligned 10px from the screen edge") {
		Automap automap(kWorldstoneKeep2, { "Ping: %PING%" });
		automap.Draw();
		REQUIRE(fake::Drawn().size() == 1);
		CHECK(fake::Drawn()[0].x + static_cast<int>(8 * fake::Drawn()[0].text.size()) == 800 - 10);
	}

	TEST_CASE("experience table gives the experience at which each level starts") {
		CHECK(ExpByLevel[0] == 0);              // level 1
		CHECK(ExpByLevel[1] == 500);            // level 2
		CHECK(ExpByLevel[2] == 1500);
		CHECK(ExpByLevel[3] == 3750);
		CHECK(ExpByLevel[4] == 7875);
		CHECK(ExpByLevel[97] == 3229426756LL);  // level 98
		CHECK(ExpByLevel[98] == 3520485254LL);  // level 99
	}

	TEST_CASE("experience meter shows progress through the level and the gain since joining") {
		ExperienceMeter meter(2, 500);  // level 2 just reached (500 .. 1500)
		CHECK(meter.Show(2, 500) == L"0.00% (+0.00%)");
		CHECK(meter.Show(2, 1000) == L"50.00% (+50.00%)");
		CHECK(meter.Show(2, 1490) == L"99.00% (+99.00%)");
	}

	TEST_CASE("experience meter counts whole levels gained since joining") {
		ExperienceMeter meter(2, 1000);  // 50% into level 2
		// Level 3 runs 1500 .. 3750: 1950 is 20% in. Gained: the other 50% of level 2, plus 20%.
		CHECK(meter.Show(3, 1950) == L"20.00% (+70.00%)");
		// Level 4 starts at 3750: the rest of level 2, all of level 3.
		CHECK(meter.Show(4, 3750) == L"0.00% (+150.00%)");
	}

	TEST_CASE("experience meter is hidden when turned off or while the help screen is open") {
		ExperienceMeter meter(2, 500);
		fake::SetUIVar(UI_HELP_MENU, 1);
		CHECK(meter.Show(2, 1000) == L"<nothing drawn>");
		fake::SetUIVar(UI_HELP_MENU, 0);
		App.game.experienceMeter.value = false;
		CHECK(meter.Show(2, 1000) == L"<nothing drawn>");
	}
}
