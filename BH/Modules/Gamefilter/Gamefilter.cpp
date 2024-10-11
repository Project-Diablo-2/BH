#include "Gamefilter.h"
#include "ParsedFilterString.h"
#include "../../D2Ptrs.h"
#include "../../D2Intercepts.h"
#include "../../D2Stubs.h"
#include <sstream>
#include "../../BH.h"

using namespace std;

list<GameListEntry*> Gamefilter::gameList;
vector<GameListEntry*> Gamefilter::filterVector;
vector<wchar_t*> Gamefilter::gServerVector;
EditBox* Gamefilter::filterBox;
Button* Gamefilter::filterNorm;
Button* Gamefilter::filterNightmare;
Button* Gamefilter::filterHell;
int Gamefilter::refreshTime;
std::string sLastFilter;

Patch* createGameBox = new Patch(Call, D2MULTI, { 0x149EF, 0xAD8F }, (int)D2MULTI_CreateGameBox_Interception, 5);
Patch* destoryGameList = new Patch(Call, D2MULTI, { 0x11DC3, 0x8413 }, (int)Gamefilter::DestroyGamelist, 5);
Patch* listRefresh = new Patch(Call, D2MULTI, { 0xDF4E, 0x121EE }, (int)D2MULTI_GameListRefresh_Interception, 5);

void Gamefilter::OnLoad() {
	if (!D2CLIENT_GetPlayerUnit()) {
		createGameBox->Install();
		destoryGameList->Install();
		listRefresh->Install();
	}
	refreshTime = App.bnet.refreshTime.value;

	LoadConfig();
}

void Gamefilter::LoadConfig() {

}

void Gamefilter::OnUnload() {
	createGameBox->Remove();
	destoryGameList->Remove();
	listRefresh->Remove();
}

void Gamefilter::OnGameJoin() {
	createGameBox->Remove();
	destoryGameList->Remove();
	listRefresh->Remove();
}

void Gamefilter::OnGameExit() {
	createGameBox->Install();
	destoryGameList->Install();
	listRefresh->Install();
}

void Gamefilter::OnRealmPacketRecv(BYTE* pPacket, bool* blockPacket) {
	//S>C 0x17 MCP_CHARLIST
	//S>C 0x19 MCP_CHARLIST2
	if (pPacket[0] == 0x19 || pPacket[0] == 0x17) {
		unsigned int nChars = *reinterpret_cast<int*>(pPacket + 3);
		for (TextBox* pControl = (TextBox*)*p_D2WIN_FirstControl; pControl; pControl = (TextBox*)pControl->pNext)
		{
			if (pControl->dwType == 4 && pControl->ptScrollBar && pControl->ptScrollBar->dwType == 5) {
				pControl->dwMaxLines = nChars;
				pControl->dwCurrentLine = 0;

				pControl->ptScrollBar->dwScrollPosition = 0;
				pControl->ptScrollBar->dwScrollEntries = nChars > 8 ? (unsigned int)(ceil((nChars - 8) / 2.0)) : 0;
			}
		}
	}
	//S>C 0x05 MCP_GAMELIST
	if (pPacket[0] == 0x05 && filterBox)
	{
		wstring wFilter(filterBox->wText);
		string sFilter(wFilter.begin(), wFilter.end());

		GameListEntry* pEntry = new GameListEntry;

		pEntry->wRequestId = *(WORD*)&pPacket[1];
		pEntry->dwIndex = *(DWORD*)&pPacket[3];
		pEntry->bPlayers = pPacket[7];
		pEntry->dwStatus = *(DWORD*)&pPacket[8];
		pEntry->gs = pPacket[12];	// This is not part of the original 1.13c packet
		pEntry->lootAlloc = pPacket[13];	// This is not part of the original 1.13c packet
		pEntry->sGameName = reinterpret_cast<CHAR*>(pPacket + 14);
		pEntry->sGameDesc = reinterpret_cast<CHAR*>(pPacket + 15 + pEntry->sGameName.length());

		if (pEntry->bPlayers == 0)
		{
			delete pEntry;
			*blockPacket = true;
			return;
		}

		if (!gameList.empty())
		{
			for (list<GameListEntry*>::iterator ListEntry = gameList.begin(); ListEntry != gameList.end(); ListEntry++)
				if (!(*ListEntry)->sGameName.compare(pEntry->sGameName))
				{
					delete pEntry;
					*blockPacket = true;
					return;
				}
		}

		bool showDifficulty;
		if (pEntry->dwStatus & 0x1000)
		{
			showDifficulty = App.bnet.showNightmareDiff.value;
		}
		else if (pEntry->dwStatus & 0x2000)
		{
			showDifficulty = App.bnet.showHellDiff.value;
		}
		else
		{
			showDifficulty = App.bnet.showNormalDiff.value;
		}

		for (unsigned int i = 0; i < sFilter.length(); i++)
			sFilter[i] = ::toupper(sFilter[i]);
		sLastFilter = sFilter;

		ParsedFilterString parsedFilter(sFilter.c_str());
		if (parsedFilter.IsIncluded(pEntry) && showDifficulty)
		{
			ControlText* pText = new ControlText;
			memset(pText, NULL, sizeof(ControlText));

			pText->dwColor = 4;
			pText->wText[0] = new wchar_t[64];
			pText->wText[1] = new wchar_t[12];

			memset(pText->wText[0], NULL, 64);
			memset(pText->wText[1], NULL, 12);

			wstring wGameName(pEntry->sGameName.begin(), pEntry->sGameName.end());

			wcscpy_s(pText->wText[0], 64, wGameName.c_str());
			pText->wText[1][0] = pEntry->bPlayers + 0x30;

			if ((*p_D2MULTI_GameListControl)->pFirstText)
			{
				(*p_D2MULTI_GameListControl)->pLastText->pNext = pText;
				(*p_D2MULTI_GameListControl)->pLastText = pText;
			}
			else {
				(*p_D2MULTI_GameListControl)->pFirstText = pText;
				(*p_D2MULTI_GameListControl)->pLastText = pText;
				(*p_D2MULTI_GameListControl)->pSelectedText = pText;
			}

			(*p_D2MULTI_GameListControl)->dwMaxLines += 1;
			(*p_D2MULTI_GameListControl)->ptScrollBar->dwScrollEntries = (*p_D2MULTI_GameListControl)->dwMaxLines > 9 ? (*p_D2MULTI_GameListControl)->dwMaxLines - 9 : 0;
			filterVector.push_back(pEntry);

			// TODO: remove this switch
			switch (pEntry->gs + 1)
			{
			case 1:
				gServerVector.push_back(L"gs1");
				break;
			case 2:
				gServerVector.push_back(L"gs2");
				break;
			case 3:
				gServerVector.push_back(L"gs3");
				break;
			case 4:
				gServerVector.push_back(L"gs4");
				break;
			case 5:
				gServerVector.push_back(L"gs5");
				break;
			case 6:
				gServerVector.push_back(L"gs6");
				break;
			case 7:
				gServerVector.push_back(L"gs7");
				break;
			case 8:
				gServerVector.push_back(L"gs8");
				break;
			case 9:
				gServerVector.push_back(L"gs9");
				break;
			default:
				gServerVector.push_back(L"");
				break;
			}
		}

		gameList.push_back(pEntry);

		return;
	}

	return;
}

BOOL __stdcall Gamefilter::Filterbox_InputHandler(EditBox* pControl, DWORD dwLength, CHAR* pChar)
{
	wstring wInput(pControl->wText);

	string sFilter(wInput.begin(), wInput.end());

	if (dwLength == 0 && *pChar != '\b')
	{
		sFilter += *pChar;
	}
	else if (dwLength > 0 && *pChar != '\b')
	{
		sFilter += *pChar;
	}

	for (unsigned int i = 0; i < sFilter.length(); i++)
	{
		sFilter[i] = ::toupper(sFilter[i]);
	}
	sLastFilter = sFilter;

	Gamefilter::BuildGameList(sFilter);

	return TRUE;
}

BOOL __stdcall Gamefilter::Filterbox_ReturnHandler(wchar_t* wText)
{
	return TRUE;
}

BOOL __stdcall Gamefilter::FilterButtonNormal_Callback(SMSGHANDLER_PARAMS* pMsg)
{
	App.bnet.showNormalDiff.value = !App.bnet.showNormalDiff.value;
	App.config->SaveConfig();

	int isPressed = D2WIN_BUTTON_IsPressed(filterNorm);
	D2WIN_BUTTON_SetIsPressed(filterNorm, isPressed ? 1 : 0);

	Gamefilter::BuildGameList(sLastFilter);

	return TRUE;
}

BOOL __stdcall Gamefilter::FilterButtonNightmare_Callback(SMSGHANDLER_PARAMS* pMsg)
{
	App.bnet.showNightmareDiff.value = !App.bnet.showNightmareDiff.value;
	App.config->SaveConfig();

	int isPressed = D2WIN_BUTTON_IsPressed(filterNightmare);
	D2WIN_BUTTON_SetIsPressed(filterNightmare, isPressed ? 1 : 0);

	Gamefilter::BuildGameList(sLastFilter);

	return TRUE;
}

BOOL __stdcall Gamefilter::FilterButtonHell_Callback(SMSGHANDLER_PARAMS* pMsg)
{
	App.bnet.showHellDiff.value = !App.bnet.showHellDiff.value;
	App.config->SaveConfig();

	int isPressed = D2WIN_BUTTON_IsPressed(filterHell);
	D2WIN_BUTTON_SetIsPressed(filterHell, isPressed ? 1 : 0);

	Gamefilter::BuildGameList(sLastFilter);

	return TRUE;
}


VOID Gamefilter::CreateGamelist(VOID)
{
	if (!gameList.empty())
	{
		for (list<GameListEntry*>::iterator ListEntry = gameList.begin(); ListEntry != gameList.end(); ListEntry++)
			delete (*ListEntry);

		gameList.clear();
		filterVector.clear();
		gServerVector.clear();

	}

	filterBox = D2WIN_CreateEditBox(599, 185, 155, 20, 7, -3, NULL, Gamefilter::Filterbox_ReturnHandler, 0, NULL, (ControlPreferences*)p_D2MULTI_EditboxPreferences);
	filterBox->pCellFile = D2CLIENT_LoadUiImage("DATA\\GLOBAL\\UI\\FrontEnd\\textbox2");
	D2WIN_SetEditBoxCallback(filterBox, Gamefilter::Filterbox_InputHandler);
	filterBox->dwMaxLength = 0x12;

	filterNorm =		D2WIN_BUTTON_Create(425, 173, 15, 16, NULL, Gamefilter::FilterButtonNormal_Callback, 0, 0, 2, NULL, NULL);
	filterNightmare =	D2WIN_BUTTON_Create(425 + 40, 173, 15, 16, NULL, Gamefilter::FilterButtonNightmare_Callback, 0, 0, 2, NULL, NULL);
	filterHell =		D2WIN_BUTTON_Create(425 + 80, 173, 15, 16, NULL, Gamefilter::FilterButtonHell_Callback, 0, 0, 2, NULL, NULL);
	filterNorm->pCellFile = D2CLIENT_LoadUiImage("DATA\\GLOBAL\\UI\\BIGMENU\\joingameclickbox");
	filterNightmare->pCellFile = D2CLIENT_LoadUiImage("DATA\\GLOBAL\\UI\\BIGMENU\\joingameclickbox");
	filterHell->pCellFile = D2CLIENT_LoadUiImage("DATA\\GLOBAL\\UI\\BIGMENU\\joingameclickbox");
	D2WIN_BUTTON_SetIsPressed(filterNorm, App.bnet.showNormalDiff.value);
	D2WIN_BUTTON_SetIsPressed(filterNightmare, App.bnet.showNightmareDiff.value);
	D2WIN_BUTTON_SetIsPressed(filterHell, App.bnet.showHellDiff.value);
}

VOID __stdcall Gamefilter::DestroyGamelist(Control* pControl)
{
	if (pControl)
	{
		if (filterBox)
		{
			if (!gameList.empty())
			{
				for (list<GameListEntry*>::iterator ListEntry = gameList.begin(); ListEntry != gameList.end(); ListEntry++)
					delete (*ListEntry);

				gameList.clear();
				filterVector.clear();
				gServerVector.clear();
			}

			D2WIN_DestroyEditBox(filterBox);

			filterBox = NULL;
		}
		if (filterNorm)
		{
			D2WIN_BUTTON_Destroy(filterNorm);
			filterNorm = NULL;
		}
		if (filterNightmare)
		{
			D2WIN_BUTTON_Destroy(filterNightmare);
			filterNightmare = NULL;
		}
		if (filterHell)
		{
			D2WIN_BUTTON_Destroy(filterHell);
			filterHell = NULL;
		}
	}

	D2WIN_DestroyControl(pControl);
}

void Gamefilter::OnOOGDraw() {
	// filterBox is instantiated in the create game box handler, so we can't
	// draw the join game screen until we have it
	if ((*p_D2MULTI_GameListControl) && filterBox) {
		// Allow tabbing between all 3 boxes
		if ((*p_D2MULTI_GameName) && (*p_D2MULTI_PassBox))
		{
			D2WIN_EDITBOX_SetNextAndPrevious(*p_D2MULTI_GameName, *p_D2MULTI_PassBox);
			D2WIN_EDITBOX_SetNextAndPrevious(*p_D2MULTI_PassBox, filterBox);
			D2WIN_EDITBOX_SetNextAndPrevious(filterBox, *p_D2MULTI_GameName);
		}
		wstringstream wFilterStream;
		wstring wFilterString = L"Games: ";
		wstring wFilter = filterBox->wText;

		wFilterStream << (int)(*p_D2MULTI_GameListControl)->dwMaxLines;
		wFilterString += wFilterStream.str().c_str();
		wFilterString += L"/";
		wFilterStream.str(L"");
		wFilterStream << (int)gameList.size();
		wFilterString += wFilterStream.str().c_str();

		DWORD dwOldSize = D2WIN_SetTextSize(1);
		D2WIN_DrawText(L"Filter", 549, 170, 4, -1);
		D2WIN_DrawText(wFilterString.c_str(), 424, 190, 4, -1);

		if (App.bnet.showDifficulty.value || App.bnet.showGameserver.value) {
			D2WIN_SetTextSize(6);
			DWORD dwListStart = (*p_D2MULTI_GameListControl)->dwCurrentLine;
			DWORD dwListEnd = filterVector.size() > 9 ? dwListStart + 9 : filterVector.size();
			for (unsigned int i = dwListStart; i < dwListEnd; i++)
			{
				unsigned int uiYPos = 230 + (i - dwListStart) * 19;
				if (App.bnet.showDifficulty.value) {
					if ((filterVector.at(i)->dwStatus & 0x1000)) {
						D2WIN_DrawText(L"NM", 547, uiYPos + 1, Blue, -1);
					}
					else if ((filterVector.at(i)->dwStatus & 0x2000)) {
						D2WIN_DrawText(L"H", 547, uiYPos + 1, Red, -1);
					}
					else {
						D2WIN_DrawText(L"N", 547, uiYPos + 1, White, -1);
					}
				}
				if (App.bnet.showGameserver.value) {
					D2WIN_DrawText(gServerVector.at(i), 528, uiYPos, Grey, -1);
				}
				if (filterVector.at(i)->lootAlloc)
				{
					D2WIN_DrawText(L"A", 577, uiYPos + 1, Grey, -1);
				}
			}
		}
		// Difficulty Filter buttons
		D2WIN_DrawText(L"N", 445, 173, White, -1);
		D2WIN_DrawText(L"NM", 485, 173, Blue, -1);
		D2WIN_DrawText(L"H", 525, 173, Red, -1);

		D2WIN_SetTextSize(dwOldSize);
	}
}

void Gamefilter::BuildGameList(string sFilter)
{
	if (!gameList.empty() && (*p_D2MULTI_GameListControl))
	{
		filterVector.clear();
		gServerVector.clear();
		ControlText* pText = (*p_D2MULTI_GameListControl)->pFirstText;

		(*p_D2MULTI_GameListControl)->pFirstText = NULL;
		(*p_D2MULTI_GameListControl)->pLastText = NULL;
		(*p_D2MULTI_GameListControl)->pSelectedText = NULL;
		(*p_D2MULTI_GameListControl)->dwMaxLines = 0;
		(*p_D2MULTI_GameListControl)->dwCurrentLine = 0;
		(*p_D2MULTI_GameListControl)->dwTopOffset = 0;

		(*p_D2MULTI_GameListControl)->ptScrollBar->bMovedDown = 0;
		(*p_D2MULTI_GameListControl)->ptScrollBar->dwScrollPosition = 0;
		(*p_D2MULTI_GameListControl)->ptScrollBar->dwScrollEntries = 0;

		while (pText)
		{
			ControlText* pNext = pText->pNext;

			delete[] pText->wText[0];
			delete[] pText->wText[1];
			delete pText;

			pText = pNext;
		}

		for (list<GameListEntry*>::iterator ListEntry = gameList.begin(); ListEntry != gameList.end(); ListEntry++)
		{
			bool showDifficulty;
			if ((*ListEntry)->dwStatus & 0x1000)
			{
				showDifficulty = App.bnet.showNightmareDiff.value;
			}
			else if ((*ListEntry)->dwStatus & 0x2000)
			{
				showDifficulty = App.bnet.showHellDiff.value;
			}
			else
			{
				showDifficulty = App.bnet.showNormalDiff.value;
			}

			ParsedFilterString parsedFilter(sFilter.c_str());
			if (parsedFilter.IsIncluded(*ListEntry) && showDifficulty)
			{

				ControlText* pText = new ControlText;
				memset(pText, NULL, sizeof(ControlText));

				pText->dwColor = 4;
				pText->wText[0] = new wchar_t[64];
				pText->wText[1] = new wchar_t[12];

				memset(pText->wText[0], NULL, 64);
				memset(pText->wText[1], NULL, 12);

				wstring wGameName((*ListEntry)->sGameName.begin(), (*ListEntry)->sGameName.end());

				wcscpy_s(pText->wText[0], 64, wGameName.c_str());
				pText->wText[1][0] = (*ListEntry)->bPlayers + 0x30;

				if ((*p_D2MULTI_GameListControl)->pFirstText)
				{
					(*p_D2MULTI_GameListControl)->pLastText->pNext = pText;
					(*p_D2MULTI_GameListControl)->pLastText = pText;
				}
				else {
					(*p_D2MULTI_GameListControl)->pFirstText = pText;
					(*p_D2MULTI_GameListControl)->pLastText = pText;
					(*p_D2MULTI_GameListControl)->pSelectedText = pText;
				}


				(*p_D2MULTI_GameListControl)->dwMaxLines += 1;
				(*p_D2MULTI_GameListControl)->ptScrollBar->dwScrollEntries = (*p_D2MULTI_GameListControl)->dwMaxLines > 9 ? (*p_D2MULTI_GameListControl)->dwMaxLines - 9 : 0;
				filterVector.push_back(*ListEntry);

				// TODO: remove this switch
				switch ((*ListEntry)->gs + 1)
				{
				case 1:
					gServerVector.push_back(L"gs1");
					break;
				case 2:
					gServerVector.push_back(L"gs2");
					break;
				case 3:
					gServerVector.push_back(L"gs3");
					break;
				case 4:
					gServerVector.push_back(L"gs4");
					break;
				case 5:
					gServerVector.push_back(L"gs5");
					break;
				case 6:
					gServerVector.push_back(L"gs6");
					break;
				case 7:
					gServerVector.push_back(L"gs7");
					break;
				case 8:
					gServerVector.push_back(L"gs8");
					break;
				case 9:
					gServerVector.push_back(L"gs9");
					break;
				default:
					gServerVector.push_back(L"");
					break;
				}

			}
		}
	}
}

VOID __declspec(naked) D2MULTI_CreateGameBox_Interception(VOID)
{
	__asm
	{
		PUSHAD
		CALL Gamefilter::CreateGamelist
		POPAD
		MOV EDX, 0x10
		RETN
	}
}

VOID __declspec(naked) D2MULTI_GameListRefresh_Interception(VOID)
{
	__asm
	{
		CMP EAX, Gamefilter::refreshTime
		RETN
	}
}
