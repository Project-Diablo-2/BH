// What one loot filter rule's output ("action", the text after `ItemDisplay[...]:`) produces:
// how BuildAction splits it into name, description and notification settings, how the %KEYWORD%
// replacements render for an item, and how the finished name/description is trimmed.
// Expected values come from the PD2 item filtering wiki
// (https://wiki.projectdiablo2.com/wiki/Item_Filtering), Diablo II's colour codes and data ids
// (BH/Constants.h), and hand computation of the D2 formulas involved.
#include "doctest/doctest.h"

#include <string>

#include "BH.h"
#include "LootFilter.h"

using support::TestItem;

namespace {

// A D2 text colour code: U+00FF, 'c', then the colour character.
std::wstring Color(const wchar_t* code) {
	return std::wstring(L"\xFF" L"c") + code;
}

// The name `item` gets from a filter whose only rule matches every item and has `action` as output.
// Each call reloads the filter, which also clears BH's name/description caches, so one item can be
// looked at with several actions in a row.
std::wstring NameWith(const std::string& action, TestItem& item, const std::wstring& baseName = L"Hand Axe") {
	support::LoadFilter("ItemDisplay[]: " + action + "\n");
	return support::NameOf(item, baseName);
}

// The description `item` gets from a filter whose only rule matches every item (reloads like NameWith).
std::wstring DescriptionWith(const std::string& action, TestItem& item) {
	support::LoadFilter("ItemDisplay[]: " + action + "\n");
	return support::DescriptionOf(item);
}

// The parsed action (name/description split, notification settings) of a rule with this output.
const Action& ActionOf(const std::wstring& action) {
	return support::ParseRule(L"", action)->action;
}

}  // namespace

TEST_SUITE("LootFilterActions") {

// ---- Colour keywords -------------------------------------------------------------------------

TEST_CASE("colour keywords become the D2 text colour codes") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	// D2's colour characters: 0 white, 1 red, 2 green (set), 3 blue (magic), 4 gold (unique), 5 gray,
	// 7 tan, 8 orange, 9 yellow (rare), ':' dark green, ';' purple.
	CHECK(NameWith("%WHITE%a%RED%b%GREEN%c%BLUE%d", item) ==
		Color(L"0") + L"a" + Color(L"1") + L"b" + Color(L"2") + L"c" + Color(L"3") + L"d");
	CHECK(NameWith("%GOLD%e%GRAY%f%TAN%g%ORANGE%h", item) ==
		Color(L"4") + L"e" + Color(L"5") + L"f" + Color(L"7") + L"g" + Color(L"8") + L"h");
	CHECK(NameWith("%YELLOW%i%DARK_GREEN%j%PURPLE%k", item) ==
		Color(L"9") + L"i" + Color(L":") + L"j" + Color(L";") + L"k");
}

TEST_CASE("colour keywords also colour the description") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(DescriptionWith("%NAME%{%GOLD%Unique axe}", item) == Color(L"4") + L"Unique axe");
}

TEST_CASE("custom colours fall back to the closest standard colour without Glide") {
	// Wiki: custom colours only work with Glide/HD text and otherwise revert to the most similar
	// non-Glide colour. Black is the standard colour 6 outside Glide.
	fake::Var(Var_D2GFX_RenderMode) = 3;  // DirectDraw
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%CORAL%a%SAGE%b%TEAL%c%LIGHT_GRAY%d%BLACK%e", item) ==
		Color(L"1") + L"a" + Color(L"2") + L"b" + Color(L"3") + L"c" + Color(L"5") + L"d" + Color(L"6") + L"e");
}

TEST_CASE("under Glide custom colours are colour codes that do not use up the 56-character name budget") {
	fake::Var(Var_D2GFX_RenderMode) = 4;  // Glide
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	std::wstring name = NameWith("%CORAL%" + std::string(56, 'x'), item);
	REQUIRE(name.size() == 59);
	CHECK(name.substr(0, 2) == L"\xFF" L"c");
	CHECK(name.substr(0, 3) != Color(L"1"));  // not the non-Glide fallback
	CHECK(name.substr(3) == std::wstring(56, L'x'));
}

TEST_CASE("transparency keywords render nothing without D2GL or HD text") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%FULL_TRANS%a%THREE_FOURTHS_TRANS%b%HALF_TRANS%c%QUARTER_TRANS%d", item) == L"abcd");
}

// ---- Keyword syntax --------------------------------------------------------------------------

TEST_CASE("keywords are case-insensitive") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.ItemLevel(85);
	CHECK(NameWith("%red%%name% [%ilvl%]", item, L"The Gnasher") == Color(L"1") + L"The Gnasher [85]");
	// text outside %...% keeps its case
	CHECK(NameWith("lower %Name% Text", item, L"The Gnasher") == L"lower The Gnasher Text");
}

TEST_CASE("unknown keywords are shown as typed and do not swallow a following keyword") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%NAME% %FOO%", item, L"Axe") == L"Axe %FOO%");
	// "%FOO%NAME%": FOO is no keyword, so its closing % can still open %NAME%.
	CHECK(NameWith("%FOO%NAME%", item, L"Axe") == L"%FOOAxe");
}

TEST_CASE("percent and brace keywords produce literal characters") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%LBRACE%%NAME%%RBRACE% 100%PERCENT%", item, L"Axe") == L"{Axe} 100%");
	CHECK(NameWith("50% %NAME%", item, L"Axe") == L"50% Axe");
}

// ---- Name / description split ----------------------------------------------------------------

TEST_CASE("text in braces is the description, the rest is the name") {
	const Action& a = ActionOf(L"%NAME%{Item Level: %ILVL%}");
	CHECK(a.name == L"%NAME%");
	CHECK(a.description == L"Item Level: %ILVL%");

	const Action& middle = ActionOf(L"before {desc} after");
	CHECK(middle.name == L"before  after");
	CHECK(middle.description == L"desc");

	const Action& none = ActionOf(L"%NAME%");
	CHECK(none.name == L"%NAME%");
	CHECK(none.description == L"");

	const Action& descOnly = ActionOf(L"{%NAME%}");
	CHECK(descOnly.name == L"");
	CHECK(descOnly.description == L"%NAME%");

	// a closing brace before the opening one is no description
	const Action& reversed = ActionOf(L"a}b{c");
	CHECK(reversed.name == L"a}b{c");
	CHECK(reversed.description == L"");
}

TEST_CASE("description keywords are replaced for the item") {
	TestItem item("hax", ITEM_QUALITY_MAGIC);
	item.ItemLevel(42);
	CHECK(DescriptionWith("%NAME%{Item Level: %ILVL%}", item) == L"Item Level: 42");
	CHECK(NameWith("%NAME%{Item Level: %ILVL%}", item, L"Axe") == L"Axe");
}

TEST_CASE("an unclosed brace is shown as text and gives no description") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%NAME% {", item, L"Axe") == L"Axe {");
	CHECK(support::DescriptionOf(item) == L"");
}

// ---- %CONTINUE% ------------------------------------------------------------------------------

TEST_CASE("%CONTINUE% outside braces makes the rule continue and is not shown") {
	const Action& stop = ActionOf(L"%NAME%");
	CHECK(stop.stopProcessing);

	const Action& go = ActionOf(L"%NAME% [%SOCKETS%]%CONTINUE%");
	CHECK_FALSE(go.stopProcessing);
	CHECK(go.name == L"%NAME% [%SOCKETS%]");

	const Action& lower = ActionOf(L"%NAME%%continue%");
	CHECK_FALSE(lower.stopProcessing);
	CHECK(lower.name == L"%NAME%");
}

TEST_CASE("%CONTINUE% inside braces does not make the rule continue") {
	// Wiki: %CONTINUE% only functions when used outside the braces.
	const Action& a = ActionOf(L"%NAME%{text%CONTINUE%}");
	CHECK(a.stopProcessing);
	CHECK(a.name == L"%NAME%");
}

// ---- Notification keywords -------------------------------------------------------------------

TEST_CASE("minimap icon keywords take a hex colour and are removed from the name") {
	const Action& a = ActionOf(L"%NAME%%BORDER-0A%%MAP-9B%%DOT-1f%%PX-20%%LINE-84%");
	CHECK(a.borderColor == 0x0A);
	CHECK(a.colorOnMap == 0x9B);
	CHECK(a.dotColor == 0x1F);
	CHECK(a.pxColor == 0x20);
	CHECK(a.lineColor == 0x84);
	CHECK(a.name == L"%NAME%");
	CHECK(a.description == L"");
}

TEST_CASE("minimap icon keywords are case-insensitive and can sit anywhere in the output") {
	const Action& a = ActionOf(L"%map-9b%%NAME%{desc}%dot-ff%");
	CHECK(a.colorOnMap == 0x9B);
	CHECK(a.dotColor == 0xFF);
	CHECK(a.name == L"%NAME%");
	CHECK(a.description == L"desc");
}

TEST_CASE("without notification keywords no icon colours, tier or sound are set") {
	const Action& a = ActionOf(L"%RED%%NAME%{%GOLD%desc}");
	CHECK(a.borderColor == UNDEFINED_COLOR);
	CHECK(a.colorOnMap == UNDEFINED_COLOR);
	CHECK(a.dotColor == UNDEFINED_COLOR);
	CHECK(a.pxColor == UNDEFINED_COLOR);
	CHECK(a.lineColor == UNDEFINED_COLOR);
	CHECK(a.notifyColor == UNDEFINED_COLOR);
	CHECK(a.pingLevel == -1);
	CHECK(a.soundID == 0);
}

TEST_CASE("%NOTIFY-x% sets the text notification colour and %NOTIFY-DEAD% disables it") {
	// Wiki: NOTIFY takes a 1-digit colour code 0-F, or DEAD to disable text notifications.
	const Action& white = ActionOf(L"%NAME%%NOTIFY-0%");
	CHECK(white.notifyColor == 0);
	CHECK(white.name == L"%NAME%");
	CHECK(ActionOf(L"%NAME%%NOTIFY-F%").notifyColor == 0xF);
	const Action& dead = ActionOf(L"%NAME%%NOTIFY-DEAD%");
	CHECK(dead.notifyColor == DEAD_COLOR);
	CHECK(dead.name == L"%NAME%");
}

TEST_CASE("legacy %MAP% uses the last colour keyword before it, white by default") {
	// MAP_COLOR_* palette values that the wiki's minimap colour table confirms (Hex Average column):
	// white 20, green 84, blue 97, tan 5A, orange 60, purple 9B.
	const Action& plain = ActionOf(L"%NAME%%MAP%");
	CHECK(plain.colorOnMap == 0x20);
	CHECK(plain.name == L"%NAME%");

	CHECK(ActionOf(L"%GREEN%%NAME%%MAP%").colorOnMap == 0x84);
	CHECK(ActionOf(L"%BLUE%%NAME%%MAP%").colorOnMap == 0x97);
	CHECK(ActionOf(L"%TAN%%NAME%%MAP%").colorOnMap == 0x5A);
	CHECK(ActionOf(L"%ORANGE%%NAME%%MAP%").colorOnMap == 0x60);
	CHECK(ActionOf(L"%PURPLE%%NAME%%MAP%").colorOnMap == 0x9B);
	// the later of two colours wins, whatever their order in the palette
	CHECK(ActionOf(L"%PURPLE%x %GREEN%%NAME%%MAP%").colorOnMap == 0x84);
	CHECK(ActionOf(L"%GREEN%x %PURPLE%%NAME%%MAP%").colorOnMap == 0x9B);
	// a colour after %MAP% does not count
	CHECK(ActionOf(L"%GREEN%%NAME%%MAP%%BLUE%!").colorOnMap == 0x84);
	// DARK_GREEN is not GREEN
	CHECK(ActionOf(L"%DARK_GREEN%x%BLUE%%NAME%%MAP%").colorOnMap == 0x97);
}

TEST_CASE("legacy %MAP% also sets the border unless the rule sets one") {
	const Action& a = ActionOf(L"%BLUE%%NAME%%MAP%");
	CHECK(a.borderColor == 0x97);
	const Action& b = ActionOf(L"%BLUE%%NAME%%MAP%%BORDER-0A%");
	CHECK(b.borderColor == 0x0A);
	CHECK(b.colorOnMap == 0x97);
}

// BUG: BuildAction's legacy %MAP% handling picks "the last colour keyword before %MAP%" (the colour
// the text is in at that point, as the test above shows), but it only looks at the FIRST occurrence
// of each colour keyword (name.find). When a colour is used again after another colour
// (%BLUE% .. %GREEN% .. %BLUE%%MAP%), the blue in effect at %MAP% loses to green and the minimap icon
// gets the wrong colour. Searching for the last occurrence before %MAP% (rfind) fixes it.
TEST_CASE("legacy %MAP% uses a colour keyword repeated just before it" * doctest::should_fail()) {
	CHECK(ActionOf(L"%BLUE%x %GREEN%y %BLUE%%NAME%%MAP%").colorOnMap == 0x97);
}

TEST_CASE("%TIER-n% sets the notification tier and is removed from the name") {
	const Action& zero = ActionOf(L"%NAME%%TIER-0%");
	CHECK(zero.pingLevel == 0);
	CHECK(zero.name == L"%NAME%");
	const Action& nine = ActionOf(L"%NAME%%tier-9%%DOT-97%");
	CHECK(nine.pingLevel == 9);
	CHECK(nine.dotColor == 0x97);
	CHECK(nine.name == L"%NAME%");
}

// BUG: ParsePingLevel only accepts one digit (%TIER-([0-9])%). The wiki's keyword table says of
// %TIER-0% "(value can be 0-12)", and the Filter Strictness section says up to 12 custom levels can be
// enabled, referenced as FILTLVL=n / %TIER-n%. %TIER-10%..%TIER-12% are ignored (the rule then
// notifies at every level) and the raw "%TIER-12%" text is shown in the item's name. Accepting
// [0-9]{1,2} fixes it.
TEST_CASE("%TIER-n% accepts the two-digit tiers 10 to 12" * doctest::should_fail()) {
	const Action& a = ActionOf(L"%NAME%%TIER-12%");
	CHECK(a.pingLevel == 12);
	CHECK(a.name == L"%NAME%");
}

TEST_CASE("%SOUNDID-n% plays only sounds that exist in sounds.txt") {
	// Wiki: indices 4714-4729 are the PoE drop sounds, so a sounds.txt with 4730 records has them.
	fake::Var(Var_D2CLIENT_SoundRecords) = 4730;
	const Action& last = ActionOf(L"%NAME%%SOUNDID-4729%");
	CHECK(last.soundID == 4729);
	CHECK(last.name == L"%NAME%");
	CHECK(ActionOf(L"%NAME%%soundid-4714%").soundID == 4714);
	// one past the last record: no sound, keyword still removed
	const Action& past = ActionOf(L"%NAME%%SOUNDID-4730%");
	CHECK(past.soundID == 0);
	CHECK(past.name == L"%NAME%");
	CHECK(ActionOf(L"%NAME%%SOUNDID-9999%").soundID == 0);
}

TEST_CASE("%SOUNDID-n% plays nothing before sounds.txt is loaded") {
	fake::Var(Var_D2CLIENT_SoundRecords) = 0;
	CHECK(ActionOf(L"%NAME%%SOUNDID-1%").soundID == 0);
}

// ---- Value keywords --------------------------------------------------------------------------

TEST_CASE("%BASENAME% is the base item name without its colour prefix") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.attrs().name = L"Hand Axe";
	CHECK(NameWith("%NAME% (%BASENAME%)", item, L"The Gnasher") == L"The Gnasher (Hand Axe)");
	TestItem rune("r30", ITEM_QUALITY_NORMAL);
	rune.attrs().name = Color(L"8") + L"Ber Rune";
	CHECK(NameWith("%BASENAME%", rune, L"Ber Rune") == L"Ber Rune");
}

TEST_CASE("item property keywords show the item's values") {
	TestItem item("hax", ITEM_QUALITY_NORMAL);
	item.ItemLevel(87).Stat(STAT_SOCKETS, 4);
	item.txt().dwspeed = static_cast<DWORD>(-10);
	item.txt().brangeadder = 2;
	CHECK(NameWith("%CODE% ilvl%ILVL% os%SOCKETS% spd%WPNSPD% rng%RANGE%", item) == L"hax ilvl87 os4 spd-10 rng2");

	TestItem arrows("aqv", ITEM_QUALITY_NORMAL);
	arrows.Stat(STAT_AMMOQUANTITY, 350);
	CHECK(NameWith("%NAME% x%QTY%", arrows, L"Arrows") == L"Arrows x350");
}

TEST_CASE("%RUNENUM% and %RUNENAME% describe runes; other items show 0 and nothing") {
	TestItem vex("r26", ITEM_QUALITY_NORMAL);
	vex.attrs().miscFlags = ITEM_GROUP_RUNE;
	vex.attrs().name = L"Vex Rune";
	CHECK(NameWith("%RUNENAME% (#%RUNENUM%)", vex, L"Vex Rune") == L"Vex (#26)");

	TestItem el("r01", ITEM_QUALITY_NORMAL);
	el.attrs().miscFlags = ITEM_GROUP_RUNE;
	el.attrs().name = L"El Rune";
	CHECK(NameWith("%RUNENAME% %RUNENUM%", el, L"El Rune") == L"El 1");

	TestItem axe("hax", ITEM_QUALITY_NORMAL);
	axe.attrs().name = L"Hand Axe";
	CHECK(NameWith("[%RUNENAME%][%RUNENUM%]", axe) == L"[][0]");
}

TEST_CASE("%GEMLEVEL% and %GEMTYPE% describe gems and are empty for other items") {
	TestItem gem("glr", ITEM_QUALITY_NORMAL);
	gem.attrs().miscFlags = ITEM_GROUP_RUBY | ITEM_GROUP_FLAWLESS;
	CHECK(NameWith("%GEMLEVEL% %GEMTYPE%", gem) == L"Flawless Ruby");
	gem.attrs().miscFlags = ITEM_GROUP_SKULL | ITEM_GROUP_CHIPPED;
	CHECK(NameWith("%GEMLEVEL% %GEMTYPE%", gem) == L"Chipped Skull");
	gem.attrs().miscFlags = ITEM_GROUP_AMETHYST | ITEM_GROUP_PERFECT;
	CHECK(NameWith("%GEMLEVEL% %GEMTYPE%", gem) == L"Perfect Amethyst");

	TestItem axe("hax", ITEM_QUALITY_NORMAL);
	CHECK(NameWith("[%GEMLEVEL%][%GEMTYPE%]", axe) == L"[][]");
}

TEST_CASE("%ALVL% follows the D2 affix level formula") {
	// alvl = ilvl - qlvl/2 if ilvl < 99 - qlvl/2, else 2*ilvl - 99; magic level adds to ilvl (max 99).
	TestItem item("wnd", ITEM_QUALITY_MAGIC);
	item.ItemLevel(40);
	item.attrs().qualityLevel = 20;
	CHECK(NameWith("%ALVL%", item) == L"30");

	item.ItemLevel(85);
	item.attrs().qualityLevel = 60;  // 85 >= 99 - 30
	CHECK(NameWith("%ALVL%", item) == L"71");

	item.ItemLevel(90);
	item.attrs().qualityLevel = 10;
	item.attrs().magicLevel = 3;
	CHECK(NameWith("%ALVL%", item) == L"93");

	item.ItemLevel(98);
	item.attrs().qualityLevel = 0;
	CHECK(NameWith("%ALVL%", item) == L"99");
}

TEST_CASE("%CRAFTALVL% uses half the character level plus half the item level") {
	// PD2 crafting: crafted ilvl = floor(clvl/2) + floor(ilvl/2), then the usual affix level formula.
	fake::SetStat(&fake::Player(), STAT_LEVEL, 91);
	TestItem item("hbl", ITEM_QUALITY_MAGIC);
	item.ItemLevel(85);
	// 45 + 42 = 87, qlvl 0 -> alvl 87
	CHECK(NameWith("%CRAFTALVL%", item) == L"87");

	item.attrs().qualityLevel = 40;
	// crafted ilvl 87 >= 99 - 20 -> 2*87 - 99 = 75
	CHECK(NameWith("%CRAFTALVL%", item) == L"75");
}

TEST_CASE("%LVLREQ% of a magic or rare item is the lowest requirement over all classes") {
	// Charges of a class skill raise the requirement for the other classes; BH shows the lowest one
	// (GetRequiredLevel: "the (lowest) level requirement (for any class)") whatever the player's class.
	fake::Player().dwTxtFileNo = 4;  // Barbarian
	TestItem rare("hax", ITEM_QUALITY_RARE);
	fake::SetLevelRequirement(rare.unit(), 30);
	fake::SetLevelRequirement(rare.unit(), 24, 2);  // Necromancer
	CHECK(NameWith("L%LVLREQ%", rare) == L"L24");
	CHECK(fake::Player().dwTxtFileNo == 4);  // the player is a Barbarian again afterwards

	TestItem magic("wnd", ITEM_QUALITY_MAGIC);
	fake::SetLevelRequirement(magic.unit(), 18);
	fake::SetLevelRequirement(magic.unit(), 12, 6);  // Assassin, the last class
	CHECK(NameWith("L%LVLREQ%", magic) == L"L12");
	CHECK(fake::Player().dwTxtFileNo == 4);
}

TEST_CASE("%PRICE% and %SELLPRICE% are the vendor sell price, %BUYPRICE% the buy price") {
	// Wiki: %PRICE% is the price when selling to a vendor, %SELLPRICE% is identical, %BUYPRICE% is
	// the price when buying from a vendor.
	TestItem item("hax", ITEM_QUALITY_RARE);
	fake::SetPrice(item.unit(), TRANSACTIONTYPE_SELL, 5000);
	fake::SetPrice(item.unit(), TRANSACTIONTYPE_BUY, 20000);
	CHECK(NameWith("%NAME% $%PRICE% $%SELLPRICE% $%BUYPRICE%", item, L"Axe") == L"Axe $5000 $5000 $20000");
}

TEST_CASE("quest items have no %PRICE%") {
	TestItem key("hdm", ITEM_QUALITY_NORMAL);
	key.txt().bquest = 1;
	fake::SetPrice(key.unit(), TRANSACTIONTYPE_SELL, 1);
	CHECK(NameWith("%NAME%[%PRICE%]", key, L"Horadric Malus") == L"Horadric Malus[]");
}

TEST_CASE("%RES% is the lowest of the four resistances, 0 unless all four are present") {
	TestItem ring("rin", ITEM_QUALITY_RARE);
	ring.Stat(STAT_FIRERESIST, 30).Stat(STAT_LIGHTNINGRESIST, 25).Stat(STAT_COLDRESIST, 40);
	CHECK(NameWith("@%RES%", ring) == L"@0");
	ring.Stat(STAT_POISONRESIST, 35);
	CHECK(NameWith("@%RES%", ring) == L"@25");
}

TEST_CASE("%ED% is the item's own enhanced defense or damage, %EDEF%/%EDAM% include runeword bonuses") {
	// Wiki: ED refers to defense for armor and damage for weapons, and does not include bonuses from
	// runewords or sockets; EDEF/EDAM do.
	TestItem weapon("hax", ITEM_QUALITY_NORMAL);
	weapon.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	weapon.Stat(STAT_ENHANCEDMAXIMUMDAMAGE, 50).Stat(STAT_ENHANCEDDEFENSE, 9);
	fake::SetRunewordStat(weapon.unit(), STAT_ENHANCEDMAXIMUMDAMAGE, 200);
	CHECK(NameWith("%ED% %EDAM%", weapon) == L"50 250");

	TestItem armor("xtp", ITEM_QUALITY_SUPERIOR);
	armor.attrs().armorFlags = ITEM_GROUP_ALLARMOR;
	armor.Stat(STAT_ENHANCEDDEFENSE, 15).Stat(STAT_ENHANCEDMAXIMUMDAMAGE, 7);
	CHECK(NameWith("%ED% %EDEF%", armor) == L"15 15");
}

TEST_CASE("stat keywords show the item's stat values") {
	TestItem item("rin", ITEM_QUALITY_RARE);
	item.Stat(STAT_LIFELEECH, 7).Stat(STAT_STRENGTH, 15).Stat(STAT_FASTERCAST, 10);
	item.Stat(STAT_MAXHP, 25 * 256);  // life is stored in 1/256 points
	CHECK(NameWith("%STAT60% %STR% %FCR% %LIFE% %DEX%", item) == L"7 15 10 25 0");
}

TEST_CASE("skill keywords read the skill's layer of the skill stats") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(STAT_SINGLESKILL, 3, 48);     // +3 Nova
	item.Stat(STAT_NONCLASSSKILL, 20, 74);  // +20 Corpse Explosion (oskill)
	item.Stat(STAT_CLASSSKILLS, 2, 2);      // +2 Necromancer skills
	item.Stat(STAT_SKILLTAB, 4, 25);        // +4 skill tab 25
	CHECK(NameWith("%SK48% %OS74% %CLSK2% %TABSK25% %SK49%", item) == L"3 20 2 4 0");
}

TEST_CASE("%MULTIstat,layer% reads any layered stat and %CHARSTATn% reads the character") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(195, 15, 3028);  // 15% chance to cast (skill 47 level 20) on attack
	fake::SetStat(&fake::Player(), STAT_STRENGTH, 150);
	CHECK(NameWith("%MULTI195,3028% %MULTI195,3029% %CHARSTAT0%", item) == L"15 0 150");
}

TEST_CASE("stat keywords read the last stat of the table and nothing past it") {
	// STAT_MAX / SKILL_MAX are the ItemStatCost.txt / Skills.txt row counts (359 / 357 by default).
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	item.Stat(358, 5);
	fake::SetStat(&fake::Player(), 358, 6);
	item.Stat(358, 7, 1);
	CHECK(NameWith("[%STAT358%][%CHARSTAT358%][%MULTI358,1%]", item) == L"[5][6][7]");
	CHECK(NameWith("[%STAT360%][%CHARSTAT360%][%MULTI360,1%]", item) == L"[][][]");
	CHECK(NameWith("[%SK358%][%OS358%][%CLSK358%][%TABSK358%]", item) == L"[][][][]");
}

// ---- New lines, conditional spaces and lines -------------------------------------------------

TEST_CASE("%NL% breaks item names only for identified magic+, runeword and superior/staffmod items") {
	// Wiki: %NL% works within descriptions, or for ID !NMAG, RW or SHOP items.
	TestItem magic("rin", ITEM_QUALITY_MAGIC);
	CHECK(NameWith("A%NL%B", magic) == L"A\nB");

	TestItem plain("hax", ITEM_QUALITY_NORMAL);
	CHECK(NameWith("A%NL%B", plain) == L"AB");

	TestItem unid("rin", ITEM_QUALITY_RARE);
	unid.Flags(0);
	CHECK(NameWith("A%NL%B", unid) == L"AB");

	TestItem runeword("hax", ITEM_QUALITY_NORMAL);
	runeword.Flags(ITEM_IDENTIFIED | ITEM_RUNEWORD);
	CHECK(NameWith("A%NL%B%NL%C", runeword) == L"A\nB\nC");
}

TEST_CASE("non-magic items that can carry staffmods or superior mods get a single %NL% in the name") {
	// ReplaceContext: "non-mag item capable of having staffmods or similar mods" may show one line.
	TestItem superior("hax", ITEM_QUALITY_SUPERIOR);
	CHECK(NameWith("A%NL%B%NL%C", superior) == L"A\nBC");

	TestItem staff("sst", ITEM_QUALITY_NORMAL);
	staff.attrs().staffmodClass = CLASS_SOR;
	CHECK(NameWith("A%NL%B%CL%C", staff) == L"A\nBC");
}

TEST_CASE("%NL% always works in descriptions") {
	TestItem plain("hax", ITEM_QUALITY_NORMAL);
	plain.ItemLevel(30);
	CHECK(DescriptionWith("{Affix Level: %ALVL%%NL%Item Level: %ILVL%}", plain) == L"Affix Level: 30\nItem Level: 30");
}

TEST_CASE("%CL% starts a new line only between non-empty lines") {
	// Wiki: like %NL% but never creates blank lines or consecutive new lines.
	TestItem item("hax", ITEM_QUALITY_NORMAL);
	CHECK(DescriptionWith("{A%CL%B}", item) == L"A\nB");
	CHECK(DescriptionWith("{A%CL%%CL%%CL%B}", item) == L"A\nB");
	CHECK(DescriptionWith("{%CL%A%CL%}", item) == L"A");
	CHECK(DescriptionWith("{A%NL%%CL%B%CL%%NL%C}", item) == L"A\nB\nC");
}

TEST_CASE("%CS% adds a space only between two non-whitespace characters") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("A%CS%B|A %CS%B|A%CS% B|A%CS%%CS%B", item) == L"A B|A B|A B|A B");
	CHECK(NameWith("%CS%A%CS%", item) == L"A");
	// an empty keyword between two conditional spaces leaves just one space
	CHECK(NameWith("%NAME%%CS%%GEMTYPE%%CS%!", item, L"Axe") == L"Axe !");
}

// ---- Length limits ---------------------------------------------------------------------------

TEST_CASE("item names are capped at 56 displayed characters") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith(std::string(56, 'x'), item) == std::wstring(56, L'x'));
	CHECK(NameWith(std::string(57, 'x'), item) == std::wstring(56, L'x'));
	CHECK(NameWith("%NAME%%NAME%", item, std::wstring(40, L'y')) == std::wstring(56, L'y'));
}

TEST_CASE("colour codes do not count toward the 56-character name cap") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(NameWith("%RED%" + std::string(28, 'a') + "%BLUE%" + std::string(30, 'b'), item) ==
		Color(L"1") + std::wstring(28, L'a') + Color(L"3") + std::wstring(28, L'b'));
	// a colour right after the 56th character is kept (it colours a shop item's price)
	CHECK(NameWith("%GOLD%" + std::string(56, 'x') + "%WHITE%", item) ==
		Color(L"4") + std::wstring(56, L'x') + Color(L"0"));
	// a colour beyond the cap is dropped with the text it would colour
	CHECK(NameWith(std::string(58, 'x') + "%WHITE%y", item) == std::wstring(56, L'x'));
}

TEST_CASE("descriptions are not limited to the name cap") {
	TestItem item("hax", ITEM_QUALITY_UNIQUE);
	CHECK(DescriptionWith("{" + std::string(300, 'd') + "}", item) == std::wstring(300, L'd'));
}

TEST_CASE("tome names and descriptions are capped at 126 characters") {
	// Constants.h: TP & ID tomes have a smaller text limit (127 including the terminator).
	TestItem tome("tbk", ITEM_QUALITY_NORMAL);
	CHECK(DescriptionWith("{" + std::string(200, 'd') + "}", tome) == std::wstring(126, L'd'));
	TestItem idTome("ibk", ITEM_QUALITY_NORMAL);
	CHECK(DescriptionWith("{" + std::string(127, 'd') + "}", idTome) == std::wstring(126, L'd'));

	// Outside a shop any name stops at 56; a tome in a shop gets the long shop limit, cut to 126.
	UnitAny akara = {};
	akara.dwTxtFileNo = NPCID_Akara;
	Inventory shop = {};
	shop.pOwner = &akara;
	tome.data().pOwnerInventory = &shop;
	CHECK(NameWith(std::string(200, 'n'), tome) == std::wstring(126, L'n'));
}

TEST_CASE("items in a shop allow longer names and %NL% in the name") {
	UnitAny charsi = {};
	charsi.dwTxtFileNo = NPCID_Charsi;
	Inventory shop = {};
	shop.pOwner = &charsi;

	TestItem item("hax", ITEM_QUALITY_NORMAL);
	item.data().pOwnerInventory = &shop;
	CHECK(NameWith(std::string(60, 'x') + "%NL%y", item) == std::wstring(60, L'x') + L"\ny");

	// the same item outside a shop
	UnitAny player = {};
	Inventory own = {};
	own.pOwner = &player;
	item.data().pOwnerInventory = &own;
	CHECK(NameWith(std::string(60, 'x') + "%NL%y", item) == std::wstring(56, L'x'));
}

}  // TEST_SUITE
