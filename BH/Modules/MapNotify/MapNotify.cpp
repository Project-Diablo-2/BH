#include "../../D2Ptrs.h"
#include "../../D2Helpers.h"
#include "../../D2Stubs.h"
#include "../../D2Intercepts.h"
#include "MapNotify.h"
#include "../../BH.h"
#include "../../Drawing.h"
#include "../Item/ItemDisplay.h"
#include "../../AsyncDrawBuffer.h"
#include "../Item/Item.h"
#include "../../Modules/GameSettings/GameSettings.h"

#pragma optimize( "", off)

using namespace Drawing;

DrawDirective automapDraw(true, 5);

MapNotify::MapNotify() : Module("MapNotify") {
	ReadConfig();
}

void MapNotify::LoadConfig() {
	ReadConfig();
}

void MapNotify::ReadConfig() {
}

void MapNotify::OnLoad() {
	ReadConfig();
}

void MapNotify::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	//bool ctrlState = ((GetKeyState(VK_LCONTROL) & 0x80) || (GetKeyState(VK_RCONTROL) & 0x80));
	//if (key == App.general.reloadConfigCtrl.hotkey && ctrlState || key == App.general.reloadConfig.hotkey && !ctrlState) {
	//	*block = true;
	//	if (up)
	//		BH::ReloadConfig();
	//	return;
	//}
	return;
}

void MapNotify::OnUnload() {
}

void MapNotify::OnLoop() {
}

Act* lastAct = NULL;

void MapNotify::OnDraw() {
	UnitAny* player = D2CLIENT_GetPlayerUnit();

	if (!player || !player->pAct || player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == 0)
		return;
	// We're looping over all items and setting 2 flags:
	// UNITFLAG_NO_EXPERIENCE - Whether the item has been checked for a drop notification (to prevent checking it again)
	// UNITFLAG_REVEALED      - Whether the item should be notified and drawn on the automap
	// To my knowledge these flags arent typically used on items. So we can abuse them for our own use.
	for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext) {
		for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext) {
			if (unit->dwType == UNIT_ITEM && (unit->dwFlags & UNITFLAG_NO_EXPERIENCE) == 0x0) {
				DWORD dwFlags = unit->pItemData->dwFlags;
				char* code = D2COMMON_GetItemText(unit->dwTxtFileNo)->szCode;
				UnitItemInfo uInfo;
				uInfo.item = unit;
				uInfo.itemCode[0] = code[0];
				uInfo.itemCode[1] = code[1] != ' ' ? code[1] : 0;
				uInfo.itemCode[2] = code[2] != ' ' ? code[2] : 0;
				uInfo.itemCode[3] = code[3] != ' ' ? code[3] : 0;
				uInfo.itemCode[4] = 0;
				if (ItemAttributeMap.find(uInfo.itemCode) != ItemAttributeMap.end()) {
					uInfo.attrs = ItemAttributeMap[uInfo.itemCode];
					for (vector<Rule*>::iterator it = MapRuleList.begin(); it != MapRuleList.end(); it++) {
						int filterLevel = Item::GetFilterLevel();
						if (filterLevel != 0 && (*it)->action.pingLevel < filterLevel && (*it)->action.pingLevel != -1) continue;

						if ((*it)->Evaluate(&uInfo))
						{
							if ((unit->dwFlags & UNITFLAG_REVEALED) == 0x0 &&
								App.lootfilter.enableFilter.value && App.lootfilter.detailedNotifications.value != 0 &&
								(App.lootfilter.detailedNotifications.value == 1 || (dwFlags & ITEM_NEW)))
							{
								SoundsTxt* pSoundsTxt = *p_D2CLIENT_SoundsTxt;
								int soundID = (*it)->action.soundID;
								if (App.pd2.pd2PlaySoundImpl && App.lootfilter.dropSounds.value && pSoundsTxt && soundID > 0 && soundID < *p_D2CLIENT_SoundRecords)
								{
									pSoundsTxt = pSoundsTxt + soundID;
									if (pSoundsTxt && pSoundsTxt->loop == 0 && pSoundsTxt->nSoundGroup != 2)
									{
										App.pd2.pd2PlaySoundImpl(NULL, soundID, pSoundsTxt->volume, pSoundsTxt->priority, TRUE);
									}
								}

								std::string itemName = GetItemName(unit);
								regex trimName("^\\s*(?:(?:\\s*?)(ÿc[0123456789;:]))*\\s*(.*?\\S)\\s*(?:ÿc[0123456789;:])*\\s*$");	// Matches on leading/trailing spaces (skips most color codes)
								itemName = regex_replace(itemName, trimName, "$1$2");												// Trims the matched spaces from notifications
								size_t start_pos = 0;
								while ((start_pos = itemName.find('\n', start_pos)) != std::string::npos) {
									itemName.replace(start_pos, 1, " - ");
									start_pos += 3;
								}
								PrintText(ItemColorFromQuality(unit->pItemData->dwQuality), "%s", itemName.c_str());
							}
							unit->dwFlags |= UNITFLAG_REVEALED;
							break;
						}
					}
				}
			}
			unit->dwFlags |= UNITFLAG_NO_EXPERIENCE;
		}
	}
}

void MapNotify::OnAutomapDraw() {
	UnitAny* player = D2CLIENT_GetPlayerUnit();

	if (!player || !player->pAct || player->pPath->pRoom1->pRoom2->pLevel->dwLevelNo == 0)
		return;

	if (lastAct != player->pAct) {
		lastAct = player->pAct;
		automapDraw.forceUpdate();
	}

	automapDraw.draw([=](AsyncDrawBuffer& automapBuffer) -> void {
		POINT MyPos;
		Drawing::Hook::ScreenToAutomap(&MyPos,
			D2CLIENT_GetUnitX(D2CLIENT_GetPlayerUnit()),
			D2CLIENT_GetUnitY(D2CLIENT_GetPlayerUnit()));
		for (Room1* room1 = player->pAct->pRoom1; room1; room1 = room1->pRoomNext) {
			for (UnitAny* unit = room1->pUnitFirst; unit; unit = unit->pListNext) {
				DWORD xPos, yPos;
				if (unit->dwType == UNIT_ITEM && (unit->dwFlags & UNITFLAG_REVEALED) == UNITFLAG_REVEALED) {
					char* code = D2COMMON_GetItemText(unit->dwTxtFileNo)->szCode;
					UnitItemInfo uInfo;
					uInfo.item = unit;
					uInfo.itemCode[0] = code[0];
					uInfo.itemCode[1] = code[1] != ' ' ? code[1] : 0;
					uInfo.itemCode[2] = code[2] != ' ' ? code[2] : 0;
					uInfo.itemCode[3] = code[3] != ' ' ? code[3] : 0;
					uInfo.itemCode[4] = 0;
					if (ItemAttributeMap.find(uInfo.itemCode) != ItemAttributeMap.end()) {
						uInfo.attrs = ItemAttributeMap[uInfo.itemCode];
						const vector<Action> actions = map_action_cache.Get(&uInfo);
						for (auto& action : actions) {
							auto color = action.colorOnMap;
							auto borderColor = action.borderColor;
							auto dotColor = action.dotColor;
							auto pxColor = action.pxColor;
							auto lineColor = action.lineColor;
							xPos = unit->pItemPath->dwPosX;
							yPos = unit->pItemPath->dwPosY;
							automapBuffer.push_top_layer(
								[color, unit, xPos, yPos, MyPos, borderColor, dotColor, pxColor, lineColor]()->void {
									POINT automapLoc;
									Drawing::Hook::ScreenToAutomap(&automapLoc, xPos, yPos);
									if (borderColor != UNDEFINED_COLOR)
										Drawing::Boxhook::Draw(automapLoc.x - 4, automapLoc.y - 4, 8, 8, borderColor, Drawing::BTHighlight);
									if (color != UNDEFINED_COLOR)
										Drawing::Boxhook::Draw(automapLoc.x - 3, automapLoc.y - 3, 6, 6, color, Drawing::BTHighlight);
									if (dotColor != UNDEFINED_COLOR)
										Drawing::Boxhook::Draw(automapLoc.x - 2, automapLoc.y - 2, 4, 4, dotColor, Drawing::BTHighlight);
									if (pxColor != UNDEFINED_COLOR)
										Drawing::Boxhook::Draw(automapLoc.x - 1, automapLoc.y - 1, 2, 2, pxColor, Drawing::BTHighlight);
								});
							if (action.stopProcessing) break;
						}
					}
					else {
						HandleUnknownItemCode(uInfo.itemCode, "on map");
					}
				}
			}
		}
		});
}

void MapNotify::OnGameJoin() {
}

void Squelch(DWORD Id, BYTE button) {
	LPBYTE aPacket = new BYTE[7];	//create packet
	*(BYTE*)&aPacket[0] = 0x5d;
	*(BYTE*)&aPacket[1] = button;
	*(BYTE*)&aPacket[2] = 1;
	*(DWORD*)&aPacket[3] = Id;
	D2NET_SendPacket(7, 0, aPacket);

	delete[] aPacket;	//clearing up data

	return;
}

void MapNotify::OnGamePacketRecv(BYTE* packet, bool* block) {
}

#pragma optimize( "", on)
