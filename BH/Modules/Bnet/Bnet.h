#pragma once
#include "../Module.h"
#include <regex>

struct Control;

#define ARENA_GUILD 0x00010000

class Bnet : public Module {
	private:
		static unsigned int failToJoin;
		static std::regex reg;
		static bool inGuildGame;

	public:

		Bnet() : Module("Bnet") {};

		void OnLoad();
		void OnUnload();
		void LoadConfig();

		void OnGameJoin();
		void OnGameExit();

		void InstallPatches();
		void RemovePatches();

		static VOID __fastcall NextGamePatch(Control* box, BOOL (__stdcall *FunCallBack)(Control*, DWORD, DWORD));
		static VOID __fastcall NextPassPatch(Control* box, BOOL(__stdcall *FunCallBack)(Control*, DWORD, DWORD));
		static VOID __fastcall GameDescPatch(Control* box, BOOL(__stdcall *FunCallBack)(Control*, DWORD, DWORD));
		static void RemovePassPatch();
};

void FailToJoin_Interception();
void RemovePass_Interception();
