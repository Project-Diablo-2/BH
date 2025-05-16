#define _DEFINE_PTRS
#include "BH.h"
#include <Shlwapi.h>
#include <psapi.h>
#include <io.h>
#include "D2Ptrs.h"
#include "D2Intercepts.h"
#include "D2Handlers.h"
#include "Modules.h"
#include "Task.h"
#include "Drawing/Stats/StatsDisplay.h"

string BH::path;
HINSTANCE BH::instance;
ModuleManager* BH::moduleManager;
Config* BH::lootFilter;
Item* itemObj;
Drawing::UI* BH::settingsUI;
Drawing::StatsDisplay* BH::statsDisplay;
bool BH::initialized;
bool BH::cGuardLoaded;
WNDPROC BH::OldWNDPROC;
BHApp App;

Patch* patches[] = {
	new Patch(Call, D2CLIENT, { 0x44230, 0x45280 }, (int)GameLoop_Interception, 7),

	new Patch(Jump, D2CLIENT, { 0xC3DB4, 0x1D7B4 }, (int)GameDraw_Interception, 6),
	new Patch(Jump, D2CLIENT, { 0x626C9, 0x73469 }, (int)GameAutomapDraw_Interception, 5),

	new Patch(Call, BNCLIENT, { 0xEAB6, 0xCEB6 }, (int)ChatPacketRecv_Interception, 0xEABE - 0xEAB6),
	new Patch(Call, D2MCPCLIENT, { 0x69D7, 0x6297 }, (int)RealmPacketRecv_Interception, 5),
	new Patch(Call, D2CLIENT, { 0xACE61, 0x83301 }, (int)GamePacketRecv_Interception, 5),
	new Patch(Call, D2CLIENT, { 0x70B75, 0xB24FF }, (int)GameInput_Interception, 5),
	new Patch(Call, D2MULTI, { 0xD753, 0x11D63 }, (int)ChannelInput_Interception, 5),
	new Patch(Call, D2MULTI, { 0x10781, 0x14A9A }, (int)ChannelWhisper_Interception, 5),
	new Patch(Jump, D2MULTI, { 0x108A0, 0x14BE0 }, (int)ChannelChat_Interception, 6),
	new Patch(Jump, D2MULTI, { 0x107A0, 0x14850 }, (int)ChannelEmote_Interception, 6),
};

Patch* BH::oogDraw = new Patch(Call, D2WIN, { 0x18911, 0xEC61 }, (int)OOGDraw_Interception, 5);

unsigned int index = 0;
bool BH::Startup(HINSTANCE instance, VOID* reserved)
{

	BH::instance = instance;
	if (reserved != NULL)
	{
		cGuardModule* pModule = (cGuardModule*)reserved;
		if (!pModule)
			return FALSE;
		path.assign(pModule->szPath);
		cGuardLoaded = true;
	}
	else
	{
		char szPath[MAX_PATH];
		GetModuleFileName(BH::instance, szPath, MAX_PATH);
		PathRemoveFileSpec(szPath);
		path.assign(szPath);
		path += "\\";
		cGuardLoaded = false;
	}


	initialized = false;
	Initialize();
	return true;
}

void BH::Initialize()
{
	moduleManager = new ModuleManager();
	App.config = new Config(App.jsonFile);
	App.config->LoadConfig();

	lootFilter = new Config("loot.filter");
	LoadLootFilter();
	CheckForD2GL();

	// Do this asynchronously because D2GFX_GetHwnd() will be null if
	// we inject on process start
	Task::Enqueue([]() -> void
		{
			std::chrono::milliseconds duration(200);
			while (!D2GFX_GetHwnd())
			{
				std::this_thread::sleep_for(duration);
			}
			BH::OldWNDPROC = (WNDPROC)GetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC);
			SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)GameWindowEvent);
		});

	//settingsUI = new Drawing::UI(SETTINGS_TEXT, App.bhui.sizeX.value, App.bhui.sizeY.value);

	Task::InitializeThreadPool(2);

	new GameSettings();
	new ScreenInfo();
	new Gamefilter();
	new Bnet();
	itemObj = new Item();
	new Party();
	new StashExport();
	new MapNotify();
	new ChatColor();

	moduleManager->LoadModules();

	statsDisplay = new Drawing::StatsDisplay("Stats");

	// Injection would occasionally deadlock (I only ever saw it when using Tabbed Diablo
	// but theoretically it could happen during regular injection):
	// installation until after all startup initialization is done.
	for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++)
	{
		patches[n]->Install();
	}

	if (!D2CLIENT_GetPlayerUnit())
		oogDraw->Install();

	// GameThread can potentially run oogDraw->Install, so create the thread after all
	// loading/installation finishes.
	CreateThread(0, 0, GameThread, 0, 0, 0);

	initialized = true;
}

bool BH::Shutdown()
{
	if (initialized)
	{
		moduleManager->UnloadModules();

		delete moduleManager;
		delete settingsUI;
		delete statsDisplay;

		SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)BH::OldWNDPROC);
		for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++)
		{
			delete patches[n];
		}

		oogDraw->Remove();
	}

	return true;
}

bool BH::ReloadConfig()
{
	if (initialized)
	{
		App.config->LoadConfig();
		LoadLootFilter();
		moduleManager->ReloadConfig();
		statsDisplay->LoadConfig();

		if (D2CLIENT_GetPlayerUnit())
		{
			PrintText(0, "Reloaded config: %s", App.config->GetConfigName().c_str());
			PrintText(0, "Reloaded filter: %s", lootFilter->GetConfigName().c_str());
		}

		// Remove nodraw flag from items when filter is reloaded. This is primarily just a QoL feature.
		// Otherwise you'd have to reload the filter + leave and rejoin the area for hidden items to be visible again
		UnitAny* player = D2CLIENT_GetPlayerUnit();
		if (player && player->pAct && player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo != 0)
		{
			for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext)
			{
				for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext)
				{
					if (unit->dwType == UNIT_ITEM && (unit->dwFlags2 & UNITFLAGEX_INVISIBLE))
					{
						unit->dwFlags2 &= ~UNITFLAGEX_INVISIBLE;
					}
				}
			}
		}
	}
	return true;
}

void BH::LoadLootFilter()
{
	std::ifstream launcherConfig("./AppData/launcherSettings.json");
	if (launcherConfig)
	{
		try
		{
			nlohmann::json launcherJson = nlohmann::json::parse(launcherConfig);
			if (launcherJson.contains("SelectedAuthorAndFilter") &&
				launcherJson["SelectedAuthorAndFilter"].contains("selectedAuthor") &&
				launcherJson["SelectedAuthorAndFilter"]["selectedAuthor"].contains("author") &&
				launcherJson["SelectedAuthorAndFilter"].contains("selectedFilter") &&
				launcherJson["SelectedAuthorAndFilter"]["selectedFilter"].contains("name"))
			{
				std::string author = launcherJson["SelectedAuthorAndFilter"]["selectedAuthor"]["author"].template get<std::string>();
				std::string filter = launcherJson["SelectedAuthorAndFilter"]["selectedFilter"]["name"].template get<std::string>();

				if (author == "Local Filter") { lootFilter->SetConfigName("filters\\local\\" + filter); }
				else { lootFilter->SetConfigName("filters\\online\\" + filter); }
			}
		}
		catch (const json::parse_error&)
		{
		}
	}

	if (!lootFilter->Parse())
	{
		// Fallback to previous behavior
		lootFilter->SetConfigName("loot.filter");
		if (!lootFilter->Parse())
		{
			lootFilter->SetConfigName("default.filter");
			if (!lootFilter->Parse())
			{
				string msg = "Could not find default loot filter.\nAttempted to load " +
					path + "loot.filter (failed).\nAttempted to load " +
					path + "default.filter (failed).\n\nDefaults loaded.";
				MessageBox(NULL, msg.c_str(), "Failed to load ProjectDiablo lootFilter", MB_OK);
			}
		}
	}
}

void BH::CheckForD2GL()
{
	typedef int(__cdecl* d2glIsReady_t)();
	static d2glIsReady_t d2glIsReadyImpl = NULL;

	typedef BOOL(__stdcall* d2glConfigQueryImpl_t)(D2GLConfigId);
	static d2glConfigQueryImpl_t d2glConfigQueryImpl = NULL;

	HMODULE d2glHandle = GetModuleHandleA("glide3x.dll");
	d2glHandle = !d2glHandle ? GetModuleHandleA("ddraw.dll") : d2glHandle;

	if (d2glHandle)
	{
		FARPROC proc = GetProcAddress(d2glHandle, "d2glIsReady");
		d2glIsReadyImpl = proc ? (d2glIsReady_t)proc : NULL;

		proc = GetProcAddress(d2glHandle, "_d2glConfigQueryImpl@4");
		d2glConfigQueryImpl = proc ? (d2glConfigQueryImpl_t)proc : NULL;

	}

	if (d2glIsReadyImpl && d2glConfigQueryImpl)
	{
		App.d2gl.usingD2GL.value = true;
		App.d2gl.usingD2GL.value = d2glConfigQueryImpl(D2GL_CONFIG_HD_TEXT);
	}
}

#ifdef __cplusplus
extern "C" {
#endif
	__declspec(dllexport) int __cdecl BHIsReady()
	{
		return BH::initialized;
	}

	__declspec(dllexport) int __stdcall BHGetConfig(BHConfigId configId)
	{
		switch (configId)
		{
		case BH_CONFIG_EXPERIENCEMETER:
			return App.game.experienceMeter.value;
		case BH_CONFIG_ADVANCEDSTATS:
			return App.general.statsOnRight.value;
		case BH_CONFIG_LOOTFILTER:
			return App.lootfilter.enableFilter.value;
		case BH_CONFIG_FILTERLEVEL:
			return App.lootfilter.filterLevel.value;
		case BH_CONFIG_NUMFILTERLEVELS:
			return itemObj->ItemFilterNames.size();
		case BH_CONFIG_LOOTNOTIFY:
			return App.lootfilter.detailedNotifications.value;
		case BH_CONFIG_SHOWITEMLEVEL:
			return App.lootfilter.showIlvl.value;
		case BH_CONFIG_ALWAYSSHOWSTATRANGE:
			return App.lootfilter.alwaysShowStatRanges.value;
		case BH_CONFIG_ALWAYSSHOWITEMS:
			return App.game.alwaysShowItems.value;
		case BH_CONFIG_DROPSOUNDS:
			return App.lootfilter.dropSounds.value;
		case BH_CONFIG_ADVANCEDSTATS_OPEN:
			return !BH::statsDisplay->IsMinimized();
		case BH_CONFIG_HIDE_GAME_PASSWORD:
			return App.screen.hideGamePassword.value;
		}

		return 0;
	}

	__declspec(dllexport) int __stdcall BHSetConfig(BHConfigId configId, int configVal)
	{
		BOOL bSave = FALSE;
		switch (configId)
		{
		case BH_CONFIG_EXPERIENCEMETER:
			App.game.experienceMeter.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_ADVANCEDSTATS:
			App.general.statsOnRight.value = configVal;
			bSave = TRUE;
			break;

		case BH_CONFIG_LOOTFILTER:
			App.lootfilter.enableFilter.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_FILTERLEVEL:
			if (App.lootfilter.filterLevel.value != configVal)
			{
				itemObj->ChangeFilterLevels(configVal);
			}
			return 1;
		case BH_CONFIG_LOOTNOTIFY:
			App.lootfilter.detailedNotifications.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_SHOWITEMLEVEL:
			App.lootfilter.showIlvl.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_ALWAYSSHOWSTATRANGE:
			App.lootfilter.alwaysShowStatRanges.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_ALWAYSSHOWITEMS:
			App.game.alwaysShowItems.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_SHOWSTATRANGEPRIMARY:
			App.lootfilter.showStatRangesPrimary.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_SHOWSTATRANGESECONDARY:
			App.lootfilter.showStatRangesSecondary.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_USINGHDTEXT:
			App.d2gl.usingHDText.value = configVal;
			return 1;
		case BH_CONFIG_DROPSOUNDS:
			App.lootfilter.dropSounds.value = configVal;
			bSave = TRUE;
			break;
		case BH_CONFIG_HIDE_GAME_PASSWORD:
			App.screen.hideGamePassword.value = configVal;
			bSave = TRUE;
		}

		if (bSave)
		{
			App.config->SaveConfig();
			return 1;
		}
		return 0;
	}

	__declspec(dllexport) int __stdcall BHInteract(BHConfigId configId)
	{
		BOOL bSave = FALSE;
		switch (configId)
		{
		case BH_CONFIG_EXPERIENCEMETER:
			App.game.experienceMeter.value = !App.game.experienceMeter.value;
			bSave = TRUE;
			break;
		case BH_CONFIG_ADVANCEDSTATS:
			if (BH::statsDisplay->IsMinimized())
			{
				BH::statsDisplay->LoadConfig();
				BH::statsDisplay->SetMinimized(false);
			}
			else
			{
				BH::statsDisplay->SetMinimized(true);
			}
			return 1;
		case BH_CONFIG_RELOAD:
			BH::ReloadConfig();
			return 1;
		case BH_CONFIG_INCREASEFILTER:
			if (App.lootfilter.filterLevel.uValue < itemObj->ItemFilterNames.size() - 1)
			{
				itemObj->ChangeFilterLevels(App.lootfilter.filterLevel.uValue + 1);
				bSave = TRUE;
			}
			break;
		case BH_CONFIG_DECREASEFILTER:
			if (App.lootfilter.filterLevel.uValue > 0)
			{
				itemObj->ChangeFilterLevels(App.lootfilter.filterLevel.uValue - 1);
				bSave = TRUE;
			}
			break;
		case BH_CONFIG_RESTOREFILTER:
			if (App.lootfilter.lastFilterLevel.uValue < itemObj->ItemFilterNames.size())
			{
				itemObj->ChangeFilterLevels(App.lootfilter.lastFilterLevel.uValue);
				bSave = TRUE;
			}
			break;
		}
		if (bSave)
		{
			App.config->SaveConfig();
			return 1;
		}
		return 0;
	}

	__declspec(dllexport) int __cdecl BHShowPlayersGear()
	{
		UnitAny* selectedUnit = D2CLIENT_GetSelectedUnit();
		if (selectedUnit && selectedUnit->dwMode != PLAYER_MODE_DEATH && selectedUnit->dwMode != PLAYER_MODE_DEAD && (
			selectedUnit->dwType == 0 ||			// Player
			selectedUnit->dwTxtFileNo == 291 ||		// Iron Golem
			selectedUnit->dwTxtFileNo == 357 ||		// Valkerie
			selectedUnit->dwTxtFileNo == 417 ||		// Shadow Warrior
			selectedUnit->dwTxtFileNo == 418)		// Shadow Master
			)
		{
			Item::viewingUnit = selectedUnit;
			if (!D2CLIENT_GetUIState(0x01))
			{
				D2CLIENT_SetUIVar(0x01, 0, 0);
			}
		}
		return 1;
	}

#ifdef __cplusplus
}
#endif
