#pragma once
#include <string>
#include <Windows.h>
#include <thread>
#include <chrono>
#include "Modules/ModuleManager.h"
#include "Config.h"
#include "Drawing.h"
#include "Patch.h"

using namespace std;

struct cGuardModule
{
	union {
		HMODULE hModule;
		DWORD dwBaseAddress;
	};
	DWORD _1;
	char szPath[MAX_PATH];
};

namespace BH {
	extern string path;
	extern HINSTANCE instance;
	extern ModuleManager* moduleManager;
	extern Config* lootFilter;
	extern Drawing::UI* settingsUI;
	extern Drawing::StatsDisplay* statsDisplay;
	extern WNDPROC OldWNDPROC;
	extern bool cGuardLoaded;
	extern bool initialized;
	extern Patch* oogDraw;

	extern bool Startup(HINSTANCE instance, VOID* reserved);
	extern "C" __declspec(dllexport) void Initialize();
	extern bool Shutdown();
	extern bool ReloadConfig();
	
	void LoadLootFilter();
};

struct BHApp {

	Config* config;
	json jsonConfig;
	std::string jsonFile = "BH.json";
	std::string jsonBackup = "BH.json.bak";

	struct {
		SettingsBool isMinimized = { true, true };
		SettingsInt openedX = { 0, 100, 0, 1068 };
		SettingsInt openedY = { 0, 100, 0, 600 };
		SettingsInt minimizedX = { 0, 203, 0, 1068 };
		SettingsInt minimizedY = { 0, 585, 0, 600 };
		SettingsInt sizeX = { 0, SETTINGS_SIZE_X, 0, 1068 };
		SettingsInt sizeY = { 0, SETTINGS_SIZE_Y, 0, 600 };
	} bhui;

	struct {
		SettingsBool autofillLastGame = { true, true };
		SettingsBool autofillNextGame = { true, true };
		SettingsBool autofillLastPass = { true, true };
		SettingsBool autofillLastDesc = { true, true };
		SettingsBool showDifficulty = { true, true };
		SettingsBool showGameserver = { true, true };
		SettingsInt failToJoin = { 4000, 4000, 4000, 10000 };
		SettingsInt refreshTime = { 1500, 1500, 1500, 10000 };
		SettingsAssoc whisperColor = { {},
			{
				{"*chat", "9"},
				{"*hc", "7"}
			}
		};
		SettingsBool showNormalDiff = { true, true };
		SettingsBool showNightmareDiff = { true, true };
		SettingsBool showHellDiff = { true, true };
	} bnet;

	struct {
		//SettingsKey reloadConfig = { 0x60, 0x60 };  // VK_NUMPAD0
		//SettingsKey reloadConfigCtrl = { 0x52, 0x52 };  // VK_R
		SettingsBool statsOnRight = { false, false };
	} general;


	struct {
		SettingsInt filterLevel = { 1, 1, 0, 12 };
		SettingsInt lastFilterLevel = { 0, 0, 0, 12 };
		SettingsKey filterLevelIncrease = { 0, 0 };
		SettingsKey filterLevelDecrease = { 0, 0 };
		SettingsKey filterLevelPrevious = { 0, 0 };
		SettingsBool enableFilter = { true, true };
		SettingsBool showIlvl = { false, false };
		SettingsBool detailedNotifications = { true, true };
		SettingsToggle allowUnknownItems = { {}, {0, false} };
		SettingsBool alwaysShowStatRanges = { false, false };
		SettingsAssoc classSkillsList = {}; // TODO??
		SettingsAssoc tabSkillsList = {};  // TODO??
	} lootfilter;

	struct {
		// Legacy Loot filter (these "features" should just be removed entirely)
		SettingsToggle showEthereal = { {}, {0, false} };
		SettingsToggle showSockets = { {}, {0, false} };
		SettingsToggle showRuneNumbers = { {}, {0, false} };
		SettingsToggle altItemStyle = { {}, {0, false} };
		SettingsToggle colorMod = { {}, {0, false} };
		SettingsToggle shortenItemNames = { {}, {0, false} };
		SettingsToggle verboseNotifications = { {}, {0, false} };
		SettingsToggle dropNotifications = { {}, {0, true} };
		SettingsToggle closeNotifications = { {}, {0, true} };
	} legacy;

	struct {
		SettingsBool experienceMeter = { false, false };
		SettingsBool alwaysShowItems = { false, false };
	} game;

	struct {
		SettingsToggle autoParty = { {}, {0, true} };
		SettingsToggle autoCorpseLoot = { {}, {0, true} };
	} party;

	struct {
		SettingsArray automapInfo = { {},
		{
			"Name: %GAMENAME%",
			"Password: %GAMEPASS%",
			"%LEVEL%",
			"%GAMEDIFF%",
			"%GAMETIME%",
			"Ping: %PING%"
		}
		};
		SettingsAssoc additionalStats = {};
	} screen;

	struct {
		SettingsToggle includeEquipment = { {}, {0, true} };
		SettingsToggle exportOnMenu = { {}, {0, false} };
		SettingsKey exportGear = { 0, 0 };
		SettingsString mustacheDefault = { "", "json" };
		SettingsArray mustacheOptions = { {}, {"json", "stash"} };
		SettingsAssoc mustacheFormat = { {},
			{
				{"stats", "{{#defense}}\n\n    >{{defense}} defense{{/defense}}{{#stats}}\n\n    > {{value}}{{#range}} ({{min}}-{{max}}){{/range}} {{^skill}}{{name}}{{/skill}}{{skill}}{{/stats}}"},
				{"header-unique", "{{#quality=Unique}}**{{^name}}{{type}}{{/name}}{{name}}** (L{{iLevel}}){{#sockets}}[{{sockets}}]{{/sockets}}{{/quality}}"},
				{"header-magic", "{{#quality$Magic|Rare}}**{{^name}}{{type}}{{/name}}{{name}}** (L{{iLevel}}){{#sockets}}[{{sockets}}]{{/sockets}}{{/quality}}"},
				{"header-else", "{{#quality^Unique|Magic|Rare}}{{^isRuneword}}{{^name}}{{type}}{{/name}}{{name}}{{/isRuneword}}{{#isRuneword}}**{{runeword}}** {{type}}{{/isRuneword}} (L{{iLevel}}){{#sockets}}[{{sockets}}]{{/sockets}}{{/quality}}"},
				{"header", "{{#isEthereal}}Eth {{/isEthereal}}{{>header-unique}}{{>header-magic}}{{>header-else}}{{#count}} **x{{count}}**{{/count}}"},
				{"item", "{{>header}}{{>stats}}{{^isRuneword}}{{#socketed}}\n\n  * {{>>item}}{{/socketed}}{{/isRuneword}}\n"},
				{"stash", "{{#this}}* {{>item}}\n\n{{/this}}"}
			}
		};
	} stash;

	std::vector<Toggle*> hotkeyToggles;
};

extern BHApp App;

typedef enum BHConfigId {
	BH_CONFIG_EXPERIENCEMETER,
	BH_CONFIG_ADVANCEDSTATS,
	BH_CONFIG_RELOAD,
	BH_CONFIG_LOOTFILTER,
	BH_CONFIG_FILTERLEVEL,
	BH_CONFIG_NUMFILTERLEVELS,
	BH_CONFIG_LOOTNOTIFY,
	BH_CONFIG_SHOWITEMLEVEL,
	BH_CONFIG_SHOWSTATRANGE,
	BH_CONFIG_ALWAYSSHOWITEMS,
	BH_CONFIG_INCREASEFILTER,
	BH_CONFIG_DECREASEFILTER,
	BH_CONFIG_RESTOREFILTER,

} BHConfigId;
