#include "ItemMover.h"
#include "../../BH.h"
#include "../../D2Ptrs.h"
#include "../../D2Stubs.h"
#include "../../D2Helpers.h"
#include "../Item/Item.h"
#include "../ScreenInfo/ScreenInfo.h"

// This module was inspired by the RedVex plugin "Item Mover", written by kaiks.
// Thanks to kaiks for sharing his code.

int INVENTORY_WIDTH = 10;
int INVENTORY_HEIGHT = 4;
int STASH_WIDTH = 6;
int LOD_STASH_HEIGHT = 8;
int CLASSIC_STASH_HEIGHT = 4;
int CUBE_WIDTH = 3;
int CUBE_HEIGHT = 4;

// These are pixel positions
int INVENTORY_LEFT = 417;
int INVENTORY_TOP = 315;
int STASH_LEFT = 153;
int LOD_STASH_TOP = 143;
int CLASSIC_STASH_TOP = 334;
int CUBE_LEFT = 197;
int CUBE_TOP = 199;
int CELL_SIZE = 29;

DWORD idBookId;
DWORD unidItemId;

void ItemMover::Init() {
	InventoryLayout classicStashLayout = {};
	InventoryLayout lodStashLayout = {};
	InventoryLayout inventoryLayout = {};
	InventoryLayout cubeLayout = {};

	int screenWidth = *p_D2CLIENT_ScreenSizeX;
	int screenHeight = *p_D2CLIENT_ScreenSizeY;
	//PrintText(1, "Got screensize %d, %d", screenWidth, screenHeight);

	BOOL bHighRes = 1;
	if (screenWidth == 640 && screenHeight == 480) {
		bHighRes = 0;
	}

	D2COMMON_10760_DATATBLS_GetInventoryGridInfo(INVENTORY_TXT_STASH_CLASSIC, bHighRes, &classicStashLayout);
	D2COMMON_10760_DATATBLS_GetInventoryGridInfo(INVENTORY_TXT_STASH_LOD, bHighRes, &lodStashLayout);
	D2COMMON_10760_DATATBLS_GetInventoryGridInfo(INVENTORY_TXT_INVENTORY, bHighRes, &inventoryLayout);
	D2COMMON_10760_DATATBLS_GetInventoryGridInfo(INVENTORY_TXT_CUBE, bHighRes, &cubeLayout);

	INVENTORY_LEFT = inventoryLayout.Left;
	INVENTORY_TOP = inventoryLayout.Top;
	STASH_LEFT = lodStashLayout.Left;
	LOD_STASH_TOP = lodStashLayout.Top;
	CLASSIC_STASH_TOP = classicStashLayout.Top;
	CUBE_LEFT = cubeLayout.Left;
	CUBE_TOP = cubeLayout.Top;

	CELL_SIZE = inventoryLayout.SlotPixelHeight;

	INVENTORY_WIDTH = inventoryLayout.SlotWidth;
	INVENTORY_HEIGHT = inventoryLayout.SlotHeight;
	STASH_WIDTH = lodStashLayout.SlotWidth;
	LOD_STASH_HEIGHT = lodStashLayout.SlotHeight;
	CLASSIC_STASH_HEIGHT = classicStashLayout.SlotHeight;
	CUBE_WIDTH = cubeLayout.SlotWidth;
	CUBE_HEIGHT = cubeLayout.SlotHeight;

	if (!InventoryItems) {
		InventoryItems = new UnitAny * [INVENTORY_WIDTH * INVENTORY_HEIGHT];
	}
	if (!StashItems) {
		StashItems = new UnitAny * [STASH_WIDTH * LOD_STASH_HEIGHT];
	}
	if (!CubeItems) {
		CubeItems = new UnitAny * [CUBE_WIDTH * CUBE_HEIGHT];
	}

	//PrintText(1, "Got positions: %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
	//	INVENTORY_WIDTH,
	//	INVENTORY_HEIGHT,
	//	STASH_WIDTH,
	//	LOD_STASH_HEIGHT,
	//	CLASSIC_STASH_HEIGHT,
	//	CUBE_WIDTH,
	//	CUBE_HEIGHT,
	//	INVENTORY_LEFT,
	//	INVENTORY_TOP,
	//	STASH_LEFT,
	//	LOD_STASH_TOP,
	//	CLASSIC_STASH_TOP,
	//	CUBE_LEFT,
	//	CUBE_TOP,
	//	CELL_SIZE
	//);
}

bool ItemMover::LoadInventory(UnitAny* unit, int xpac, int source, int sourceX, int sourceY, bool shiftState, bool ctrlState, int stashUI, int invUI) {
	bool returnValue = false;
	if (!InventoryItems || !StashItems || !CubeItems)
	{
		return false;
	}

	memset(InventoryItems, 0, INVENTORY_WIDTH * INVENTORY_HEIGHT * sizeof(int));
	memset(StashItems, 0, STASH_WIDTH * LOD_STASH_HEIGHT * sizeof(int));
	memset(CubeItems, 0, CUBE_WIDTH * CUBE_HEIGHT * sizeof(int));

	if (!xpac) {
		for (int x = 0; x < STASH_WIDTH; x++) {
			for (int y = CLASSIC_STASH_HEIGHT; y < LOD_STASH_HEIGHT; y++) {
				StashItems[y * STASH_WIDTH + x] = (UnitAny*)NULL;
			}
		}
	}

	unsigned int itemId = 0;
	BYTE itemXSize, itemYSize;
	bool cubeInInventory = false, cubeAnywhere = false;
	UnitAny* pTargetItem = NULL;
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		UnitAny** p;
		int width;
		if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY) {
			p = InventoryItems;
			width = INVENTORY_WIDTH;
		}
		else if (pItem->pItemData->ItemLocation == STORAGE_STASH) {
			p = StashItems;
			width = STASH_WIDTH;
		}
		else if (pItem->pItemData->ItemLocation == STORAGE_CUBE) {
			p = CubeItems;
			width = CUBE_WIDTH;
		}
		else {
			continue;
		}

		bool box = false;
		char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
		if (code[0] == 'b' && code[1] == 'o' && code[2] == 'x') {
			if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY) {
				cubeInInventory = true;
				cubeAnywhere = true;
			}
			if (pItem->pItemData->ItemLocation == STORAGE_STASH) {
				cubeAnywhere = true;
			}
			box = true;
		}

		int xStart = pItem->pObjectPath->dwPosX;
		int yStart = pItem->pObjectPath->dwPosY;
		ItemsTxt* pItemText = D2COMMON_GetItemText(pItem->dwTxtFileNo);
		BYTE xSize = pItemText->binvwidth;
		BYTE ySize = pItemText->binvheight;
		for (int x = xStart; x < xStart + xSize; x++) {
			for (int y = yStart; y < yStart + ySize; y++) {
				p[y * width + x] = pItem;

				// If you click to move the cube into itself, your character ends up in
				// the amusing (and apparently permanent) state where he has no visible
				// cube and yet is unable to pick one up. Logging out does not fix it.
				// So we disable all cube movements to be on the safe side.
				if (x == sourceX && y == sourceY && pItem->pItemData->ItemLocation == source && !box) {
					// This is the item we want to move
					itemId = pItem->dwUnitId;
					pTargetItem = pItem;
					itemXSize = xSize;
					itemYSize = ySize;
				}
			}
		}
	}

	int destination;
	if (ctrlState && shiftState && ((stashUI && cubeAnywhere) || (invUI && cubeInInventory)) && source != STORAGE_CUBE) {
		destination = STORAGE_CUBE;
	}
	else if (ctrlState) {
		destination = STORAGE_NULL;  // i.e. the ground
	}
	else if (source == STORAGE_STASH || source == STORAGE_CUBE) {
		destination = STORAGE_INVENTORY;
	}
	else if (source == STORAGE_INVENTORY && D2CLIENT_GetUIState(UI_STASH)) {
		destination = STORAGE_STASH;
	}
	else if (source == STORAGE_INVENTORY && D2CLIENT_GetUIState(UI_CUBE)) {
		destination = STORAGE_CUBE;
	}
	else {
		return false;
	}

	// Find a spot for the item in the destination container
	if (itemId > 0) {
		returnValue = FindDestination(unit, pTargetItem, xpac, destination, itemId, itemXSize, itemYSize);
	}

	FirstInit = true;
	return returnValue;
}

bool ItemMover::FindDestination(UnitAny* unit, UnitAny* pItem, int xpac, int destination, unsigned int itemId, BYTE xSize, BYTE ySize) {
	UnitAny** p;
	int width = 0, height = 0;
	if (destination == STORAGE_INVENTORY) {
		p = InventoryItems;
		width = INVENTORY_WIDTH;
		height = INVENTORY_HEIGHT;
	}
	else if (destination == STORAGE_STASH) {
		p = StashItems;
		width = STASH_WIDTH;
		height = xpac ? LOD_STASH_HEIGHT : CLASSIC_STASH_HEIGHT;
	}
	else if (destination == STORAGE_CUBE) {
		p = CubeItems;
		width = CUBE_WIDTH;
		height = CUBE_HEIGHT;
	}

	bool found = false;
	int destX = 0, destY = 0, invItemId = 0;
	if (width) {
		bool first_y = true;
		// test once for stacked items
		ItemsTxt* pItemsTxt = D2COMMON_GetItemText(pItem->dwTxtFileNo);
		if (pItem->pItemData && pItem->pItemData->dwQuality == ITEM_QUALITY_NORMAL && destination != STORAGE_NULL && pItemsTxt->bstackable)
		{
			int nQuantity = D2COMMON_GetUnitStat(pItem, STAT_AMMOQUANTITY, 0);
			for (UnitAny* pInvItem = unit->pInventory->pFirstItem; pInvItem; pInvItem = pInvItem->pItemData->pNextInvItem) {
				if (pInvItem->dwTxtFileNo == pItem->dwTxtFileNo && pInvItem->pItemData && pInvItem->pItemData->dwQuality == ITEM_QUALITY_NORMAL)
				{
					if (pInvItem->pItemData->ItemLocation == destination)
					{
						int nTargetQuantity = D2COMMON_GetUnitStat(pInvItem, STAT_AMMOQUANTITY, 0);
						if (nQuantity + nTargetQuantity <= pItemsTxt->dwmaxstack)
						{
							destX = -1;
							destY = -1;
							invItemId = pInvItem->dwUnitId;
							found = true;
							break;
						}
						else if (nTargetQuantity != pItemsTxt->dwmaxstack)
						{
							invItemId = pInvItem->dwUnitId;
							break;
						}
					}
				}
			}
		}

		if (!found)
		{
			for (int x = 0; x < width; x++) {
				for (int y = 0; y < height; y++) {
					bool abort = false;
					int vacancies = 0;
					for (int testx = x; testx < x + xSize && testx < width; testx++) {
						for (int testy = y; testy < y + ySize && testy < height; testy++) {
							if (p[testy * width + testx]) {
								abort = true;
								break;
							}
							else {
								vacancies++;
							}
						}
						if (abort) {
							break;
						}
					}
					if (vacancies == xSize * ySize) {
						// Found an empty spot that's big enough for the item
						found = true;
						destX = x;
						destY = y;
						break;
					}
					if (xSize == 1) {
						if (first_y) {
							if (x + 1 < width) {
								x++;
								y--;
								first_y = false;
							}
						}
						else {
							first_y = true;
							x--;
						}
					}
				} // end y loop
				if (found) {
					break;
				}
				if (xSize == 2 && x % 2 == 0 && x + 2 >= width) {
					x = 0;
				}
				else {
					x++;
				}
			} // end x loop
		}
	}
	else {
		found = true;
	}

	if (found) {
		Lock();
		if (ActivePacket.startTicks == 0) {
			ActivePacket.itemId = itemId;
			ActivePacket.x = destX;
			ActivePacket.y = destY;
			ActivePacket.startTicks = BHGetTickCount();
			ActivePacket.destination = destination;
			ActivePacket.itemTargetId = invItemId;
		}
		Unlock();
	}

	return found;
}

void ItemMover::StackItem() {
	BYTE PacketData[9] = { 0x21,0,0,0,0 };
	*reinterpret_cast<int*>(PacketData + 1) = ActivePacket.itemId;
	*reinterpret_cast<int*>(PacketData + 5) = ActivePacket.itemTargetId;
	D2NET_SendPacket(9, 1, PacketData);
}

void ItemMover::PickUpItem() {
	BYTE PacketData[5] = { 0x19,0,0,0,0 };
	*reinterpret_cast<int*>(PacketData + 1) = ActivePacket.itemId;
	D2NET_SendPacket(5, 1, PacketData);
}

void ItemMover::PutItemInContainer() {
	BYTE PacketData[17] = { 0x18,0,0,0,0 };
	*reinterpret_cast<int*>(PacketData + 1) = ActivePacket.itemId;
	*reinterpret_cast<int*>(PacketData + 5) = ActivePacket.x;
	*reinterpret_cast<int*>(PacketData + 9) = ActivePacket.y;
	*reinterpret_cast<int*>(PacketData + 13) = ActivePacket.destination;
	D2NET_SendPacket(17, 1, PacketData);
}

void ItemMover::PutItemOnGround() {
	BYTE PacketData[5] = { 0x17,0,0,0,0 };
	*reinterpret_cast<int*>(PacketData + 1) = ActivePacket.itemId;
	D2NET_SendPacket(5, 1, PacketData);
}

void ItemMover::OnLeftClick(bool up, int x, int y, bool* block) {
	BnetData* pData = (*p_D2LAUNCH_BnData);
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	bool shiftState = ((GetKeyState(VK_LSHIFT) & 0x80) || (GetKeyState(VK_RSHIFT) & 0x80));

	if (up || !pData || !unit || !shiftState || D2CLIENT_GetCursorItem() > 0 || (!D2CLIENT_GetUIState(UI_INVENTORY) && !D2CLIENT_GetUIState(UI_STASH) && !D2CLIENT_GetUIState(UI_CUBE) && !D2CLIENT_GetUIState(UI_NPCSHOP))) {
		return;
	}

	Init();

	unidItemId = 0;
	idBookId = 0;

	int xpac = pData->nCharFlags & PLAYER_TYPE_EXPANSION;

	int mouseX, mouseY;

	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
		if ((pItem->pItemData->dwFlags & ITEM_IDENTIFIED) <= 0) {
			int xStart = pItem->pObjectPath->dwPosX;
			int yStart = pItem->pObjectPath->dwPosY;
			BYTE xSize = D2COMMON_GetItemText(pItem->dwTxtFileNo)->binvwidth;
			BYTE ySize = D2COMMON_GetItemText(pItem->dwTxtFileNo)->binvheight;
			if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY) {
				mouseX = (*p_D2CLIENT_MouseX - INVENTORY_LEFT) / CELL_SIZE;
				mouseY = (*p_D2CLIENT_MouseY - INVENTORY_TOP) / CELL_SIZE;
			}
			else if (pItem->pItemData->ItemLocation == STORAGE_STASH) {
				mouseX = (*p_D2CLIENT_MouseX - STASH_LEFT) / CELL_SIZE;
				if (xpac) {
					mouseY = (*p_D2CLIENT_MouseY - LOD_STASH_TOP) / CELL_SIZE;
				}
				else {
					mouseY = (*p_D2CLIENT_MouseY - CLASSIC_STASH_TOP) / CELL_SIZE;
				}
			}
			else if (pItem->pItemData->ItemLocation == STORAGE_CUBE) {
				mouseX = (*p_D2CLIENT_MouseX - CUBE_LEFT) / CELL_SIZE;
				mouseY = (*p_D2CLIENT_MouseY - CUBE_TOP) / CELL_SIZE;
			}
			for (int x = xStart; x < xStart + xSize; x++) {
				for (int y = yStart; y < yStart + ySize; y++) {
					if (x == mouseX && y == mouseY) {
						if ((pItem->pItemData->ItemLocation == STORAGE_STASH && !D2CLIENT_GetUIState(UI_STASH)) || (pItem->pItemData->ItemLocation == STORAGE_CUBE && !D2CLIENT_GetUIState(UI_CUBE))) {
							return;
						}
						unidItemId = pItem->dwUnitId;
					}
				}
			}
		}

		BOOL bIsIdBook = code[0] == 'i' && code[1] == 'b' && code[2] == 'k';
		BOOL bIsUnlimitedIdBook = code[0] == 'r' && code[1] == 'i' && code[2] == 'd';
		BOOL bItemIsInInventory = pItem->pItemData->ItemLocation == STORAGE_INVENTORY;
		if (bItemIsInInventory && (bIsUnlimitedIdBook || (bIsIdBook && D2COMMON_GetUnitStat(pItem, STAT_AMMOQUANTITY, 0) > 0))) {
			idBookId = pItem->dwUnitId;
		}
		if (unidItemId > 0 && idBookId > 0) {
			BYTE PacketData[13] = { 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			*reinterpret_cast<int*>(PacketData + 1) = idBookId;
			*reinterpret_cast<WORD*>(PacketData + 5) = (WORD)unit->pPath->xPos;
			*reinterpret_cast<WORD*>(PacketData + 9) = (WORD)unit->pPath->yPos;
			D2NET_SendPacket(13, 0, PacketData);
			*block = true;
			return;
		}
	}
}

void ItemMover::OnRightClick(bool up, int x, int y, bool* block) {
	BnetData* pData = (*p_D2LAUNCH_BnData);
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	bool shiftState = ((GetKeyState(VK_LSHIFT) & 0x80) || (GetKeyState(VK_RSHIFT) & 0x80));
	bool ctrlState = ((GetKeyState(VK_LCONTROL) & 0x80) || (GetKeyState(VK_RCONTROL) & 0x80));
	if (up || !pData || !unit || !(shiftState || ctrlState)) {
		return;
	}

	Init();

	int xpac = pData->nCharFlags & PLAYER_TYPE_EXPANSION;

	int inventoryRight = INVENTORY_LEFT + (CELL_SIZE * INVENTORY_WIDTH);
	int inventoryBottom = INVENTORY_TOP + (CELL_SIZE * INVENTORY_HEIGHT);
	int stashRight = STASH_LEFT + (CELL_SIZE * STASH_WIDTH);
	int stashTop = xpac ? LOD_STASH_TOP : CLASSIC_STASH_TOP;
	int stashHeight = xpac ? LOD_STASH_HEIGHT : CLASSIC_STASH_HEIGHT;
	int stashBottom = stashTop + (CELL_SIZE * stashHeight);
	int cubeRight = CUBE_LEFT + (CELL_SIZE * CUBE_WIDTH);
	int cubeBottom = CUBE_TOP + (CELL_SIZE * CUBE_HEIGHT);

	int source, sourceX, sourceY;
	int invUI = D2CLIENT_GetUIState(UI_INVENTORY);
	int stashUI = D2CLIENT_GetUIState(UI_STASH);
	int cubeUI = D2CLIENT_GetUIState(UI_CUBE);
	if ((invUI || stashUI || cubeUI) && x >= INVENTORY_LEFT && x <= inventoryRight && y >= INVENTORY_TOP && y <= inventoryBottom) {
		source = STORAGE_INVENTORY;
		sourceX = (x - INVENTORY_LEFT) / CELL_SIZE;
		sourceY = (y - INVENTORY_TOP) / CELL_SIZE;
	}
	else if (stashUI && x >= STASH_LEFT && x <= stashRight && y >= stashTop && y <= stashBottom) {
		source = STORAGE_STASH;
		sourceX = (x - STASH_LEFT) / CELL_SIZE;
		sourceY = (y - stashTop) / CELL_SIZE;
	}
	else if (cubeUI && x >= CUBE_LEFT && x <= cubeRight && y >= CUBE_TOP && y <= cubeBottom) {
		source = STORAGE_CUBE;
		sourceX = (x - CUBE_LEFT) / CELL_SIZE;
		sourceY = (y - CUBE_TOP) / CELL_SIZE;
	}
	else {
		return;
	}

	bool moveItem = LoadInventory(unit, xpac, source, sourceX, sourceY, shiftState, ctrlState, stashUI, invUI);
	if (moveItem) {
		PickUpItem();
	}
	*block = true;
}

void ItemMover::LoadConfig() {
}

void ItemMover::OnLoad() {
	LoadConfig();
}

void ItemMover::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return;

	/*if (!up && (key == HealKey || key == ManaKey)) {
		char firstChar = key == HealKey ? 'h' : 'm';
		char minPotion = 127;
		DWORD minItemId = 0;
		bool isBelt = false;
		for (UnitAny *pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
			if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY ||
				pItem->pItemData->ItemLocation == STORAGE_NULL && pItem->pItemData->NodePage == NODEPAGE_BELTSLOTS) {
				char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
				if (code[0] == firstChar && code[1] == 'p' && code[2] < minPotion) {
					minPotion = code[2];
					minItemId = pItem->dwUnitId;
					isBelt = pItem->pItemData->NodePage == NODEPAGE_BELTSLOTS;
				}
			}
			//char *code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
			//if (code[0] == 'b' && code[1] == 'o' && code[2] == 'x') {
			//	// Hack to pick up cube to fix cube-in-cube problem
			//	BYTE PacketDataCube[5] = {0x19,0,0,0,0};
			//	*reinterpret_cast<int*>(PacketDataCube + 1) = pItem->dwUnitId;
			//	D2NET_SendPacket(5, 1, PacketDataCube);
			//	break;
			//}
		}
		if (minItemId > 0) {
			if (isBelt){
				BYTE PacketData[13] = { 0x26, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = minItemId;
				D2NET_SendPacket(13, 0, PacketData);
			}
			else{
				//PrintText(1, "Sending packet %d, %d, %d", minItemId, unit->pPath->xPos, unit->pPath->yPos);
				BYTE PacketData[13] = { 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
				*reinterpret_cast<int*>(PacketData + 1) = minItemId;
				*reinterpret_cast<WORD*>(PacketData + 5) = (WORD)unit->pPath->xPos;
				*reinterpret_cast<WORD*>(PacketData + 9) = (WORD)unit->pPath->yPos;
				D2NET_SendPacket(13, 0, PacketData);
			}
			*block = true;
		}
	}*/
	/*if (!up && (key == TpKey)) {
		DWORD tpId = 0;
		int tp_quantity = 0;
		for (UnitAny *pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
			if (pItem->pItemData->ItemLocation == STORAGE_INVENTORY) {
				char* code = D2COMMON_GetItemText(pItem->dwTxtFileNo)->szCode;
				if (code[0] == 't' && code[1] == 'b' && code[2] =='k') {
					tp_quantity = D2COMMON_GetUnitStat(pItem, STAT_AMMOQUANTITY, 0);
					if (tp_quantity > 0) {
						tpId = pItem->dwUnitId;
						break;
					}
				}
			}
		}
		if (tpId > 0) {
			BYTE PacketData[13] = { 0x20, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
			*reinterpret_cast<int*>(PacketData + 1) = tpId;
			*reinterpret_cast<WORD*>(PacketData + 5) = (WORD)unit->pPath->xPos;
			*reinterpret_cast<WORD*>(PacketData + 9) = (WORD)unit->pPath->yPos;
			if (tp_quantity < tp_warn_quantity) {
				PrintText(Red, "TP tome is running low!");
			}
			D2NET_SendPacket(13, 0, PacketData);
			*block = true;
		}
	}*/
}

void ItemMover::OnGamePacketRecv(BYTE* packet, bool* block) {
	switch (packet[0])
	{
	case 0x3F:
	{
		// We get this packet after our cursor change. Will only ID if we found book and item previously. packet[1] = 0 guarantees the cursor is changing to "id ready" state.
		if (packet[1] == 0 && idBookId > 0 && unidItemId > 0) {
			BYTE PacketData[9] = { 0x27,0,0,0,0,0,0,0,0 };
			*reinterpret_cast<int*>(PacketData + 1) = unidItemId;
			*reinterpret_cast<int*>(PacketData + 5) = idBookId;
			D2NET_SendPacket(9, 0, PacketData);
			*block = true;
			// Reseting variables after we ID an item so the next ID works.
			unidItemId = 0;
			idBookId = 0;
		}
		break;
	}
	case 0x42:
	{
		// Clear Cursor
		Lock();
		ActivePacket.itemId = 0;
		ActivePacket.itemTargetId = 0;
		ActivePacket.x = 0;
		ActivePacket.y = 0;
		ActivePacket.startTicks = 0;
		ActivePacket.destination = 0;
		Unlock();
		break;
	}
	case 0x9c:
	{
		// We get this packet after placing an item in a container or on the ground
		if (FirstInit) {
			BYTE action = packet[1];
			unsigned int itemId = *(unsigned int*)&packet[4];
			Lock();
			if (itemId == ActivePacket.itemId) {
				//PrintText(1, "Placed item id %d", itemId);
				ActivePacket.itemId = 0;
				ActivePacket.itemTargetId = 0;
				ActivePacket.x = 0;
				ActivePacket.y = 0;
				ActivePacket.startTicks = 0;
				ActivePacket.destination = 0;
			}
			Unlock();
		}
		break;
	}
	case 0x9d:
	{
		// We get this packet after picking up an item
		if (FirstInit) {
			BYTE action = packet[1];
			unsigned int itemId = *(unsigned int*)&packet[4];
			Lock();
			if (itemId == ActivePacket.itemId) {
				//PrintText(2, "Picked up item id %d", itemId);
				if (ActivePacket.itemTargetId != 0) {
					StackItem();
					if (ActivePacket.x != -1 && ActivePacket.y != -1) {
						PutItemInContainer();
					}
					ActivePacket.itemId = 0;
					ActivePacket.itemTargetId = 0;
					ActivePacket.x = 0;
					ActivePacket.y = 0;
					ActivePacket.startTicks = 0;
					ActivePacket.destination = 0;
				}
				else if (ActivePacket.destination == STORAGE_NULL) {
					PutItemOnGround();
				}
				else {
					PutItemInContainer();
				}
			}
			Unlock();
		}
		break;
	}
	default:
		break;
	}
	return;
}

void ItemMover::OnGameExit() {
	ActivePacket.itemId = 0;
	ActivePacket.itemTargetId = 0;
	ActivePacket.x = 0;
	ActivePacket.y = 0;
	ActivePacket.startTicks = 0;
	ActivePacket.destination = 0;
}

