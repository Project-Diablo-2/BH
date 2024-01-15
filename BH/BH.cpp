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

string BH::path;
HINSTANCE BH::instance;
ModuleManager* BH::moduleManager;
Config* BH::lootFilter;
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
bool BH::Startup(HINSTANCE instance, VOID* reserved) {

	BH::instance = instance;
	if (reserved != NULL) {
		cGuardModule* pModule = (cGuardModule*)reserved;
		if (!pModule)
			return FALSE;
		path.assign(pModule->szPath);
		cGuardLoaded = true;
	}
	else {
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
	if (!lootFilter->Parse()) {
		lootFilter->SetConfigName("default.filter");
		if (!lootFilter->Parse()) {
			string msg = "Could not find default loot filter.\nAttempted to load " +
				path + "loot.filter (failed).\nAttempted to load " +
				path + "default.filter (failed).\n\nDefaults loaded.";
			MessageBox(NULL, msg.c_str(), "Failed to load ProjectDiablo lootFilter", MB_OK);
		}
	}


	// Do this asynchronously because D2GFX_GetHwnd() will be null if
	// we inject on process start
	Task::Enqueue([]() -> void {
		std::chrono::milliseconds duration(200);
		while (!D2GFX_GetHwnd()) {
			std::this_thread::sleep_for(duration);
		}
		BH::OldWNDPROC = (WNDPROC)GetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC);
		SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)GameWindowEvent);
		});

	settingsUI = new Drawing::UI(SETTINGS_TEXT, App.bhui.sizeX.value, App.bhui.sizeY.value);

	Task::InitializeThreadPool(2);

	new GameSettings();
	new ScreenInfo();
	new Gamefilter();
	new Bnet();
	new Item();
	new Party();
	new ItemMover();
	new StashExport();
	new MapNotify();
	new ChatColor();

	moduleManager->LoadModules();

	statsDisplay = new Drawing::StatsDisplay("Stats");

	// Injection would occasionally deadlock (I only ever saw it when using Tabbed Diablo
	// but theoretically it could happen during regular injection):
	// installation until after all startup initialization is done.
	for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++) {
		patches[n]->Install();
	}

	if (!D2CLIENT_GetPlayerUnit())
		oogDraw->Install();

	// GameThread can potentially run oogDraw->Install, so create the thread after all
	// loading/installation finishes.
	CreateThread(0, 0, GameThread, 0, 0, 0);

	initialized = true;
}

bool BH::Shutdown() {
	if (initialized) {
		moduleManager->UnloadModules();

		delete moduleManager;
		delete settingsUI;
		delete statsDisplay;

		SetWindowLong(D2GFX_GetHwnd(), GWL_WNDPROC, (LONG)BH::OldWNDPROC);
		for (int n = 0; n < (sizeof(patches) / sizeof(Patch*)); n++) {
			delete patches[n];
		}

		oogDraw->Remove();
	}

	return true;
}

bool BH::ReloadConfig() {
	if (initialized) {
		if (D2CLIENT_GetPlayerUnit()) {
			PrintText(0, "Reloading config: %s", App.config->GetConfigName().c_str());
			PrintText(0, "Reloading filter: %s", lootFilter->GetConfigName().c_str());
		}
		App.config->LoadConfig();
		lootFilter->Parse();
		moduleManager->ReloadConfig();
		statsDisplay->LoadConfig();

		// Remove nodraw flag from items when filter is reloaded. This is primarily just a QoL feature.
		// Otherwise you'd have to reload the filter + leave and rejoin the area for hidden items to be visible again
		UnitAny* player = D2CLIENT_GetPlayerUnit();
		if (player && player->pAct && player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo != 0) {
			for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext) {
				for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext) {
					if (unit->dwType == UNIT_ITEM && (unit->dwFlags2 & UNITFLAGEX_INVISIBLE)) {
						unit->dwFlags2 &= ~UNITFLAGEX_INVISIBLE;
					}
				}
			}
		}
	}
	return true;
}
