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
	union
	{
		HMODULE hModule;
		DWORD dwBaseAddress;
	};
	DWORD _1;
	char szPath[MAX_PATH];
};

namespace BH
{
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
	void CheckForD2GL();
	void CheckForPD2();
};

typedef BOOL(__stdcall* pd2PlaySoundImpl_t)(UnitAny* pUnit, int nSound, int nVolume, int nPriority, BOOL bDropSound);

struct BHApp
{

	Config* config;
	json jsonConfig;
	std::string jsonFile = "BH.json";
	std::string jsonBackup = "BH.json.bak";

	struct
	{
		SettingsBool isMinimized = { true, true };
		SettingsInt openedX = { 0, 100, 0, 1068 };
		SettingsInt openedY = { 0, 100, 0, 600 };
		SettingsInt minimizedX = { 0, 203, 0, 1068 };
		SettingsInt minimizedY = { 0, 585, 0, 600 };
		SettingsInt sizeX = { 0, SETTINGS_SIZE_X, 0, 1068 };
		SettingsInt sizeY = { 0, SETTINGS_SIZE_Y, 0, 600 };
	} bhui;

	struct
	{
		SettingsBool autofillLastGame = { true, true };
		SettingsBool autofillNextGame = { true, true };
		SettingsBool autofillLastPass = { true, true };
		SettingsBool autofillLastDesc = { true, true };
		SettingsString saveLastGame = { "", "" };
		SettingsString saveLastPass = { "", "" };
		SettingsString saveLastDesc = { "", "" };
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

	struct
	{
		//SettingsKey reloadConfig = { 0x60, 0x60 };  // VK_NUMPAD0
		//SettingsKey reloadConfigCtrl = { 0x52, 0x52 };  // VK_R
		SettingsBool statsOnRight = { false, false };
	} general;


	struct
	{
		SettingsInt filterLevel = { 1, 1, 0, 12 };
		SettingsInt lastFilterLevel = { 0, 0, 0, 12 };
		SettingsKey filterLevelIncrease = { 0, 0 };
		SettingsKey filterLevelDecrease = { 0, 0 };
		SettingsKey filterLevelPrevious = { 0, 0 };
		SettingsBool enableFilter = { true, true };
		SettingsBool showIlvl = { false, false };
		SettingsInt detailedNotifications = { 1, 1, 0, 2 };	// 0: Off, 1: On, 2: Only "new"
		SettingsToggle allowUnknownItems = { {}, {0, false} };
		SettingsInt showStatRangesPrimary = { 0, 0, 0x08, 0xE0 };
		SettingsInt showStatRangesSecondary = { 0, 0, 0x08, 0xE0 };
		SettingsBool alwaysShowStatRanges = { false, false };
		SettingsAssoc classSkillsList = {}; // TODO??
		SettingsAssoc tabSkillsList = {};  // TODO??
		SettingsBool dropSounds = { true, true };
	} lootfilter;

	struct
	{
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

	struct
	{
		SettingsBool experienceMeter = { false, false };
		SettingsBool alwaysShowItems = { false, false };
	} game;

	struct
	{
		SettingsToggle autoParty = { {}, {0, true} };
		SettingsToggle autoCorpseLoot = { {}, {0, true} };
	} party;

	struct
	{
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
		SettingsBool hideGamePassword = { false, false };
	} screen;

	struct
	{
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

	struct
	{
		SettingsBool usingD2GL = { false, false };
		SettingsBool usingHDText = { false, false };
	} d2gl;

	struct
	{
		pd2PlaySoundImpl_t pd2PlaySoundImpl = NULL;
	} pd2;

	std::vector<Toggle*> hotkeyToggles;
};

extern BHApp App;

typedef enum BHConfigId
{
	BH_CONFIG_EXPERIENCEMETER,
	BH_CONFIG_ADVANCEDSTATS,
	BH_CONFIG_RELOAD,
	BH_CONFIG_LOOTFILTER,
	BH_CONFIG_FILTERLEVEL,
	BH_CONFIG_NUMFILTERLEVELS,
	BH_CONFIG_LOOTNOTIFY,
	BH_CONFIG_SHOWITEMLEVEL,
	BH_CONFIG_ALWAYSSHOWSTATRANGE,
	BH_CONFIG_ALWAYSSHOWITEMS,
	BH_CONFIG_INCREASEFILTER,
	BH_CONFIG_DECREASEFILTER,
	BH_CONFIG_RESTOREFILTER,
	BH_CONFIG_SHOWSTATRANGEPRIMARY,
	BH_CONFIG_SHOWSTATRANGESECONDARY,
	BH_CONFIG_USINGHDTEXT,
	BH_CONFIG_DROPSOUNDS,
	BH_CONFIG_ADVANCEDSTATS_OPEN,
	BH_CONFIG_HIDE_GAME_PASSWORD,
} BHConfigId;

typedef enum D2GLConfigId
{
	D2GL_CONFIG_VSYNC,
	D2GL_CONFIG_CURSOR_UNLOCKED,
	D2GL_CONFIG_HD_CURSOR,
	D2GL_CONFIG_HD_TEXT,
	D2GL_CONFIG_MOTION_PREDICTION,
	D2GL_CONFIG_MINI_MAP,
	D2GL_CONFIG_SHOW_ITEM_QUANTITY,
	D2GL_CONFIG_SHOW_MONSTER_RES,
	D2GL_CONFIG_SHOW_FPS,
} D2GLConfigId;