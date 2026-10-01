// The character stats panel (Drawing/Stats/StatsDisplay.cpp): the FCR/FHR breakpoint tables and the
// breakpoint line it prints, the weapon class it derives from the equipped weapon (which picks the
// Paladin spear/staff and Druid one-hand FHR tables), the act used for the experience penalty, and
// how the panel is placed, opened and closed.
//
// Expected breakpoints come from the Project Diablo 2 wiki (https://wiki.projectdiablo2.com/wiki/Breakpoints);
// weapon rows from the game's Weapons.txt order (txt row = dwTxtFileNo).
#include <cstring>
#include <map>
#include <new>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "BH.h"
#include "Constants.h"
#include "Drawing/Stats/StatsDisplay.h"
#include "FakeEngine.h"
#include "Modules/Item/Item.h"

// The breakpoint tables StatsDisplay.cpp draws from (globals in that file), keyed by character class,
// mercenary monster id, or one of the file's alias keys below.
extern std::map<DWORD, std::vector<int>> faster_hit_recovery_frames;
extern std::map<DWORD, std::vector<int>> faster_cast_rate_frames;

namespace {

using Drawing::StatsDisplay;

// StatsDisplay.cpp's alias keys for the shape-shifted / weapon-specific tables.
const DWORD kWolfForm = 139;
const DWORD kBearForm = 140;
const DWORD kDruidOneHandSwinging = 141;
const DWORD kPaladinSpearOrStaff = 142;
const DWORD kSorceressSlowSpells = 143;

std::vector<int> V(std::initializer_list<int> values) {
	return std::vector<int>(values);
}

// A stats panel on a width x height screen (800x600 by default).
//
// StatsDisplay's constructor reads members it has not set yet: it calls SetXSize(300), which checks
// the size against GetX(), before x is ever assigned, and LoadConfig's SetYSize keeps the old
// (uninitialised) height when it rejects a new one. In the game those members hold whatever the
// heap had. Here the panel is built in zeroed storage so that the tests are deterministic; this
// stands in for the missing initialisation and so hides that constructor bug (see the 800x600
// should_fail below for the user-visible half of it).
//
// The panel is never destroyed: the constructor stores `this` in the static StatsDisplay::display
// that BH's draw/click/key hooks use, so the object must outlive the test.
struct Panel {
	StatsDisplay* display;

	explicit Panel(DWORD width = 800, DWORD height = 600) {
		fake::Var(Var_D2CLIENT_ScreenSizeX) = width;
		fake::Var(Var_D2CLIENT_ScreenSizeY) = height;
		void* storage = ::operator new(sizeof(StatsDisplay));
		std::memset(storage, 0, sizeof(StatsDisplay));
		display = new (storage) StatsDisplay("Stats");
	}
	StatsDisplay* operator->() {
		return display;
	}
};

// The breakpoint line for a player with `value` of `stat`.
std::string BreakpointLine(int stat, int value, const std::vector<int>& table) {
	Panel panel;
	fake::SetStat(&fake::Player(), stat, value);
	char line[256] = "";
	panel->GetBreakpointString(&fake::Player(), stat, table, line);
	return line;
}

// AllStatList entries for stat ids 0..count-1, removed again when the test ends.
struct StatNames {
	std::vector<StatProperties> props;
	explicit StatNames(int count) : props(count) {
		for (int i = 0; i < count; i++) {
			props[i].name = L"stat" + std::to_wstring(i);
			props[i].statId = static_cast<unsigned short>(i);
		}
		AllStatList.clear();
		for (auto& p : props) {
			AllStatList.push_back(&p);
		}
	}
	~StatNames() {
		AllStatList.clear();
	}
};

}  // namespace

TEST_SUITE("StatsDisplay") {
	TEST_CASE("FCR breakpoint tables match the PD2 cast-rate breakpoints for every class") {
		CHECK(faster_cast_rate_frames.at(CLASS_AMA) == V({ 7, 14, 22, 32, 48, 68, 99, 152 }));
		CHECK(faster_cast_rate_frames.at(CLASS_ASN) == V({ 8, 16, 27, 42, 65, 102, 174 }));
		CHECK(faster_cast_rate_frames.at(CLASS_BAR) == V({ 9, 20, 37, 63, 105, 200 }));
		CHECK(faster_cast_rate_frames.at(CLASS_DRU) == V({ 4, 10, 19, 30, 46, 68, 99, 163 }));
		CHECK(faster_cast_rate_frames.at(CLASS_NEC) == V({ 9, 18, 30, 48, 75, 125 }));
		CHECK(faster_cast_rate_frames.at(CLASS_PAL) == V({ 9, 18, 30, 48, 75, 125 }));
		CHECK(faster_cast_rate_frames.at(CLASS_SOR) == V({ 9, 20, 37, 63, 105, 200 }));
	}

	TEST_CASE("FCR tables for wereforms, slow sorceress spells and casting mercenaries match PD2") {
		CHECK(faster_cast_rate_frames.at(kBearForm) == V({ 7, 15, 26, 40, 63, 99, 163 }));
		CHECK(faster_cast_rate_frames.at(kWolfForm) == V({ 6, 14, 26, 40, 60, 95, 157 }));
		// Chain Lightning / Frozen Orb
		CHECK(faster_cast_rate_frames.at(kSorceressSlowSpells) == V({ 7, 15, 23, 35, 52, 78, 117, 194 }));
		CHECK(faster_cast_rate_frames.at(MERC_A3) == V({ 8, 15, 26, 39, 58, 86, 138 }));
		CHECK(faster_cast_rate_frames.at(MERC_A4) == V({ 9, 18, 30, 48, 75, 125 }));
	}

	TEST_CASE("FHR breakpoint tables match the PD2 hit-recovery breakpoints for every class") {
		CHECK(faster_hit_recovery_frames.at(CLASS_AMA) == V({ 6, 13, 20, 32, 52, 86, 174, 600 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_ASN) == V({ 7, 15, 27, 48, 86, 200 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_BAR) == V({ 7, 15, 27, 48, 86, 200 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_DRU) == V({ 5, 10, 16, 26, 39, 56, 86, 152, 377 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_NEC) == V({ 5, 10, 16, 26, 39, 56, 86, 152, 377 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_PAL) == V({ 7, 15, 27, 48, 86, 200 }));
		CHECK(faster_hit_recovery_frames.at(CLASS_SOR) == V({ 5, 9, 14, 20, 30, 42, 60, 86, 142, 280 }));
	}

	TEST_CASE("FHR tables for wereforms, druid one-hand swinging weapons and mercenaries match PD2") {
		CHECK(faster_hit_recovery_frames.at(kBearForm) == V({ 5, 10, 16, 24, 37, 54, 86, 152, 360 }));
		CHECK(faster_hit_recovery_frames.at(kWolfForm) == V({ 9, 20, 42, 86, 280 }));
		CHECK(faster_hit_recovery_frames.at(kDruidOneHandSwinging) == V({ 3, 7, 13, 19, 29, 42, 63, 99, 174, 456 }));
		CHECK(faster_hit_recovery_frames.at(MERC_A1) == V({ 6, 13, 20, 32, 52, 86, 174, 600 }));
		CHECK(faster_hit_recovery_frames.at(MERC_A2) == V({ 5, 9, 14, 20, 30, 42, 60, 86, 142, 280 }));
		CHECK(faster_hit_recovery_frames.at(MERC_A3) == V({ 5, 8, 13, 18, 24, 32, 46, 63, 86, 133, 232, 600 }));
	}

	// Vanilla Diablo II's Paladin spear/staff table (13 frames): 0/3/7/13/20/32/48/75/129/280. The PD2
	// wiki's row shows 120 for the 5-frame breakpoint, but the same page says PD2 changed no FHR
	// breakpoints, and only 129 is consistent with the 3% and 280% breakpoints of a 13-frame animation.
	TEST_CASE("FHR table for a Paladin holding a spear or staff is the vanilla 13-frame table") {
		CHECK(faster_hit_recovery_frames.at(kPaladinSpearOrStaff) == V({ 3, 7, 13, 20, 32, 48, 75, 129, 280 }));
	}

	// BUG: the Act 4 mercenary (Ascendant) uses the Druid wolf-form FHR table (7 frames: 9/20/42/86/280).
	// The PD2 wiki lists the Ascendant with a 13-frame hit recovery whose breakpoints start at 3% and
	// number nine (0/3/7/13/20/32/48/75/1xx/280, the Paladin spear/staff row). Only the certain part is
	// asserted: the wiki's 120 for the 5-frame breakpoint is unverified (vanilla 13-frame tables say 129).
	TEST_CASE("FHR table for the Act 4 mercenary is a 13-frame table" * doctest::should_fail()) {
		const std::vector<int>& table = faster_hit_recovery_frames.at(MERC_A4);
		CHECK(table.size() == 9);
		CHECK(table.front() == 3);
	}

	TEST_CASE("breakpoint line highlights the highest breakpoint reached") {
		const std::vector<int> sorc = { 9, 20, 37, 63, 105, 200 };
		// Below the first breakpoint nothing is highlighted.
		CHECK(BreakpointLine(STAT_FASTERCAST, 0, sorc) == "9 / 20 / 37 / 63 / 105 / 200");
		CHECK(BreakpointLine(STAT_FASTERCAST, 8, sorc) == "9 / 20 / 37 / 63 / 105 / 200");
		// Exactly on a breakpoint, and one short of the next.
		CHECK(BreakpointLine(STAT_FASTERCAST, 9, sorc) == "ÿc89ÿc0 / 20 / 37 / 63 / 105 / 200");
		CHECK(BreakpointLine(STAT_FASTERCAST, 62, sorc) == "9 / 20 / ÿc837ÿc0 / 63 / 105 / 200");
		CHECK(BreakpointLine(STAT_FASTERCAST, 63, sorc) == "9 / 20 / 37 / ÿc863ÿc0 / 105 / 200");
		CHECK(BreakpointLine(STAT_FASTERCAST, 199, sorc) == "9 / 20 / 37 / 63 / ÿc8105ÿc0 / 200");
	}

	TEST_CASE("breakpoint line highlights the last breakpoint once it is reached or exceeded") {
		const std::vector<int> sorc = { 9, 20, 37, 63, 105, 200 };
		CHECK(BreakpointLine(STAT_FASTERCAST, 200, sorc) == "9 / 20 / 37 / 63 / 105 / ÿc8200");
		CHECK(BreakpointLine(STAT_FASTERCAST, 350, sorc) == "9 / 20 / 37 / 63 / 105 / ÿc8200");
	}

	TEST_CASE("breakpoint line reads the stat it is asked for") {
		const std::vector<int> pal = { 7, 15, 27, 48, 86, 200 };
		Panel panel;
		fake::SetStat(&fake::Player(), STAT_FASTERCAST, 100);
		fake::SetStat(&fake::Player(), STAT_FASTERHITRECOVERY, 27);
		char line[256] = "";
		panel->GetBreakpointString(&fake::Player(), STAT_FASTERHITRECOVERY, pal, line);
		CHECK(std::string(line) == "7 / 15 / ÿc827ÿc0 / 48 / 86 / 200");
	}

	TEST_CASE("weapon class follows the Weapons.txt rows of normal, exceptional and elite bases") {
		// Normal tier
		CHECK(StatsDisplay::GetCurrentWeaponType(0) == WeaponType::kAxe);         // hax Hand Axe
		CHECK(StatsDisplay::GetCurrentWeaponType(4) == WeaponType::kAxe);         // wax War Axe
		CHECK(StatsDisplay::GetCurrentWeaponType(5) == WeaponType::kAxe2H);       // lax Large Axe
		CHECK(StatsDisplay::GetCurrentWeaponType(9) == WeaponType::kAxe2H);       // gix Giant Axe
		CHECK(StatsDisplay::GetCurrentWeaponType(10) == WeaponType::kWand);       // wnd Wand
		CHECK(StatsDisplay::GetCurrentWeaponType(14) == WeaponType::kClub);       // clb Club
		CHECK(StatsDisplay::GetCurrentWeaponType(15) == WeaponType::kScepter);    // scp Scepter
		CHECK(StatsDisplay::GetCurrentWeaponType(18) == WeaponType::kClub);       // spc Spiked Club
		CHECK(StatsDisplay::GetCurrentWeaponType(19) == WeaponType::kMace);       // mac Mace
		CHECK(StatsDisplay::GetCurrentWeaponType(22) == WeaponType::kHammer);     // whm War Hammer
		CHECK(StatsDisplay::GetCurrentWeaponType(23) == WeaponType::kHammer2H);   // mau Maul
		CHECK(StatsDisplay::GetCurrentWeaponType(25) == WeaponType::kSword);      // ssd Short Sword
		CHECK(StatsDisplay::GetCurrentWeaponType(32) == WeaponType::kSword);      // wsd War Sword
		CHECK(StatsDisplay::GetCurrentWeaponType(33) == WeaponType::kSword2H);    // 2hs Two-Handed Sword
		CHECK(StatsDisplay::GetCurrentWeaponType(39) == WeaponType::kKnife);      // dgr Dagger
		CHECK(StatsDisplay::GetCurrentWeaponType(43) == WeaponType::kThrowing);   // tkf Throwing Knife
		CHECK(StatsDisplay::GetCurrentWeaponType(47) == WeaponType::kJavelin);    // jav Javelin
		CHECK(StatsDisplay::GetCurrentWeaponType(52) == WeaponType::kSpear);      // spr Spear
		CHECK(StatsDisplay::GetCurrentWeaponType(57) == WeaponType::kPole);       // bar Bardiche
		CHECK(StatsDisplay::GetCurrentWeaponType(63) == WeaponType::kStaff);      // sst Short Staff
		CHECK(StatsDisplay::GetCurrentWeaponType(68) == WeaponType::kBow);        // sbw Short Bow
		CHECK(StatsDisplay::GetCurrentWeaponType(76) == WeaponType::kCrossbow);   // lxb Light Crossbow
		CHECK(StatsDisplay::GetCurrentWeaponType(80) == WeaponType::kThrowingPot); // gps Rancid Gas Potion
		// Quest weapons
		CHECK(StatsDisplay::GetCurrentWeaponType(87) == WeaponType::kKnife);      // g33 The Gidbinn
		CHECK(StatsDisplay::GetCurrentWeaponType(88) == WeaponType::kClub);       // leg Wirt's Leg
		CHECK(StatsDisplay::GetCurrentWeaponType(89) == WeaponType::kHammer);     // hdm Horadric Malus
		CHECK(StatsDisplay::GetCurrentWeaponType(91) == WeaponType::kStaff);      // hst Horadric Staff
		CHECK(StatsDisplay::GetCurrentWeaponType(173) == WeaponType::kMace);      // qf1 Khalim's Flail
		CHECK(StatsDisplay::GetCurrentWeaponType(174) == WeaponType::kMace);      // qf2 Khalim's Will
		// Exceptional and elite tiers
		CHECK(StatsDisplay::GetCurrentWeaponType(93) == WeaponType::kAxe);        // 9ha Hatchet
		CHECK(StatsDisplay::GetCurrentWeaponType(145) == WeaponType::kSpear);     // 9sr War Spear
		CHECK(StatsDisplay::GetCurrentWeaponType(156) == WeaponType::kStaff);     // 8ss Jo Staff
		CHECK(StatsDisplay::GetCurrentWeaponType(196) == WeaponType::kAxe);       // 7ha Tomahawk
		CHECK(StatsDisplay::GetCurrentWeaponType(210) == WeaponType::kClub);      // 7cl Truncheon
		CHECK(StatsDisplay::GetCurrentWeaponType(248) == WeaponType::kSpear);     // 7sr Hyperion Spear
		CHECK(StatsDisplay::GetCurrentWeaponType(259) == WeaponType::kStaff);     // 6ss Walking Stick
		CHECK(StatsDisplay::GetCurrentWeaponType(275) == WeaponType::kCrossbow);  // 6rx Demon Crossbow
	}

	TEST_CASE("weapon class of claws, orbs and Amazon weapons") {
		for (int row = 175; row <= 195; row++) {  // ktr Katar .. 7qr Scissors Suwayyah
			CAPTURE(row);
			WeaponType type = StatsDisplay::GetCurrentWeaponType(row);
			CHECK((type == WeaponType::kClaw1 || type == WeaponType::kClaw2));
		}
		CHECK(StatsDisplay::GetCurrentWeaponType(276) == WeaponType::kOrb);       // ob1 Eagle Orb
		CHECK(StatsDisplay::GetCurrentWeaponType(281) == WeaponType::kAmaBow);    // am1 Stag Bow
		CHECK(StatsDisplay::GetCurrentWeaponType(283) == WeaponType::kAmaSpear);  // am3 Maiden Spear
		CHECK(StatsDisplay::GetCurrentWeaponType(285) == WeaponType::kAmaJav);    // am5 Maiden Javelin
		CHECK(StatsDisplay::GetCurrentWeaponType(300) == WeaponType::kOrb);       // obf Dimensional Shard
		CHECK(StatsDisplay::GetCurrentWeaponType(305) == WeaponType::kAmaJav);    // amf Matriarchal Javelin
	}

	TEST_CASE("weapon class is unknown outside the Weapons.txt rows") {
		CHECK(StatsDisplay::GetCurrentWeaponType(-1) == WeaponType::kUnknown);
		CHECK(StatsDisplay::GetCurrentWeaponType(306) == WeaponType::kUnknown);
		CHECK(StatsDisplay::GetCurrentWeaponType(1000) == WeaponType::kUnknown);
	}

	TEST_CASE("act index for the experience penalty follows the act boundaries") {
		// Act 1: Rogue Encampment .. Cow Level
		CHECK(StatsDisplay::GetActIndex(MAP_A1_ROGUE_ENCAMPMENT, 0) == 0);
		CHECK(StatsDisplay::GetActIndex(39, 0) == 0);
		// Act 2: Lut Gholein .. Arcane Sanctuary / Duriel's Lair
		CHECK(StatsDisplay::GetActIndex(MAP_A2_LUT_GHOLEIN, 0) == 1);
		CHECK(StatsDisplay::GetActIndex(MAP_A2_ARCANE_SANCTUARY, 0) == 1);
		CHECK(StatsDisplay::GetActIndex(MAP_A3_KURAST_DOCKS - 1, 0) == 1);
		// Act 3: Kurast Docks .. Durance of Hate 3
		CHECK(StatsDisplay::GetActIndex(MAP_A3_KURAST_DOCKS, 0) == 2);
		CHECK(StatsDisplay::GetActIndex(MAP_A3_DURANCE_OF_HATE_LEVEL_3, 0) == 2);
		// Act 4: Pandemonium Fortress .. Chaos Sanctuary
		CHECK(StatsDisplay::GetActIndex(MAP_A4_THE_PANDEMONIUM_FORTRESS, 0) == 3);
		CHECK(StatsDisplay::GetActIndex(MAP_A4_THE_CHAOS_SANCTUARY, 0) == 3);
		// Act 5: Harrogath .. Worldstone Chamber, Pandemonium event areas and Uber Tristram
		CHECK(StatsDisplay::GetActIndex(MAP_A5_HARROGATH, 0) == 4);
		CHECK(StatsDisplay::GetActIndex(MAP_A5_WORLDSTONE_KEEP, 0) == 4);
		CHECK(StatsDisplay::GetActIndex(MAP_A5_FURNACE_OF_PAIN, 0) == 4);
		CHECK(StatsDisplay::GetActIndex(MAP_A5_TRISTRAM, 0) == 4);
	}

	TEST_CASE("act index counts five acts per difficulty") {
		CHECK(StatsDisplay::GetActIndex(MAP_A1_ROGUE_ENCAMPMENT, 1) == 5);
		CHECK(StatsDisplay::GetActIndex(MAP_A1_ROGUE_ENCAMPMENT, 2) == 10);
		CHECK(StatsDisplay::GetActIndex(MAP_A3_KURAST_DOCKS, 1) == 7);
		CHECK(StatsDisplay::GetActIndex(MAP_A5_HARROGATH, 2) == 14);
		CHECK(StatsDisplay::GetActIndex(MAP_A5_TRISTRAM, 2) == 14);
	}

	TEST_CASE("act index is unknown when the area is unknown") {
		CHECK(StatsDisplay::GetActIndex(MAP_UNKNOWN, 0) == -1);
		CHECK(StatsDisplay::GetActIndex(MAP_UNKNOWN, 2) == -1);
	}

	TEST_CASE("panel sits 10px from the left edge by default and 10px from the right edge with statsOnRight") {
		{
			Panel panel;
			CHECK(panel->GetX() == 10);
			CHECK(panel->GetY() == 10);
		}
		App.general.statsOnRight.value = true;
		Panel panel;
		CHECK(panel->GetX() + panel->GetXSize() == 800 - 10);
		CHECK(panel->GetY() == 10);
	}

	TEST_CASE("the panel grows by one 16px row per extra stat from the config, plus one 8px gap") {
		StatNames names(10);
		STAT_MAX = 10;
		int base;
		{
			Panel panel(1024, 768);
			base = panel->GetYSize();
		}
		App.screen.additionalStats.values["7"] = "";
		{
			Panel panel(1024, 768);
			CHECK(panel->GetYSize() == base + 16 + 8);
		}
		App.screen.additionalStats.values[" 9 "] = "1";  // STAT_MAX - 1, the last stat id
		Panel panel(1024, 768);
		CHECK(panel->GetYSize() == base + 2 * 16 + 8);
	}

	TEST_CASE("extra stats that are not stat ids are ignored") {
		StatNames names(10);
		STAT_MAX = 10;
		int base;
		{
			Panel panel(1024, 768);
			base = panel->GetYSize();
		}
		App.screen.additionalStats.values["life"] = "";
		App.screen.additionalStats.values["11"] = "";  // STAT_MAX + 1
		App.screen.additionalStats.values["-1"] = "";
		Panel panel(1024, 768);
		CHECK(panel->GetYSize() == base);
	}

	// BUG: on the game's 800x600 screen, two extra stats make the computed height (558 + 2*16 + 8 = 598)
	// taller than the space below the panel (600 - 10), and SetYSize silently refuses it instead of
	// clamping. The panel keeps its previous height, which on a fresh panel was never set (0 here;
	// whatever the heap held in the game), so the box no longer covers the stat rows it draws and
	// clicking on them does not close it.
	TEST_CASE("with two extra stats on an 800x600 screen the open panel still covers its rows" * doctest::should_fail()) {
		StatNames names(10);
		STAT_MAX = 10;
		App.screen.additionalStats.values["7"] = "";
		App.screen.additionalStats.values["8"] = "";
		Panel panel(800, 600);
		panel->SetMinimized(false);
		CHECK(panel->OnClick(false, panel->GetX() + 5, panel->GetY() + 500));
		CHECK(panel->IsMinimized());
	}

	TEST_CASE("clicking inside the open panel closes it and swallows the click") {
		Panel panel;
		panel->SetMinimized(false);
		unsigned int right = panel->GetX() + panel->GetXSize();
		unsigned int bottom = panel->GetY() + panel->GetYSize();

		CHECK_FALSE(panel->OnClick(false, right + 1, panel->GetY()));
		CHECK_FALSE(panel->OnClick(false, panel->GetX(), bottom + 1));
		CHECK_FALSE(panel->IsMinimized());

		CHECK(panel->OnClick(false, right, bottom));
		CHECK(panel->IsMinimized());
	}

	TEST_CASE("clicks pass through while the panel is closed") {
		Panel panel;
		panel->SetMinimized(true);
		CHECK_FALSE(panel->OnClick(false, panel->GetX() + 5, panel->GetY() + 5));
		CHECK(panel->IsMinimized());
	}

	TEST_CASE("Escape closes the open panel and is swallowed; other keys pass through") {
		Panel panel;
		panel->SetMinimized(false);
		CHECK_FALSE(panel->OnKey(false, 'A', 0));
		CHECK_FALSE(panel->OnKey(true, VK_ESCAPE, 0));
		CHECK_FALSE(panel->IsMinimized());

		CHECK(panel->OnKey(false, VK_ESCAPE, 0));
		CHECK(panel->IsMinimized());

		// Closed: Escape belongs to the game again.
		CHECK_FALSE(panel->OnKey(false, VK_ESCAPE, 0));
	}
}
