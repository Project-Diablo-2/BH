// Loot filter condition language: item identity/property conditions and the boolean grammar
// (Condition::BuildConditions, Condition::ProcessConditions, Rule::Convert / EvaluateTree).
// Expected values follow the PD2 item filtering wiki (https://wiki.projectdiablo2.com/wiki/Item_Filtering),
// the game's data (BH/Constants.h mirrors it) and the Arreat Summit affix level formula.
#include "doctest/doctest.h"

#include <cstring>
#include <string>

#include "BH.h"
#include "Constants.h"
#include "FakeEngine.h"
#include "LootFilter.h"

using support::Matches;
using support::TestItem;

namespace {

std::wstring Wide(const char* s) {
	return std::wstring(s, s + std::strlen(s));
}

// Puts `item` into `inv`, an inventory owned by `owner`, at storage `location`.
void PlaceIn(TestItem& item, Inventory& inv, UnitAny* owner, BYTE location) {
	std::memset(&inv, 0, sizeof(inv));
	inv.pOwner = owner;
	item.data().pOwnerInventory = &inv;
	item.data().ItemLocation = location;
}

// ItemAttributes field a group keyword reads.
enum GroupField { kBase, kWeapon, kArmor, kMisc };

unsigned int& Field(TestItem& item, GroupField field) {
	switch (field) {
		case kBase: return item.attrs().baseFlags;
		case kWeapon: return item.attrs().weaponFlags;
		case kArmor: return item.attrs().armorFlags;
		default: return item.attrs().miscFlags;
	}
}

struct GroupKeyword {
	const char* name;
	GroupField field;
	unsigned int flag;
};

// Every item group keyword the wiki lists, with the ItemTypes-derived group it stands for.
const GroupKeyword kGroups[] = {
	{"NORM", kBase, ITEM_GROUP_NORMAL},
	{"EXC", kBase, ITEM_GROUP_EXCEPTIONAL},
	{"ELT", kBase, ITEM_GROUP_ELITE},
	{"CLASS", kBase, ITEM_GROUP_CLASS},
	{"WEAPON", kWeapon, ITEM_GROUP_ALLWEAPON},
	{"MACE", kWeapon, ITEM_GROUP_ALLMACE},
	{"AXE", kWeapon, ITEM_GROUP_AXE},
	{"CLUB", kWeapon, ITEM_GROUP_CLUB},
	{"TMACE", kWeapon, ITEM_GROUP_TIPPED_MACE},
	{"HAMMER", kWeapon, ITEM_GROUP_HAMMER},
	{"SWORD", kWeapon, ITEM_GROUP_SWORD},
	{"DAGGER", kWeapon, ITEM_GROUP_DAGGER},
	{"THROWING", kWeapon, ITEM_GROUP_THROWING},
	{"JAV", kWeapon, ITEM_GROUP_JAVELIN},
	{"SPEAR", kWeapon, ITEM_GROUP_SPEAR},
	{"POLEARM", kWeapon, ITEM_GROUP_POLEARM},
	{"BOW", kWeapon, ITEM_GROUP_BOW},
	{"XBOW", kWeapon, ITEM_GROUP_CROSSBOW},
	{"STAFF", kWeapon, ITEM_GROUP_STAFF},
	{"WAND", kWeapon, ITEM_GROUP_WAND},
	{"SCEPTER", kWeapon, ITEM_GROUP_SCEPTER},
	{"ZON", kWeapon, ITEM_GROUP_AMAZON_WEAPON},
	{"SIN", kWeapon, ITEM_GROUP_ASSASSIN_KATAR},
	{"SOR", kWeapon, ITEM_GROUP_SORCERESS_ORB},
	{"ARMOR", kArmor, ITEM_GROUP_ALLARMOR},
	{"HELM", kArmor, ITEM_GROUP_HELM},
	{"CHEST", kArmor, ITEM_GROUP_BODY_ARMOR},
	{"SHIELD", kArmor, ITEM_GROUP_SHIELD},
	{"GLOVES", kArmor, ITEM_GROUP_GLOVES},
	{"BOOTS", kArmor, ITEM_GROUP_BOOTS},
	{"BELT", kArmor, ITEM_GROUP_BELT},
	{"CIRC", kArmor, ITEM_GROUP_CIRCLET},
	{"BAR", kArmor, ITEM_GROUP_BARBARIAN_HELM},
	{"DRU", kArmor, ITEM_GROUP_DRUID_PELT},
	{"NEC", kArmor, ITEM_GROUP_NECROMANCER_SHIELD},
	{"DIN", kArmor, ITEM_GROUP_PALADIN_SHIELD},
	{"MISC", kMisc, ITEM_GROUP_ALLMISC},
	{"JEWELRY", kMisc, ITEM_GROUP_JEWELRY},
	{"CHARM", kMisc, ITEM_GROUP_CHARM},
	{"QUIVER", kMisc, ITEM_GROUP_QUIVER},
};

}  // namespace

TEST_SUITE("ItemConditions") {

// ---- Item codes ------------------------------------------------------------------------------

TEST_CASE("a lowercase 3-letter token is an item code that matches only that base") {
	TestItem axe("hax", ITEM_QUALITY_NORMAL);
	TestItem doubleAxe("2ax", ITEM_QUALITY_NORMAL);
	TestItem rune("r01", ITEM_QUALITY_NORMAL);

	CHECK(Matches(L"hax", axe));
	CHECK_FALSE(Matches(L"axe", axe));
	CHECK_FALSE(Matches(L"hax", doubleAxe));
	// Codes may contain digits anywhere.
	CHECK(Matches(L"2ax", doubleAxe));
	CHECK(Matches(L"r01", rune));
	CHECK_FALSE(Matches(L"r02", rune));
	// A 4th character is part of the code: "haxx" is a different code.
	CHECK_FALSE(Matches(L"haxx", axe));
	// Codes combine with other conditions.
	CHECK(Matches(L"hax NMAG", axe));
	CHECK_FALSE(Matches(L"hax MAG", axe));
}

TEST_CASE("keywords are case-sensitive where an item code and a keyword share letters") {
	// "axe" is the Axe's item code, AXE the axe group keyword; "bar" is the Bardiche's code, BAR the
	// barbarian helm keyword.
	TestItem axe("axe", ITEM_QUALITY_NORMAL);
	axe.attrs().weaponFlags = ITEM_GROUP_ALLWEAPON | ITEM_GROUP_AXE;
	TestItem handAxe("hax", ITEM_QUALITY_NORMAL);
	handAxe.attrs().weaponFlags = ITEM_GROUP_ALLWEAPON | ITEM_GROUP_AXE;
	CHECK(Matches(L"axe", axe));
	CHECK_FALSE(Matches(L"axe", handAxe));
	CHECK(Matches(L"AXE", axe));
	CHECK(Matches(L"AXE", handAxe));

	TestItem bardiche("bar", ITEM_QUALITY_NORMAL);
	bardiche.attrs().weaponFlags = ITEM_GROUP_ALLWEAPON | ITEM_GROUP_POLEARM;
	TestItem barbHelm("ba1", ITEM_QUALITY_NORMAL);
	barbHelm.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM | ITEM_GROUP_BARBARIAN_HELM;
	CHECK(Matches(L"bar", bardiche));
	CHECK_FALSE(Matches(L"bar", barbHelm));
	CHECK(Matches(L"BAR", barbHelm));
	CHECK_FALSE(Matches(L"BAR", bardiche));
}

// ---- Quality and properties ------------------------------------------------------------------

TEST_CASE("each rarity keyword matches exactly its item quality") {
	struct QualityKeyword {
		const char* name;
		DWORD quality;
	};
	const QualityKeyword keywords[] = {
		{"INF", ITEM_QUALITY_INFERIOR},
		{"SUP", ITEM_QUALITY_SUPERIOR},
		{"MAG", ITEM_QUALITY_MAGIC},
		{"SET", ITEM_QUALITY_SET},
		{"RARE", ITEM_QUALITY_RARE},
		{"UNI", ITEM_QUALITY_UNIQUE},
		{"CRAFT", ITEM_QUALITY_CRAFT},
	};
	for (DWORD quality = ITEM_QUALITY_INFERIOR; quality <= ITEM_QUALITY_CRAFT; quality++) {
		TestItem item("lsd", quality);
		CAPTURE(quality);
		for (const auto& kw : keywords) {
			INFO(kw.name);
			CHECK(Matches(Wide(kw.name), item) == (kw.quality == quality));
		}
	}
}

TEST_CASE("NMAG matches inferior, normal and superior items but nothing magical") {
	for (DWORD quality = ITEM_QUALITY_INFERIOR; quality <= ITEM_QUALITY_CRAFT; quality++) {
		TestItem item("lsd", quality);
		CAPTURE(quality);
		CHECK(Matches(L"NMAG", item) == (quality <= ITEM_QUALITY_SUPERIOR));
	}
}

TEST_CASE("NORM, EXC and ELT are base tiers, not rarities") {
	TestItem normalQualityEliteBase("7ls", ITEM_QUALITY_NORMAL);
	normalQualityEliteBase.attrs().baseFlags = ITEM_GROUP_ELITE;
	CHECK_FALSE(Matches(L"NORM", normalQualityEliteBase));
	CHECK_FALSE(Matches(L"EXC", normalQualityEliteBase));
	CHECK(Matches(L"ELT", normalQualityEliteBase));

	TestItem uniqueNormalBase("lsd", ITEM_QUALITY_UNIQUE);
	uniqueNormalBase.attrs().baseFlags = ITEM_GROUP_NORMAL;
	CHECK(Matches(L"NORM", uniqueNormalBase));
	CHECK(Matches(L"NORM UNI", uniqueNormalBase));
}

TEST_CASE("ETH, RW and ID test the item's ethereal, runeword and identified flags") {
	TestItem plain("lsd", ITEM_QUALITY_NORMAL);
	plain.Flags(0);
	CHECK_FALSE(Matches(L"ETH", plain));
	CHECK_FALSE(Matches(L"RW", plain));
	CHECK_FALSE(Matches(L"ID", plain));

	TestItem runeword("lsd", ITEM_QUALITY_NORMAL);
	runeword.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL | ITEM_RUNEWORD);
	CHECK(Matches(L"ETH", runeword));
	CHECK(Matches(L"RW", runeword));
	CHECK(Matches(L"ID", runeword));
	CHECK(Matches(L"ETH RW ID", runeword));
	CHECK_FALSE(Matches(L"!ID", runeword));
}

TEST_CASE("GEMMED matches items with something socketed into them") {
	TestItem empty("lsd", ITEM_QUALITY_NORMAL);
	CHECK_FALSE(Matches(L"GEMMED", empty));

	Inventory sockets;
	std::memset(&sockets, 0, sizeof(sockets));
	TestItem socketedNothing("lsd", ITEM_QUALITY_NORMAL);
	socketedNothing.unit()->pInventory = &sockets;
	CHECK_FALSE(Matches(L"GEMMED", socketedNothing));

	Inventory filled;
	std::memset(&filled, 0, sizeof(filled));
	filled.dwItemCount = 1;
	TestItem gemmed("lsd", ITEM_QUALITY_NORMAL);
	gemmed.unit()->pInventory = &filled;
	CHECK(Matches(L"GEMMED", gemmed));
}

TEST_CASE("FOOLS matches only items with both max damage and attack rating per level") {
	TestItem fools("lsd", ITEM_QUALITY_MAGIC);
	fools.Stat(STAT_MAXDAMAGEPERLEVEL, 8).Stat(STAT_ATTACKRATINGPERLEVEL, 16);
	CHECK(Matches(L"FOOLS", fools));

	TestItem damageOnly("lsd", ITEM_QUALITY_MAGIC);
	damageOnly.Stat(STAT_MAXDAMAGEPERLEVEL, 8);
	CHECK_FALSE(Matches(L"FOOLS", damageOnly));

	TestItem ratingOnly("lsd", ITEM_QUALITY_MAGIC);
	ratingOnly.Stat(STAT_ATTACKRATINGPERLEVEL, 16);
	CHECK_FALSE(Matches(L"FOOLS", ratingOnly));
}

// ---- Runes, gems, gold -----------------------------------------------------------------------

TEST_CASE("RUNE compares the rune number (El = 1 .. Zod = 33) and only applies to runes") {
	TestItem el("r01", ITEM_QUALITY_NORMAL);
	el.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_RUNE;
	TestItem ber("r30", ITEM_QUALITY_NORMAL);
	ber.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_RUNE;
	TestItem zod("r33", ITEM_QUALITY_NORMAL);
	zod.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_RUNE;

	CHECK(Matches(L"RUNE=1", el));
	CHECK_FALSE(Matches(L"RUNE>1", el));
	CHECK(Matches(L"RUNE=30", ber));
	CHECK(Matches(L"RUNE>29", ber));
	CHECK_FALSE(Matches(L"RUNE>30", ber));
	CHECK(Matches(L"RUNE<31", ber));
	CHECK(Matches(L"RUNE=33", zod));
	CHECK(Matches(L"RUNE~30-33", zod));
	CHECK_FALSE(Matches(L"RUNE~30-33", el));

	// Not a rune (no rune item type): never satisfies a RUNE comparison.
	TestItem notRune("r30", ITEM_QUALITY_NORMAL);
	notRune.attrs().miscFlags = ITEM_GROUP_ALLMISC;
	CHECK_FALSE(Matches(L"RUNE=30", notRune));
	CHECK_FALSE(Matches(L"RUNE<34", notRune));
}

TEST_CASE("GEMLEVEL/GEM compare gem quality 1-5 (Chipped..Perfect)") {
	const unsigned int levels[] = {ITEM_GROUP_CHIPPED, ITEM_GROUP_FLAWED, ITEM_GROUP_REGULAR, ITEM_GROUP_FLAWLESS,
		ITEM_GROUP_PERFECT};
	for (int level = 1; level <= 5; level++) {
		TestItem gem("gem", ITEM_QUALITY_NORMAL);
		gem.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_RUBY | levels[level - 1];
		CAPTURE(level);
		CHECK(Matches(L"GEMLEVEL=" + std::to_wstring(level), gem));
		CHECK(Matches(L"GEM=" + std::to_wstring(level), gem));
		CHECK_FALSE(Matches(L"GEM>" + std::to_wstring(level), gem));
		CHECK_FALSE(Matches(L"GEM<" + std::to_wstring(level), gem));
	}

	TestItem notGem("r01", ITEM_QUALITY_NORMAL);
	notGem.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_RUNE;
	CHECK_FALSE(Matches(L"GEM<6", notGem));
	CHECK_FALSE(Matches(L"GEMLEVEL~1-5", notGem));
}

TEST_CASE("GEMTYPE compares gem type 1-7 (Amethyst, Diamond, Emerald, Ruby, Sapphire, Topaz, Skull)") {
	const unsigned int types[] = {ITEM_GROUP_AMETHYST, ITEM_GROUP_DIAMOND, ITEM_GROUP_EMERALD, ITEM_GROUP_RUBY,
		ITEM_GROUP_SAPPHIRE, ITEM_GROUP_TOPAZ, ITEM_GROUP_SKULL};
	for (int type = 1; type <= 7; type++) {
		TestItem gem("gem", ITEM_QUALITY_NORMAL);
		gem.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_PERFECT | types[type - 1];
		CAPTURE(type);
		CHECK(Matches(L"GEMTYPE=" + std::to_wstring(type), gem));
		CHECK_FALSE(Matches(L"GEMTYPE=" + std::to_wstring(type % 7 + 1), gem));
		CHECK(Matches(L"GEMTYPE=" + std::to_wstring(type) + L" GEMLEVEL=5", gem));
	}

	TestItem notGem("cm1", ITEM_QUALITY_MAGIC);
	notGem.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_CHARM;
	CHECK_FALSE(Matches(L"GEMTYPE<8", notGem));
}

TEST_CASE("GOLD compares the stack size of gold piles only") {
	TestItem pile("gld", ITEM_QUALITY_NORMAL);
	pile.Stat(STAT_GOLD, 99);
	TestItem hundred("gld", ITEM_QUALITY_NORMAL);
	hundred.Stat(STAT_GOLD, 100);

	CHECK(Matches(L"GOLD<100", pile));
	CHECK_FALSE(Matches(L"GOLD<100", hundred));
	CHECK(Matches(L"GOLD=100", hundred));
	CHECK(Matches(L"GOLD>99", hundred));

	// The wiki's "ItemDisplay[GOLD<100]: hides gold stacks if they are less than 100" must not hide
	// everything else.
	TestItem sword("lsd", ITEM_QUALITY_NORMAL);
	CHECK_FALSE(Matches(L"GOLD<100", sword));
}

// ---- Item, quality and affix levels ----------------------------------------------------------

TEST_CASE("ILVL compares the item level with strict < and >, exact = and inclusive ~ ranges") {
	TestItem item("lsd", ITEM_QUALITY_RARE);
	item.ItemLevel(50);

	CHECK(Matches(L"ILVL=50", item));
	CHECK_FALSE(Matches(L"ILVL=49", item));
	CHECK(Matches(L"ILVL>49", item));
	CHECK_FALSE(Matches(L"ILVL>50", item));
	CHECK(Matches(L"ILVL<51", item));
	CHECK_FALSE(Matches(L"ILVL<50", item));
	CHECK(Matches(L"ILVL~50-60", item));
	CHECK(Matches(L"ILVL~40-50", item));
	CHECK_FALSE(Matches(L"ILVL~51-60", item));
	CHECK_FALSE(Matches(L"ILVL~40-49", item));
}

TEST_CASE("a value condition without a comparison operator never matches") {
	TestItem item("lsd", ITEM_QUALITY_RARE);
	item.ItemLevel(50);
	CHECK_FALSE(Matches(L"ILVL", item));
	CHECK_FALSE(Matches(L"RARE ILVL", item));
}

TEST_CASE("QLVL compares the base item's quality level") {
	TestItem item("xsk", ITEM_QUALITY_RARE);
	item.attrs().qualityLevel = 34;
	CHECK(Matches(L"QLVL=34", item));
	CHECK(Matches(L"QLVL>33", item));
	CHECK_FALSE(Matches(L"QLVL>34", item));
}

TEST_CASE("ALVL follows the game's affix level formula") {
	// alvl = ilvl - qlvl/2 while ilvl < 99 - qlvl/2, else 2*ilvl - 99 (ilvl raised to qlvl, capped at 99);
	// with a magic level, alvl = ilvl + mlvl capped at 99.
	TestItem low("xsk", ITEM_QUALITY_MAGIC);
	low.ItemLevel(40);
	low.attrs().qualityLevel = 20;
	CHECK(Matches(L"ALVL=30", low));
	CHECK_FALSE(Matches(L"ALVL=40", low));

	TestItem belowKnee("xsk", ITEM_QUALITY_MAGIC);
	belowKnee.ItemLevel(68);
	belowKnee.attrs().qualityLevel = 60;
	CHECK(Matches(L"ALVL=38", belowKnee));

	TestItem aboveKnee("xsk", ITEM_QUALITY_MAGIC);
	aboveKnee.ItemLevel(70);
	aboveKnee.attrs().qualityLevel = 60;
	CHECK(Matches(L"ALVL=41", aboveKnee));

	TestItem high("xsk", ITEM_QUALITY_MAGIC);
	high.ItemLevel(85);
	high.attrs().qualityLevel = 60;
	CHECK(Matches(L"ALVL=71", high));

	// Item level below the base's quality level counts as the quality level.
	TestItem underQlvl("xsk", ITEM_QUALITY_MAGIC);
	underQlvl.ItemLevel(10);
	underQlvl.attrs().qualityLevel = 30;
	CHECK(Matches(L"ALVL=15", underQlvl));

	TestItem wand("wnd", ITEM_QUALITY_MAGIC);
	wand.ItemLevel(30);
	wand.attrs().qualityLevel = 10;
	wand.attrs().magicLevel = 1;
	CHECK(Matches(L"ALVL=31", wand));

	TestItem cappedWand("wnd", ITEM_QUALITY_MAGIC);
	cappedWand.ItemLevel(99);
	cappedWand.attrs().qualityLevel = 10;
	cappedWand.attrs().magicLevel = 3;
	CHECK(Matches(L"ALVL=99", cappedWand));

	TestItem overLevel("rin", ITEM_QUALITY_MAGIC);
	overLevel.ItemLevel(120);
	CHECK(Matches(L"ALVL=99", overLevel));
}

TEST_CASE("CRAFTALVL is the affix level of a craft made by this character from the item") {
	// Crafted item level = clvl/2 + ilvl/2.
	fake::SetStat(&fake::Player(), STAT_LEVEL, 90);
	TestItem amulet("amu", ITEM_QUALITY_MAGIC);
	amulet.ItemLevel(84);
	amulet.attrs().qualityLevel = 60;  // craft ilvl 87 -> 2*87 - 99
	CHECK(Matches(L"CRAFTALVL=75", amulet));
	CHECK_FALSE(Matches(L"CRAFTALVL>75", amulet));

	fake::SetStat(&fake::Player(), STAT_LEVEL, 40);
	TestItem ring("rin", ITEM_QUALITY_MAGIC);
	ring.ItemLevel(20);  // craft ilvl 30, qlvl 0
	CHECK(Matches(L"CRAFTALVL=30", ring));
}

TEST_CASE("REROLLALVL is the affix level after the cube reroll recipe") {
	fake::SetStat(&fake::Player(), STAT_LEVEL, 90);

	// Magic items keep their item level.
	TestItem magic("xsk", ITEM_QUALITY_MAGIC);
	magic.ItemLevel(50);
	magic.attrs().qualityLevel = 20;
	CHECK(Matches(L"REROLLALVL=40", magic));

	// Rares reroll at 40% of the item level + 40% of the character level: 32 + 36 = 68.
	TestItem rare("xsk", ITEM_QUALITY_RARE);
	rare.ItemLevel(80);
	rare.attrs().qualityLevel = 40;
	CHECK(Matches(L"REROLLALVL=48", rare));

	// Items that cannot be rerolled have no reroll affix level.
	TestItem unique("xsk", ITEM_QUALITY_UNIQUE);
	unique.ItemLevel(80);
	CHECK_FALSE(Matches(L"REROLLALVL>0", unique));

	TestItem map("t11", ITEM_QUALITY_MAGIC);
	map.ItemLevel(80);
	map.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_MAP;
	CHECK_FALSE(Matches(L"REROLLALVL>0", map));

	TestItem corrupted("xsk", ITEM_QUALITY_MAGIC);
	corrupted.ItemLevel(50);
	corrupted.Stat(STAT_CORRUPTED, 1);
	CHECK_FALSE(Matches(L"REROLLALVL>0", corrupted));
}

// ---- Affixes -----------------------------------------------------------------------------------

// PREFIX/AUTOMOD values are compared after BH subtracts the affix-table offset Item.cpp computes at
// game join (PREFIX_OFFSET/AUTOMOD_OFFSET). These tests pin that subtraction with arbitrary offsets;
// they make no claim about which numbering the game's tables use.
TEST_CASE("PREFIX matches any of the item's prefixes, less the prefix table offset") {
	PREFIX_OFFSET = 700;
	TestItem item("cm3", ITEM_QUALITY_MAGIC);
	item.data().wPrefix[1] = 700 + 279;

	CHECK(Matches(L"PREFIX=279", item));
	CHECK_FALSE(Matches(L"PREFIX=979", item));
	CHECK_FALSE(Matches(L"PREFIX=280", item));
	// The wiki: ranges use ~.
	CHECK(Matches(L"PREFIX~279-353", item));
	CHECK_FALSE(Matches(L"PREFIX~280-353", item));
	// The wiki: < and > have no effect with PREFIX.
	CHECK_FALSE(Matches(L"PREFIX>1", item));
	CHECK_FALSE(Matches(L"PREFIX<1000", item));
	// Several PREFIX conditions can be required at once.
	item.data().wPrefix[2] = 700 + 12;
	CHECK(Matches(L"PREFIX=279 PREFIX=12", item));
}

TEST_CASE("SUFFIX matches any of the item's suffixes") {
	TestItem item("rin", ITEM_QUALITY_RARE);
	item.data().wSuffix[0] = 662;
	item.data().wSuffix[2] = 158;
	CHECK(Matches(L"SUFFIX=662", item));
	CHECK(Matches(L"SUFFIX=158", item));
	CHECK(Matches(L"RARE SUFFIX=662 SUFFIX=158", item));
	CHECK_FALSE(Matches(L"SUFFIX=159", item));
	CHECK_FALSE(Matches(L"SUFFIX>100", item));
	CHECK(Matches(L"SUFFIX~150-160", item));
}

TEST_CASE("affixes of unidentified rares are unknown to PREFIX and SUFFIX") {
	TestItem item("rin", ITEM_QUALITY_RARE);
	item.Flags(0);
	item.data().wPrefix[0] = 5;
	item.data().wSuffix[0] = 7;
	CHECK_FALSE(Matches(L"PREFIX=5", item));
	CHECK_FALSE(Matches(L"SUFFIX=7", item));
	item.Flags(ITEM_IDENTIFIED);
	CHECK(Matches(L"PREFIX=5", item));
	CHECK(Matches(L"SUFFIX=7", item));
}

TEST_CASE("AUTOMOD compares the automod id, less the automod table offset") {
	AUTOMOD_OFFSET = 1500;
	TestItem item("ob1", ITEM_QUALITY_MAGIC);
	item.data().wAutoPrefix = 1500 + 3;
	CHECK(Matches(L"AUTOMOD=3", item));
	CHECK_FALSE(Matches(L"AUTOMOD=1503", item));
	CHECK(Matches(L"AUTOMOD~1-5", item));

	// Not visible until a magic or rare item is identified.
	item.Flags(0);
	CHECK_FALSE(Matches(L"AUTOMOD=3", item));
}

// ---- Item groups -------------------------------------------------------------------------------

TEST_CASE("each item group keyword matches exactly its own group") {
	for (const auto& group : kGroups) {
		TestItem item("xxx", ITEM_QUALITY_NORMAL);
		Field(item, group.field) = group.flag;
		INFO(group.name);
		for (const auto& keyword : kGroups) {
			INFO(keyword.name);
			const bool same = keyword.field == group.field && keyword.flag == group.flag;
			CHECK(Matches(Wide(keyword.name), item) == same);
		}
	}
}

TEST_CASE("numbered group codes are synonyms of the named ones (EQ1-7, WP1-13, CL1-7)") {
	struct Synonym {
		const char* numbered;
		const char* named;
	};
	const Synonym synonyms[] = {
		{"EQ1", "HELM"}, {"EQ2", "CHEST"}, {"EQ3", "SHIELD"}, {"EQ4", "GLOVES"}, {"EQ5", "BOOTS"}, {"EQ6", "BELT"},
		{"EQ7", "CIRC"}, {"WP1", "AXE"}, {"WP2", "MACE"}, {"WP3", "SWORD"}, {"WP4", "DAGGER"}, {"WP5", "THROWING"},
		{"WP6", "JAV"}, {"WP7", "SPEAR"}, {"WP8", "POLEARM"}, {"WP9", "BOW"}, {"WP10", "XBOW"}, {"WP11", "STAFF"},
		{"WP12", "WAND"}, {"WP13", "SCEPTER"}, {"CL1", "DRU"}, {"CL2", "BAR"}, {"CL3", "DIN"}, {"CL4", "NEC"},
		{"CL5", "SIN"}, {"CL6", "SOR"}, {"CL7", "ZON"},
	};
	for (const auto& synonym : synonyms) {
		INFO(synonym.numbered);
		const GroupKeyword* group = nullptr;
		for (const auto& g : kGroups) {
			if (std::strcmp(g.name, synonym.named) == 0) {
				group = &g;
			}
		}
		REQUIRE(group != nullptr);
		TestItem member("xxx", ITEM_QUALITY_NORMAL);
		Field(member, group->field) = group->flag;
		TestItem other("yyy", ITEM_QUALITY_NORMAL);
		Field(other, group->field) = ~group->flag;
		CHECK(Matches(Wide(synonym.numbered), member));
		CHECK_FALSE(Matches(Wide(synonym.numbered), other));
	}
}

TEST_CASE("group keywords read weapon, armor and misc groups separately even where bits coincide") {
	// Throwing axe as Item.cpp classifies it: all weapons + axes + throwing weapons, normal tier.
	TestItem throwingAxe("tax", ITEM_QUALITY_NORMAL);
	throwingAxe.attrs().baseFlags = ITEM_GROUP_NORMAL;
	throwingAxe.attrs().weaponFlags = ITEM_GROUP_ALLWEAPON | ITEM_GROUP_AXE | ITEM_GROUP_THROWING;
	CHECK(Matches(L"WEAPON AXE THROWING NORM", throwingAxe));
	CHECK_FALSE(Matches(L"JAV", throwingAxe));
	CHECK_FALSE(Matches(L"DAGGER", throwingAxe));
	CHECK_FALSE(Matches(L"ARMOR", throwingAxe));
	CHECK_FALSE(Matches(L"MISC", throwingAxe));

	// Club: a mace subtype.
	TestItem club("clb", ITEM_QUALITY_NORMAL);
	club.attrs().weaponFlags = ITEM_GROUP_ALLWEAPON | ITEM_GROUP_ALLMACE | ITEM_GROUP_CLUB;
	CHECK(Matches(L"MACE CLUB", club));
	CHECK_FALSE(Matches(L"TMACE", club));
	CHECK_FALSE(Matches(L"HAMMER", club));
	// HELM shares its bit with MACE in the armor flags; a mace is not a helm.
	CHECK_FALSE(Matches(L"HELM", club));

	// Paladin shield: a shield and a class item.
	TestItem paladinShield("pa1", ITEM_QUALITY_NORMAL);
	paladinShield.attrs().baseFlags = ITEM_GROUP_NORMAL | ITEM_GROUP_CLASS;
	paladinShield.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_SHIELD | ITEM_GROUP_PALADIN_SHIELD;
	CHECK(Matches(L"ARMOR SHIELD DIN CLASS", paladinShield));
	CHECK_FALSE(Matches(L"NEC", paladinShield));
	CHECK_FALSE(Matches(L"WEAPON", paladinShield));
	CHECK_FALSE(Matches(L"HELM", paladinShield));

	// Grand charm: misc only.
	TestItem charm("cm3", ITEM_QUALITY_MAGIC);
	charm.attrs().baseFlags = ITEM_GROUP_NORMAL;
	charm.attrs().miscFlags = ITEM_GROUP_ALLMISC | ITEM_GROUP_CHARM;
	CHECK(Matches(L"MISC CHARM", charm));
	CHECK_FALSE(Matches(L"JEWELRY", charm));
	CHECK_FALSE(Matches(L"WEAPON", charm));
	CHECK_FALSE(Matches(L"ARMOR", charm));
}

TEST_CASE("1H and 2H follow the weapon's handedness in the game") {
	// "ssp" is the Short Spear, a javelin; "sst" is the Short Staff.
	const char* oneHanded[] = {"hax", "wnd", "clb", "ssd", "tkf", "jav", "ssp", "ktr", "ob1", "am5"};
	const char* twoHanded[] = {"lax", "sst", "spr", "sbw", "lbb", "am1"};
	for (const char* code : oneHanded) {
		TestItem weapon(code, ITEM_QUALITY_NORMAL);
		INFO(code);
		CHECK(Matches(L"1H", weapon));
		CHECK_FALSE(Matches(L"2H", weapon));
	}
	for (const char* code : twoHanded) {
		TestItem weapon(code, ITEM_QUALITY_NORMAL);
		INFO(code);
		CHECK(Matches(L"2H", weapon));
		CHECK_FALSE(Matches(L"1H", weapon));
	}
	TestItem helm("cap", ITEM_QUALITY_NORMAL);
	CHECK_FALSE(Matches(L"1H", helm));
	CHECK_FALSE(Matches(L"2H", helm));
}

TEST_CASE("WIDTH, HEIGHT and AREA compare the inventory size") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	item.attrs().width = 2;
	item.attrs().height = 3;
	CHECK(Matches(L"WIDTH=2", item));
	CHECK_FALSE(Matches(L"WIDTH=3", item));
	CHECK(Matches(L"HEIGHT=3", item));
	CHECK(Matches(L"AREA=6", item));
	CHECK(Matches(L"AREA>5", item));
	CHECK_FALSE(Matches(L"AREA>6", item));
}

TEST_CASE("MAPTIER compares the map tier (0 PvP, 1-5)") {
	TestItem t1("t11", ITEM_QUALITY_MAGIC);
	t1.attrs().category = ITEM_TYPE_T1_MAP;
	TestItem t3("t31", ITEM_QUALITY_RARE);
	t3.attrs().category = ITEM_TYPE_T3_MAP;
	TestItem pvp("pv1", ITEM_QUALITY_NORMAL);
	pvp.attrs().category = ITEM_TYPE_PVP_MAP_DESERT;

	CHECK(Matches(L"MAPTIER=1", t1));
	CHECK(Matches(L"MAPTIER<2", t1));
	CHECK(Matches(L"MAPTIER=3", t3));
	CHECK(Matches(L"MAPTIER>2", t3));
	CHECK_FALSE(Matches(L"MAPTIER>3", t3));
	CHECK(Matches(L"MAPTIER=0", pvp));
	CHECK_FALSE(Matches(L"MAPTIER>0", pvp));
}

// BUG: the wiki defines MAPTIER as "map tier, 0 - 5 (PvP, T1, T2, T3, Dungeon, Unique)"; an item that is
// not a map has no tier. MapTierCondition gives it -1 and compares that, so MAPTIER<N is true for
// every non-map item: a rule meant to hide low-tier maps, e.g. ItemDisplay[MAPTIER<2]:, hides every
// sword, charm and rune too.
TEST_CASE("MAPTIER never matches items that are not maps" * doctest::should_fail()) {
	TestItem sword("lsd", ITEM_QUALITY_UNIQUE);
	CHECK_FALSE(Matches(L"MAPTIER>0", sword));
	CHECK_FALSE(Matches(L"MAPTIER<2", sword));
}

// ---- Character and game state ------------------------------------------------------------------

TEST_CASE("class keywords match the player's character class") {
	const char* classes[] = {"AMAZON", "SORCERESS", "NECROMANCER", "PALADIN", "BARBARIAN", "DRUID", "ASSASSIN"};
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	for (DWORD playerClass = 0; playerClass < 7; playerClass++) {
		fake::Player().dwTxtFileNo = playerClass;
		CAPTURE(playerClass);
		for (DWORD keyword = 0; keyword < 7; keyword++) {
			INFO(classes[keyword]);
			CHECK(Matches(Wide(classes[keyword]), item) == (keyword == playerClass));
		}
	}
}

TEST_CASE("CLVL compares the character's level") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	fake::SetStat(&fake::Player(), STAT_LEVEL, 10);
	CHECK_FALSE(Matches(L"CLVL>10", item));
	CHECK(Matches(L"CLVL=10", item));
	fake::SetStat(&fake::Player(), STAT_LEVEL, 11);
	CHECK(Matches(L"CLVL>10", item));
	CHECK(Matches(L"CLVL~11-99", item));
}

TEST_CASE("DIFF compares the difficulty (0 Normal, 1 Nightmare, 2 Hell)") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	CHECK(Matches(L"DIFF=0", item));
	CHECK_FALSE(Matches(L"DIFF>0", item));
	fake::SetDifficulty(2);
	CHECK(Matches(L"DIFF=2", item));
	CHECK(Matches(L"DIFF>1", item));
	CHECK_FALSE(Matches(L"DIFF<2", item));
}

TEST_CASE("FILTLVL compares the selected filter strictness level") {
	TestItem item("yps", ITEM_QUALITY_NORMAL);
	App.lootfilter.filterLevel.uValue = 1;
	CHECK_FALSE(Matches(L"yps FILTLVL>1", item));
	App.lootfilter.filterLevel.uValue = 2;
	CHECK(Matches(L"yps FILTLVL>1", item));
	App.lootfilter.filterLevel.uValue = 0;
	CHECK(Matches(L"FILTLVL=0", item));
	CHECK_FALSE(Matches(L"FILTLVL>0", item));
}

TEST_CASE("MAPID compares the zone the character is in, and is false outside any zone") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	fake::SetAreaId(1);  // Rogue Encampment
	CHECK(Matches(L"MAPID=1", item));
	CHECK_FALSE(Matches(L"MAPID>1", item));
	fake::SetAreaId(132);  // Worldstone Chamber
	CHECK(Matches(L"MAPID~128-132", item));
	fake::SetAreaId(0);
	CHECK_FALSE(Matches(L"MAPID<5", item));
	CHECK_FALSE(Matches(L"MAPID=0", item));
}

TEST_CASE("PRICE and SELLPRICE compare the vendor sell value, BUYPRICE the buy value") {
	TestItem item("lsd", ITEM_QUALITY_RARE);
	fake::SetPrice(item.unit(), TRANSACTIONTYPE_SELL, 1000);
	fake::SetPrice(item.unit(), TRANSACTIONTYPE_BUY, 5000);
	CHECK(Matches(L"PRICE=1000", item));
	CHECK(Matches(L"PRICE>999", item));
	CHECK_FALSE(Matches(L"PRICE>1000", item));
	CHECK_FALSE(Matches(L"PRICE=5000", item));
	CHECK(Matches(L"SELLPRICE=1000", item));
	CHECK_FALSE(Matches(L"SELLPRICE=5000", item));
	CHECK(Matches(L"BUYPRICE=5000", item));
	CHECK_FALSE(Matches(L"BUYPRICE=1000", item));
}

// ---- Item location -----------------------------------------------------------------------------

TEST_CASE("EQUIPPED matches items worn by the character only") {
	Inventory inv;
	TestItem worn("cap", ITEM_QUALITY_NORMAL);
	PlaceIn(worn, inv, &fake::Player(), STORAGE_NULL);
	worn.data().BodyLocation = 1;  // head
	worn.unit()->dwMode = ITEM_MODE_EQUIPPED;
	CHECK(Matches(L"EQUIPPED", worn));
	CHECK_FALSE(Matches(L"INVENTORY", worn));
	CHECK_FALSE(Matches(L"STASH", worn));
	CHECK_FALSE(Matches(L"GROUND", worn));
	// No mercenary: nothing is merc-equipped.
	CHECK_FALSE(Matches(L"MERC", worn));

	UnitAny otherPlayer;
	std::memset(&otherPlayer, 0, sizeof(otherPlayer));
	otherPlayer.dwType = UNIT_PLAYER;
	Inventory otherInv;
	TestItem theirs("cap", ITEM_QUALITY_NORMAL);
	PlaceIn(theirs, otherInv, &otherPlayer, STORAGE_NULL);
	theirs.data().BodyLocation = 1;
	theirs.unit()->dwMode = ITEM_MODE_EQUIPPED;
	CHECK_FALSE(Matches(L"EQUIPPED", theirs));
}

TEST_CASE("INVENTORY, STASH and CUBE match the character's storage the item is in") {
	Inventory inv1, inv2, inv3, inv4;
	TestItem inInventory("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(inInventory, inv1, &fake::Player(), STORAGE_INVENTORY);
	CHECK(Matches(L"INVENTORY", inInventory));
	CHECK_FALSE(Matches(L"STASH", inInventory));
	CHECK_FALSE(Matches(L"CUBE", inInventory));
	CHECK_FALSE(Matches(L"EQUIPPED", inInventory));

	TestItem inStash("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(inStash, inv2, &fake::Player(), STORAGE_STASH);
	CHECK(Matches(L"STASH", inStash));
	CHECK_FALSE(Matches(L"INVENTORY", inStash));
	CHECK_FALSE(Matches(L"CUBE", inStash));

	TestItem inCube("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(inCube, inv3, &fake::Player(), STORAGE_CUBE);
	CHECK(Matches(L"CUBE", inCube));
	CHECK_FALSE(Matches(L"INVENTORY", inCube));
	CHECK_FALSE(Matches(L"STASH", inCube));

	TestItem inBelt("hp5", ITEM_QUALITY_NORMAL);
	PlaceIn(inBelt, inv4, &fake::Player(), STORAGE_BELT);
	inBelt.unit()->dwMode = ITEM_MODE_IN_BELT;
	CHECK_FALSE(Matches(L"INVENTORY", inBelt));
}

TEST_CASE("GROUND matches items lying on or being dropped to the ground") {
	TestItem lying("rin", ITEM_QUALITY_MAGIC);
	lying.unit()->dwMode = ITEM_MODE_ON_GROUND;
	CHECK(Matches(L"GROUND", lying));
	CHECK_FALSE(Matches(L"INVENTORY", lying));

	TestItem dropping("rin", ITEM_QUALITY_MAGIC);
	dropping.unit()->dwMode = ITEM_MODE_BEING_DROPPED;
	CHECK(Matches(L"GROUND", dropping));

	Inventory inv;
	TestItem carried("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(carried, inv, &fake::Player(), STORAGE_INVENTORY);
	carried.unit()->dwMode = ITEM_MODE_INV_STASH_CUBE_STORE;
	CHECK_FALSE(Matches(L"GROUND", carried));
}

TEST_CASE("SHOP matches items owned by a merchant") {
	UnitAny akara;
	std::memset(&akara, 0, sizeof(akara));
	akara.dwType = UNIT_MONSTER;
	akara.dwTxtFileNo = NPCID_Akara;
	Inventory shopInv;
	TestItem forSale("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(forSale, shopInv, &akara, STORAGE_INVENTORY);
	CHECK(Matches(L"SHOP", forSale));

	// Kashya hires out mercenaries; she sells nothing.
	UnitAny kashya;
	std::memset(&kashya, 0, sizeof(kashya));
	kashya.dwType = UNIT_MONSTER;
	kashya.dwTxtFileNo = NPCID_Kashya;
	Inventory npcInv;
	TestItem npcItem("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(npcItem, npcInv, &kashya, STORAGE_INVENTORY);
	CHECK_FALSE(Matches(L"SHOP", npcItem));

	Inventory mine;
	TestItem carried("rin", ITEM_QUALITY_MAGIC);
	PlaceIn(carried, mine, &fake::Player(), STORAGE_INVENTORY);
	CHECK_FALSE(Matches(L"SHOP", carried));

	TestItem lying("rin", ITEM_QUALITY_MAGIC);
	lying.unit()->dwMode = ITEM_MODE_ON_GROUND;
	CHECK_FALSE(Matches(L"SHOP", lying));
}

// ---- Boolean grammar ---------------------------------------------------------------------------

TEST_CASE("a rule without conditions matches every item") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	CHECK(Matches(L"", item));
}

TEST_CASE("TRUE and FALSE are constant conditions") {
	TestItem item("lsd", ITEM_QUALITY_NORMAL);
	CHECK(Matches(L"TRUE", item));
	CHECK_FALSE(Matches(L"FALSE", item));
	CHECK_FALSE(Matches(L"TRUE FALSE", item));
	CHECK(Matches(L"FALSE OR TRUE", item));
}

TEST_CASE("adjacent conditions are implicitly ANDed") {
	TestItem ethUnique("lsd", ITEM_QUALITY_UNIQUE);
	ethUnique.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem unique("lsd", ITEM_QUALITY_UNIQUE);

	CHECK(Matches(L"UNI ETH", ethUnique));
	CHECK_FALSE(Matches(L"UNI ETH", unique));
	CHECK(Matches(L"UNI AND ETH", ethUnique));
	CHECK_FALSE(Matches(L"UNI AND ETH", unique));
	CHECK(Matches(L"UNI && ETH", ethUnique));
	CHECK_FALSE(Matches(L"UNI && ETH", unique));
}

TEST_CASE("OR and || match when either side does") {
	TestItem unique("lsd", ITEM_QUALITY_UNIQUE);
	TestItem set("lsd", ITEM_QUALITY_SET);
	TestItem rare("lsd", ITEM_QUALITY_RARE);
	CHECK(Matches(L"UNI OR SET", unique));
	CHECK(Matches(L"UNI OR SET", set));
	CHECK_FALSE(Matches(L"UNI OR SET", rare));
	CHECK(Matches(L"UNI || SET", set));
	CHECK_FALSE(Matches(L"UNI || SET", rare));
}

TEST_CASE("! negates the condition or group right after it") {
	TestItem ethRare("lsd", ITEM_QUALITY_RARE);
	ethRare.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem ethUnique("lsd", ITEM_QUALITY_UNIQUE);
	ethUnique.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem rare("lsd", ITEM_QUALITY_RARE);

	CHECK(Matches(L"!UNI", rare));
	CHECK_FALSE(Matches(L"!UNI", ethUnique));
	// Negation binds to the next operand only.
	CHECK(Matches(L"!UNI ETH", ethRare));
	CHECK_FALSE(Matches(L"!UNI ETH", ethUnique));
	CHECK_FALSE(Matches(L"!UNI ETH", rare));
	CHECK(Matches(L"ETH !UNI", ethRare));
	CHECK_FALSE(Matches(L"ETH !UNI", ethUnique));
	CHECK(Matches(L"!UNI OR ETH", ethUnique));
	CHECK_FALSE(Matches(L"!ETH OR UNI", ethRare));
	// Groups.
	CHECK(Matches(L"!(UNI OR SET)", rare));
	CHECK_FALSE(Matches(L"!(UNI OR SET)", ethUnique));
	// Double negation, and "!" as its own token.
	CHECK(Matches(L"!!ETH", ethRare));
	CHECK_FALSE(Matches(L"!!ETH", rare));
	CHECK(Matches(L"! ETH", rare));
	CHECK_FALSE(Matches(L"! ETH", ethRare));
}

TEST_CASE("parentheses group conditions") {
	TestItem ethSet("lsd", ITEM_QUALITY_SET);
	ethSet.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem set("lsd", ITEM_QUALITY_SET);
	TestItem ethRare("lsd", ITEM_QUALITY_RARE);
	ethRare.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);

	CHECK(Matches(L"(UNI OR SET) ETH", ethSet));
	CHECK_FALSE(Matches(L"(UNI OR SET) ETH", set));
	CHECK_FALSE(Matches(L"(UNI OR SET) ETH", ethRare));
	CHECK(Matches(L"ETH (UNI OR SET)", ethSet));
	CHECK_FALSE(Matches(L"ETH (UNI OR SET)", ethRare));
	// Separate paren tokens, redundant nesting.
	CHECK(Matches(L"( UNI OR SET ) ETH", ethSet));
	CHECK(Matches(L"((SET))", set));
	CHECK(Matches(L"ETH ( RARE OR ( SET ETH ) )", ethSet));
	CHECK_FALSE(Matches(L"ETH ( RARE OR ( SET !ETH ) )", ethSet));
}

TEST_CASE("AND and OR bind equally and group left to right") {
	// The wiki defines no precedence. ProcessConditions gives AND and OR equal precedence, grouping
	// left to right ("A OR B C" is "(A OR B) AND C"); this protects that de-facto behaviour, which
	// existing unparenthesised filters depend on.
	TestItem unique("lsd", ITEM_QUALITY_UNIQUE);
	TestItem ethUnique("lsd", ITEM_QUALITY_UNIQUE);
	ethUnique.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem ethSet("lsd", ITEM_QUALITY_SET);
	ethSet.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	TestItem set("lsd", ITEM_QUALITY_SET);

	CHECK_FALSE(Matches(L"UNI OR SET ETH", unique));
	CHECK(Matches(L"UNI OR SET ETH", ethUnique));
	CHECK(Matches(L"UNI OR SET AND ETH", ethSet));
	// An explicit AND has the same precedence as the implicit one.
	CHECK_FALSE(Matches(L"UNI OR SET AND ETH", unique));
	// (ETH AND UNI) OR SET
	CHECK(Matches(L"ETH UNI OR SET", set));
	CHECK_FALSE(Matches(L"ETH UNI OR SET", unique));
}

TEST_CASE("the wiki's gold example: (GOLD<100 OR (GOLD<1000 CLVL>50))") {
	const std::wstring rule = L"(GOLD<100 OR (GOLD<1000 CLVL>50))";
	TestItem tiny("gld", ITEM_QUALITY_NORMAL);
	tiny.Stat(STAT_GOLD, 50);
	TestItem medium("gld", ITEM_QUALITY_NORMAL);
	medium.Stat(STAT_GOLD, 500);
	TestItem large("gld", ITEM_QUALITY_NORMAL);
	large.Stat(STAT_GOLD, 1500);

	fake::SetStat(&fake::Player(), STAT_LEVEL, 40);
	CHECK(Matches(rule, tiny));
	CHECK_FALSE(Matches(rule, medium));
	CHECK_FALSE(Matches(rule, large));
	fake::SetStat(&fake::Player(), STAT_LEVEL, 51);
	CHECK(Matches(rule, tiny));
	CHECK(Matches(rule, medium));
	CHECK_FALSE(Matches(rule, large));
}

TEST_CASE("the wiki's helm example: MAG !ID HELM !(BAR OR DRU OR ELT)") {
	const std::wstring rule = L"MAG !ID HELM !(BAR OR DRU OR ELT)";
	TestItem helm("cap", ITEM_QUALITY_MAGIC);
	helm.Flags(0);
	helm.attrs().baseFlags = ITEM_GROUP_NORMAL;
	helm.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM;
	CHECK(Matches(rule, helm));

	TestItem barbHelm("ba1", ITEM_QUALITY_MAGIC);
	barbHelm.Flags(0);
	barbHelm.attrs().baseFlags = ITEM_GROUP_NORMAL | ITEM_GROUP_CLASS;
	barbHelm.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM | ITEM_GROUP_BARBARIAN_HELM;
	CHECK_FALSE(Matches(rule, barbHelm));

	TestItem eliteHelm("uap", ITEM_QUALITY_MAGIC);
	eliteHelm.Flags(0);
	eliteHelm.attrs().baseFlags = ITEM_GROUP_ELITE;
	eliteHelm.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM;
	CHECK_FALSE(Matches(rule, eliteHelm));

	TestItem identified("cap", ITEM_QUALITY_MAGIC);
	identified.attrs().baseFlags = ITEM_GROUP_NORMAL;
	identified.attrs().armorFlags = ITEM_GROUP_ALLARMOR | ITEM_GROUP_HELM;
	CHECK_FALSE(Matches(rule, identified));
}

TEST_CASE("a rule with a dangling operator never matches") {
	TestItem unique("lsd", ITEM_QUALITY_UNIQUE);
	CHECK_FALSE(Matches(L"UNI OR", unique));
	CHECK_FALSE(Matches(L"OR UNI", unique));
	CHECK_FALSE(Matches(L"UNI AND", unique));
	CHECK_FALSE(Matches(L"UNI AND AND UNI", unique));
	CHECK_FALSE(Matches(L"UNI OR OR UNI", unique));
	CHECK_FALSE(Matches(L"UNI !", unique));
	CHECK_FALSE(Matches(L"!", unique));
}

TEST_CASE("an unknown upper-case keyword does not stop the other conditions being evaluated") {
	TestItem rare("lsd", ITEM_QUALITY_RARE);
	CHECK_FALSE(Matches(L"SET NOTAKEYWORD", rare));
	CHECK_FALSE(Matches(L"NOTAKEYWORD SET", rare));
}

// BUG: an unmatched ")" makes ProcessConditions stop and return the conditions read so far, so the
// valid conditions after it are silently dropped: "ETH ) UNI" acts as plain "ETH". The wiki says
// conditions written one after another are all required (implicit AND), and BH already rejects other
// malformed expressions (a dangling AND/OR: the rule never matches). Whether the stray paren makes the
// rule never match or is ignored ("ETH UNI"), an ethereal rare must not match.
TEST_CASE("an unmatched closing parenthesis does not drop the rest of the rule" * doctest::should_fail()) {
	TestItem ethRare("lsd", ITEM_QUALITY_RARE);
	ethRare.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	CHECK_FALSE(Matches(L"ETH ) UNI", ethRare));
	CHECK_FALSE(Matches(L"ETH) UNI", ethRare));
}

}  // TEST_SUITE
