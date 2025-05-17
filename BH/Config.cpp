#include "Config.h"
#include "BH.h"
#include <algorithm>
#include <sstream>


using json = nlohmann::ordered_json;


int Config::GetInt(json::json_pointer sectionKey, std::string key, SettingsInt def)
{
	int val = def.defValue;
	int min = def.minValue ? def.minValue : INT_MIN;
	int max = def.maxValue ? def.maxValue : INT_MAX;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		try
		{
			val = App.jsonConfig[sectionKey][key].template get<int>();
		}
		catch (const std::exception& e)
		{
			// Handle conversion error here
			//error_log("Error converting int config key '%s' in section '%s', check your values! Exception: %s", key.c_str(), sectionKey.c_str(), e.what());
		}
	}

	if (min != INT_MIN && val < min)
	{
		val = min;
	}

	if (max != INT_MAX && val > max)
	{
		val = max;
	}

	return val;
}

float Config::GetFloat(json::json_pointer sectionKey, std::string key, SettingsFloat def)
{
	float val = def.defValue;
	float min = def.minValue;
	float max = def.maxValue;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		try
		{
			val = App.jsonConfig[sectionKey][key].template get<float>();
		}
		catch (const std::exception& e)
		{
			// Handle conversion error here
			//error_log("Error converting float config key '%s' in section '%s', check your values! Exception: %s", key.c_str(), sectionKey.c_str(), e.what());
		}
	}

	if (val < min)
	{
		val = min;
	}

	if (val > max)
	{
		val = max;
	}

	return val;
}

bool Config::GetBool(json::json_pointer sectionKey, std::string key, SettingsBool def)
{
	bool val = def.defValue;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		try
		{
			val = App.jsonConfig[sectionKey][key].template get<bool>();
		}
		catch (const std::exception& e)
		{
			// Handle conversion error here
			//error_log("Error converting bool config key '%s' in section '%s', check your values! Exception: %s", key.c_str(), sectionKey.c_str(), e.what());
		}
	}
	return val;
}

std::string Config::GetString(json::json_pointer sectionKey, std::string key, SettingsString def)
{
	std::string val = def.defValue;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		try
		{
			val = App.jsonConfig[sectionKey][key].template get<std::string>();

		}
		catch (const std::exception& e)
		{
			// Handle conversion error here
			//error_log("Error converting string config key '%s' in section '%s', check your values! Exception: %s", key.c_str(), sectionKey.c_str(), e.what());
		}
	}
	return val;
}

Toggle Config::GetToggle(json::json_pointer sectionKey, std::string key, SettingsToggle def)
{
	Toggle ret;
	ret.isEnabled = def.defToggle.isEnabled;
	ret.hotkey = def.defToggle.hotkey;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		if (App.jsonConfig[sectionKey][key].contains("enabled"))
		{
			ret.isEnabled = App.jsonConfig[sectionKey][key]["enabled"].template get<bool>();
		}

		if (App.jsonConfig[sectionKey][key].contains("hotkey"))
		{
			ret.hotkey = GetKeyCode(App.jsonConfig[sectionKey][key]["hotkey"].template get<std::string>().c_str()).value;
		}
	}
	return ret;
}

unsigned int Config::GetKey(json::json_pointer sectionKey, std::string key, SettingsKey def)
{
	unsigned int val = def.defHotkey;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		val = GetKeyCode(App.jsonConfig[sectionKey][key].template get<std::string>().c_str()).value;
	}

	return val;
}

std::vector<std::string> Config::GetArray(json::json_pointer sectionKey, std::string key, SettingsArray def)
{
	std::vector<std::string> val = def.defValues;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		val = App.jsonConfig[sectionKey][key].template get<std::vector<std::string>>();
	}
	return val;
}

std::map<std::string, std::string> Config::GetAssoc(json::json_pointer sectionKey, std::string key, SettingsAssoc def)
{
	std::map<std::string, std::string> val = def.defValues;
	if (App.jsonConfig.contains(sectionKey) && App.jsonConfig[sectionKey].contains(key))
	{
		val = App.jsonConfig[sectionKey][key].template get<std::map<std::string, std::string>>();
	}
	return val;
}

static HMODULE pd2Handle;
static FARPROC refreshProc;
typedef BOOL(__cdecl* bhConfigMessage_t)();
static bhConfigMessage_t bhConfigMessage = NULL;

bool bCreateFile = false;

void Config::SaveConfig()
{
	// Bnet settings
	json jsonBnet;
	jsonBnet["autofill_last_game"] = App.bnet.autofillLastGame.value;
	jsonBnet["autofill_next_game"] = App.bnet.autofillNextGame.value;
	jsonBnet["autofill_last_password"] = App.bnet.autofillLastPass.value;
	jsonBnet["autofill_description"] = App.bnet.autofillLastDesc.value;
	jsonBnet["save_last_game"] = App.bnet.saveLastGame.value;
	jsonBnet["save_last_pass"] = App.bnet.saveLastPass.value;
	jsonBnet["save_last_desc"] = App.bnet.saveLastDesc.value;
	jsonBnet["show_difficulty"] = App.bnet.showDifficulty.value;
	jsonBnet["show_gameserver"] = App.bnet.showGameserver.value;
	jsonBnet["fail_to_join"] = App.bnet.failToJoin.value;
	jsonBnet["game_list_refresh"] = App.bnet.refreshTime.value;
	jsonBnet["whisper_color"] = App.bnet.whisperColor.values;
	jsonBnet["show_normal_difficulty"] = App.bnet.showNormalDiff.value;
	jsonBnet["show_nightmare_difficulty"] = App.bnet.showNightmareDiff.value;
	jsonBnet["show_hell_difficulty"] = App.bnet.showHellDiff.value;

	App.jsonConfig["bnet"] = jsonBnet;

	// General settings
	json jsonGeneral;
	jsonGeneral["stats_on_right"] = App.general.statsOnRight.value;
	App.jsonConfig["general"] = jsonGeneral;

	// Legacy Loot filter (these "features" should just be removed entirely)
	json jsonLegacyLoot;
	jsonLegacyLoot["show_ethereal"]["enabled"] = App.legacy.showEthereal.toggle.isEnabled;
	jsonLegacyLoot["show_ethereal"]["hotkey"] = GetKeyCode(App.legacy.showEthereal.toggle.hotkey).name;
	jsonLegacyLoot["show_sockets"]["enabled"] = App.legacy.showSockets.toggle.isEnabled;
	jsonLegacyLoot["show_sockets"]["hotkey"] = GetKeyCode(App.legacy.showSockets.toggle.hotkey).name;
	jsonLegacyLoot["show_rune_numbers"]["enabled"] = App.legacy.showRuneNumbers.toggle.isEnabled;
	jsonLegacyLoot["show_rune_numbers"]["hotkey"] = GetKeyCode(App.legacy.showRuneNumbers.toggle.hotkey).name;
	jsonLegacyLoot["alt_item_style"]["enabled"] = App.legacy.altItemStyle.toggle.isEnabled;
	jsonLegacyLoot["alt_item_style"]["hotkey"] = GetKeyCode(App.legacy.altItemStyle.toggle.hotkey).name;
	jsonLegacyLoot["color_mod"]["enabled"] = App.legacy.colorMod.toggle.isEnabled;
	jsonLegacyLoot["color_mod"]["hotkey"] = GetKeyCode(App.legacy.colorMod.toggle.hotkey).name;
	jsonLegacyLoot["shorten_item_names"]["enabled"] = App.legacy.shortenItemNames.toggle.isEnabled;
	jsonLegacyLoot["shorten_item_names"]["hotkey"] = GetKeyCode(App.legacy.shortenItemNames.toggle.hotkey).name;
	jsonLegacyLoot["drop_notifications"]["enabled"] = App.legacy.dropNotifications.toggle.isEnabled;
	jsonLegacyLoot["drop_notifications"]["hotkey"] = GetKeyCode(App.legacy.dropNotifications.toggle.hotkey).name;
	jsonLegacyLoot["close_notifications"]["enabled"] = App.legacy.closeNotifications.toggle.isEnabled;
	jsonLegacyLoot["close_notifications"]["hotkey"] = GetKeyCode(App.legacy.closeNotifications.toggle.hotkey).name;
	jsonLegacyLoot["verbose_notifications"]["enabled"] = App.legacy.verboseNotifications.toggle.isEnabled;
	jsonLegacyLoot["verbose_notifications"]["hotkey"] = GetKeyCode(App.legacy.verboseNotifications.toggle.hotkey).name;

	// Loot filter
	json jsonLoot;
	jsonLoot["filter_level"] = App.lootfilter.filterLevel.uValue;
	jsonLoot["last_filter_level"] = App.lootfilter.lastFilterLevel.uValue;
	jsonLoot["filter_level_increase"] = GetKeyCode(App.lootfilter.filterLevelIncrease.hotkey).name;
	jsonLoot["filter_level_decrease"] = GetKeyCode(App.lootfilter.filterLevelDecrease.hotkey).name;
	jsonLoot["filter_level_previous"] = GetKeyCode(App.lootfilter.filterLevelPrevious.hotkey).name;
	jsonLoot["advanced_item_display"] = App.lootfilter.enableFilter.value;
	jsonLoot["show_iLvl"] = App.lootfilter.showIlvl.value;
	jsonLoot["detailed_notifications"] = App.lootfilter.detailedNotifications.value;
	jsonLoot["allow_unknown_items"]["enabled"] = App.lootfilter.allowUnknownItems.toggle.isEnabled;
	jsonLoot["allow_unknown_items"]["hotkey"] = GetKeyCode(App.lootfilter.allowUnknownItems.toggle.hotkey).name;
	jsonLoot["always_show_stat_ranges"] = App.lootfilter.alwaysShowStatRanges.value;
	jsonLoot["drop_sounds"] = App.lootfilter.dropSounds.value;
	if (App.lootfilter.classSkillsList.values.size() > 0) { jsonLoot["class_skills_list"] = App.lootfilter.classSkillsList.values; }
	if (App.lootfilter.tabSkillsList.values.size() > 0) { jsonLoot["tab_skills_list"] = App.lootfilter.tabSkillsList.values; }
	jsonLoot["legacy_settings"] = jsonLegacyLoot;
	App.jsonConfig["lootfilter"] = jsonLoot;

	// Game settings
	json jsonGame;
	jsonGame["experience_meter"] = App.game.experienceMeter.value;
	jsonGame["always_show_items"] = App.game.alwaysShowItems.value;
	App.jsonConfig["game"] = jsonGame;

	// Party settings
	json jsonParty;
	jsonParty["auto_party"]["enabled"] = App.party.autoParty.toggle.isEnabled;
	jsonParty["auto_party"]["hotkey"] = GetKeyCode(App.party.autoParty.toggle.hotkey).name;
	jsonParty["auto_corpse_loot"]["enabled"] = App.party.autoCorpseLoot.toggle.isEnabled;
	jsonParty["auto_corpse_loot"]["hotkey"] = GetKeyCode(App.party.autoCorpseLoot.toggle.hotkey).name;
	App.jsonConfig["party"] = jsonParty;

	// Screen Info
	json jsonScreen;
	jsonScreen["automap_info"] = App.screen.automapInfo.values;
	if (App.screen.additionalStats.values.size() > 0) { jsonScreen["additional_stats"] = App.screen.additionalStats.values; }
	jsonScreen["hide_game_password"] = App.screen.hideGamePassword.value;
	App.jsonConfig["screen_info"] = jsonScreen;

	// Stash Export
	json jsonStash;
	jsonStash["include_equipment"]["enabled"] = App.stash.includeEquipment.toggle.isEnabled;
	jsonStash["include_equipment"]["hotkey"] = GetKeyCode(App.stash.includeEquipment.toggle.hotkey).name;
	jsonStash["export_on_menu"]["enabled"] = App.stash.exportOnMenu.toggle.isEnabled;
	jsonStash["export_on_menu"]["hotkey"] = GetKeyCode(App.stash.exportOnMenu.toggle.hotkey).name;
	jsonStash["export_gear"] = GetKeyCode(App.stash.exportGear.hotkey).name;
	jsonStash["mustache_default"] = App.stash.mustacheDefault.value;
	jsonStash["mustache_options"] = App.stash.mustacheOptions.values;
	jsonStash["mustache_format"] = App.stash.mustacheFormat.values;
	App.jsonConfig["stash_export"] = jsonStash;

	// BH UI window
	json jsonBHUI;
	jsonBHUI["is_minimized"] = App.bhui.isMinimized.value;
	jsonBHUI["opened_x"] = App.bhui.openedX.value;
	jsonBHUI["opened_y"] = App.bhui.openedY.value;
	jsonBHUI["minimized_x"] = App.bhui.minimizedX.value;
	jsonBHUI["minimized_y"] = App.bhui.minimizedY.value;
	jsonBHUI["size_x"] = App.bhui.sizeX.value;
	jsonBHUI["size_y"] = App.bhui.sizeY.value;
	App.jsonConfig["bh_ui"] = jsonBHUI;

	std::ofstream fout(App.jsonFile);
	fout << std::setw(4) << App.jsonConfig << std::endl;
	fout.close();

	if (bCreateFile)
	{
		std::ofstream foutBak(App.jsonBackup);
		foutBak << std::setw(4) << App.jsonConfig << std::endl;
		foutBak.close();
	}
}


void Config::LoadConfig()
{
	std::ifstream ifile(App.jsonFile);
	if (!ifile)
	{
		std::ofstream output(App.jsonFile);
		output << "{}";
		output.close();
		bCreateFile = true;
	}
	std::ifstream ibakfile(App.jsonBackup);
	if (!ibakfile)
	{
		std::ofstream outputBak(App.jsonBackup);
		outputBak << "{}";
		outputBak.close();
	}

	try
	{
		App.jsonConfig = json::parse(std::ifstream{ App.jsonFile });

		if (!bCreateFile)
		{
			std::ofstream foutBak(App.jsonBackup);
			foutBak << std::setw(4) << App.jsonConfig << std::endl;
			foutBak.close();
		}
	}
	catch (const json::parse_error&)
	{
		try
		{
			App.jsonConfig = json::parse(std::ifstream{ App.jsonBackup });

			std::ofstream fout(App.jsonFile);
			fout << std::setw(4) << App.jsonConfig << std::endl;
			fout.close();
		}
		catch (const json::parse_error&)
		{
			std::ofstream output(App.jsonFile);
			output << "{}";
			output.close();
			std::ofstream outputBak(App.jsonBackup);
			outputBak << "{}";
			outputBak.close();
			bCreateFile = true;

			App.jsonConfig = json::parse(std::ifstream{ App.jsonFile });
		}
	}

	// Bnet settings
	App.bnet.autofillLastGame.value = GetBool("/bnet"_json_pointer, "autofill_last_game", App.bnet.autofillLastGame);
	App.bnet.autofillNextGame.value = GetBool("/bnet"_json_pointer, "autofill_next_game", App.bnet.autofillNextGame);
	App.bnet.autofillLastPass.value = GetBool("/bnet"_json_pointer, "autofill_last_password", App.bnet.autofillLastPass);
	App.bnet.autofillLastDesc.value = GetBool("/bnet"_json_pointer, "autofill_description", App.bnet.autofillLastDesc);
	App.bnet.saveLastGame.value = GetString("/bnet"_json_pointer, "save_last_game", App.bnet.saveLastGame);
	App.bnet.saveLastPass.value = GetString("/bnet"_json_pointer, "save_last_pass", App.bnet.saveLastPass);
	App.bnet.saveLastDesc.value = GetString("/bnet"_json_pointer, "save_last_desc", App.bnet.saveLastDesc);
	App.bnet.showDifficulty.value = GetBool("/bnet"_json_pointer, "show_difficulty", App.bnet.showDifficulty);
	App.bnet.showGameserver.value = GetBool("/bnet"_json_pointer, "show_gameserver", App.bnet.showGameserver);
	App.bnet.failToJoin.value = GetInt("/bnet"_json_pointer, "fail_to_join", App.bnet.failToJoin);
	App.bnet.refreshTime.value = GetInt("/bnet"_json_pointer, "game_list_refresh", App.bnet.refreshTime);
	App.bnet.whisperColor.values = GetAssoc("/bnet"_json_pointer, "whisper_color", App.bnet.whisperColor);
	App.bnet.showNormalDiff.value = GetBool("/bnet"_json_pointer, "show_normal_difficulty", App.bnet.showNormalDiff);
	App.bnet.showNightmareDiff.value = GetBool("/bnet"_json_pointer, "show_nightmare_difficulty", App.bnet.showNightmareDiff);
	App.bnet.showHellDiff.value = GetBool("/bnet"_json_pointer, "show_hell_difficulty", App.bnet.showHellDiff);

	// General settings
	App.general.statsOnRight.value = GetBool("/general"_json_pointer, "stats_on_right", App.general.statsOnRight);

	// Loot filter
	App.lootfilter.filterLevel.uValue = (unsigned int)GetInt("/lootfilter"_json_pointer, "filter_level", App.lootfilter.filterLevel);
	App.lootfilter.lastFilterLevel.uValue = (unsigned int)GetInt("/lootfilter"_json_pointer, "last_filter_level", App.lootfilter.lastFilterLevel);
	App.lootfilter.filterLevelIncrease.hotkey = GetKey("/lootfilter"_json_pointer, "filter_level_increase", App.lootfilter.filterLevelIncrease);
	App.lootfilter.filterLevelDecrease.hotkey = GetKey("/lootfilter"_json_pointer, "filter_level_decrease", App.lootfilter.filterLevelDecrease);
	App.lootfilter.filterLevelPrevious.hotkey = GetKey("/lootfilter"_json_pointer, "filter_level_previous", App.lootfilter.filterLevelPrevious);
	App.lootfilter.enableFilter.value = GetBool("/lootfilter"_json_pointer, "advanced_item_display", App.lootfilter.enableFilter);
	App.lootfilter.showIlvl.value = GetBool("/lootfilter"_json_pointer, "show_iLvl", App.lootfilter.showIlvl);
	App.lootfilter.detailedNotifications.value = GetInt("/lootfilter"_json_pointer, "detailed_notifications", App.lootfilter.detailedNotifications);
	App.lootfilter.allowUnknownItems.toggle = GetToggle("/lootfilter"_json_pointer, "allow_unknown_items", App.lootfilter.allowUnknownItems);
	App.lootfilter.alwaysShowStatRanges.value = GetBool("/lootfilter"_json_pointer, "always_show_stat_ranges", App.lootfilter.alwaysShowStatRanges);
	App.lootfilter.dropSounds.value = GetBool("/lootfilter"_json_pointer, "drop_sounds", App.lootfilter.dropSounds);
	App.lootfilter.classSkillsList.values = GetAssoc("/lootfilter"_json_pointer, "class_skills_list", App.lootfilter.classSkillsList);
	App.lootfilter.tabSkillsList.values = GetAssoc("/lootfilter"_json_pointer, "tab_skills_list", App.lootfilter.tabSkillsList);

	// Legacy Loot filter (these "features" should just be removed entirely)
	App.legacy.showEthereal.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "show_ethereal", App.legacy.showEthereal);
	App.legacy.showSockets.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "show_sockets", App.legacy.showSockets);
	App.legacy.showRuneNumbers.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "show_rune_numbers", App.legacy.showRuneNumbers);
	App.legacy.altItemStyle.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "alt_item_style", App.legacy.altItemStyle);
	App.legacy.colorMod.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "color_mod", App.legacy.colorMod);
	App.legacy.shortenItemNames.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "shorten_item_names", App.legacy.shortenItemNames);
	App.legacy.dropNotifications.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "drop_notifications", App.legacy.dropNotifications);
	App.legacy.closeNotifications.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "close_notifications", App.legacy.closeNotifications);
	App.legacy.verboseNotifications.toggle = GetToggle("/lootfilter/legacy_settings"_json_pointer, "verbose_notifications", App.legacy.verboseNotifications);

	// Game settings
	App.game.experienceMeter.value = GetBool("/game"_json_pointer, "experience_meter", App.game.experienceMeter);
	App.game.alwaysShowItems.value = GetBool("/game"_json_pointer, "always_show_items", App.game.alwaysShowItems);

	// Party settings
	App.party.autoParty.toggle = GetToggle("/party"_json_pointer, "auto_party", App.party.autoParty);
	App.party.autoCorpseLoot.toggle = GetToggle("/party"_json_pointer, "auto_corpse_loot", App.party.autoCorpseLoot);

	// Screen Info
	App.screen.automapInfo.values = GetArray("/screen_info"_json_pointer, "automap_info", App.screen.automapInfo);
	App.screen.additionalStats.values = GetAssoc("/screen_info"_json_pointer, "additional_stats", App.screen.additionalStats);
	App.screen.hideGamePassword.value = GetBool("/screen_info"_json_pointer, "hide_game_password", App.screen.hideGamePassword);

	// Stash Export
	App.stash.includeEquipment.toggle = GetToggle("/stash_export"_json_pointer, "include_equipment", App.stash.includeEquipment);
	App.stash.exportOnMenu.toggle = GetToggle("/stash_export"_json_pointer, "export_on_menu", App.stash.exportOnMenu);
	App.stash.exportGear.hotkey = GetKey("/stash_export"_json_pointer, "export_gear", App.stash.exportGear);
	App.stash.mustacheDefault.value = GetString("/stash_export"_json_pointer, "mustache_default", App.stash.mustacheDefault);
	App.stash.mustacheOptions.values = GetArray("/stash_export"_json_pointer, "mustache_options", App.stash.mustacheOptions);
	App.stash.mustacheFormat.values = GetAssoc("/stash_export"_json_pointer, "mustache_format", App.stash.mustacheFormat);

	App.bhui.isMinimized.value = GetBool("/bh_ui"_json_pointer, "is_minimized", App.bhui.isMinimized);
	App.bhui.openedX.value = GetInt("/bh_ui"_json_pointer, "opened_x", App.bhui.openedX);
	App.bhui.openedY.value = GetInt("/bh_ui"_json_pointer, "opened_y", App.bhui.openedY);
	App.bhui.minimizedX.value = GetInt("/bh_ui"_json_pointer, "minimized_x", App.bhui.minimizedX);
	App.bhui.minimizedY.value = GetInt("/bh_ui"_json_pointer, "minimized_y", App.bhui.minimizedY);
	App.bhui.sizeX.value = GetInt("/bh_ui"_json_pointer, "size_x", App.bhui.sizeX);
	App.bhui.sizeY.value = GetInt("/bh_ui"_json_pointer, "size_y", App.bhui.sizeY);


	if (bCreateFile)
	{
		SaveConfig();
	}

	App.hotkeyToggles = {
		// General

		// Lootfilter
		&App.lootfilter.allowUnknownItems.toggle,
		// Legacy Lootfilter
		&App.legacy.showEthereal.toggle,
		&App.legacy.showSockets.toggle,
		&App.legacy.showRuneNumbers.toggle,
		&App.legacy.altItemStyle.toggle,
		&App.legacy.colorMod.toggle,
		&App.legacy.shortenItemNames.toggle,
		&App.legacy.verboseNotifications.toggle,
		&App.legacy.dropNotifications.toggle,
		&App.legacy.closeNotifications.toggle,
		// Game

		// Party
		&App.party.autoParty.toggle,
		&App.party.autoCorpseLoot.toggle,
		// Stash
		&App.stash.includeEquipment.toggle,
		&App.stash.exportOnMenu.toggle
	};
}


/* Parse()
Parse the configuration file and stores the results in a key->value pair.
Can be called multiple times so you can reload the configuration.
*/
bool Config::Parse()
{

	//If this is a config with a pre-set of values, can't parse!
	if (configName.length() == 0)
		return false;

	//Open the configuration file
	fstream file(BH::path + configName);
	if (!file.is_open())
		return false;

	//Lock Critical Section then wipe contents incase we are reloading.
	contents.erase(contents.begin(), contents.end());
	orderedKeyVals.clear();

	//Begin to loop the configuration file one line at a time.
	std::string line;
	int lineNo = 0;
	while (std::getline(file, line))
	{
		lineNo++;
		std::string comment;
		//Remove any comments from the config
		if (line.find("//") != string::npos)
		{
			comment = line.substr(line.find("//"));
			line = line.erase(line.find("//"));
		}

		//Insure we have something in the line now.
		if (line.length() == 0)
			continue;

		//Grab the Key and Value

		ConfigEntry entry;
		entry.line = lineNo;
		entry.key = Trim(line.substr(0, line.find_first_of(":")));
		entry.value = Trim(line.substr(line.find_first_of(":") + 1));

		entry.comment = line.substr(line.find_first_of(":") + 1, line.find(entry.value) - line.find_first_of(":") - 1);
		entry.pointer = NULL;

		//Store them!
		contents.insert(pair<string, ConfigEntry>(entry.key, entry));
		orderedKeyVals.push_back(pair<string, string>(entry.key, entry.value));
	}
	file.close();
	return true;
}

std::string Config::GetConfigName()
{
	return BH::path + configName;
}

void Config::SetConfigName(std::string name)
{
	configName = name;
}

vector<pair<string, string>> Config::ReadMapList(std::string key, vector<pair<string, string>>& values)
{

	for (vector<pair<string, string>>::iterator it = orderedKeyVals.begin(); it != orderedKeyVals.end(); it++)
	{
		if (!(*it).first.find(key + "["))
		{
			pair<string, string> assoc;
			//Pull the value from between the []'s
			assoc.first = (*it).first.substr((*it).first.find("[") + 1, (*it).first.length() - (*it).first.find("[") - 2);
			//Also store the value
			assoc.second = (*it).second;
			values.push_back(assoc);
		}
	}

	return values;
}
