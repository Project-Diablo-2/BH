// Loot filter rule-list behaviour, end to end through a real filter file (Config::Parse +
// ItemDisplay::InitializeItemRules): rule order, %CONTINUE%, hiding, filter levels, aliases,
// formulas, filter level names, notification (map) rules and the per-item lookup caches.
// Expected behaviour follows the PD2 filter documentation
// (https://wiki.projectdiablo2.com/wiki/Item_Filtering) unless a test says otherwise.
#include "doctest/doctest.h"

#include <string>
#include <vector>

#include "BH.h"
#include "BHGlobals.h"
#include "FakeEngine.h"
#include "LootFilter.h"

using support::DescriptionOf;
using support::LoadFilter;
using support::NameOf;
using support::TestItem;

namespace {

void SetFilterLevel(unsigned int level) {
	App.lootfilter.filterLevel.uValue = level;
}

bool Contains(const std::vector<Rule*>& list, const Rule* rule) {
	for (const Rule* r : list) {
		if (r == rule) {
			return true;
		}
	}
	return false;
}

// Loads a one-rule filter and reports whether the rule is a notification rule (MapRuleList, which
// MapNotify and Item.cpp's drop handling walk).
bool IsNotificationRule(const std::string& action) {
	LoadFilter("ItemDisplay[hax]: " + action + "\n");
	REQUIRE(RuleList.size() == 1);
	return Contains(MapRuleList, RuleList[0]);
}

}  // namespace

TEST_SUITE("LootFilterRules") {

// ---- Rule order ----------------------------------------------------------------------------

TEST_CASE("The first matching rule decides the name and later matching rules are not applied") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem sword("ssd", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: First %NAME%\n"
		"ItemDisplay[UNI]: Second %NAME%\n"
		"ItemDisplay[]: Third %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"First The Gnasher");
	// The first rule does not match the sword, so the next matching one applies.
	CHECK(NameOf(sword, L"Rixot's Keen") == L"Second Rixot's Keen");
}

TEST_CASE("An item no rule matches keeps its default name") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[SET]: Set %NAME%\n"
		"ItemDisplay[ssd]: Sword %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
}

TEST_CASE("An empty filter leaves names unchanged") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("");
	CHECK(RuleList.empty());
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
}

TEST_CASE("A hiding rule above a showing rule hides the item, and below it does not") {
	SUBCASE("hide first") {
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(
			"ItemDisplay[hax]:\n"
			"ItemDisplay[]: Shown %NAME%\n");
		CHECK(NameOf(axe, L"The Gnasher") == L"");
	}
	SUBCASE("show first") {
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(
			"ItemDisplay[]: Shown %NAME%\n"
			"ItemDisplay[hax]:\n");
		CHECK(NameOf(axe, L"The Gnasher") == L"Shown The Gnasher");
	}
}

TEST_CASE("A rule without conditions applies to every item") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem charm("cm1", ITEM_QUALITY_MAGIC);
	LoadFilter("ItemDisplay[]: Any %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"Any The Gnasher");
	CHECK(NameOf(charm, L"Small Charm") == L"Any Small Charm");
}

TEST_CASE("One leading and one trailing space of the game's name are dropped before rules apply") {
	// Magic items without a prefix/suffix come with a stray space (comment in make_cached_T).
	TestItem ring("rin", ITEM_QUALITY_MAGIC);
	LoadFilter("ItemDisplay[]: [%NAME%]\n");
	CHECK(NameOf(ring, L" Ring of Ice ") == L"[Ring of Ice]");
	TestItem ring2("rin", ITEM_QUALITY_MAGIC);
	CHECK(NameOf(ring2, L"  Ring  ") == L"[ Ring ]");
}

// ---- %CONTINUE% -------------------------------------------------------------------------------

TEST_CASE("%CONTINUE% feeds a rule's output into %NAME% of the next matching rule") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME% [axe]%CONTINUE%\n"
		"ItemDisplay[SET]: never %NAME%\n"
		"ItemDisplay[UNI]: unique %NAME%%CONTINUE%\n"
		"ItemDisplay[]: <%NAME%>\n"
		"ItemDisplay[]: too late %NAME%\n");
	// Wiki: each %CONTINUE% output replaces %NAME%; the chain ends at the first matching rule
	// without %CONTINUE%; non-matching rules in between are skipped.
	CHECK(NameOf(axe, L"The Gnasher") == L"<unique The Gnasher [axe]>");
}

TEST_CASE("A %CONTINUE% chain that no later rule ends shows the last continued output") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME% [axe]%CONTINUE%\n"
		"ItemDisplay[SET]: never %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher [axe]");
}

TEST_CASE("%CONTINUE% does not apply when its rule does not match") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[ETH]: eth %NAME%%CONTINUE%\n"
		"ItemDisplay[]: %NAME%!\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher!");
	TestItem eth("hax", ITEM_QUALITY_UNIQUE);
	eth.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	CHECK(NameOf(eth, L"The Gnasher") == L"eth The Gnasher!");
}

TEST_CASE("A hiding rule after a %CONTINUE% rule still hides the item") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[]: %NAME% [x]%CONTINUE%\n"
		"ItemDisplay[hax]:\n"
		"ItemDisplay[]: %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"");
}

TEST_CASE("Descriptions follow the same first-match and %CONTINUE% chaining as names") {
	SUBCASE("first match wins") {
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(
			"ItemDisplay[hax]: %NAME%{first}\n"
			"ItemDisplay[]: %NAME%{second}\n");
		CHECK(DescriptionOf(axe) == L"first");
	}
	SUBCASE("continued description is %NAME% inside the next rule's braces") {
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(
			"ItemDisplay[hax]: %NAME%{A}%CONTINUE%\n"
			"ItemDisplay[]: %NAME%{%NAME% B}\n");
		CHECK(DescriptionOf(axe) == L"A B");
		CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
	}
	SUBCASE("a final rule without braces leaves no description") {
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(
			"ItemDisplay[hax]: %NAME%{A}%CONTINUE%\n"
			"ItemDisplay[]: %NAME%\n");
		CHECK(DescriptionOf(axe) == L"");
	}
	SUBCASE("a hidden item still shows its description") {
		// Wiki: "ItemDisplay[]: {%NAME%}  items hidden but their descriptions still shown".
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter("ItemDisplay[hax]: {Hidden but described}\n");
		CHECK(NameOf(axe, L"The Gnasher") == L"");
		CHECK(DescriptionOf(axe) == L"Hidden but described");
	}
}

// ---- Hiding and filter levels -------------------------------------------------------------

TEST_CASE("A rule with an empty output hides the item") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem sword("ssd", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]:\n"
		"ItemDisplay[ssd]: %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"");
	CHECK(NameOf(sword, L"Rixot's Keen") == L"Rixot's Keen");
}

TEST_CASE("Filter level 0 (Show All Items) shows hidden items with their default name") {
	SetFilterLevel(0);
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem charm("cm1", ITEM_QUALITY_MAGIC);
	LoadFilter(
		"ItemDisplay[hax]:\n"
		"ItemDisplay[cm1]: Charm %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
	// Level 0 only unhides; rules that rename still rename.
	CHECK(NameOf(charm, L"Small Charm") == L"Charm Small Charm");
}

TEST_CASE("Switching to filter level 0 reveals hidden items, switching back hides them") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[hax]:\n");
	SetFilterLevel(1);
	CHECK(NameOf(axe, L"The Gnasher") == L"");
	SetFilterLevel(0);
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
	SetFilterLevel(1);
	CHECK(NameOf(axe, L"The Gnasher") == L"");
}

TEST_CASE("FILTLVL conditions select rules by the chosen filter level") {
	const char* filter =
		"ItemDisplayFilterName[]: Relaxed\n"
		"ItemDisplayFilterName[]: Strict\n"
		"ItemDisplay[FILTLVL>1 hax]:\n"
		"ItemDisplay[FILTLVL=1 hax]: Relaxed %NAME%\n";
	SUBCASE("level 2 hides") {
		SetFilterLevel(2);
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(filter);
		CHECK(NameOf(axe, L"The Gnasher") == L"");
	}
	SUBCASE("level 1 renames") {
		SetFilterLevel(1);
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(filter);
		CHECK(NameOf(axe, L"The Gnasher") == L"Relaxed The Gnasher");
	}
	SUBCASE("level 0 matches neither") {
		SetFilterLevel(0);
		TestItem axe("hax", ITEM_QUALITY_UNIQUE);
		LoadFilter(filter);
		CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
	}
}

// ---- Filter level names -------------------------------------------------------------------

TEST_CASE("ItemDisplayFilterName entries are numbered after the built-in level 0") {
	LoadFilter(
		"ItemDisplayFilterName[]: Low Strictness\n"
		"ItemDisplay[]: %NAME%\n"
		"ItemDisplayFilterName[ignored]: High Strictness\n");
	std::vector<std::string> expected = {"0 - Show All Items", "1 - Low Strictness", "2 - High Strictness"};
	CHECK(fake::ItemModule().ItemFilterNames == expected);
}

TEST_CASE("Without ItemDisplayFilterName entries the levels are Show All Items and Standard") {
	LoadFilter("ItemDisplay[]: %NAME%\n");
	std::vector<std::string> expected = {"0 - Show All Items", "1 - Standard"};
	CHECK(fake::ItemModule().ItemFilterNames == expected);
}

TEST_CASE("At most 12 custom filter levels are taken") {
	std::string filter;
	for (int i = 1; i <= 15; i++) {
		filter += "ItemDisplayFilterName[]: L" + std::to_string(i) + "\n";
	}
	LoadFilter(filter);
	const auto& names = fake::ItemModule().ItemFilterNames;
	REQUIRE(names.size() == 13);  // level 0 + 12 custom levels (wiki: "up to 12 other levels")
	CHECK(names[1] == "1 - L1");
	CHECK(names[12] == "12 - L12");
}

TEST_CASE("A selected filter level the loaded filter does not define falls back to level 1") {
	const char* twoLevels =
		"ItemDisplayFilterName[]: A\n"
		"ItemDisplayFilterName[]: B\n";
	SUBCASE("one past the last level") {
		SetFilterLevel(3);
		LoadFilter(twoLevels);
		CHECK(App.lootfilter.filterLevel.uValue == 1);
	}
	SUBCASE("the last level is kept") {
		SetFilterLevel(2);
		LoadFilter(twoLevels);
		CHECK(App.lootfilter.filterLevel.uValue == 2);
	}
	SUBCASE("level 0 is always kept") {
		SetFilterLevel(0);
		LoadFilter("");
		CHECK(App.lootfilter.filterLevel.uValue == 0);
	}
	SUBCASE("a filter without names only has level 1 besides 0") {
		SetFilterLevel(2);
		LoadFilter("ItemDisplay[]: %NAME%\n");
		CHECK(App.lootfilter.filterLevel.uValue == 1);
	}
}

// ---- Filter file syntax --------------------------------------------------------------------

TEST_CASE("Comments and blank lines in a filter file are ignored") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem sword("ssd", ITEM_QUALITY_UNIQUE);
	TestItem charm("cm1", ITEM_QUALITY_MAGIC);
	LoadFilter(
		"// a header comment\n"
		"\n"
		"   \n"
		"//ItemDisplay[hax]: disabled rule\n"
		"ItemDisplay[hax]: Axe %NAME% // trailing note\n"
		"\t\n"
		"ItemDisplay[ssd]: //%NAME%\n"
		"ItemDisplay[cm1]: Charm\n");
	REQUIRE(RuleList.size() == 3);
	CHECK(NameOf(axe, L"The Gnasher") == L"Axe The Gnasher");
	// Wiki: "ItemDisplay[tsc]: //%NAME%  this rule hides TP scrolls".
	CHECK(NameOf(sword, L"Rixot's Keen") == L"");
	CHECK(NameOf(charm, L"Small Charm") == L"Charm");
}

TEST_CASE("Filter files with Windows line endings work") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplayFilterName[]: Only\r\n"
		"ItemDisplay[hax]: Found %NAME%%CONTINUE%\r\n"
		"ItemDisplay[UNI]: %NAME%!\r\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"Found The Gnasher!");
	CHECK(fake::ItemModule().ItemFilterNames.back() == "1 - Only");
}

TEST_CASE("Spaces then tabs around a rule's output are trimmed, so tabs protect highlight spaces") {
	// Wiki: "Whitespace surrounding the Output of each rule gets removed prior to evaluation
	// (spaces first, followed by tabs), so tabs are often the best non-space character to use."
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[hax]:   \t %NAME% \t   \n");
	CHECK(NameOf(axe, L"The Gnasher") == L" The Gnasher ");
}

TEST_CASE("Lines that are not ItemDisplay rules do not become rules") {
	LoadFilter(
		"Alias[X]: UNI\n"
		"Formula[F]: 1\n"
		"ItemDisplayFilterName[]: Level\n"
		"NotARule[hax]: x\n"
		"some text without a colon\n"
		"ItemDisplay[hax]: one\n");
	CHECK(RuleList.size() == 1);
}

// ---- Reload --------------------------------------------------------------------------------

TEST_CASE("Reloading a filter replaces the old rules, aliases and level names") {
	LoadFilter(
		"Alias[TAG]: [old]\n"
		"ItemDisplayFilterName[]: Old A\n"
		"ItemDisplayFilterName[]: Old B\n"
		"ItemDisplay[hax]: Old %NAME%%TAG%\n"
		"ItemDisplay[hax]: %NAME%%MAP-55%\n"
		"ItemDisplay[ssd]:\n");
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem sword("ssd", ITEM_QUALITY_UNIQUE);
	CHECK(NameOf(axe, L"The Gnasher") == L"Old The Gnasher[old]");
	CHECK(NameOf(sword, L"Rixot's Keen") == L"");

	LoadFilter(
		"ItemDisplayFilterName[]: New\n"
		"ItemDisplay[hax]: New %NAME%%TAG%\n");
	CHECK(RuleList.size() == 1);
	CHECK(MapRuleList.empty());
	// Same unit: the reload must not serve the name cached from the old filter. TAG is no longer
	// an alias, so it is an unknown keyword, shown as typed.
	CHECK(NameOf(axe, L"The Gnasher") == L"New The Gnasher%TAG%");
	CHECK(NameOf(sword, L"Rixot's Keen") == L"Rixot's Keen");
	std::vector<std::string> expected = {"0 - Show All Items", "1 - New"};
	CHECK(fake::ItemModule().ItemFilterNames == expected);
}

TEST_CASE("Initializing again without uninitializing keeps the loaded rules unchanged") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME% +%CONTINUE%\n"
		"ItemDisplay[]: %NAME%\n");
	ItemDisplay::InitializeItemRules();
	CHECK(RuleList.size() == 2);
	CHECK(NameOf(axe, L"Axe") == L"Axe +");
}

TEST_CASE("Uninitializing removes every rule") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]:\n"
		"ItemDisplay[hax]: %NAME%%MAP-55%\n");
	ItemDisplay::UninitializeItemRules();
	CHECK(RuleList.empty());
	CHECK(MapRuleList.empty());
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
}

// ---- Aliases -------------------------------------------------------------------------------

TEST_CASE("An alias in conditions is replaced by its text before the rule is parsed") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	TestItem sword("ssd", ITEM_QUALITY_UNIQUE);
	TestItem setAxe("hax", ITEM_QUALITY_SET);
	LoadFilter(
		"Alias[GOODBASE]: (hax OR axe)\n"
		"ItemDisplay[GOODBASE UNI]: Good %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"Good The Gnasher");
	CHECK(NameOf(sword, L"Rixot's Keen") == L"Rixot's Keen");
	// The parentheses from the alias keep "hax OR axe" grouped before the implicit AND.
	CHECK(NameOf(setAxe, L"Set Axe") == L"Set Axe");
}

TEST_CASE("Aliases can be defined after the rules that use them") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[MYAXE]: %NAME%%TAG%\n"
		"Alias[MYAXE]: hax\n"
		"Alias[TAG]: !\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher!");
}

TEST_CASE("An alias in output is written as %KEY% and every use is replaced") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"Alias[TAG]: [T]\n"
		"ItemDisplay[hax]: %TAG%%NAME%%TAG%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"[T]The Gnasher[T]");
}

TEST_CASE("An alias may expand to output keywords") {
	// Wiki example: Alias[SOCKETCOUNT]: %WHITE% [%GRAY%%SOCK%%WHITE%]
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"Alias[MARK]: %CONTINUE%\n"
		"ItemDisplay[hax]: %NAME% marked%MARK%\n"
		"ItemDisplay[]: <%NAME%>\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"<The Gnasher marked>");
}

TEST_CASE("An alias defined in lower case is used as an upper-case output keyword") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"Alias[tag]: [T]\n"
		"ItemDisplay[hax]: %NAME%%TAG%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher[T]");
}

TEST_CASE("An alias key is trimmed and limited to its first word") {
	TestItem low("hax", ITEM_QUALITY_UNIQUE);
	TestItem high("hax", ITEM_QUALITY_UNIQUE);
	high.ItemLevel(60);
	LoadFilter(
		"Alias[ HIGH level items ]: ILVL>50\n"
		"ItemDisplay[HIGH]: High %NAME%\n");
	CHECK(NameOf(high, L"Axe") == L"High Axe");
	CHECK(NameOf(low, L"Axe") == L"Axe");
}

TEST_CASE("An alias with an empty key is ignored") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"Alias[]: SET\n"
		"Alias[   ]: SET\n"
		"ItemDisplay[UNI]: Unique %NAME%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"Unique The Gnasher");
}

// ---- Formulas ------------------------------------------------------------------------------

TEST_CASE("A Formula is referenced from conditions as FORMULA<KEY>") {
	TestItem low("hax", ITEM_QUALITY_UNIQUE);
	TestItem high("hax", ITEM_QUALITY_UNIQUE);
	high.ItemLevel(60);
	LoadFilter(
		"ItemDisplay[FORMULAHIGH]: High %NAME%\n"
		"Formula[HIGH]: ilvl>50\n");
	CHECK(NameOf(high, L"Axe") == L"High Axe");
	CHECK(NameOf(low, L"Axe") == L"Axe");
}

TEST_CASE("A Formula is referenced from output as %FORMULA<KEY>%") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	axe.ItemLevel(21);
	LoadFilter(
		"Formula[_TWICE]: ilvl*2\n"
		"ItemDisplay[hax]: %NAME% %FORMULA_TWICE%\n");
	CHECK(NameOf(axe, L"Axe") == L"Axe 42");
}

TEST_CASE("Formulas do not survive a reload of a filter that no longer defines them") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	axe.ItemLevel(21);
	LoadFilter(
		"Formula[_TWICE]: ilvl*2\n"
		"ItemDisplay[hax]: %NAME% %FORMULA_TWICE%\n");
	LoadFilter("ItemDisplay[hax]: %NAME% %FORMULA_TWICE%\n");
	// An unknown keyword is shown as typed.
	CHECK(NameOf(axe, L"Axe") == L"Axe %FORMULA_TWICE%");
}

// ---- Notification (map) rules ---------------------------------------------------------------

TEST_CASE("Rules with a map, border, dot, px, line or sound keyword are notification rules") {
	fake::Var(Var_D2CLIENT_SoundRecords) = 5000;
	SUBCASE("MAP") {
		CHECK(IsNotificationRule("%NAME%%MAP-55%"));
		CHECK(RuleList[0]->action.colorOnMap == 0x55);
	}
	SUBCASE("BORDER") {
		CHECK(IsNotificationRule("%NAME%%BORDER-9B%"));
		CHECK(RuleList[0]->action.borderColor == 0x9B);
	}
	SUBCASE("DOT") {
		CHECK(IsNotificationRule("%NAME%%DOT-20%"));
	}
	SUBCASE("PX") {
		CHECK(IsNotificationRule("%NAME%%PX-0B%"));
	}
	SUBCASE("LINE") {
		CHECK(IsNotificationRule("%NAME%%LINE-62%"));
	}
	SUBCASE("SOUNDID within the game's sound table") {
		CHECK(IsNotificationRule("%NAME%%SOUNDID-4714%"));
		CHECK(RuleList[0]->action.soundID == 4714);
	}
	SUBCASE("legacy %MAP%") {
		CHECK(IsNotificationRule("%NAME%%MAP%"));
	}
	SUBCASE("a map keyword with no visible name") {
		CHECK(IsNotificationRule("%MAP-55%"));
	}
}

TEST_CASE("Rules without map keywords are not notification rules") {
	fake::Var(Var_D2CLIENT_SoundRecords) = 100;
	SUBCASE("plain output") {
		CHECK_FALSE(IsNotificationRule("%NAME%"));
	}
	SUBCASE("tier only") {
		CHECK_FALSE(IsNotificationRule("%NAME%%TIER-2%"));
	}
	SUBCASE("a sound id past the end of the sound table plays nothing") {
		CHECK_FALSE(IsNotificationRule("%NAME%%SOUNDID-100%"));
		CHECK(RuleList[0]->action.soundID == 0);
	}
	SUBCASE("hiding rule") {
		CHECK_FALSE(IsNotificationRule(""));
	}
}

TEST_CASE("Map keywords are removed from the displayed name") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[hax]: %NAME%%MAP-55%%BORDER-20%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"The Gnasher");
}

TEST_CASE("Notification rules apply even after an earlier rule has decided the name") {
	// Wiki: "All notification keywords bypass the normal rule-handling procedure ... can apply
	// even after the process has halted."
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: Plain %NAME%\n"
		"ItemDisplay[hax]: %NAME%%MAP-55%\n"
		"ItemDisplay[SET]: %NAME%%DOT-20%\n"
		"ItemDisplay[UNI]: %NAME%%BORDER-9B%\n");
	CHECK(NameOf(axe, L"The Gnasher") == L"Plain The Gnasher");
	std::vector<Action> actions = map_action_cache.Get(axe.info());
	REQUIRE(actions.size() == 2);
	CHECK(actions[0].colorOnMap == 0x55);
	CHECK(actions[1].borderColor == 0x9B);
}

TEST_CASE("An item matching no notification rule has no map actions") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter(
		"ItemDisplay[hax]: %NAME%\n"
		"ItemDisplay[SET]: %NAME%%MAP-55%\n");
	CHECK(map_action_cache.Get(axe.info()).empty());
}

// ---- Lookup caches -------------------------------------------------------------------------

TEST_CASE("Changing an item's flags, mode or location recomputes its name") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[ILVL>50]: High %NAME%\n");
	CHECK(NameOf(axe, L"Axe") == L"Axe");
	axe.ItemLevel(60);
	SUBCASE("flags") {
		axe.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
		CHECK(NameOf(axe, L"Axe") == L"High Axe");
	}
	SUBCASE("mode") {
		axe.unit()->dwMode = 3;  // e.g. on the ground
		CHECK(NameOf(axe, L"Axe") == L"High Axe");
	}
	SUBCASE("location") {
		axe.data().ItemLocation = 4;
		CHECK(NameOf(axe, L"Axe") == L"High Axe");
	}
}

TEST_CASE("The caches are keyed per item") {
	TestItem first("hax", ITEM_QUALITY_UNIQUE);
	TestItem second("hax", ITEM_QUALITY_UNIQUE);
	second.ItemLevel(60);
	LoadFilter("ItemDisplay[ILVL>50]: High %NAME%\n");
	CHECK(NameOf(first, L"Axe") == L"Axe");
	CHECK(NameOf(second, L"Axe") == L"High Axe");
}

TEST_CASE("Clearing an item's cache entry recomputes its name") {
	// Item.cpp clears the entry of the hovered item when its flags change.
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[ILVL>50]: High %NAME%\n");
	CHECK(NameOf(axe, L"Axe") == L"Axe");
	axe.ItemLevel(60);
	item_name_cache.Clear(axe.info());
	CHECK(NameOf(axe, L"Axe") == L"High Axe");
}

TEST_CASE("Description and map actions are recomputed when an item's flags change") {
	TestItem axe("hax", ITEM_QUALITY_UNIQUE);
	LoadFilter("ItemDisplay[ILVL>50]: %NAME%{high}%MAP-55%\n");
	CHECK(DescriptionOf(axe) == L"");
	CHECK(map_action_cache.Get(axe.info()).empty());
	// Identifying, socketing or picking up changes the item's flags; that must refresh both.
	axe.ItemLevel(60);
	axe.Flags(ITEM_IDENTIFIED | ITEM_ETHEREAL);
	CHECK(DescriptionOf(axe) == L"high");
	CHECK(map_action_cache.Get(axe.info()).size() == 1);
}

}  // TEST_SUITE
