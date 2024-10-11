#pragma once
#include "../Module.h"
#include <Windows.h>
#include <list>
#include <vector>

struct GameListEntry
{
	WORD wRequestId;
	DWORD dwIndex;
	BYTE bPlayers;
	DWORD dwStatus;
	BYTE gs;
	BYTE lootAlloc;
	std::string sGameName;
	std::string sGameDesc;
};
struct Control;
struct EditBox;
struct Button;
struct SMSGHANDLER_PARAMS;

class Gamefilter : public Module {
	public:
		static std::list<GameListEntry*> gameList;
		static std::vector<GameListEntry*> filterVector;
		static std::vector<wchar_t*> gServerVector;
		static EditBox* filterBox;
		static Button* filterNorm;
		static Button* filterNightmare;
		static Button* filterHell;
		static int refreshTime;
	
		Gamefilter() : Module("Gamefilter") {};

		void OnLoad();
		void OnUnload();
		void LoadConfig();

		void OnGameJoin();
		void OnGameExit();

		void OnRealmPacketRecv(BYTE* pPacket, bool* blockPacket);
		void OnOOGDraw();

		static void CreateGamelist();
		static void __stdcall DestroyGamelist(Control* pControl);
		static void BuildGameList(std::string sFilter);
		static BOOL __stdcall Filterbox_InputHandler(EditBox* pControl, DWORD dwLength, CHAR* pChar);
		static BOOL __stdcall Filterbox_ReturnHandler(wchar_t* wText);

		static BOOL __stdcall FilterButtonNormal_Callback(SMSGHANDLER_PARAMS* pMsg);
		static BOOL __stdcall FilterButtonNightmare_Callback(SMSGHANDLER_PARAMS* pMsg);
		static BOOL __stdcall FilterButtonHell_Callback(SMSGHANDLER_PARAMS* pMsg);
};

VOID D2MULTI_CreateGameBox_Interception(VOID);
VOID D2MULTI_GameListRefresh_Interception(VOID);