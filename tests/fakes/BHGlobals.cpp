// Definitions that BH keeps in files the test build does not compile (BH.cpp, Item.cpp, Module.cpp,
// ModuleManager.cpp). Those files install game hooks and start BH up, so they are left out; the
// globals they own are plain data here, with the defaults the tests rely on noted.
#include "BH.h"
#include "Modules/Item/Item.h"
#include "Modules/Item/ItemDisplay.h"
#include "Modules/Module.h"
#include "Modules/ModuleManager.h"

#include "BHGlobals.h"
#include "ResetEachTest.h"

// BH.cpp
string BH::path;
HINSTANCE BH::instance;
ModuleManager* BH::moduleManager = &fake::Modules();
Config* BH::lootFilter;
BHApp App;
bool BH::cGuardLoaded;  // false: no cGuard in the test process

// Item.cpp. The real values come from the game's data tables at game join; tests set what they need.
unsigned int STAT_MAX;
unsigned int SKILL_MAX;
unsigned int PREFIX_OFFSET;
unsigned int AUTOMOD_OFFSET;
std::vector<StatProperties*> AllStatList;
std::vector<CharStats*> CharList;
std::map<std::string, ItemAttributes*> ItemAttributeMap;  // item code -> attributes; empty

// Item.cpp: true once the data tables above are loaded, which the tests treat as always.
bool IsInitialized() {
	return true;
}

// Item.cpp (same body).
void ResetCaches() {
	item_desc_cache.ResetCache();
	item_name_cache.ResetCache();
	map_action_cache.ResetCache();
}

// Module.cpp / ModuleManager.cpp / Item.cpp: just enough of the module system for
// ItemDisplay::InitializeItemRules, which publishes the filter level names on the "item" module.
Module::Module(string name) : name(name), active(false) {}
Module::~Module() {}

ModuleManager::ModuleManager() {}
ModuleManager::~ModuleManager() {}

Module* ModuleManager::Get(string name) {
	return name == "item" ? &fake::ItemModule() : nullptr;
}

void Item::OnLoad() {}
void Item::OnUnload() {}
void Item::LoadConfig() {}
void Item::OnGameJoin() {}
void Item::OnLoop() {}
void Item::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {}
void Item::OnLeftClick(bool up, int x, int y, bool* block) {}

namespace fake {
namespace {

// Every test case starts with BH's settings at their defaults, no loot filter loaded, and the
// data-table sizes of the 1.13c game (ItemStatCost.txt and Skills.txt row counts).
void ResetBHState() {
	ItemDisplay::UninitializeItemRules();
	UnknownItemCodes.clear();
	ItemAttributeMap.clear();
	ItemModule().ItemFilterNames.clear();
	App = BHApp();
	BH::path.clear();
	BH::lootFilter = nullptr;
	STAT_MAX = 359;
	SKILL_MAX = 357;
	PREFIX_OFFSET = 0;
	AUTOMOD_OFFSET = 0;
}

}  // namespace

REGISTER_LISTENER("bh-state-reset", 1, ResetEachTest<&ResetBHState>);

ModuleManager& Modules() {
	static ModuleManager manager;
	return manager;
}

Item& ItemModule() {
	static Item item;
	return item;
}

}  // namespace fake
