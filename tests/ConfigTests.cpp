#include "doctest/doctest.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "BH.h"
#include "Config.h"

// Config.cpp: set once LoadConfig had to create BH.json, never cleared by BH itself. Tests that
// load settings start from a fresh process state, so they clear it (see JsonFiles).
extern bool bCreateFile;

namespace {

typedef std::vector<std::pair<std::string, std::string>> KeyVals;

std::string TestDir() {
	char path[MAX_PATH] = {};
	GetTempPathA(MAX_PATH, path);
	std::string dir = std::string(path) + "bh-config-tests-" + std::to_string(GetCurrentProcessId()) + "\\";
	CreateDirectoryA(dir.c_str(), nullptr);
	return dir;
}

void WriteText(const std::string& path, const std::string& text) {
	std::ofstream out(path, std::ios::binary | std::ios::trunc);
	out << text;
}

std::string ReadText(const std::string& path) {
	std::ifstream in(path, std::ios::binary);
	std::stringstream ss;
	ss << in.rdbuf();
	return ss.str();
}

bool Exists(const std::string& path) {
	return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

// Loot filter files under a temp BH::path; every file written is deleted afterwards.
struct FilterDir {
	std::string dir;
	std::vector<std::string> written;

	FilterDir() : dir(TestDir()) { BH::path = dir; }
	~FilterDir() {
		for (size_t i = 0; i < written.size(); i++) DeleteFileA((dir + written[i]).c_str());
		RemoveDirectoryA(dir.c_str());
	}
	void Write(const std::string& name, const std::string& text) {
		WriteText(dir + name, text);
		written.push_back(name);
	}
};

// "key=value" lines, so mismatches print readably.
std::string Dump(const KeyVals& kv) {
	std::string out;
	for (size_t i = 0; i < kv.size(); i++) out += "[" + kv[i].first + "]=[" + kv[i].second + "]\n";
	return out;
}

std::string Rules(Config& config, const std::string& key = "ItemDisplay") {
	KeyVals values;
	config.ReadMapList(key, values);
	return Dump(values);
}

// Parses `text` as loot.filter and returns the ItemDisplay rules read back from it.
std::string ParseRules(const std::string& text, const std::string& key = "ItemDisplay") {
	FilterDir files;
	files.Write("loot.filter", text);
	Config config("loot.filter");
	REQUIRE(config.Parse());
	return Rules(config, key);
}

// BH.json / BH.json.bak in a temp dir, deleted afterwards.
struct JsonFiles {
	std::string dir, main, bak;

	JsonFiles() : dir(TestDir()), main(dir + "BH.json"), bak(dir + "BH.json.bak") {
		DeleteFileA(main.c_str());
		DeleteFileA(bak.c_str());
		App.jsonFile = main;
		App.jsonBackup = bak;
		bCreateFile = false;
	}
	~JsonFiles() {
		DeleteFileA(main.c_str());
		DeleteFileA(bak.c_str());
		RemoveDirectoryA(dir.c_str());
		bCreateFile = false;
	}
	json Main() { return json::parse(ReadText(main)); }
	json Bak() { return json::parse(ReadText(bak)); }
};

json::json_pointer Ptr(const char* p) {
	return json::json_pointer(p);
}

}  // namespace

TEST_SUITE("Config") {

// ---------------------------------------------------------------------------------------------
// Loot filter file reader: Config::Parse + ReadMapList
// ---------------------------------------------------------------------------------------------

TEST_CASE("Parse reads key: value lines in file order and trims spaces and tabs around both") {
	CHECK(ParseRules(
		"ItemDisplay[hax]:   Hand Axe   \n"
		"   ItemDisplay[UNI]\t:\tUnique\t\n"
		"ItemDisplay[SET]:Set\n") ==
		"[hax]=[Hand Axe]\n"
		"[UNI]=[Unique]\n"
		"[SET]=[Set]\n");
}

TEST_CASE("Parse keeps every colon after the first as part of the value") {
	CHECK(ParseRules("ItemDisplay[r33]: Zod: 33\n") == "[r33]=[Zod: 33]\n");
}

TEST_CASE("Parse treats // as the start of a comment anywhere on the line") {
	CHECK(ParseRules(
		"// whole-line comment: ItemDisplay[hax]: Hidden\n"
		"//ItemDisplay[axe]: Hidden too\n"
		"ItemDisplay[UNI]: Unique // trailing note\n"
		"ItemDisplay[SET]: Set//no space before the comment\n") ==
		"[UNI]=[Unique]\n"
		"[SET]=[Set]\n");
}

TEST_CASE("Parse skips blank lines, whitespace-only lines and an empty file") {
	CHECK(ParseRules(
		"\n"
		"   \n"
		"\t\n"
		"ItemDisplay[hax]: A\n"
		"\n"
		"ItemDisplay[axe]: B\n") ==
		"[hax]=[A]\n"
		"[axe]=[B]\n");
	CHECK(ParseRules("") == "");
}

TEST_CASE("Parse reads a last line that has no trailing newline") {
	CHECK(ParseRules("ItemDisplay[hax]: A\nItemDisplay[axe]: B") == "[hax]=[A]\n[axe]=[B]\n");
}

TEST_CASE("Parse does not leave carriage returns of CRLF files in keys or values") {
	CHECK(ParseRules("ItemDisplay[hax]: A\r\nItemDisplay[axe]: B\r\n") == "[hax]=[A]\n[axe]=[B]\n");
}

// PD2 wiki, Item Filtering: "Whitespace surrounding the Output of each rule gets removed prior to
// evaluation (spaces first, followed by tabs), so tabs are often the best non-space character to
// use". Filters rely on this to pad names with spaces, e.g. `ItemDisplay[RUNE>9]:<TAB> %NAME% <TAB>`.
TEST_CASE("Parse strips spaces first and then tabs, so tabs protect padding spaces in the output") {
	CHECK(ParseRules("ItemDisplay[RUNE>9]:\t %NAME% \t\n") == "[RUNE>9]=[ %NAME% ]\n");
	CHECK(ParseRules("ItemDisplay[RUNE>9]:  \t  %NAME%  \t  \n") == "[RUNE>9]=[  %NAME%  ]\n");
}

// BUG: a line without ':' is not a rule. PD2 wiki, Item Filtering: "If a line doesn't follow this
// format, it won't be considered a rule, which means it won't affect how items will be displayed".
// Parse computes the value from find(':') + 1, which is npos + 1 == 0, so the whole line becomes the
// value and "ItemDisplay[hax]" (colon missing) renames hand axes to the literal "ItemDisplay[hax]".
TEST_CASE("Parse does not turn a line without ':' into a rule" * doctest::should_fail()) {
	CHECK(ParseRules("ItemDisplay[hax]\n") == "");
}

TEST_CASE("Parse keeps duplicate keys as separate entries in file order") {
	CHECK(ParseRules(
		"ItemDisplay[hax]: first\n"
		"ItemDisplay[UNI]: middle\n"
		"ItemDisplay[hax]: second\n") ==
		"[hax]=[first]\n"
		"[UNI]=[middle]\n"
		"[hax]=[second]\n");
}

TEST_CASE("ReadMapList only returns keys that start with exactly key[") {
	const std::string filter =
		"ItemDisplayFilterName[]: Strict\n"
		"ItemDisplay[hax]: Axe\n"
		"ItemDisplayFilter[x]: not a rule\n"
		"ItemDisplay [axe]: space before bracket\n"
		"Something ItemDisplay[2ax]: not at the start\n"
		"ItemDisplay: no brackets\n"
		"ItemDisplay[UNI]: Unique\n";
	CHECK(ParseRules(filter) == "[hax]=[Axe]\n[UNI]=[Unique]\n");
	CHECK(ParseRules(filter, "ItemDisplayFilterName") == "[]=[Strict]\n");
	CHECK(ParseRules(filter, "ItemDisplayFilter") == "[x]=[not a rule]\n");
}

TEST_CASE("ReadMapList returns the text inside ItemDisplay[...]") {
	CHECK(ParseRules("ItemDisplay[]: empty\n") == "[]=[empty]\n");
	CHECK(ParseRules("ItemDisplay[UNI (hax OR axe)]: A\n") == "[UNI (hax OR axe)]=[A]\n");
}

TEST_CASE("ReadMapList appends to the caller's list and returns the combined list") {
	FilterDir files;
	files.Write("loot.filter", "ItemDisplay[hax]: A\n");
	Config config("loot.filter");
	REQUIRE(config.Parse());

	KeyVals values;
	values.push_back(std::make_pair(std::string("existing"), std::string("X")));
	KeyVals returned = config.ReadMapList("ItemDisplay", values);
	CHECK(Dump(values) == "[existing]=[X]\n[hax]=[A]\n");
	CHECK(Dump(returned) == "[existing]=[X]\n[hax]=[A]\n");
}

TEST_CASE("Parse reloads replace the previous contents") {
	FilterDir files;
	files.Write("loot.filter", "ItemDisplay[hax]: A\nItemDisplay[axe]: B\n");
	Config config("loot.filter");
	REQUIRE(config.Parse());
	REQUIRE(Rules(config) == "[hax]=[A]\n[axe]=[B]\n");

	files.Write("loot.filter", "ItemDisplay[2ax]: C\n");
	REQUIRE(config.Parse());
	CHECK(Rules(config) == "[2ax]=[C]\n");

	files.Write("loot.filter", "");
	REQUIRE(config.Parse());
	CHECK(Rules(config) == "");
}

TEST_CASE("Parse returns false for a missing file or an empty name and keeps the loaded rules") {
	FilterDir files;
	Config missing("does-not-exist.filter");
	CHECK_FALSE(missing.Parse());
	CHECK(Rules(missing) == "");

	Config unnamed("");
	CHECK_FALSE(unnamed.Parse());

	files.Write("loot.filter", "ItemDisplay[hax]: A\n");
	Config config("loot.filter");
	REQUIRE(config.Parse());
	DeleteFileA((files.dir + "loot.filter").c_str());
	CHECK_FALSE(config.Parse());
	CHECK(Rules(config) == "[hax]=[A]\n");
}

TEST_CASE("Config files are resolved relative to BH::path") {
	FilterDir files;
	Config config("loot.filter");
	CHECK(config.GetConfigName() == files.dir + "loot.filter");

	files.Write("loot.filter", "ItemDisplay[hax]: default\n");
	files.Write("other.filter", "ItemDisplay[axe]: other\n");
	config.SetConfigName("other.filter");
	CHECK(config.GetConfigName() == files.dir + "other.filter");
	REQUIRE(config.Parse());
	CHECK(Rules(config) == "[axe]=[other]\n");
}

// ---------------------------------------------------------------------------------------------
// BH.json accessors
// ---------------------------------------------------------------------------------------------

TEST_CASE("GetInt reads the value and falls back to the default for a missing or malformed entry") {
	Config config("");
	SettingsInt setting = { 0, 7, 0, 0 };  // no bounds
	CHECK(config.GetInt(Ptr("/s"), "n", setting) == 7);  // no section

	App.jsonConfig = json::parse(R"({"s": {"n": -25, "str": "12", "arr": [1], "obj": {"n": 3}}, "notobj": 5})");
	CHECK(config.GetInt(Ptr("/s"), "n", setting) == -25);
	CHECK(config.GetInt(Ptr("/s"), "missing", setting) == 7);
	CHECK(config.GetInt(Ptr("/s"), "str", setting) == 7);
	CHECK(config.GetInt(Ptr("/s"), "arr", setting) == 7);
	CHECK(config.GetInt(Ptr("/s"), "obj", setting) == 7);
	CHECK(config.GetInt(Ptr("/notobj"), "n", setting) == 7);
	CHECK(config.GetInt(Ptr("/s/obj"), "n", setting) == 3);
}

TEST_CASE("GetInt clamps to the setting's bounds, keeping values exactly on them") {
	Config config("");
	const SettingsInt& join = App.bnet.failToJoin;
	App.jsonConfig = json::object();
	App.jsonConfig["bnet"]["lo"] = join.minValue - 1;
	App.jsonConfig["bnet"]["min"] = join.minValue;
	App.jsonConfig["bnet"]["max"] = join.maxValue;
	App.jsonConfig["bnet"]["hi"] = join.maxValue + 1;
	App.jsonConfig["bnet"]["neg"] = -1;
	CHECK(config.GetInt(Ptr("/bnet"), "lo", join) == join.minValue);
	CHECK(config.GetInt(Ptr("/bnet"), "min", join) == join.minValue);
	CHECK(config.GetInt(Ptr("/bnet"), "max", join) == join.maxValue);
	CHECK(config.GetInt(Ptr("/bnet"), "hi", join) == join.maxValue);
	CHECK(config.GetInt(Ptr("/bnet"), "neg", join) == join.minValue);

	const SettingsInt& level = App.lootfilter.filterLevel;
	App.jsonConfig["lootfilter"]["a"] = level.maxValue;
	App.jsonConfig["lootfilter"]["b"] = level.maxValue + 1;
	App.jsonConfig["lootfilter"]["c"] = 0;
	CHECK(config.GetInt(Ptr("/lootfilter"), "a", level) == level.maxValue);
	CHECK(config.GetInt(Ptr("/lootfilter"), "b", level) == level.maxValue);
	CHECK(config.GetInt(Ptr("/lootfilter"), "c", level) == 0);
}

TEST_CASE("GetBool reads JSON booleans only and otherwise keeps the default") {
	Config config("");
	SettingsBool on = { true, true };
	SettingsBool off = { false, false };
	App.jsonConfig = json::parse(R"({"s": {"t": true, "f": false, "one": 1, "str": "true", "nul": null}})");
	CHECK(config.GetBool(Ptr("/s"), "t", off) == true);
	CHECK(config.GetBool(Ptr("/s"), "f", on) == false);
	CHECK(config.GetBool(Ptr("/s"), "one", off) == false);
	CHECK(config.GetBool(Ptr("/s"), "str", off) == false);
	CHECK(config.GetBool(Ptr("/s"), "nul", on) == true);
	CHECK(config.GetBool(Ptr("/s"), "missing", on) == true);
	CHECK(config.GetBool(Ptr("/other"), "t", off) == false);
}

TEST_CASE("GetString reads strings and keeps the default for other types") {
	Config config("");
	SettingsString setting = { "", "json" };
	App.jsonConfig = json::parse(R"({"s": {"str": "stash", "empty": "", "num": 5, "arr": ["x"]}})");
	CHECK(config.GetString(Ptr("/s"), "str", setting) == "stash");
	CHECK(config.GetString(Ptr("/s"), "empty", setting) == "");
	CHECK(config.GetString(Ptr("/s"), "num", setting) == "json");
	CHECK(config.GetString(Ptr("/s"), "arr", setting) == "json");
	CHECK(config.GetString(Ptr("/s"), "missing", setting) == "json");
}

TEST_CASE("GetToggle reads enabled and hotkey independently, each defaulting when absent") {
	Config config("");
	SettingsToggle setting = { {}, { 0x41, true } };  // VK_A, enabled
	App.jsonConfig = json::parse(R"({"party": {
		"both": {"enabled": false, "hotkey": "VK_F5"},
		"enabledOnly": {"enabled": false},
		"hotkeyOnly": {"hotkey": "VK_NUMPAD0"},
		"none": {}
	}})");

	Toggle t = config.GetToggle(Ptr("/party"), "both", setting);
	CHECK(t.isEnabled == false);
	CHECK(t.hotkey == 0x74u);  // VK_F5

	t = config.GetToggle(Ptr("/party"), "enabledOnly", setting);
	CHECK(t.isEnabled == false);
	CHECK(t.hotkey == 0x41u);

	t = config.GetToggle(Ptr("/party"), "hotkeyOnly", setting);
	CHECK(t.isEnabled == true);
	CHECK(t.hotkey == 0x60u);  // VK_NUMPAD0

	t = config.GetToggle(Ptr("/party"), "none", setting);
	CHECK(t.isEnabled == true);
	CHECK(t.hotkey == 0x41u);

	t = config.GetToggle(Ptr("/party"), "missing", setting);
	CHECK(t.isEnabled == true);
	CHECK(t.hotkey == 0x41u);
}

TEST_CASE("GetToggle resolves hotkey names case-insensitively and unknown names to no key") {
	Config config("");
	SettingsToggle setting = { {}, { 0x41, false } };
	App.jsonConfig = json::parse(R"({"lootfilter": {"legacy_settings": {
		"lower": {"hotkey": "vk_f12"},
		"none": {"hotkey": "None"},
		"bogus": {"hotkey": "VK_NOPE"}
	}}})");
	CHECK(config.GetToggle(Ptr("/lootfilter/legacy_settings"), "lower", setting).hotkey == 0x7Bu);
	CHECK(config.GetToggle(Ptr("/lootfilter/legacy_settings"), "none", setting).hotkey == 0u);
	CHECK(config.GetToggle(Ptr("/lootfilter/legacy_settings"), "bogus", setting).hotkey == 0u);
}

TEST_CASE("GetKey resolves a key name and keeps the default when the entry is missing") {
	Config config("");
	SettingsKey setting = { 0, 0x74 };
	App.jsonConfig = json::parse(R"({"lootfilter": {"inc": "VK_ADD_NOT_A_KEY", "dec": "VK_PAGEDN", "prev": "vk_back"}})");
	CHECK(config.GetKey(Ptr("/lootfilter"), "dec", setting) == 0x22u);
	CHECK(config.GetKey(Ptr("/lootfilter"), "prev", setting) == 0x08u);
	CHECK(config.GetKey(Ptr("/lootfilter"), "inc", setting) == 0u);
	CHECK(config.GetKey(Ptr("/lootfilter"), "missing", setting) == 0x74u);
	CHECK(config.GetKey(Ptr("/stash_export"), "dec", setting) == 0x74u);
}

TEST_CASE("GetArray and GetAssoc replace the defaults with the configured values") {
	Config config("");
	App.jsonConfig = json::parse(R"({"screen_info": {
		"automap_info": ["%GAMENAME%", "%PING%"],
		"empty": [],
		"additional_stats": {"b": "2", "a": "1"}
	}})");

	std::vector<std::string> arr = config.GetArray(Ptr("/screen_info"), "automap_info", App.screen.automapInfo);
	REQUIRE(arr.size() == 2);
	CHECK(arr[0] == "%GAMENAME%");
	CHECK(arr[1] == "%PING%");
	CHECK(config.GetArray(Ptr("/screen_info"), "empty", App.screen.automapInfo).empty());
	CHECK(config.GetArray(Ptr("/screen_info"), "missing", App.screen.automapInfo) == App.screen.automapInfo.defValues);

	std::map<std::string, std::string> assoc = config.GetAssoc(Ptr("/screen_info"), "additional_stats", App.bnet.whisperColor);
	REQUIRE(assoc.size() == 2);
	CHECK(assoc["a"] == "1");
	CHECK(assoc["b"] == "2");
	CHECK(config.GetAssoc(Ptr("/bnet"), "whisper_color", App.bnet.whisperColor) == App.bnet.whisperColor.defValues);
}

// BUG (this test and the next three): GetInt/GetFloat/GetBool/GetString catch conversion errors and
// keep the default, but GetToggle, GetKey, GetArray and GetAssoc do not. A hand-edited BH.json with
// a wrong type in one of these entries makes LoadConfig throw nlohmann::type_error out of BH startup
// (or a config reload) instead of falling back to the default like every other setting.
TEST_CASE("GetToggle falls back to the default for wrongly typed entries" * doctest::should_fail()) {
	Config config("");
	SettingsToggle setting = { {}, { 0x41, true } };
	App.jsonConfig = json::parse(R"({"party": {"auto_party": {"enabled": "no", "hotkey": 116}}})");
	Toggle t = { 0, false };
	REQUIRE_NOTHROW(t = config.GetToggle(Ptr("/party"), "auto_party", setting));
	CHECK(t.isEnabled == true);
	CHECK(t.hotkey == 0x41u);
}

TEST_CASE("GetKey falls back to the default for a non-string entry" * doctest::should_fail()) {
	Config config("");
	SettingsKey setting = { 0, 0x74 };
	App.jsonConfig = json::parse(R"({"stash_export": {"export_gear": 116}})");
	unsigned int key = 0;
	REQUIRE_NOTHROW(key = config.GetKey(Ptr("/stash_export"), "export_gear", setting));
	CHECK(key == 0x74u);
}

TEST_CASE("GetArray falls back to the default for a non-array entry" * doctest::should_fail()) {
	Config config("");
	App.jsonConfig = json::parse(R"({"screen_info": {"automap_info": "Name: %GAMENAME%"}})");
	std::vector<std::string> arr;
	REQUIRE_NOTHROW(arr = config.GetArray(Ptr("/screen_info"), "automap_info", App.screen.automapInfo));
	CHECK(arr == App.screen.automapInfo.defValues);
}

TEST_CASE("GetAssoc falls back to the default for a non-object entry" * doctest::should_fail()) {
	Config config("");
	App.jsonConfig = json::parse(R"({"bnet": {"whisper_color": {"*chat": 9}}})");
	std::map<std::string, std::string> assoc;
	REQUIRE_NOTHROW(assoc = config.GetAssoc(Ptr("/bnet"), "whisper_color", App.bnet.whisperColor));
	CHECK(assoc == App.bnet.whisperColor.defValues);
}

// ---------------------------------------------------------------------------------------------
// LoadConfig / SaveConfig
// ---------------------------------------------------------------------------------------------

TEST_CASE("LoadConfig puts each BH.json key into its App setting") {
	JsonFiles files;
	WriteText(files.main, R"({
		"bnet": {"fail_to_join": 6000, "game_list_refresh": 2500, "save_last_game": "baal-01",
		         "autofill_next_game": false, "show_hell_difficulty": false,
		         "whisper_color": {"*friend": "2"}},
		"general": {"stats_on_right": true},
		"lootfilter": {"filter_level": 5, "last_filter_level": 3, "filter_level_increase": "VK_F5",
		               "filter_level_decrease": "VK_F6", "filter_level_previous": "VK_F7",
		               "advanced_item_display": false, "show_iLvl": true, "detailed_notifications": 2,
		               "allow_unknown_items": {"enabled": true, "hotkey": "VK_U"},
		               "always_show_stat_ranges": true, "drop_sounds": false,
		               "legacy_settings": {"show_ethereal": {"enabled": true, "hotkey": "VK_E"},
		                                   "drop_notifications": {"enabled": false}}},
		"game": {"experience_meter": true, "always_show_items": true},
		"party": {"auto_party": {"enabled": false, "hotkey": "VK_P"}},
		"screen_info": {"automap_info": ["%LEVEL%"], "hide_game_password": true},
		"stash_export": {"export_gear": "VK_G", "mustache_default": "stash",
		                 "export_on_menu": {"enabled": true}},
		"bh_ui": {"is_minimized": false, "opened_x": 300, "opened_y": 250, "size_x": 5000}
	})");
	Config config(App.jsonFile);
	config.LoadConfig();

	CHECK(App.bnet.failToJoin.value == 6000);
	CHECK(App.bnet.refreshTime.value == 2500);
	CHECK(App.bnet.saveLastGame.value == "baal-01");
	CHECK(App.bnet.autofillNextGame.value == false);
	CHECK(App.bnet.autofillLastGame.value == App.bnet.autofillLastGame.defValue);  // absent
	CHECK(App.bnet.showHellDiff.value == false);
	CHECK(App.bnet.showNormalDiff.value == App.bnet.showNormalDiff.defValue);  // absent
	CHECK(App.bnet.whisperColor.values.size() == 1);
	CHECK(App.bnet.whisperColor.values["*friend"] == "2");
	CHECK(App.general.statsOnRight.value == true);

	CHECK(App.lootfilter.filterLevel.uValue == 5u);
	CHECK(App.lootfilter.lastFilterLevel.uValue == 3u);
	CHECK(App.lootfilter.filterLevelIncrease.hotkey == 0x74u);
	CHECK(App.lootfilter.filterLevelDecrease.hotkey == 0x75u);
	CHECK(App.lootfilter.filterLevelPrevious.hotkey == 0x76u);
	CHECK(App.lootfilter.enableFilter.value == false);
	CHECK(App.lootfilter.showIlvl.value == true);
	CHECK(App.lootfilter.detailedNotifications.value == 2);
	CHECK(App.lootfilter.allowUnknownItems.toggle.isEnabled == true);
	CHECK(App.lootfilter.allowUnknownItems.toggle.hotkey == 0x55u);
	CHECK(App.lootfilter.alwaysShowStatRanges.value == true);
	CHECK(App.lootfilter.dropSounds.value == false);
	CHECK(App.legacy.showEthereal.toggle.isEnabled == true);
	CHECK(App.legacy.showEthereal.toggle.hotkey == 0x45u);
	CHECK(App.legacy.dropNotifications.toggle.isEnabled == false);
	CHECK(App.legacy.closeNotifications.toggle.isEnabled == App.legacy.closeNotifications.defToggle.isEnabled);  // absent

	CHECK(App.game.experienceMeter.value == true);
	CHECK(App.game.alwaysShowItems.value == true);
	CHECK(App.party.autoParty.toggle.isEnabled == false);
	CHECK(App.party.autoParty.toggle.hotkey == 0x50u);
	CHECK(App.party.autoCorpseLoot.toggle.isEnabled == App.party.autoCorpseLoot.defToggle.isEnabled);  // absent
	REQUIRE(App.screen.automapInfo.values.size() == 1);
	CHECK(App.screen.automapInfo.values[0] == "%LEVEL%");
	CHECK(App.screen.hideGamePassword.value == true);
	CHECK(App.stash.exportGear.hotkey == 0x47u);
	CHECK(App.stash.mustacheDefault.value == "stash");
	CHECK(App.stash.exportOnMenu.toggle.isEnabled == true);
	CHECK(App.stash.includeEquipment.toggle.isEnabled == App.stash.includeEquipment.defToggle.isEnabled);  // absent
	CHECK(App.stash.mustacheOptions.values == App.stash.mustacheOptions.defValues);

	CHECK(App.bhui.isMinimized.value == false);
	CHECK(App.bhui.openedX.value == 300);
	CHECK(App.bhui.openedY.value == 250);
	CHECK(App.bhui.sizeX.value == App.bhui.sizeX.maxValue);  // 5000 clamped to the maximum
	CHECK(App.bhui.minimizedX.value == App.bhui.minimizedX.defValue);  // absent
}

TEST_CASE("LoadConfig hands every hotkey toggle in BH.json to hotkey handling") {
	JsonFiles files;
	// Each toggle with a hotkey in BH.json (the sections SaveConfig writes), given its own key.
	struct { const char* path; const char* key; unsigned int code; } toggles[] = {
		{ "/lootfilter/allow_unknown_items", "VK_A", 0x41 },
		{ "/lootfilter/legacy_settings/show_ethereal", "VK_B", 0x42 },
		{ "/lootfilter/legacy_settings/show_sockets", "VK_C", 0x43 },
		{ "/lootfilter/legacy_settings/show_rune_numbers", "VK_D", 0x44 },
		{ "/lootfilter/legacy_settings/alt_item_style", "VK_E", 0x45 },
		{ "/lootfilter/legacy_settings/color_mod", "VK_F", 0x46 },
		{ "/lootfilter/legacy_settings/shorten_item_names", "VK_G", 0x47 },
		{ "/lootfilter/legacy_settings/drop_notifications", "VK_H", 0x48 },
		{ "/lootfilter/legacy_settings/close_notifications", "VK_I", 0x49 },
		{ "/lootfilter/legacy_settings/verbose_notifications", "VK_J", 0x4A },
		{ "/party/auto_party", "VK_K", 0x4B },
		{ "/party/auto_corpse_loot", "VK_L", 0x4C },
		{ "/stash_export/include_equipment", "VK_M", 0x4D },
		{ "/stash_export/export_on_menu", "VK_N", 0x4E },
	};
	json config = json::object();
	for (size_t i = 0; i < sizeof(toggles) / sizeof(toggles[0]); i++)
		config[Ptr(toggles[i].path) / "hotkey"] = toggles[i].key;
	WriteText(files.main, config.dump());
	Config loader(App.jsonFile);
	loader.LoadConfig();

	for (size_t i = 0; i < sizeof(toggles) / sizeof(toggles[0]); i++) {
		CAPTURE(toggles[i].path);
		int registered = 0;
		for (size_t t = 0; t < App.hotkeyToggles.size(); t++)
			if (App.hotkeyToggles[t]->hotkey == toggles[i].code) registered++;
		CHECK(registered == 1);
	}
}

TEST_CASE("LoadConfig creates BH.json and its backup with the default settings when missing") {
	JsonFiles files;
	Config config(App.jsonFile);
	config.LoadConfig();

	REQUIRE(Exists(files.main));
	REQUIRE(Exists(files.bak));
	json main = files.Main();
	CHECK(main.at(Ptr("/lootfilter/filter_level")) == App.lootfilter.filterLevel.defValue);
	CHECK(main.at(Ptr("/bnet/fail_to_join")) == App.bnet.failToJoin.defValue);
	CHECK(main.at(Ptr("/bnet/game_list_refresh")) == App.bnet.refreshTime.defValue);
	CHECK(main.at(Ptr("/party/auto_party/enabled")) == App.party.autoParty.defToggle.isEnabled);
	CHECK(main.at(Ptr("/stash_export/mustache_default")) == App.stash.mustacheDefault.defValue);
	CHECK(main.at(Ptr("/bnet/whisper_color")).get<std::map<std::string, std::string>>() == App.bnet.whisperColor.defValues);
	CHECK(files.Bak() == main);

	CHECK(App.lootfilter.filterLevel.value == App.lootfilter.filterLevel.defValue);
	CHECK(App.bnet.failToJoin.value == App.bnet.failToJoin.defValue);
}

TEST_CASE("LoadConfig refreshes the backup from a valid BH.json") {
	JsonFiles files;
	WriteText(files.main, R"({"lootfilter": {"filter_level": 4}, "custom": {"kept": 1}})");
	WriteText(files.bak, R"({"lootfilter": {"filter_level": 9}})");
	Config config(App.jsonFile);
	config.LoadConfig();

	CHECK(App.lootfilter.filterLevel.uValue == 4u);
	json bak = files.Bak();
	CHECK(bak.at(Ptr("/lootfilter/filter_level")) == 4);
	CHECK(bak.at(Ptr("/custom/kept")) == 1);
	// A normal load does not rewrite the user's file.
	CHECK(files.Main() == json::parse(R"({"lootfilter": {"filter_level": 4}, "custom": {"kept": 1}})"));
}

TEST_CASE("LoadConfig restores a corrupt or empty BH.json from the backup") {
	const char* corrupt[] = { "{\"lootfilter\": {\"filter_level\": 4", "", "not json" };
	for (size_t i = 0; i < sizeof(corrupt) / sizeof(corrupt[0]); i++) {
		CAPTURE(corrupt[i]);
		App = BHApp();
		JsonFiles files;
		WriteText(files.main, corrupt[i]);
		WriteText(files.bak, R"({"lootfilter": {"filter_level": 9}, "bnet": {"fail_to_join": 7000}})");
		Config config(App.jsonFile);
		config.LoadConfig();

		CHECK(App.lootfilter.filterLevel.uValue == 9u);
		CHECK(App.bnet.failToJoin.value == 7000);
		json main = files.Main();  // repaired: valid JSON again
		CHECK(main.at(Ptr("/lootfilter/filter_level")) == 9);
		CHECK(main.at(Ptr("/bnet/fail_to_join")) == 7000);
	}
}

TEST_CASE("LoadConfig falls back to defaults and rewrites both files when both are corrupt") {
	JsonFiles files;
	WriteText(files.main, "{ broken");
	WriteText(files.bak, "also broken");
	App.lootfilter.filterLevel.uValue = 11;  // stale in-memory value must not survive
	Config config(App.jsonFile);
	config.LoadConfig();

	CHECK(App.lootfilter.filterLevel.value == App.lootfilter.filterLevel.defValue);
	CHECK(App.bnet.failToJoin.value == App.bnet.failToJoin.defValue);
	json main = files.Main();
	CHECK(main.at(Ptr("/lootfilter/filter_level")) == App.lootfilter.filterLevel.defValue);
	CHECK(main.at(Ptr("/bnet/fail_to_join")) == App.bnet.failToJoin.defValue);
	CHECK(files.Bak() == main);
}

TEST_CASE("SaveConfig writes settings under their documented keys with hotkeys as key names") {
	JsonFiles files;
	App.lootfilter.filterLevel.uValue = 7;
	App.lootfilter.filterLevelIncrease.hotkey = 0x74;  // F5
	App.lootfilter.filterLevelDecrease.hotkey = 0;     // unset
	App.lootfilter.showIlvl.value = true;
	App.bnet.failToJoin.value = 5500;
	App.party.autoParty.toggle.isEnabled = false;
	App.party.autoParty.toggle.hotkey = 0x7B;  // F12
	App.legacy.colorMod.toggle.isEnabled = true;
	App.legacy.colorMod.toggle.hotkey = 0x43;  // C
	App.bhui.openedX.value = 321;
	App.stash.exportGear.hotkey = 0x60;  // numpad 0
	Config config(App.jsonFile);
	config.SaveConfig();

	json main = files.Main();
	CHECK(main.at(Ptr("/lootfilter/filter_level")) == 7);
	CHECK(main.at(Ptr("/lootfilter/filter_level_increase")) == "VK_F5");
	CHECK(main.at(Ptr("/lootfilter/filter_level_decrease")) == "None");
	CHECK(main.at(Ptr("/lootfilter/show_iLvl")) == true);
	CHECK(main.at(Ptr("/bnet/fail_to_join")) == 5500);
	CHECK(main.at(Ptr("/party/auto_party/enabled")) == false);
	CHECK(main.at(Ptr("/party/auto_party/hotkey")) == "VK_F12");
	CHECK(main.at(Ptr("/lootfilter/legacy_settings/color_mod/enabled")) == true);
	CHECK(main.at(Ptr("/lootfilter/legacy_settings/color_mod/hotkey")) == "VK_C");
	CHECK(main.at(Ptr("/bh_ui/opened_x")) == 321);
	CHECK(main.at(Ptr("/stash_export/export_gear")) == "VK_NUMPAD0");
}

TEST_CASE("SaveConfig then LoadConfig restores every changed setting") {
	JsonFiles files;
	App.bnet.autofillLastGame.value = false;
	App.bnet.saveLastGame.value = "cows-12";
	App.bnet.saveLastPass.value = "pw";
	App.bnet.saveLastDesc.value = "desc";
	App.bnet.failToJoin.value = 9000;
	App.bnet.refreshTime.value = 3000;
	App.bnet.whisperColor.values = { { "*chat", "3" } };
	App.bnet.showNightmareDiff.value = false;
	App.general.statsOnRight.value = true;
	App.lootfilter.filterLevel.uValue = 12;
	App.lootfilter.lastFilterLevel.uValue = 6;
	App.lootfilter.filterLevelPrevious.hotkey = 0x76;  // F7
	App.lootfilter.enableFilter.value = false;
	App.lootfilter.detailedNotifications.value = 0;
	App.lootfilter.allowUnknownItems.toggle = { 0x55, true };
	App.lootfilter.dropSounds.value = false;
	App.lootfilter.classSkillsList.values = { { "amazon", "1" } };
	App.legacy.verboseNotifications.toggle = { 0x56, true };
	App.game.alwaysShowItems.value = true;
	App.party.autoCorpseLoot.toggle = { 0x4C, false };
	App.screen.automapInfo.values = { "%GAMETIME%" };
	App.screen.additionalStats.values = { { "Faster Cast Rate", "105" } };
	App.screen.hideGamePassword.value = true;
	App.stash.includeEquipment.toggle = { 0x49, false };
	App.stash.mustacheDefault.value = "stash";
	App.stash.mustacheOptions.values = { "stash" };
	App.stash.mustacheFormat.values = { { "item", "{{name}}" } };
	App.bhui.isMinimized.value = false;
	App.bhui.minimizedY.value = 400;
	App.bhui.sizeY.value = 500;
	Config saver(App.jsonFile);
	saver.SaveConfig();

	std::string main = files.main, bak = files.bak;
	App = BHApp();
	App.jsonFile = main;
	App.jsonBackup = bak;
	Config loader(App.jsonFile);
	loader.LoadConfig();

	CHECK(App.bnet.autofillLastGame.value == false);
	CHECK(App.bnet.saveLastGame.value == "cows-12");
	CHECK(App.bnet.saveLastPass.value == "pw");
	CHECK(App.bnet.saveLastDesc.value == "desc");
	CHECK(App.bnet.failToJoin.value == 9000);
	CHECK(App.bnet.refreshTime.value == 3000);
	CHECK(App.bnet.whisperColor.values == std::map<std::string, std::string>{ { "*chat", "3" } });
	CHECK(App.bnet.showNightmareDiff.value == false);
	CHECK(App.general.statsOnRight.value == true);
	CHECK(App.lootfilter.filterLevel.uValue == 12u);
	CHECK(App.lootfilter.lastFilterLevel.uValue == 6u);
	CHECK(App.lootfilter.filterLevelPrevious.hotkey == 0x76u);
	CHECK(App.lootfilter.enableFilter.value == false);
	CHECK(App.lootfilter.detailedNotifications.value == 0);
	CHECK(App.lootfilter.allowUnknownItems.toggle.hotkey == 0x55u);
	CHECK(App.lootfilter.allowUnknownItems.toggle.isEnabled == true);
	CHECK(App.lootfilter.dropSounds.value == false);
	CHECK(App.lootfilter.classSkillsList.values == std::map<std::string, std::string>{ { "amazon", "1" } });
	CHECK(App.legacy.verboseNotifications.toggle.hotkey == 0x56u);
	CHECK(App.legacy.verboseNotifications.toggle.isEnabled == true);
	CHECK(App.game.alwaysShowItems.value == true);
	CHECK(App.party.autoCorpseLoot.toggle.hotkey == 0x4Cu);
	CHECK(App.party.autoCorpseLoot.toggle.isEnabled == false);
	CHECK(App.screen.automapInfo.values == std::vector<std::string>{ "%GAMETIME%" });
	CHECK(App.screen.additionalStats.values == std::map<std::string, std::string>{ { "Faster Cast Rate", "105" } });
	CHECK(App.screen.hideGamePassword.value == true);
	CHECK(App.stash.includeEquipment.toggle.hotkey == 0x49u);
	CHECK(App.stash.includeEquipment.toggle.isEnabled == false);
	CHECK(App.stash.mustacheDefault.value == "stash");
	CHECK(App.stash.mustacheOptions.values == std::vector<std::string>{ "stash" });
	CHECK(App.stash.mustacheFormat.values == std::map<std::string, std::string>{ { "item", "{{name}}" } });
	CHECK(App.bhui.isMinimized.value == false);
	CHECK(App.bhui.minimizedY.value == 400);
	CHECK(App.bhui.sizeY.value == 500);
}

}  // TEST_SUITE
