#include "GameSettings.h"
#include "../../Drawing.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"
#include "../../BH.h"
#include "../Item/Item.h"

// This module was inspired by the RedVex plugin "Item Mover", written by kaiks.
// Thanks to kaiks for sharing his code.

map<std::string, Toggle> GameSettings::Toggles;
unsigned int GameSettings::KeyHookOffset = 300;

void GameSettings::Init() {
}

void GameSettings::LoadConfig() {
	BH::config->ReadToggle("Quick Cast", "None", false, GameSettings::Toggles["Quick Cast"]);
	BH::config->ReadToggle("Skill Bar", "None", false, GameSettings::Toggles["Skill Bar"]);
	BH::config->ReadToggle("Skill Bar Disable", "None", false, GameSettings::Toggles["Skill Bar Disable"]);
	BH::config->ReadToggle("Buff Timers", "None", false, GameSettings::Toggles["Buff Timers"]);

	BH::config->ReadToggle("Developer Aura", "None", true, GameSettings::Toggles["Developer Aura"]);
	BH::config->ReadToggle("99 Aura", "None", true, GameSettings::Toggles["99 Aura"]);
	BH::config->ReadToggle("Rathma Aura", "None", true, GameSettings::Toggles["Rathma Aura"]);
	BH::config->ReadToggle("Dclone Aura", "None", true, GameSettings::Toggles["Dclone Aura"]);

	BH::config->ReadKey("Show Players Gear", "VK_0", showPlayer);
	BH::config->ReadKey("Resync Hotkey", "VK_9", resyncKey);
	BH::config->ReadKey("Character Stats", "VK_8", advStatMenuKey);
}

void GameSettings::LoadGeneralTab() {
	Drawing::Texthook* colored_text;

	Drawing::UITab* generalTab = new Drawing::UITab("General", BH::settingsUI);

	unsigned int x_tab = 15;
	unsigned int x = 8;
	unsigned int y = 7;

	// General
	new Drawing::Texthook(generalTab, x, (y), "General");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Show Player's Gear");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &showPlayer, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Resync");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &resyncKey, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Advanced Stat Display");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &advStatMenuKey, "");


	// Quick Cast
	y += 20;
	new Drawing::Texthook(generalTab, x, (y), "Quick Cast");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Quick Cast"].state, "Quick Cast");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Quick Cast"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Skill Bar"].state, "Skill Bar");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Skill Bar"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x + x_tab, y, &GameSettings::Toggles["Skill Bar Disable"].state, "Only enable with Quick Cast");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Skill Bar Disable"].toggle, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &GameSettings::Toggles["Buff Timers"].state, "Always show Buff Timers");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Buff Timers"].toggle, "");

}

void GameSettings::LoadInteractionTab() {
	Drawing::Texthook* colored_text;
	Drawing::UITab* tab = new Drawing::UITab("Interaction", BH::settingsUI);

	unsigned int x = 8;
	unsigned int y = 7;
	int offset = 150;
	int indent = 5;

	// QoL
	colored_text = new Drawing::Texthook(tab, x, y,
		"Hovering an item");

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Shift+Left Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"ID with an ID tome");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Shift+Right Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Move between inventories");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Right Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Move to ground");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+Right Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Move to closed cube");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+Left Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Toggle stacked/unstacked");
	colored_text->SetColor(Gold);

	y += 20;
	colored_text = new Drawing::Texthook(tab, x, y,
		"Holding an item");

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Left Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Place 1 stacked item");
	colored_text->SetColor(Gold);

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Shift+Left Click");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Place 1 unstacked item");
	colored_text->SetColor(Gold);

	// Auras
	y += 20;
	new Drawing::Texthook(tab, x, (y), "Auras (only for top 3 players)");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["99 Aura"].state, "Show 99 Aura");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["99 Aura"].toggle, "");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Dclone Aura"].state, "Show Dclone Aura");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Dclone Aura"].toggle, "");

	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Rathma Aura"].state, "Show Rathma Aura");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Rathma Aura"].toggle, "");

	y += 20;
	new Drawing::Texthook(tab, x, (y), "Developer Only");
	y += 15;
	new Drawing::Checkhook(tab, x, y, &GameSettings::Toggles["Developer Aura"].state, "Show Developer Aura");
	new Drawing::Keyhook(tab, GameSettings::KeyHookOffset, y + 2, &GameSettings::Toggles["Developer Aura"].toggle, "");
}

void GameSettings::OnLoad() {
	LoadConfig();
	LoadGeneralTab();
	LoadInteractionTab();
}

void GameSettings::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	if (key == showPlayer) {
		*block = true;
		if (up)
			return;
		UnitAny* selectedUnit = D2CLIENT_GetSelectedUnit();
		if (selectedUnit && selectedUnit->dwMode != 0 && selectedUnit->dwMode != 17 && ( // Alive
			selectedUnit->dwType == 0 ||					// Player
			selectedUnit->dwTxtFileNo == 291 ||		// Iron Golem
			selectedUnit->dwTxtFileNo == 357 ||		// Valkerie
			selectedUnit->dwTxtFileNo == 418)) {	// Shadow Master
			Item::viewingUnit = selectedUnit;
			if (!D2CLIENT_GetUIState(0x01))
				D2CLIENT_SetUIVar(0x01, 0, 0);
			return;
		}
	}
}
