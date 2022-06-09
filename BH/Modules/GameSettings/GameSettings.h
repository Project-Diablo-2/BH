#pragma once
#include "../../Drawing.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"

class GameSettings : public Module {
private:
	unsigned int showPlayer;
public:
	static unsigned int KeyHookOffset;
	static map<std::string, Toggle> Toggles;
	unsigned int resyncKey;
	unsigned int advStatMenuKey;

	GameSettings() : Module("GameSettings") {};
	~GameSettings() {};

	void Init();

	void LoadConfig();
	void OnLoad();
	void LoadGeneralTab();
	void LoadInteractionTab();

	void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);
};
