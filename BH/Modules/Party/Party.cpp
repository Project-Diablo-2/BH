#include "Party.h"
#include "../../D2Ptrs.h"
#include "../../BH.h"
#include "../../D2Stubs.h"
#include "../../D2Helpers.h"

using namespace std;
using namespace Drawing;
Drawing::Hook* PartyHook;
Drawing::Hook* LootHook;

void Party::OnLoad() {
	c = 0;

	PartyHook = new Drawing::Checkhook(Drawing::InGame, 100, 100, &App.party.autoParty.toggle.isEnabled, "Autoparty Enabled");
	PartyHook->SetActive(0);
	LootHook = new Drawing::Checkhook(Drawing::InGame, 240, 100, &App.party.autoCorpseLoot.toggle.isEnabled, "Autoloot Enabled");
	LootHook->SetActive(0);
}

void Party::OnGameJoin() {
}

void Party::OnUnload() {
	
}

void Party::OnLoop() {
	if (App.party.autoParty.toggle.isEnabled || App.party.autoCorpseLoot.toggle.isEnabled)
		CheckParty();
	if(D2CLIENT_GetUIState(0x16) && PartyHook->IsActive() == 0) {
		PartyHook->SetBaseX(*p_D2CLIENT_PanelOffsetX + 20);
		PartyHook->SetActive(1);
	}
	else if(D2CLIENT_GetUIState(0x16) == 0 && PartyHook->IsActive())
		PartyHook->SetActive(0);

	BnetData* pData = (*p_D2LAUNCH_BnData);
	if(!pData)
		return;
	if(D2CLIENT_GetUIState(0x16) && LootHook->IsActive() == 0 && pData->nCharFlags & PLAYER_TYPE_HARDCORE) {
		LootHook->SetBaseX(*p_D2CLIENT_PanelOffsetX + 160);
		LootHook->SetActive(1);
	}
	else if(D2CLIENT_GetUIState(0x16) == 0 && LootHook->IsActive())
		LootHook->SetActive(0);
}

void Party::CheckParty() {
	if(c == 0) {
		std::map<std::string, bool> CurrentParty;
		UnitAny* Me = *p_D2CLIENT_PlayerUnit;
		RosterUnit* MyRoster = FindPlayerRoster(Me->dwUnitId);
		BnetData* pData = (*p_D2LAUNCH_BnData);

		WORD current_min_party_id = 0xFFFF;

		// first pass: check that the data is sane
		RosterUnit *Party = *p_D2CLIENT_PlayerUnitList; if (!Party) return;
		do {
			if(!_stricmp(Party->szName, MyRoster->szName))
				continue;
			if(!Party->wLevel) {
				//PrintText(1, "!Party->wLevel");
				c++;
				return;
			}
			if ((Party->wPartyId == INVALID_PARTY_ID && Party->dwPartyFlags & PARTY_IN_PARTY) ||
				(Party->wPartyId != INVALID_PARTY_ID && Party->dwPartyFlags & PARTY_NOT_IN_PARTY)) {
				// Avoid crashing when multiple players in a game have auto-party enabled
				// (there seems to be a brief window in which the party data can be invalid)
				c++; return;
			}
		} while (Party = Party->pNext);
		
		// second pass: gather some info
		Party = *p_D2CLIENT_PlayerUnitList; if (!Party) return;
		do {
			if(!_stricmp(Party->szName, MyRoster->szName))
				continue;
			current_min_party_id = min(Party->wPartyId, current_min_party_id);
		} while (Party=Party->pNext);

		// third pass: do stuff
		Party = *p_D2CLIENT_PlayerUnitList; if (!Party) return;
		do {
			if(!_stricmp(Party->szName, MyRoster->szName))
				continue;
			if (pData && pData->nCharFlags & PLAYER_TYPE_HARDCORE) {
				CurrentParty[Party->szName] = true;
				if (App.party.autoCorpseLoot.toggle.isEnabled) {
					string s(Party->szName);
					if (LootingPermission.find(s) == LootingPermission.end()) {
						//PrintText(1, "Enabling loot for %s.", s.c_str());
						BYTE PacketData[7] = {0x5d,1,1,0,0,0,0};
						*reinterpret_cast<int*>(PacketData + 3) = Party->dwUnitId;
						D2NET_SendPacket(7, 1, PacketData);
						LootingPermission[s] = true;
					}
				}
			}
			if ((Party->wPartyId == INVALID_PARTY_ID || Party->wPartyId != MyRoster->wPartyId) && App.party.autoParty.toggle.isEnabled) {
				//PrintText(1, "Party->wPartyID=%hu, MyRoster->wPartyId=%hu, min_party_id=%hu", 
				//		Party->wPartyId, MyRoster->wPartyId, current_min_party_id);
				if(Party->dwPartyFlags & PARTY_INVITED_YOU) {
					if (current_min_party_id != INVALID_PARTY_ID) {
						if (Party->wPartyId == current_min_party_id) {
							//PrintText(1, "Found the right party");
							D2CLIENT_ClickParty(Party, 2);
							c++;
							return;
						}
					} else {
						//PrintText(1, "PARTY_INVITED_YOU, clicking party");
						D2CLIENT_ClickParty(Party, 2);
						c++;
						return;
					}
				}
				if(Party->wPartyId == INVALID_PARTY_ID) {
					//PrintText(1, "INVALID_PARTY_ID");
					if(Party->dwPartyFlags & PARTY_INVITED_BY_YOU) {
						//PrintText(1, "PARTY_INVITED_BY_YOU");
						continue;
					}
					if (current_min_party_id != INVALID_PARTY_ID) {
						if (MyRoster->wPartyId == current_min_party_id) {
							//PrintText(1, "I'm in the right party, inviting another.");
							D2CLIENT_ClickParty(Party, 2);
							c++;
							return;
						}
					} else {
						//PrintText(1, "There's no master party, trying to form one.");
						D2CLIENT_ClickParty(Party, 2);
						c++;
						return;
					}
				}
			}

		} while (Party = Party->pNext);
		// Leave the party if we're in the wrong one
		if (App.party.autoParty.toggle.isEnabled && current_min_party_id != INVALID_PARTY_ID
			&& MyRoster->wPartyId != current_min_party_id && MyRoster->wPartyId != INVALID_PARTY_ID) {
			//PrintText(1, "Not in the right party!");
			//PrintText(1, "min_party_id=%hu, MyRoster->wPartyId=%hu", current_min_party_id, MyRoster->wPartyId);
			D2CLIENT_LeaveParty();
			c++;
			return;
		}
		// Remove looting permissions for players no longer in the game
		for (auto it = LootingPermission.cbegin(); it != LootingPermission.cend(); ) {
			if (CurrentParty.find((*it).first) == CurrentParty.end()) {
				//PrintText(1, "Removing %s from looting map.", ((*it).first).c_str());
				LootingPermission.erase(it++);
			} else {
				++it;
			}
		}
	}
	c++;
	if(c>=25)
		c = 0;
}

void Party::OnKey(bool up, BYTE key, LPARAM lParam, bool* block)  {
	if(!D2CLIENT_GetPlayerUnit())
		return;

	if (D2CLIENT_GetUIState(UI_INVENTORY) || D2CLIENT_GetUIState(UI_STASH) || D2CLIENT_GetUIState(UI_CUBE)) {
		// Avoid toggling state when pressing number keys to drop/withdraw/deposit gold.
		// There is no UI state for the gold dialogs, so we have to disable whenever
		// stash/inventory are open.
		return;
	}
}

void Party::OnGameExit() {
	LootingPermission.clear();
}
