#include "Bnet.h"
#include "../../D2Ptrs.h"
#include "../../BH.h"

unsigned int Bnet::failToJoin;
std::regex Bnet::reg = std::regex("^(.*?)(\\d+)$");

Patch* nextGame1 = new Patch(Call, D2MULTI, { 0x14D29, 0xADAB }, (int)Bnet::NextGamePatch, 5);
Patch* nextGame2 = new Patch(Call, D2MULTI, { 0x14A0B, 0xB5E9 }, (int)Bnet::NextGamePatch, 5);
Patch* nextPass1 = new Patch(Call, D2MULTI, { 0x14D64, 0xADE6 }, (int)Bnet::NextPassPatch, 5);
Patch* nextPass2 = new Patch(Call, D2MULTI, { 0x14A46, 0xB624 }, (int)Bnet::NextPassPatch, 5);

Patch* gameDesc = new Patch(Call, D2MULTI, { 0x14D8F, 0xB64F }, (int)Bnet::GameDescPatch, 5);

Patch* ftjPatch = new Patch(Call, D2CLIENT, { 0x4363E, 0x443FE }, (int)FailToJoin_Interception, 6);
Patch* removePass = new Patch(Call, D2MULTI, { 0x1250, 0x1AD0 }, (int)RemovePass_Interception, 5);

void Bnet::OnLoad() {
	failToJoin = App.bnet.failToJoin.value;
	LoadConfig();
}

void Bnet::LoadConfig() {
	InstallPatches();
}

void Bnet::InstallPatches() {
	if (App.bnet.autofillLastGame.value || App.bnet.autofillNextGame.value) {
		nextGame1->Install();
		nextGame2->Install();
	}

	if (App.bnet.autofillLastPass.value) {
		nextPass1->Install();
		nextPass2->Install();
		removePass->Install();
	}

	if (App.bnet.autofillLastDesc.value) {
		gameDesc->Install();
	}

	if (failToJoin > 0 && !D2CLIENT_GetPlayerUnit())
		ftjPatch->Install();
}

void Bnet::RemovePatches() {
	nextGame1->Remove();
	nextGame2->Remove();

	nextPass1->Remove();
	nextPass2->Remove();

	gameDesc->Remove();

	ftjPatch->Remove();
	removePass->Remove();
}

void Bnet::OnUnload() {
	RemovePatches();
}

void Bnet::OnGameJoin() {
	if (strlen((*p_D2LAUNCH_BnData)->szGameName) > 0)
	{
		App.bnet.saveLastGame.value = (*p_D2LAUNCH_BnData)->szGameName;
	}

	if (strlen((*p_D2LAUNCH_BnData)->szGamePass) > 0)
	{
		App.bnet.saveLastPass.value = (*p_D2LAUNCH_BnData)->szGamePass;
	}
	else
	{
		App.bnet.saveLastPass.value = "";
	}

	if (strlen((*p_D2LAUNCH_BnData)->szGameDesc) > 0)
	{
		App.bnet.saveLastDesc.value = (*p_D2LAUNCH_BnData)->szGameDesc;
	}
	else
	{
		App.bnet.saveLastDesc.value = "";
	}

	RemovePatches();
}

void Bnet::OnGameExit() {
	if (App.bnet.autofillNextGame.value) {
		std::smatch match;
		if (std::regex_search(App.bnet.saveLastGame.value, match, Bnet::reg) && match.size() == 3) {
			std::string name = match.format("$1");
			if (name.length() != 0) {
				int count = atoi(match.format("$2").c_str());

				//Restart at 1 if the next number would exceed the max game name length of 15
				if (App.bnet.saveLastGame.value.length() == 15) {
					int maxCountLength = 15 - name.length();
					int countLength = 1;
					int tempCount = count + 1;
					while (tempCount > 9) {
						countLength++;
						tempCount /= 10;
					}
					if (countLength > maxCountLength) {
						count = 1;
					} else {
						count++;
					}
				} else {
					count++;
				}
				char buffer[16];
				sprintf_s(buffer, sizeof(buffer), "%s%d", name.c_str(), count);
				App.bnet.saveLastGame.value = std::string(buffer);
			}
		}
	}

	InstallPatches();
}

VOID __fastcall Bnet::NextGamePatch(Control* box, BOOL (__stdcall *FunCallBack)(Control*, DWORD, DWORD)) {
	if (App.bnet.saveLastGame.value.size() == 0)
		return;

	wchar_t *wszLastGameName = AnsiToUnicode(App.bnet.saveLastGame.value.c_str());

	D2WIN_SetControlText(box, wszLastGameName);
	D2WIN_SelectEditBoxText(box);

	// original code
	D2WIN_SetEditBoxProc(box, FunCallBack);
	delete [] wszLastGameName;
}

VOID __fastcall Bnet::NextPassPatch(Control* box, BOOL(__stdcall *FunCallBack)(Control*, DWORD, DWORD)) {
	if (App.bnet.saveLastPass.value.size() == 0)
		return;
	wchar_t *wszLastPass = AnsiToUnicode(App.bnet.saveLastPass.value.c_str());
	
	D2WIN_SetControlText(box, wszLastPass);
	
	// original code
	D2WIN_SetEditBoxProc(box, FunCallBack);
	delete[] wszLastPass;
}

VOID __fastcall Bnet::GameDescPatch(Control* box, BOOL(__stdcall *FunCallBack)(Control*, DWORD, DWORD)) {
	if (App.bnet.saveLastDesc.value.size() == 0)
		return;
	wchar_t *wszLastDesc = AnsiToUnicode(App.bnet.saveLastDesc.value.c_str());
	
	D2WIN_SetControlText(box, wszLastDesc);
	
	// original code
	D2WIN_SetEditBoxProc(box, FunCallBack);
	delete[] wszLastDesc;
}

void __declspec(naked) RemovePass_Interception() {
	__asm {
		PUSHAD
		CALL [Bnet::RemovePassPatch]
		POPAD

		; Original code
		XOR EAX, EAX
		SUB ECX, 01
		RET
	}
}

void Bnet::RemovePassPatch() {
	Control* box = *p_D2MULTI_PassBox;

	if (App.bnet.saveLastPass.value.size() == 0 || box == nullptr) {
		return;
	}

	wchar_t *wszLastPass = AnsiToUnicode("");
	D2WIN_SetControlText(box, wszLastPass);
	delete[] wszLastPass;
}

void __declspec(naked) FailToJoin_Interception()
{
	/*
	Changes the amount of time, in milliseconds, that we wait for the loading
	door to open before the client confirms that it failed to join the game.
	*/
	__asm
	{
		cmp esi, Bnet::failToJoin;
		ret;
	}
}
