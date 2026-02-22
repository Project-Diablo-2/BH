/**
*
* Item.h
* BH: Copyright 2011 (C) McGod
* SlashDiablo Maphack: Copyright (C) SlashDiablo Community
*
*  This file is part of SlashDiablo Maphack.
*
*  SlashDiablo Maphack is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as published
*  by the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
*   ==========================================================
*   D2Ex2
*   https://github.com/lolet/D2Ex2
*   ==========================================================
*   Copyright (c) 2011-2014 Bartosz Jankowski
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*   ==========================================================
*
*/

#pragma once
#include "../Module.h"
#include "../../BH.h"
#include "../../Constants.h"
#include "../../Config.h"
#include "../../Drawing.h"
#include "../../D2Structs.h"
#include <set>

struct UnitAny;
struct UnitItemInfo;

class Item : public Module {
private:
	Drawing::UITab* settingsTab;
public:
	static UnitAny* viewingUnit;
	vector<string> ItemFilterNames;

	Item() : Module("Item") {};

	void OnLoad();
	void OnUnload();
	void ReplaceItemFilters(vector<string> itemFilterNames);
	void ChangeFilterLevels(int newLevel);

	void LoadConfig();
	void DrawSettings();

	void OnGameJoin();

	void OnLoop();
	void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);
	void OnLeftClick(bool up, int x, int y, bool* block);

	static void __fastcall ItemNamePatch(wchar_t* name, UnitAny* pItem, int nameSize);
	static void OrigGetItemName(UnitAny* item, string& itemName, char* code);
	static void __stdcall OnProperties(wchar_t* wTxt);
	static BOOL __stdcall OnDamagePropertyBuild(UnitAny* pItem, DamageStats* pDmgStats, int nStat, wchar_t* wOut);
	static void __stdcall OnPropertyBuild(wchar_t* wOut, int nStat, UnitAny* pItem, int nStatParam);

	static BOOL PermShowItemsPatch1();
	static BOOL PermShowItemsPatch2();
	static BOOL PermShowItemsPatch3();

	static void ProcessItemPacketFilterRules(UnitItemInfo* uInfo, px9c* pPacket);

	static UnitAny* GetViewUnit();
	static UnitAny* GetViewUnitAndDrawBox();

	static unsigned int GetFilterLevel() { return App.lootfilter.filterLevel.uValue; }
};

void ItemName_Interception();
void __fastcall GetProperties_Interception();
void GetItemPropertyStringDamage_Interception();
void GetItemPropertyString_Interception();
void ViewInventoryPatch1_ASM();
void ViewInventoryPatch2_ASM();
void ViewInventoryPatch3_ASM();
void PermShowItemsPatch1_ASM();
void PermShowItemsPatch2_ASM();
void PermShowItemsPatch3_ASM();
void PermShowItemsPatch4_ASM();

int CreateUnitItemInfo(UnitItemInfo* uInfo, UnitAny* item);
void __stdcall GetItemFromPacket_NewGround(px9c* packet);
void __stdcall GetItemFromPacket_DropToGround(px9c* packet);
void __stdcall GetItemFromPacket_OldGround(px9c* packet);
void __stdcall GetItemFromPacket_PutInContainer(px9c* pPacket);
int ItemGetCorruptor(UnitAny* pItem, int nStat);
BOOL StatIsCorrupted(int nStat, int nCorruptor);

// reset all rule lookup caches
void ResetCaches();

void GetCharStats();
void GetItemStats();
void GetItemTypeMaps();
std::string GetTxtItemCode(ItemsTxt* pItemsTxt);
void GetWeaponAttributes();
void GetArmorAttributes();
void GetMiscAttributes();
void GetTotalSkills();
void GetAffixOffsets();
bool IsInitialized();

// Item attributes from ItemTypes.txt and Weapon/Armor/Misc.txt
struct ItemAttributes {
	std::string name;
	WORD category;
	BYTE width;
	BYTE height;
	BYTE stackable;
	BYTE useable;
	BYTE throwable;
	unsigned int baseFlags;
	unsigned int weaponFlags;
	unsigned int armorFlags;
	unsigned int miscFlags;
	BYTE qualityLevel;
	BYTE magicLevel;
	BYTE staffmodClass;
};

// Properties from ItemStatCost.txt
struct StatProperties {
	std::string name;
	//std::string localizedName;
	ItemStatCostTxt* pItemStatCostTxt;
	unsigned short statId;
};

struct CharStats {
	int toHitFactor;
};

void FindAncestorTypes(WORD type, std::set<WORD>& ancestors, std::map<WORD, WORD>& map1, std::map<WORD, WORD>& map2);
unsigned int AssignClassFlags(WORD type, std::set<WORD>& ancestors, unsigned int flags);

extern unsigned int STAT_MAX;
extern unsigned int SKILL_MAX;
extern unsigned int PREFIX_OFFSET;
extern unsigned int AUTOMOD_OFFSET;

extern std::vector<StatProperties*> AllStatList;
extern std::vector<CharStats*> CharList;
extern std::map<std::string, ItemAttributes*> ItemAttributeMap;
