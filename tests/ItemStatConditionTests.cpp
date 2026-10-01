#include "doctest/doctest.h"

#include <cstring>
#include <string>

#include "Constants.h"
#include "FakeEngine.h"
#include "LootFilter.h"

// Stat-based loot filter conditions (ItemDisplay.cpp): STAT/CHARSTAT/MULTI, the named stat codes,
// skills, resists, ED/EDEF/EDAM, durability, requirements, upgrades, base damage/block, max
// sockets, MINDMG/MAXDMG and "+" sums. Stat ids come from Constants.h (the game's
// ItemStatCost.txt order); rules and expected values from the PD2 filter documentation
// (https://wiki.projectdiablo2.com/wiki/Item_Filtering) and Diablo II's item rules.

using support::Matches;
using support::TestItem;

namespace {

// A 4-character item code as the game stores it in Weapons/Armor/Misc.txt (space padded).
DWORD Code(const char* code) {
	char buf[4] = { ' ', ' ', ' ', ' ' };
	std::memcpy(buf, code, (std::min)(std::strlen(code), sizeof(buf)));
	DWORD value;
	std::memcpy(&value, buf, sizeof(value));
	return value;
}

std::wstring Widen(const char* s) {
	return std::wstring(s, s + std::strlen(s));
}

// Txt file numbers for upgrade records, far away from the ones TestItem hands out.
const DWORD kExceptionalTxt = 60001;
const DWORD kEliteTxt = 60002;

// The Hand Axe line from Weapons.txt: hax (normal) -> 9ha Hatchet -> 7ha Tomahawk.
void LinkHandAxeCodes(ItemsTxt& txt) {
	txt.dwnormcode = Code("hax");
	txt.dwubercode = Code("9ha");
	txt.dwultracode = Code("7ha");
}

void AddHandAxeUpgrades() {
	ItemsTxt& hatchet = fake::AddItemTxt(kExceptionalTxt, "9ha");
	LinkHandAxeCodes(hatchet);
	hatchet.wreqstr = 25;
	hatchet.wreqdex = 25;
	hatchet.blevelreq = 19;
	ItemsTxt& tomahawk = fake::AddItemTxt(kEliteTxt, "7ha");
	LinkHandAxeCodes(tomahawk);
	tomahawk.wreqstr = 125;
	tomahawk.wreqdex = 67;
	tomahawk.blevelreq = 40;
}

}  // namespace

TEST_SUITE("ItemStatConditions") {

// ---- STAT<id> / MULTI / comparison operators ---------------------------------------------------

TEST_CASE("STAT<id> compares the item's stat with =, <, > and ~ at exact thresholds") {
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_FASTERCAST, 10);

	CHECK(Matches(L"STAT105=10", ring));
	CHECK_FALSE(Matches(L"STAT105=9", ring));
	CHECK_FALSE(Matches(L"STAT105=11", ring));

	CHECK(Matches(L"STAT105>9", ring));
	CHECK_FALSE(Matches(L"STAT105>10", ring));
	CHECK(Matches(L"STAT105<11", ring));
	CHECK_FALSE(Matches(L"STAT105<10", ring));

	// BETWEEN is inclusive at both ends.
	CHECK(Matches(L"STAT105~10-20", ring));
	CHECK(Matches(L"STAT105~5-10", ring));
	CHECK_FALSE(Matches(L"STAT105~11-20", ring));
	CHECK_FALSE(Matches(L"STAT105~1-9", ring));
}

TEST_CASE("STAT<id> on a stat the item lacks compares as zero") {
	TestItem ring("rin", ITEM_QUALITY_MAGIC);
	CHECK(Matches(L"STAT80=0", ring));
	CHECK_FALSE(Matches(L"STAT80>0", ring));
}

TEST_CASE("STAT<id> accepts negative thresholds for negative stats") {
	// "Requirements -20%" is stat 91 with value -20.
	TestItem armor("xtp", ITEM_QUALITY_UNIQUE);
	armor.Stat(STAT_REDUCEDREQUIREMENTS, -20);
	CHECK(Matches(L"STAT91=-20", armor));
	CHECK(Matches(L"STAT91<-10", armor));
	CHECK_FALSE(Matches(L"STAT91<-20", armor));
	CHECK_FALSE(Matches(L"STAT91>0", armor));
}

// How the id-bound tests observe whether a condition was evaluated, without asserting what a
// rejected condition does to the rest of the rule: the item is unique and carries the stat value
// the condition asks for, so "SET OR <cond>" matches only if <cond> was evaluated, and
// "UNI <cond-asking-for-another-value>" fails only if it was evaluated.

TEST_CASE("STAT<id> reads ids inside the ItemStatCost table and not ids past it") {
	// STAT_MAX is the number of ItemStatCost.txt rows (Item.cpp), so valid ids are 0..STAT_MAX-1.
	TestItem charm("cm3", ITEM_QUALITY_UNIQUE);
	charm.Stat(358, 7);
	charm.Stat(360, 7);
	charm.Stat(361, 7);

	STAT_MAX = 359;
	CHECK(Matches(L"SET OR STAT358=7", charm));
	CHECK_FALSE(Matches(L"UNI STAT358=8", charm));
	CHECK_FALSE(Matches(L"SET OR STAT360=7", charm));

	// STAT_MAX follows the game's table at runtime (PD2 has more rows than 1.13c).
	STAT_MAX = 362;
	CHECK(Matches(L"SET OR STAT361=7", charm));
	CHECK_FALSE(Matches(L"UNI STAT361=8", charm));
}

TEST_CASE("MULTI<stat>,<layer> reads one layer of a multi-layered stat") {
	// Wiki examples: MULTI107,20=3 (Thunderstroke, +3 Lightning Bolt), MULTI83,2=2 (Hellfire Torch).
	TestItem jav("7tw", ITEM_QUALITY_UNIQUE);
	jav.Stat(STAT_SINGLESKILL, 3, 20);
	jav.Stat(STAT_SINGLESKILL, 1, 21);

	CHECK(Matches(L"MULTI107,20=3", jav));
	CHECK(Matches(L"MULTI107,21=1", jav));
	CHECK_FALSE(Matches(L"MULTI107,20>3", jav));
	CHECK(Matches(L"MULTI107,22=0", jav));
	// The layer is part of the identity: layer 0 is a different stat entry.
	CHECK_FALSE(Matches(L"MULTI107,0>0", jav));
}

TEST_CASE("a MULTI stat id too large for an int does not abort loading the filter"
	* doctest::should_fail()) {
	// BUG: COND_MULTI accepts up to 10 digits per number ([0-9]{1,10}) and converts them with
	// std::stoi, which throws std::out_of_range for 9999999999. Nothing catches it, so one typo
	// aborts Condition::BuildConditions and ItemDisplay::InitializeItemRules, i.e. the whole filter
	// load, while every other malformed number is simply rejected by the parser. The valid rule
	// comes first and the bad one cannot match a ring (SET), so the expectation holds however the
	// bad condition ends up being rejected.
	TestItem ring("rin", ITEM_QUALITY_RARE);
	support::LoadFilter(
		"ItemDisplay[rin]: Found %NAME%\n"
		"ItemDisplay[SET MULTI9999999999,0=1]: Bad %NAME%\n");
	CHECK(support::NameOf(ring, L"Ring") == L"Found Ring");
}

// ---- Named stat codes ------------------------------------------------------------------------

TEST_CASE("each named stat code reads its own stat and no other") {
	struct Named {
		const char* code;
		int stat;
		int raw;  // the value as stored on the item for a displayed value of 17
	};
	// Stat ids: Constants.h / ItemStatCost.txt. Life and mana are stored in 1/256 units.
	const Named named[] = {
		{ "FRES", STAT_FIRERESIST, 17 },
		{ "CRES", STAT_COLDRESIST, 17 },
		{ "LRES", STAT_LIGHTNINGRESIST, 17 },
		{ "PRES", STAT_POISONRESIST, 17 },
		{ "IAS", STAT_IAS, 17 },
		{ "FCR", STAT_FASTERCAST, 17 },
		{ "FHR", STAT_FASTERHITRECOVERY, 17 },
		{ "FBR", STAT_FASTERBLOCK, 17 },
		{ "FRW", STAT_FASTERRUNWALK, 17 },
		{ "MFIND", STAT_MAGICFIND, 17 },
		{ "GFIND", STAT_GOLDFIND, 17 },
		{ "STR", STAT_STRENGTH, 17 },
		{ "DEX", STAT_DEXTERITY, 17 },
		{ "AR", STAT_ATTACKRATING, 17 },
		{ "ARPER", STAT_TOHITPERCENT, 17 },
		{ "DTM", STAT_DAMAGETOMANA, 17 },
		{ "MAEK", STAT_MANAAFTEREACHKILL, 17 },
		{ "REPLIFE", STAT_REPLENISHLIFE, 17 },
		{ "REPQUANT", STAT_REPLENISHESQUANTITY, 17 },
		{ "REPAIR", STAT_REPAIRSDURABILITY, 17 },
		{ "QTY", STAT_AMMOQUANTITY, 17 },
		{ "DEF", STAT_DEFENSE, 17 },
		{ "EDEF", STAT_ENHANCEDDEFENSE, 17 },
		{ "EDAM", STAT_ENHANCEDMAXIMUMDAMAGE, 17 },
		{ "ALLSK", STAT_ALLSKILLS, 17 },
		{ "LIFE", STAT_MAXHP, 17 * 256 },
		{ "MANA", STAT_MAXMANA, 17 * 256 },
	};
	for (const Named& n : named) {
		INFO("item carries only the stat for ", n.code);
		TestItem item("amu", ITEM_QUALITY_RARE);
		item.Stat(n.stat, n.raw);
		const std::wstring code = Widen(n.code);
		CHECK(Matches(code + L"=17", item));
		CHECK(Matches(code + L">16", item));
		CHECK_FALSE(Matches(code + L">17", item));
		CHECK(Matches(code + L"~17-17", item));
		for (const Named& other : named) {
			if (other.stat == n.stat) {
				continue;
			}
			INFO("reading ", other.code);
			CHECK_FALSE(Matches(Widen(other.code) + L">0", item));
		}
	}
}

TEST_CASE("LIFE and MANA compare whole points although the game stores them in 1/256 units") {
	TestItem amulet("amu", ITEM_QUALITY_RARE);
	amulet.Stat(STAT_MAXHP, 30 * 256);
	amulet.Stat(STAT_MAXMANA, 45 * 256);

	CHECK(Matches(L"LIFE=30", amulet));
	CHECK(Matches(L"LIFE>29", amulet));
	CHECK_FALSE(Matches(L"LIFE>30", amulet));
	CHECK(Matches(L"LIFE~20-30", amulet));
	CHECK(Matches(L"LIFE~30-40", amulet));
	CHECK_FALSE(Matches(L"LIFE~31-40", amulet));
	CHECK_FALSE(Matches(L"LIFE~10-29", amulet));

	CHECK(Matches(L"MANA=45", amulet));
	CHECK(Matches(L"MANA<46", amulet));
	CHECK_FALSE(Matches(L"MANA<45", amulet));

	// STAT7/STAT9 are the same stats and use the same whole-point scale as LIFE/MANA
	// (and as the %STAT7%/%STAT9% keywords, which divide by 256).
	CHECK(Matches(L"STAT7=30", amulet));
	CHECK(Matches(L"STAT9=45", amulet));
}

// ---- Skills ----------------------------------------------------------------------------------

TEST_CASE("SK, OS, CLSK, TABSK and ALLSK read the matching skill stats") {
	TestItem item("7tw", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_SINGLESKILL, 3, 20);  // +3 Lightning Bolt (Thunderstroke, wiki: SK20=3)
	item.Stat(STAT_NONCLASSSKILL, 20, 74);  // +20 Corpse Explosion oskill (Corpsemourn, OS74=20)
	item.Stat(STAT_CLASSSKILLS, 2, 2);  // +2 Necromancer skills (Hellfire Torch, CLSK2=2)
	item.Stat(STAT_SKILLTAB, 5, 25);  // +5 Offensive Auras (Cloudcrack, TABSK25>4)
	item.Stat(STAT_ALLSKILLS, 1);

	CHECK(Matches(L"SK20=3", item));
	CHECK_FALSE(Matches(L"SK20>3", item));
	CHECK(Matches(L"SK21=0", item));

	CHECK(Matches(L"OS74=20", item));
	CHECK_FALSE(Matches(L"OS20>0", item));  // single skill +3 is not an oskill

	CHECK(Matches(L"CLSK2=2", item));
	CHECK_FALSE(Matches(L"CLSK1>0", item));

	CHECK(Matches(L"TABSK25>4", item));
	CHECK_FALSE(Matches(L"TABSK25>5", item));
	CHECK_FALSE(Matches(L"TABSK24>0", item));

	CHECK(Matches(L"ALLSK=1", item));
	CHECK_FALSE(Matches(L"ALLSK>1", item));
}

TEST_CASE("skill codes read ids inside their tables and not ids past them") {
	// SKILL_MAX is the number of Skills.txt rows (Item.cpp), so valid skill ids are 0..SKILL_MAX-1.
	TestItem item("amu", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_SINGLESKILL, 1, 356);
	item.Stat(STAT_SINGLESKILL, 1, 358);
	item.Stat(STAT_NONCLASSSKILL, 1, 356);
	item.Stat(STAT_NONCLASSSKILL, 1, 358);
	item.Stat(STAT_CLASSSKILLS, 1, 6);
	item.Stat(STAT_CLASSSKILLS, 1, 7);
	item.Stat(STAT_SKILLTAB, 1, 50);
	item.Stat(STAT_SKILLTAB, 1, 51);
	item.Stat(STAT_CHARGED, 1, 356 * 64 + 1);
	item.Stat(STAT_CHARGED, 1, 358 * 64 + 1);

	SKILL_MAX = 357;
	CHECK(Matches(L"SET OR SK356=1", item));
	CHECK_FALSE(Matches(L"UNI SK356=2", item));
	CHECK_FALSE(Matches(L"SET OR SK358=1", item));
	CHECK(Matches(L"SET OR OS356=1", item));
	CHECK_FALSE(Matches(L"UNI OS356=2", item));
	CHECK_FALSE(Matches(L"SET OR OS358=1", item));
	CHECK(Matches(L"SET OR CHSK356=1", item));
	CHECK_FALSE(Matches(L"UNI CHSK356=2", item));
	CHECK_FALSE(Matches(L"SET OR CHSK358=1", item));

	// Seven classes: ids 0 (Amazon) .. 6 (Assassin).
	CHECK(Matches(L"SET OR CLSK6=1", item));
	CHECK_FALSE(Matches(L"UNI CLSK6=2", item));
	CHECK_FALSE(Matches(L"SET OR CLSK7=1", item));

	// Skill tabs run up to 50 (Assassin Martial Arts).
	CHECK(Matches(L"SET OR TABSK50=1", item));
	CHECK_FALSE(Matches(L"UNI TABSK50=2", item));
	CHECK_FALSE(Matches(L"SET OR TABSK51=1", item));
}

TEST_CASE("CHSK compares the highest level of charges for that skill") {
	// Charged skills are stat 204 with layer (skill_id * 64) + skill_level.
	TestItem wand("wnd", ITEM_QUALITY_UNIQUE);
	wand.Stat(STAT_CHARGED, 20, 54 * 64 + 3);  // Teleport level 3
	wand.Stat(STAT_CHARGED, 5, 54 * 64 + 7);  // Teleport level 7
	wand.Stat(STAT_CHARGED, 30, 53 * 64 + 20);  // Chain Lightning level 20 (another skill)

	CHECK(Matches(L"CHSK54=7", wand));
	CHECK(Matches(L"CHSK54>6", wand));
	CHECK_FALSE(Matches(L"CHSK54>7", wand));
	CHECK(Matches(L"CHSK53=20", wand));
	CHECK(Matches(L"CHSK52=0", wand));
	// A plain +skill is not a charge.
	TestItem orb("ob1", ITEM_QUALITY_MAGIC);
	orb.Stat(STAT_SINGLESKILL, 3, 54);
	CHECK_FALSE(Matches(L"CHSK54>0", orb));
}

// ---- Character stats -------------------------------------------------------------------------

TEST_CASE("CHARSTAT<id> reads the character's stats, not the item's") {
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_STRENGTH, 5);
	fake::SetStat(&fake::Player(), STAT_STRENGTH, 120);
	fake::SetStat(&fake::Player(), STAT_LEVEL, 75);

	CHECK(Matches(L"CHARSTAT0=120", ring));
	CHECK(Matches(L"CHARSTAT0>119", ring));
	CHECK_FALSE(Matches(L"CHARSTAT0>120", ring));
	CHECK_FALSE(Matches(L"CHARSTAT0=5", ring));
	CHECK(Matches(L"CHARSTAT12~70-75", ring));
	CHECK_FALSE(Matches(L"CHARSTAT12~76-99", ring));
	// The item's own +5 strength is STAT0, unaffected by the character.
	CHECK(Matches(L"STAT0=5", ring));
}

TEST_CASE("CHARSTAT reads ids inside the ItemStatCost table and not ids past it") {
	TestItem ring("rin", ITEM_QUALITY_RARE);
	fake::SetStat(&fake::Player(), 358, 4);
	fake::SetStat(&fake::Player(), 360, 4);
	STAT_MAX = 359;
	CHECK(Matches(L"SET OR CHARSTAT358=4", ring));
	CHECK_FALSE(Matches(L"RARE CHARSTAT358=5", ring));
	CHECK_FALSE(Matches(L"SET OR CHARSTAT360=4", ring));
}

// ---- Resistances -----------------------------------------------------------------------------

TEST_CASE("RES requires every one of the four resistances to pass") {
	TestItem uniform("amu", ITEM_QUALITY_RARE);
	uniform.Stat(STAT_FIRERESIST, 25);
	uniform.Stat(STAT_COLDRESIST, 25);
	uniform.Stat(STAT_LIGHTNINGRESIST, 25);
	uniform.Stat(STAT_POISONRESIST, 25);
	CHECK(Matches(L"RES=25", uniform));
	CHECK(Matches(L"RES>24", uniform));
	CHECK_FALSE(Matches(L"RES>25", uniform));
	CHECK(Matches(L"RES~20-25", uniform));
	CHECK_FALSE(Matches(L"RES~26-30", uniform));

	// RES>N requires each resistance to exceed N: one resistance at 29 fails RES>29.
	TestItem lowPoison("amu", ITEM_QUALITY_RARE);
	lowPoison.Stat(STAT_FIRERESIST, 30);
	lowPoison.Stat(STAT_COLDRESIST, 30);
	lowPoison.Stat(STAT_LIGHTNINGRESIST, 30);
	lowPoison.Stat(STAT_POISONRESIST, 29);
	CHECK(Matches(L"RES>28", lowPoison));
	CHECK_FALSE(Matches(L"RES>29", lowPoison));

	// A single resistance is not all resistances.
	TestItem fireOnly("amu", ITEM_QUALITY_RARE);
	fireOnly.Stat(STAT_FIRERESIST, 40);
	CHECK_FALSE(Matches(L"RES>0", fireOnly));
	CHECK(Matches(L"FRES=40", fireOnly));
}

TEST_CASE("MAXRES is the lowest of the four maximum resistances, 0 unless all four are present") {
	TestItem amulet("amu", ITEM_QUALITY_UNIQUE);
	amulet.Stat(STAT_MAXFIRERESIST, 5);
	amulet.Stat(STAT_MAXCOLDRESIST, 5);
	amulet.Stat(STAT_MAXLIGHTNINGRESIST, 5);
	amulet.Stat(STAT_MAXPOISONRESIST, 4);
	CHECK(Matches(L"MAXRES=4", amulet));
	CHECK_FALSE(Matches(L"MAXRES>4", amulet));

	TestItem fireMax("amu", ITEM_QUALITY_UNIQUE);
	fireMax.Stat(STAT_MAXFIRERESIST, 5);
	fireMax.Stat(STAT_MAXCOLDRESIST, 5);
	fireMax.Stat(STAT_MAXLIGHTNINGRESIST, 5);
	CHECK(Matches(L"MAXRES=0", fireMax));
	CHECK_FALSE(Matches(L"MAXRES>0", fireMax));
}

TEST_CASE("ALLATTRIB is the lowest of the four attributes, 0 unless all four are present") {
	TestItem charm("cm1", ITEM_QUALITY_UNIQUE);
	charm.Stat(STAT_STRENGTH, 20);
	charm.Stat(STAT_DEXTERITY, 15);
	charm.Stat(STAT_VITALITY, 20);
	charm.Stat(STAT_ENERGY, 20);
	CHECK(Matches(L"ALLATTRIB=15", charm));
	CHECK(Matches(L"ALLATTRIB>14", charm));
	CHECK_FALSE(Matches(L"ALLATTRIB>15", charm));

	// Strength, dexterity and vitality without energy is not "+X to all attributes".
	TestItem noEnergy("cm1", ITEM_QUALITY_UNIQUE);
	noEnergy.Stat(STAT_STRENGTH, 20);
	noEnergy.Stat(STAT_DEXTERITY, 20);
	noEnergy.Stat(STAT_VITALITY, 20);
	CHECK(Matches(L"ALLATTRIB=0", noEnergy));
	CHECK_FALSE(Matches(L"ALLATTRIB>0", noEnergy));
}

// ---- Enhanced defense / damage, durability ----------------------------------------------------

TEST_CASE("ED reads enhanced defense on armor and enhanced damage on weapons") {
	TestItem helm("uap", ITEM_QUALITY_MAGIC);
	helm.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM;
	helm.Stat(STAT_ENHANCEDDEFENSE, 100);
	helm.Stat(STAT_ENHANCEDMAXIMUMDAMAGE, 40);
	CHECK(Matches(L"ED=100", helm));
	CHECK(Matches(L"ED>99", helm));
	CHECK_FALSE(Matches(L"ED>100", helm));

	TestItem sword("7cr", ITEM_QUALITY_MAGIC);
	sword.Stat(STAT_ENHANCEDMAXIMUMDAMAGE, 200);
	sword.Stat(STAT_ENHANCEDMINIMUMDAMAGE, 200);
	sword.Stat(STAT_ENHANCEDDEFENSE, 30);
	CHECK(Matches(L"ED=200", sword));
	CHECK(Matches(L"ED~150-200", sword));
	CHECK_FALSE(Matches(L"ED~201-300", sword));
}

TEST_CASE("EDEF and EDAM include runeword bonuses but ED does not") {
	// Wiki: EDEF/EDAM include bonuses from runewords and sockets; ED does not include these.
	TestItem armor("xtp", ITEM_QUALITY_SUPERIOR);
	armor.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_BODY_ARMOR;
	armor.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	armor.Stat(STAT_ENHANCEDDEFENSE, 15);
	fake::SetRunewordStat(armor.unit(), STAT_ENHANCEDDEFENSE, 200);

	CHECK(Matches(L"EDEF=215", armor));
	CHECK_FALSE(Matches(L"EDEF>215", armor));
	CHECK(Matches(L"ED=15", armor));

	TestItem weapon("7cr", ITEM_QUALITY_SUPERIOR);
	weapon.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	weapon.Stat(STAT_ENHANCEDMAXIMUMDAMAGE, 10);
	fake::SetRunewordStat(weapon.unit(), STAT_ENHANCEDMAXIMUMDAMAGE, 300);
	CHECK(Matches(L"EDAM=310", weapon));
	CHECK(Matches(L"ED=10", weapon));
}

TEST_CASE("runeword state stats count only on items flagged as runewords") {
	TestItem armor("xtp", ITEM_QUALITY_SUPERIOR);
	armor.Stat(STAT_ENHANCEDDEFENSE, 15);
	fake::SetRunewordStat(armor.unit(), STAT_ENHANCEDDEFENSE, 200);
	CHECK(Matches(L"EDEF=15", armor));

	armor.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	CHECK(Matches(L"EDEF=215", armor));
}

TEST_CASE("EDEF includes enhanced defense from socketed items but ED does not") {
	TestItem jewel("jew", ITEM_QUALITY_MAGIC);
	jewel.Stat(STAT_ENHANCEDDEFENSE, 15);
	TestItem rune("r09", ITEM_QUALITY_NORMAL);
	rune.Stat(STAT_ENHANCEDDEFENSE, 0);  // a socketed item without the stat adds nothing

	Inventory sockets = {};
	sockets.pFirstItem = jewel.unit();
	jewel.data().pNextInvItem = rune.unit();

	TestItem armor("xtp", ITEM_QUALITY_MAGIC);
	armor.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_BODY_ARMOR;
	armor.unit()->pInventory = &sockets;
	armor.Stat(STAT_SOCKETS, 2);
	armor.Stat(STAT_ENHANCEDDEFENSE, 50);

	CHECK(Matches(L"EDEF=65", armor));
	CHECK(Matches(L"ED=50", armor));
}

TEST_CASE("MAXDUR reads Increase Maximum Durability %") {
	TestItem armor("xtp", ITEM_QUALITY_MAGIC);
	armor.Stat(STAT_ENHANCEDMAXDURABILITY, 25);
	CHECK(Matches(L"MAXDUR=25", armor));
	CHECK(Matches(L"MAXDUR>24", armor));
	CHECK_FALSE(Matches(L"MAXDUR>25", armor));
	CHECK(Matches(L"MAXDUR~20-30", armor));

	TestItem plain("xtp", ITEM_QUALITY_NORMAL);
	plain.Stat(STAT_MAXDURABILITY, 50);  // plain durability is not the percentage bonus
	CHECK(Matches(L"MAXDUR=0", plain));
}

// ---- MINDMG / MAXDMG ---------------------------------------------------------------------------

TEST_CASE("MINDMG and MAXDMG take the largest of the one-hand, two-hand and throwing bonuses") {
	TestItem axe("7ga", ITEM_QUALITY_RARE);
	axe.Stat(STAT_MINIMUMDAMAGE, 5);
	axe.Stat(STAT_SECONDARYMINIMUMDAMAGE, 8);
	axe.Stat(STAT_MAXIMUMDAMAGE, 12);
	axe.Stat(STAT_SECONDARYMAXIMUMDAMAGE, 10);

	CHECK(Matches(L"MINDMG=8", axe));
	CHECK_FALSE(Matches(L"MINDMG>8", axe));
	CHECK(Matches(L"MAXDMG=12", axe));
	CHECK(Matches(L"MAXDMG~12-15", axe));

	TestItem jav("7ja", ITEM_QUALITY_RARE);
	jav.Stat(STAT_MINIMUMDAMAGE, 3);
	jav.Stat(STAT_MINIMUMTHROWINGDAMAGE, 9);
	jav.Stat(STAT_MAXIMUMTHROWINGDAMAGE, 20);
	CHECK(Matches(L"MINDMG=9", jav));
	CHECK(Matches(L"MAXDMG=20", jav));

	TestItem ring("rin", ITEM_QUALITY_RARE);
	CHECK(Matches(L"MINDMG=0", ring));
	CHECK_FALSE(Matches(L"MAXDMG>0", ring));
}

TEST_CASE("MAXDMG includes runeword bonuses") {
	TestItem sword("7cr", ITEM_QUALITY_SUPERIOR);
	sword.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	sword.Stat(STAT_MAXIMUMDAMAGE, 2);
	fake::SetRunewordStat(sword.unit(), STAT_MAXIMUMDAMAGE, 10);
	CHECK(Matches(L"MAXDMG=12", sword));
}

// ---- Base damage, block, sockets -------------------------------------------------------------

TEST_CASE("BASE damage codes read the base item's damage for its own damage type") {
	TestItem axe("7ga", ITEM_QUALITY_NORMAL);  // Great Axe-like two-hander
	axe.txt().b2handmindam = 24;
	axe.txt().b2handmaxdam = 38;
	CHECK(Matches(L"BASEMINTWOH=24", axe));
	CHECK(Matches(L"BASEMAXTWOH=38", axe));
	CHECK_FALSE(Matches(L"BASEMAXTWOH>38", axe));
	CHECK(Matches(L"BASEMINONEH=0", axe));

	TestItem jav("jav", ITEM_QUALITY_NORMAL);  // Javelin: 1-14 melee, 6-22 thrown
	jav.txt().bmindam = 1;
	jav.txt().bmaxdam = 14;
	jav.txt().bminmisdam = 6;
	jav.txt().bmaxmisdam = 22;
	CHECK(Matches(L"BASEMINONEH=1", jav));
	CHECK(Matches(L"BASEMAXONEH=14", jav));
	CHECK(Matches(L"BASEMINTHROW=6", jav));
	CHECK(Matches(L"BASEMAXTHROW=22", jav));
	CHECK(Matches(L"BASEMAXTHROW~20-22", jav));
}

TEST_CASE("smite damage comes only from shields and kick damage only from boots") {
	TestItem shield("xpk", ITEM_QUALITY_NORMAL);
	shield.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_SHIELD;
	shield.txt().bmindam = 12;
	shield.txt().bmaxdam = 16;
	CHECK(Matches(L"BASEMINSMITE=12", shield));
	CHECK(Matches(L"BASEMAXSMITE=16", shield));
	CHECK(Matches(L"BASEMINKICK=0", shield));
	// A shield's damage columns are smite damage, not one-handed weapon damage.
	CHECK(Matches(L"BASEMINONEH=0", shield));
	CHECK(Matches(L"BASEMAXONEH=0", shield));

	TestItem boots("xvb", ITEM_QUALITY_NORMAL);
	boots.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_BOOTS;
	boots.txt().bmindam = 28;
	boots.txt().bmaxdam = 35;
	CHECK(Matches(L"BASEMINKICK=28", boots));
	CHECK(Matches(L"BASEMAXKICK=35", boots));
	CHECK(Matches(L"BASEMAXSMITE=0", boots));
	CHECK(Matches(L"BASEMAXONEH=0", boots));

	TestItem sword("7cr", ITEM_QUALITY_NORMAL);
	sword.txt().bmindam = 10;
	sword.txt().bmaxdam = 20;
	CHECK(Matches(L"BASEMINSMITE=0", sword));
	CHECK(Matches(L"BASEMAXKICK=0", sword));
}

TEST_CASE("ethereal weapons get PD2's x1.25 base damage bonus") {
	// PD2 wiki (Throwing/Swords pages): ethereal weapons gain a x1.25 damage bonus instead of
	// x1.5. The bases are chosen so that 1.25 * d is a whole number and needs no rounding rule.
	TestItem axe("7ga", ITEM_QUALITY_NORMAL);
	axe.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	axe.txt().b2handmindam = 24;  // 30
	axe.txt().b2handmaxdam = 40;  // 50
	CHECK(Matches(L"BASEMINTWOH=30", axe));
	CHECK(Matches(L"BASEMAXTWOH=50", axe));

	TestItem jav("jav", ITEM_QUALITY_NORMAL);
	jav.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	jav.txt().bmindam = 4;  // 5
	jav.txt().bmaxdam = 16;  // 20
	jav.txt().bminmisdam = 8;  // 10
	jav.txt().bmaxmisdam = 24;  // 30
	CHECK(Matches(L"BASEMINONEH=5", jav));
	CHECK(Matches(L"BASEMAXONEH=20", jav));
	CHECK(Matches(L"BASEMINTHROW=10", jav));
	CHECK(Matches(L"BASEMAXTHROW=30", jav));

	// Non-ethereal items keep the base damage.
	TestItem plain("jav", ITEM_QUALITY_NORMAL);
	plain.txt().bmaxmisdam = 24;
	CHECK(Matches(L"BASEMAXTHROW=24", plain));
}

TEST_CASE("ethereal base damage is 1.25x for odd damage values too" * doctest::should_fail()) {
	// BUG: BaseWeaponDamageCondition computes ethereal damage as (d + d/2) * 5 / 6, truncating
	// d/2 before scaling, so odd bases lose up to a point against PD2's x1.25 (wiki). A Flail's
	// minimum damage 1 becomes (1 + 0) * 5 / 6 = 0, although 1 * 1.25 = 1.25 is 1 under any
	// rounding; base 5 gives 35 / 6 = 5 instead of 6 (6.25).
	TestItem flail("fla", ITEM_QUALITY_NORMAL);
	flail.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	flail.txt().bmindam = 1;
	flail.txt().bmaxdam = 24;
	CHECK(Matches(L"BASEMINONEH=1", flail));
	CHECK(Matches(L"BASEMAXONEH=30", flail));

	TestItem twoHander("7ga", ITEM_QUALITY_NORMAL);
	twoHander.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	twoHander.txt().b2handmindam = 5;
	CHECK(Matches(L"BASEMINTWOH=6", twoHander));
}

TEST_CASE("BASEBLOCK reads the base item's block chance") {
	TestItem shield("xpk", ITEM_QUALITY_NORMAL);
	shield.txt().bblock = 30;
	CHECK(Matches(L"BASEBLOCK=30", shield));
	CHECK(Matches(L"BASEBLOCK>29", shield));
	CHECK_FALSE(Matches(L"BASEBLOCK>30", shield));
	CHECK(Matches(L"BASEBLOCK~25-30", shield));
}

TEST_CASE("MAXSOCKETS is capped by the number of inventory cells the item occupies") {
	TestItem armor("xtp", ITEM_QUALITY_NORMAL);  // body armor: 2x3 = 6 cells
	armor.txt().binvwidth = 2;
	armor.txt().binvheight = 3;
	fake::SetMaxSockets(armor.unit(), 4);
	CHECK(Matches(L"MAXSOCKETS=4", armor));
	CHECK_FALSE(Matches(L"MAXSOCKETS>4", armor));

	TestItem shield("xpk", ITEM_QUALITY_NORMAL);  // 2x2 = 4 cells
	shield.txt().binvwidth = 2;
	shield.txt().binvheight = 2;
	fake::SetMaxSockets(shield.unit(), 6);
	CHECK(Matches(L"MAXSOCKETS=4", shield));

	TestItem wand("wnd", ITEM_QUALITY_NORMAL);  // 1x2 = 2 cells
	wand.txt().binvwidth = 1;
	wand.txt().binvheight = 2;
	fake::SetMaxSockets(wand.unit(), 2);
	CHECK(Matches(L"MAXSOCKETS=2", wand));
	CHECK(Matches(L"MAXSOCKETS~1-2", wand));
}

// ---- Requirements ----------------------------------------------------------------------------

TEST_CASE("REQSTR and REQDEX apply the item's requirement reduction") {
	TestItem armor("xtp", ITEM_QUALITY_MAGIC);
	armor.txt().wreqstr = 100;
	armor.txt().wreqdex = 50;
	CHECK(Matches(L"REQSTR=100", armor));
	CHECK(Matches(L"REQDEX=50", armor));

	armor.Stat(STAT_REDUCEDREQUIREMENTS, -20);  // Requirements -20%
	CHECK(Matches(L"REQSTR=80", armor));
	CHECK(Matches(L"REQDEX=40", armor));
	CHECK_FALSE(Matches(L"REQSTR>80", armor));
	CHECK(Matches(L"REQSTR<81", armor));

	TestItem heavy("xtp", ITEM_QUALITY_MAGIC);
	heavy.txt().wreqstr = 100;
	heavy.Stat(STAT_REDUCEDREQUIREMENTS, 50);  // Requirements +50%
	CHECK(Matches(L"REQSTR=150", heavy));
}

TEST_CASE("ethereal items need 10 less strength and dexterity, never below zero") {
	TestItem armor("xtp", ITEM_QUALITY_NORMAL);
	armor.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	armor.txt().wreqstr = 100;
	armor.txt().wreqdex = 0;
	CHECK(Matches(L"REQSTR=90", armor));
	CHECK(Matches(L"REQDEX=0", armor));
	CHECK_FALSE(Matches(L"REQDEX<0", armor));

	armor.Stat(STAT_REDUCEDREQUIREMENTS, -20);
	CHECK(Matches(L"REQSTR=70", armor));

	TestItem light("cap", ITEM_QUALITY_NORMAL);
	light.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	light.txt().wreqstr = 5;
	CHECK(Matches(L"REQSTR=0", light));
}

TEST_CASE("REQLVL is the item's level requirement") {
	TestItem ring("rin", ITEM_QUALITY_UNIQUE);
	fake::SetLevelRequirement(ring.unit(), 29);
	CHECK(Matches(L"REQLVL=29", ring));
	CHECK(Matches(L"REQLVL>28", ring));
	CHECK_FALSE(Matches(L"REQLVL>29", ring));
	CHECK(Matches(L"REQLVL~1-29", ring));
}

TEST_CASE("REQLVL is the requirement for the viewing character's class") {
	// A class-specific affix (e.g. +skills of one class) lowers the level requirement for that
	// class only; "required character level" is what the current character needs.
	TestItem amulet("amu", ITEM_QUALITY_MAGIC);
	fake::SetLevelRequirement(amulet.unit(), 30);
	fake::SetLevelRequirement(amulet.unit(), 24, 1);  // Sorceress

	fake::Player().dwTxtFileNo = 1;
	CHECK(Matches(L"REQLVL=24", amulet));
	fake::Player().dwTxtFileNo = 0;  // Amazon
	CHECK(Matches(L"REQLVL=30", amulet));
}

// ---- Upgrade requirements ----------------------------------------------------------------------

TEST_CASE("UPSTR, UPDEX and UPLVL give the exceptional version's requirements for a normal item") {
	AddHandAxeUpgrades();
	TestItem axe("hax", ITEM_QUALITY_MAGIC);
	LinkHandAxeCodes(axe.txt());
	fake::SetLevelRequirement(axe.unit(), 12);

	CHECK(Matches(L"UPSTR=25", axe));
	CHECK(Matches(L"UPDEX=25", axe));
	CHECK(Matches(L"UPLVL=19", axe));
	CHECK_FALSE(Matches(L"UPLVL>19", axe));
}

TEST_CASE("UPSTR, UPDEX and UPLVL give the elite version's requirements for an exceptional item") {
	AddHandAxeUpgrades();
	TestItem axe("9ha", ITEM_QUALITY_RARE);
	LinkHandAxeCodes(axe.txt());
	fake::SetLevelRequirement(axe.unit(), 30);

	CHECK(Matches(L"UPSTR=125", axe));
	CHECK(Matches(L"UPDEX=67", axe));
	CHECK(Matches(L"UPLVL=40", axe));
}

TEST_CASE("UPLVL keeps the item's own level requirement when it is higher") {
	AddHandAxeUpgrades();
	TestItem unique("hax", ITEM_QUALITY_UNIQUE);
	LinkHandAxeCodes(unique.txt());
	fake::SetLevelRequirement(unique.unit(), 30);
	CHECK(Matches(L"UPLVL=30", unique));
}

TEST_CASE("UPLVL uses the viewing character's own requirement for the current item") {
	AddHandAxeUpgrades();  // the exceptional Hatchet needs level 19
	TestItem axe("hax", ITEM_QUALITY_MAGIC);
	LinkHandAxeCodes(axe.txt());
	fake::SetLevelRequirement(axe.unit(), 30);
	fake::SetLevelRequirement(axe.unit(), 22, 4);  // Barbarian

	fake::Player().dwTxtFileNo = 4;
	CHECK(Matches(L"UPLVL=22", axe));
	fake::Player().dwTxtFileNo = 6;  // Assassin
	CHECK(Matches(L"UPLVL=30", axe));
}

TEST_CASE("upgrade requirements keep the item's requirement reduction and ethereal bonus") {
	AddHandAxeUpgrades();
	TestItem axe("9ha", ITEM_QUALITY_RARE);
	LinkHandAxeCodes(axe.txt());
	axe.Stat(STAT_REDUCEDREQUIREMENTS, -20);
	CHECK(Matches(L"UPSTR=100", axe));  // 125 - 25

	axe.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	CHECK(Matches(L"UPSTR=90", axe));  // 125 - 25 - 10
}

TEST_CASE("UPSTR, UPDEX and UPLVL are 0 when no upgrade exists") {
	AddHandAxeUpgrades();
	TestItem elite("7ha", ITEM_QUALITY_RARE);
	LinkHandAxeCodes(elite.txt());
	fake::SetLevelRequirement(elite.unit(), 40);
	CHECK(Matches(L"UPSTR=0", elite));
	CHECK(Matches(L"UPDEX=0", elite));
	CHECK(Matches(L"UPLVL=0", elite));

	TestItem ring("rin", ITEM_QUALITY_RARE);  // misc items have no upgrade codes
	fake::SetLevelRequirement(ring.unit(), 20);
	CHECK(Matches(L"UPLVL=0", ring));
	CHECK_FALSE(Matches(L"UPSTR>0", ring));
}

// ---- "+" sums --------------------------------------------------------------------------------

TEST_CASE("codes joined with + compare the sum of their values") {
	// Wiki example: FRES+CRES+LRES+PRES>79 marks rares with 80+ total resistance.
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_FIRERESIST, 30);
	ring.Stat(STAT_COLDRESIST, 20);
	ring.Stat(STAT_LIGHTNINGRESIST, 20);
	ring.Stat(STAT_POISONRESIST, 10);
	CHECK(Matches(L"FRES+CRES+LRES+PRES>79", ring));
	CHECK_FALSE(Matches(L"FRES+CRES+LRES+PRES>80", ring));
	CHECK(Matches(L"FRES+CRES+LRES+PRES=80", ring));
	CHECK(Matches(L"FRES+CRES<51", ring));
	CHECK_FALSE(Matches(L"FRES+CRES<50", ring));
}

TEST_CASE("+ sums accept STAT and MULTI terms") {
	// Wiki example: STAT60+STAT62>10 (total life and mana leech).
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_LIFELEECH, 6);
	ring.Stat(STAT_MANALEECH, 5);
	ring.Stat(STAT_SINGLESKILL, 2, 54);
	ring.Stat(STAT_SINGLESKILL, 3, 56);
	CHECK(Matches(L"STAT60+STAT62>10", ring));
	CHECK_FALSE(Matches(L"STAT60+STAT62>11", ring));
	CHECK(Matches(L"MULTI107,54+MULTI107,56=5", ring));
	CHECK(Matches(L"STAT60+MULTI107,56=9", ring));
}

TEST_CASE("+ sums use whole life/mana points and the MINDMG/MAXDMG and EDEF/EDAM semantics") {
	TestItem amulet("amu", ITEM_QUALITY_RARE);
	amulet.Stat(STAT_MAXHP, 40 * 256);
	amulet.Stat(STAT_MAXMANA, 30 * 256);
	amulet.Stat(STAT_STRENGTH, 10);
	CHECK(Matches(L"LIFE+MANA=70", amulet));
	CHECK(Matches(L"LIFE+STR>49", amulet));
	CHECK_FALSE(Matches(L"LIFE+STR>50", amulet));

	TestItem axe("7ga", ITEM_QUALITY_RARE);
	axe.Stat(STAT_MAXIMUMDAMAGE, 4);
	axe.Stat(STAT_SECONDARYMAXIMUMDAMAGE, 9);
	axe.Stat(STAT_SECONDARYMINIMUMDAMAGE, 3);
	// MAXDMG is the largest of the max damage bonuses (9), MINDMG likewise (3).
	CHECK(Matches(L"MINDMG+MAXDMG=12", axe));

	TestItem armor("xtp", ITEM_QUALITY_SUPERIOR);
	armor.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	armor.Stat(STAT_ENHANCEDDEFENSE, 10);
	armor.Stat(STAT_STRENGTH, 5);
	fake::SetRunewordStat(armor.unit(), STAT_ENHANCEDDEFENSE, 100);
	CHECK(Matches(L"EDEF+STR=115", armor));
}

TEST_CASE("+ sums support the ~ range operator like every other value condition"
	* doctest::should_fail()) {
	// BUG: BuildConditions parses both bounds of "~50-100" but passes only the first to
	// AddCondition, whose IntegerCompare then sees the range 50..0, so a "+" sum with '~' never
	// matches. The wiki documents '~' for value conditions and "+" for STAT/MULTI codes with no
	// exception for sums.
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_FIRERESIST, 30);
	ring.Stat(STAT_COLDRESIST, 30);
	CHECK(Matches(L"FRES+CRES~50-100", ring));
	CHECK(Matches(L"FRES+CRES~60-60", ring));
}

}  // TEST_SUITE
