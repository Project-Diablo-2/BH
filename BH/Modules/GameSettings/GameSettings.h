#pragma once
#include "../../Drawing.h"
#include "../Module.h"
#include "../../Config.h"
#include "../../Common.h"

class GameSettings : public Module {
public:
	static unsigned int KeyHookOffset;

	GameSettings() : Module("GameSettings") {};
	~GameSettings() {};

	void Init();

	void LoadConfig();
	void OnLoad();
	void LoadGeneralTab();
	void LoadInteractionTab();

	void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);
};
