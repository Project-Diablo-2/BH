#include "GameSettings.h"
#include "../../Drawing.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"
#include "../../BH.h"
#include "../Item/Item.h"
#include "../ScreenInfo/ScreenInfo.h"

// This module was inspired by the RedVex plugin "Item Mover", written by kaiks.
// Thanks to kaiks for sharing his code.

unsigned int GameSettings::KeyHookOffset = 300;

void GameSettings::Init() {
}

void GameSettings::LoadConfig() {

}

void GameSettings::LoadGeneralTab() {
	/*
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
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.showPlayersGear.hotkey, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Resync");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.resyncHotkey.hotkey, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Advanced Stat Display");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.characterStats.hotkey, "");

	y += 15;
	colored_text = new Drawing::Texthook(generalTab, x, (y), "Reload Config");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.general.reloadConfig.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.experienceMeter.toggle.isEnabled, "Experience Meter");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.experienceMeter.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.screenshake.toggle.isEnabled, "Enable Screenshake");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.screenshake.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.dpsCounter.toggle.isEnabled, "Enable DPS Counter");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.dpsCounter.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.beltStatus.toggle.isEnabled, "Enable Belt Status");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.beltStatus.toggle.hotkey, "");


	// Quick Cast
	y += 20;
	new Drawing::Texthook(generalTab, x, (y), "Quick Cast");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.quickCast.toggle.isEnabled, "Quick Cast");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.quickCast.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.skillBar.toggle.isEnabled, "Skill Bar");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.skillBar.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x + x_tab, y, &App.game.skillBarDisable.toggle.isEnabled, "Only enable with Quick Cast");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.skillBarDisable.toggle.hotkey, "");

	y += 15;
	new Drawing::Checkhook(generalTab, x, y, &App.game.buffTimers.toggle.isEnabled, "Always show Buff Timers");
	new Drawing::Keyhook(generalTab, GameSettings::KeyHookOffset, y + 2, &App.game.buffTimers.toggle.hotkey, "");
	*/
}

void GameSettings::LoadInteractionTab() {
	/*
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

	y += 20;
	colored_text = new Drawing::Texthook(tab, x, y,
		"Changing filter levels");

	y += 15;
	colored_text = new Drawing::Texthook(tab, x + indent, y,
		"Ctrl+Numpad [0 - 9]");
	colored_text->SetColor(Gold);
	colored_text = new Drawing::Texthook(tab, x + indent + offset, y,
		"Set filter level");
	colored_text->SetColor(Gold);
	*/
}

void GameSettings::OnLoad() {
	LoadConfig();
	LoadGeneralTab();
	LoadInteractionTab();
}

void GameSettings::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	bool ctrlState = ((GetKeyState(VK_LCONTROL) & 0x80) || (GetKeyState(VK_RCONTROL) & 0x80));
	for (vector<Toggle*>::iterator it = App.hotkeyToggles.begin(); it != App.hotkeyToggles.end(); it++) {
		if (key == (*it)->hotkey && !ctrlState && D2CLIENT_GetUIState(UI_GAME)) {
			*block = true;
			if (up) {
				(*it)->isEnabled = !(*it)->isEnabled;
				App.config->SaveConfig();
			}
			return;
		}
	}
	/*
	if (key == App.game.showPlayersGear.hotkey) {
		*block = true;
		if (up)
			return;
		UnitAny* selectedUnit = D2CLIENT_GetSelectedUnit();
		if (selectedUnit && selectedUnit->dwMode != 0 && selectedUnit->dwMode != 17 && ( // Alive
			selectedUnit->dwType == 0 ||					// Player
			selectedUnit->dwTxtFileNo == 291 ||		// Iron Golem
			selectedUnit->dwTxtFileNo == 357 ||		// Valkerie
			selectedUnit->dwTxtFileNo == 417 ||		// Shadow Warrior
			selectedUnit->dwTxtFileNo == 418)) {	// Shadow Master
			Item::viewingUnit = selectedUnit;
			if (!D2CLIENT_GetUIState(0x01))
				D2CLIENT_SetUIVar(0x01, 0, 0);
			return;
		}
	}

	if (!BH::settingsUI->IsMinimized() && key == VK_ESCAPE) {
		*block = true;
		if (up)
			return;
		BH::settingsUI->SetMinimized(true, true);
		return;
	}
	*/
}
