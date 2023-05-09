/**
 *
 * Item.cpp
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

#include "Item.h"
#include "../../D2Helpers.h"
#include "../../D2Ptrs.h"
#include "../../D2Strings.h"
#include "../../BH.h"
#include "../../D2Stubs.h"
#include "ItemDisplay.h"
#include "../../MPQInit.h"
#include "../../lrucache.hpp"
#include "../GameSettings/GameSettings.h"

ItemsTxtStat* GetAllStatModifier(ItemsTxtStat* pStats, int nStats, int nStat, ItemsTxtStat* pOrigin);
ItemsTxtStat* GetMaxElemStatFromMin(ItemsTxtStat* pStats, int nStats, int nStat, ItemsTxtStat* pOrigin);
ItemsTxtStat* GetItemsTxtStatByMod(ItemsTxtStat* pStats, int nStats, int nStat, int nStatParam);
RunesTxt* GetRunewordTxtById(int rwId);

void FixDecimalString(wchar_t* s, int n);

unsigned int STAT_MAX;
unsigned int SKILL_MAX;
unsigned int PREFIX_OFFSET;
unsigned int AUTOMOD_OFFSET;

std::map<WORD, BYTE> throwableMap;
std::map<WORD, BYTE> bodyLocMap;
std::map<WORD, WORD> parentMap1;
std::map<WORD, WORD> parentMap2;

std::vector<StatProperties*> AllStatList;
std::vector<CharStats*> CharList;
std::map<std::string, ItemAttributes*> ItemAttributeMap;

map<std::string, Toggle> Item::Toggles;
unsigned int Item::filterLevelSetting;
UnitAny* Item::viewingUnit;

Patch* itemNamePatch = new Patch(Call, D2CLIENT, { 0x92366, 0x96736 }, (int)ItemName_Interception, 6);
Patch* itemPropertiesPatch = new Patch(Jump, D2CLIENT, { 0x5612C, 0x2E3FC }, (int)GetProperties_Interception, 6);
Patch* itemPropertyStringDamagePatch = new Patch(Call, D2CLIENT, { 0x55D7B, 0x2E04B }, (int)GetItemPropertyStringDamage_Interception, 5);
Patch* itemPropertyStringPatch = new Patch(Call, D2CLIENT, { 0x55D9D, 0x2E06D }, (int)GetItemPropertyString_Interception, 5);
Patch* viewInvPatch1 = new Patch(Call, D2CLIENT, { 0x953E2, 0x997B2 }, (int)ViewInventoryPatch1_ASM, 6);
Patch* viewInvPatch2 = new Patch(Call, D2CLIENT, { 0x94AB4, 0x98E84 }, (int)ViewInventoryPatch2_ASM, 6);
Patch* viewInvPatch3 = new Patch(Call, D2CLIENT, { 0x93A6F, 0x97E3F }, (int)ViewInventoryPatch3_ASM, 5);

//ported to 1.13c/d from https://github.com/jieaido/d2hackmap/blob/master/PermShowItem.cpp
Patch* permShowItems1 = new Patch(Call, D2CLIENT, { 0xC3D4E, 0x1D74E }, (int)PermShowItemsPatch1_ASM, 6);
Patch* permShowItems2 = new Patch(Call, D2CLIENT, { 0xC0E9A, 0x1A89A }, (int)PermShowItemsPatch1_ASM, 6);
Patch* permShowItems3 = new Patch(Call, D2CLIENT, { 0x59483, 0x4EA13 }, (int)PermShowItemsPatch2_ASM, 6);
Patch* permShowItems4 = new Patch(Call, D2CLIENT, { 0x5908A, 0x4E61A }, (int)PermShowItemsPatch3_ASM, 6);
Patch* permShowItems5 = new Patch(Call, D2CLIENT, { 0xA6BA3, 0x63443 }, (int)PermShowItemsPatch4_ASM, 6);

// Don't play item drop sounds for new items. We'll play them later, if necessary
Patch* itemDropSoundIntercept1 = new Patch(NOP, D2CLIENT, { 0x84C38 }, 0, 5);

Patch* newGroundIntercept = new Patch(Call, D2CLIENT, { 0xAE0DA }, (int)D2CLIENT_GetItemFromPacketIntercept_NewGround_STUB, 5);
Patch* oldGroundIntercept = new Patch(Call, D2CLIENT, { 0xAE108 }, (int)GetItemFromPacket_OldGround, 5);

using namespace Drawing;

void Item::OnLoad() {
	LoadConfig();

	viewInvPatch1->Install();
	viewInvPatch2->Install();
	viewInvPatch3->Install();

	permShowItems1->Install();
	permShowItems2->Install();
	permShowItems3->Install();
	permShowItems4->Install();
	permShowItems5->Install();

	itemDropSoundIntercept1->Install();

	newGroundIntercept->Install();
	oldGroundIntercept->Install();

	itemPropertiesPatch->Install();
	itemPropertyStringDamagePatch->Install();
	itemPropertyStringPatch->Install();

	if (Toggles["Show Ethereal"].state || Toggles["Show Sockets"].state || Toggles["Show iLvl"].state || Toggles["Color Mod"].state ||
		Toggles["Show Rune Numbers"].state || Toggles["Alt Item Style"].state || Toggles["Shorten Item Names"].state || Toggles["Advanced Item Display"].state)
		itemNamePatch->Install();

	DrawSettings();
}

void ResetCaches() {
	item_desc_cache.ResetCache();
	item_name_cache.ResetCache();
	map_action_cache.ResetCache();
}

void GetCharStats()
{
	DWORD dwCharsStatsRecs = (*p_D2COMMON_sgptDataTable)->dwCharsStatsRecs;
	for (auto d = 0; d < dwCharsStatsRecs; d++)
	{
		CharStatsTxt* pCharStatsTxt = &(*p_D2COMMON_sgptDataTable)->pCharStatsTxt[d];
		if (pCharStatsTxt->dwToHitFactor > 0)
		{
			CharStats* bits = new CharStats();
			bits->toHitFactor = pCharStatsTxt->dwToHitFactor;
			CharList.push_back(bits);
		}
	}
}

void GetItemStats()
{
	STAT_MAX = (*p_D2COMMON_sgptDataTable)->dwItemStatCostRecs;
	for (auto id = 0; id < STAT_MAX; id++)
	{
		StatProperties* bits = new StatProperties();
		bits->pItemStatCostTxt = &(*p_D2COMMON_sgptDataTable)->pItemStatCostTxt[id];
		bits->statId = id;
		char* statString = UnicodeToAnsi(GetTblEntryByIndex(bits->pItemStatCostTxt->wDescStrPos, TBLOFFSET_STRING));
		bits->name = statString;
		AllStatList.push_back(bits);
	}
}

void GetItemTypeMaps()
{
	DWORD dwItemsTypeRecs = (*p_D2COMMON_sgptDataTable)->dwItemsTypeRecs;
	for (auto d = 0; d < dwItemsTypeRecs; d++)
	{
		D2ItemTypesTxt* pItemsTypeTxt = &(*p_D2COMMON_sgptDataTable)->pItemsTypeTxt[d];
		if (pItemsTypeTxt->szCode)
		{
			throwableMap[d] = pItemsTypeTxt->nThrowable;
			bodyLocMap[d] = pItemsTypeTxt->nBodyLoc1;
			if (pItemsTypeTxt->nEquiv1)
			{
				parentMap1[d] = pItemsTypeTxt->nEquiv1;
			}
			if (pItemsTypeTxt->nEquiv2)
			{
				parentMap2[d] = pItemsTypeTxt->nEquiv2;
			}
		}
	}
}

void FindAncestorTypes(WORD type, std::set<WORD>& ancestors, std::map<WORD, WORD>& map1, std::map<WORD, WORD>& map2)
{
	ancestors.insert(type);
	std::map<WORD, WORD>::iterator it1 = map1.find(type);
	if (it1 != map1.end())
	{
		FindAncestorTypes(it1->second, ancestors, map1, map2);
	}
	std::map<WORD, WORD>::iterator it2 = map2.find(type);
	if (it2 != map2.end())
	{
		FindAncestorTypes(it2->second, ancestors, map1, map2);
	}
}

unsigned int AssignClassFlags(WORD type, std::set<WORD>& ancestors, unsigned int flags) {
	if (ancestors.find(ITEM_TYPE_AMAZON) != ancestors.end())
	{
		flags |= ITEM_GROUP_AMAZON_WEAPON;
	}
	else if (ancestors.find(ITEM_TYPE_BARBARIAN) != ancestors.end())
	{
		flags |= ITEM_GROUP_BARBARIAN_HELM;
	}
	else if (ancestors.find(ITEM_TYPE_NECROMANCER) != ancestors.end())
	{
		flags |= ITEM_GROUP_NECROMANCER_SHIELD;
	}
	else if (ancestors.find(ITEM_TYPE_PALADIN) != ancestors.end())
	{
		flags |= ITEM_GROUP_PALADIN_SHIELD;
	}
	else if (ancestors.find(ITEM_TYPE_SORCERESS) != ancestors.end())
	{
		flags |= ITEM_GROUP_SORCERESS_ORB;
	}
	else if (ancestors.find(ITEM_TYPE_ASSASSIN) != ancestors.end())
	{
		flags |= ITEM_GROUP_ASSASSIN_KATAR;
	}
	else if (ancestors.find(ITEM_TYPE_DRUID) != ancestors.end())
	{
		flags |= ITEM_GROUP_DRUID_PELT;
	}
	return flags;
}

std::string GetTxtItemCode(ItemsTxt* pItemsTxt)
{
	char code[5] = {};
	code[0] = pItemsTxt->szCode[0];
	code[1] = pItemsTxt->szCode[1] != ' ' ? pItemsTxt->szCode[1] : 0;
	code[2] = pItemsTxt->szCode[2] != ' ' ? pItemsTxt->szCode[2] : 0;
	code[3] = pItemsTxt->szCode[3] != ' ' ? pItemsTxt->szCode[3] : 0;
	code[4] = 0;

	return code;
}

void GetWeaponAttributes()
{
	D2ItemDataTbl* pItemDataTables = D2COMMON_10535_DATATBLS_GetItemDataTables();

	for (auto d = 0; d < pItemDataTables->nWeaponsTxtRecordCount; d++)
	{
		ItemsTxt* pWeapons = &pItemDataTables->pWeapons[d];
		BYTE stackable = pWeapons->bstackable > 0 ? pWeapons->bstackable : 0;
		BYTE useable = pWeapons->buseable > 0 ? pWeapons->buseable : 0;
		BYTE throwable = throwableMap[pWeapons->nType] > 0 ? throwableMap[pWeapons->nType] : 0;
		unsigned int flags = ITEM_GROUP_ALLWEAPON, flags2 = 0;

		std::set<WORD> ancestorTypes;
		FindAncestorTypes(pWeapons->nType, ancestorTypes, parentMap1, parentMap2);

		if (pWeapons->dwcode == pWeapons->dwultracode)
		{
			flags |= ITEM_GROUP_ELITE;
		}
		else if (pWeapons->dwcode == pWeapons->dwubercode)
		{
			flags |= ITEM_GROUP_EXCEPTIONAL;
		}
		else
		{
			flags |= ITEM_GROUP_NORMAL;
		}

		if (ancestorTypes.find(ITEM_TYPE_CLUB) != ancestorTypes.end() ||
			ancestorTypes.find(ITEM_TYPE_HAMMER) != ancestorTypes.end() ||
			ancestorTypes.find(ITEM_TYPE_MACE) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_MACE;
		}
		else if (ancestorTypes.find(ITEM_TYPE_WAND) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_WAND;
		}
		else if (ancestorTypes.find(ITEM_TYPE_STAFF) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_STAFF;
		}
		else if (ancestorTypes.find(ITEM_TYPE_BOW) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_BOW;
		}
		else if (ancestorTypes.find(ITEM_TYPE_AXE) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_AXE;
		}
		else if (ancestorTypes.find(ITEM_TYPE_SCEPTER) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_SCEPTER;
		}
		else if (ancestorTypes.find(ITEM_TYPE_SWORD) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_SWORD;
		}
		else if (ancestorTypes.find(ITEM_TYPE_KNIFE) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_DAGGER;
		}
		else if (ancestorTypes.find(ITEM_TYPE_SPEAR) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_SPEAR;
		}
		else if (ancestorTypes.find(ITEM_TYPE_POLEARM) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_POLEARM;
		}
		else if (ancestorTypes.find(ITEM_TYPE_CROSSBOW) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_CROSSBOW;
		}
		else if (ancestorTypes.find(ITEM_TYPE_JAVELIN) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_JAVELIN;  // TODO: this isn't being used. Fixme
		}
		if (ancestorTypes.find(ITEM_TYPE_THROWN) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_THROWING;
		}

		ItemAttributes* attrs = new ItemAttributes();
		attrs->name = UnicodeToAnsi(GetTblEntryByIndex(pWeapons->wnamestr, TBLOFFSET_STRING));
		attrs->code[0] = 0;
		attrs->code[1] = 0;
		attrs->code[2] = 0;
		attrs->code[3] = 0;
		attrs->code[4] = 0;
		attrs->category = pWeapons->nType;
		attrs->width = 0;
		attrs->height = 0;
		attrs->stackable = stackable;
		attrs->useable = useable;
		attrs->throwable = throwable;
		attrs->itemLevel = 0;
		attrs->unusedFlags = 0;
		attrs->flags = flags;
		attrs->flags2 = flags2;
		attrs->qualityLevel = pWeapons->blevel;
		attrs->magicLevel = pWeapons->bmagiclvl;
		attrs->maxac = 0;
		attrs->cost = 0;
		ItemAttributeMap[GetTxtItemCode(pWeapons)] = attrs;
	}
}
void GetArmorAttributes()
{
	D2ItemDataTbl* pItemDataTables = D2COMMON_10535_DATATBLS_GetItemDataTables();

	for (auto d = 0; d < pItemDataTables->nArmorTxtRecordCount; d++)
	{
		ItemsTxt* pArmor = &pItemDataTables->pArmor[d];
		BYTE stackable = pArmor->bstackable > 0 ? pArmor->bstackable : 0;
		BYTE useable = pArmor->buseable > 0 ? pArmor->buseable : 0;
		BYTE throwable = throwableMap[pArmor->nType] > 0 ? throwableMap[pArmor->nType] : 0; // Hey, you never know
		unsigned int flags = ITEM_GROUP_ALLARMOR, flags2 = 0;

		std::set<WORD> ancestorTypes;
		FindAncestorTypes(pArmor->nType, ancestorTypes, parentMap1, parentMap2);

		if (pArmor->dwcode == pArmor->dwultracode)
		{
			flags |= ITEM_GROUP_ELITE;
		}
		else if (pArmor->dwcode == pArmor->dwubercode)
		{
			flags |= ITEM_GROUP_EXCEPTIONAL;
		}
		else
		{
			flags |= ITEM_GROUP_NORMAL;
		}

		if (ancestorTypes.find(ITEM_TYPE_CIRCLET) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_CIRCLET;  // TODO: This kinda seems like it should be separate
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_HEAD)
		{
			flags |= ITEM_GROUP_HELM;
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_BODY)
		{
			flags |= ITEM_GROUP_ARMOR;
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_GLOVES)
		{
			flags |= ITEM_GROUP_GLOVES;
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_FEET)
		{
			flags |= ITEM_GROUP_BOOTS;
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_BELT)
		{
			flags |= ITEM_GROUP_BELT;
		}
		else if (bodyLocMap[pArmor->nType] == EQUIP_RIGHT_PRIMARY && ancestorTypes.find(ITEM_TYPE_ALLSHIELD) != ancestorTypes.end())
		{
			flags |= ITEM_GROUP_SHIELD;
		}
		flags = AssignClassFlags(pArmor->nType, ancestorTypes, flags);

		ItemAttributes* attrs = new ItemAttributes();
		attrs->name = UnicodeToAnsi(GetTblEntryByIndex(pArmor->wnamestr, TBLOFFSET_STRING));
		attrs->code[0] = 0;
		attrs->code[1] = 0;
		attrs->code[2] = 0;
		attrs->code[3] = 0;
		attrs->code[4] = 0;
		attrs->category = pArmor->nType;
		attrs->width = 0;
		attrs->height = 0;
		attrs->stackable = stackable;
		attrs->useable = useable;
		attrs->throwable = throwable;
		attrs->itemLevel = 0;
		attrs->unusedFlags = 0;
		attrs->flags = flags;
		attrs->flags2 = flags2;
		attrs->qualityLevel = pArmor->blevel;
		attrs->magicLevel = pArmor->bmagiclvl;
		attrs->maxac = 0;
		attrs->cost = 0;
		ItemAttributeMap[GetTxtItemCode(pArmor)] = attrs;
	}
}
void GetMiscAttributes()
{
	D2ItemDataTbl* pItemDataTables = D2COMMON_10535_DATATBLS_GetItemDataTables();

	for (auto d = 0; d < pItemDataTables->nMiscTxtRecordCount; d++)
	{
		ItemsTxt* pMisc = &pItemDataTables->pMisc[d];
		BYTE stackable = pMisc->bstackable > 0 ? pMisc->bstackable : 0;
		BYTE useable = pMisc->buseable > 0 ? pMisc->buseable : 0;
		BYTE throwable = throwableMap[pMisc->nType] > 0 ? throwableMap[pMisc->nType] : 0;
		unsigned int flags = 0, flags2 = 0;

		std::set<WORD> ancestorTypes;
		FindAncestorTypes(pMisc->nType, ancestorTypes, parentMap1, parentMap2);
		FindAncestorTypes(pMisc->wtype2, ancestorTypes, parentMap1, parentMap2);

		if (ancestorTypes.find(ITEM_TYPE_RUNE) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_STACK_RUNE) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_RUNE;
		}

		// Gem Quality
		if (ancestorTypes.find(ITEM_TYPE_CHIPPED_GEM) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_CHIPPED;
		}
		else if (ancestorTypes.find(ITEM_TYPE_FLAWED_GEM) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_FLAWED;
		}
		else if (ancestorTypes.find(ITEM_TYPE_STANDARD_GEM) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_REGULAR;
		}
		else if (ancestorTypes.find(ITEM_TYPE_GEN_FLAWLESS_GEM) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_STACK_FLAWLESS) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_FLAWLESS;
		}
		else if (ancestorTypes.find(ITEM_TYPE_GEN_PERFECT_GEM) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_STACK_PERFECT) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_PERFECT;
		}

		// Gem Type
		if (ancestorTypes.find(ITEM_TYPE_AMETHYST) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_AMETHYST) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_AMETHYST;
		}
		else if (ancestorTypes.find(ITEM_TYPE_DIAMOND) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_DIAMOND) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_DIAMOND;
		}
		else if (ancestorTypes.find(ITEM_TYPE_EMERALD) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_EMERALD) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_EMERALD;
		}
		else if (ancestorTypes.find(ITEM_TYPE_RUBY) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_RUBY) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_RUBY;
		}
		else if (ancestorTypes.find(ITEM_TYPE_SAPPHIRE) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_SAPPHIRE) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_SAPPHIRE;
		}
		else if (ancestorTypes.find(ITEM_TYPE_TOPAZ) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_TOPAZ) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_TOPAZ;
		}
		else if (ancestorTypes.find(ITEM_TYPE_SKULL) != ancestorTypes.end() || ancestorTypes.find(ITEM_TYPE_GEN_SKULL) != ancestorTypes.end()) {
			flags2 |= ITEM_GROUP_SKULL;
		}

		ItemAttributes* attrs = new ItemAttributes();
		attrs->name = UnicodeToAnsi(GetTblEntryByIndex(pMisc->wnamestr, TBLOFFSET_STRING));
		attrs->code[0] = 0;
		attrs->code[1] = 0;
		attrs->code[2] = 0;
		attrs->code[3] = 0;
		attrs->code[4] = 0;
		attrs->category = pMisc->nType;
		attrs->width = 0;
		attrs->height = 0;
		attrs->stackable = stackable;
		attrs->useable = useable;
		attrs->throwable = throwable;
		attrs->itemLevel = 0;
		attrs->unusedFlags = 0;
		attrs->flags = flags;
		attrs->flags2 = flags2;
		attrs->qualityLevel = pMisc->blevel;
		attrs->magicLevel = 0;
		attrs->maxac = 0;
		attrs->cost = 0;
		ItemAttributeMap[GetTxtItemCode(pMisc)] = attrs;
	}
}

void GetTotalSkills()
{
	SKILL_MAX = (*p_D2COMMON_sgptDataTable)->dwSkillsRecs;
}

void GetAffixOffsets()
{
	D2MagicAffixDataTbl* pMagicAffixInfo = D2COMMON_10492_DATATBLS_GetMagicAffixDataTables();
	PREFIX_OFFSET = ((int)pMagicAffixInfo->pMagicPrefix - (int)pMagicAffixInfo->pMagicAffixTxt) / sizeof(AutoMagicTxt);
	AUTOMOD_OFFSET = ((int)pMagicAffixInfo->pAutoMagic - (int)pMagicAffixInfo->pMagicAffixTxt) / sizeof(AutoMagicTxt);
}

void Item::OnGameJoin() {
	// reset the item name cache upon joining games
	// (GUIDs not unique across games)
	ResetCaches();

	GetCharStats();
	GetItemStats();
	GetItemTypeMaps();
	GetWeaponAttributes();
	GetArmorAttributes();
	GetMiscAttributes();
	GetTotalSkills();
	GetAffixOffsets();
}

void Item::LoadConfig() {
	BH::config->ReadToggle("Show Ethereal", "None", true, Toggles["Show Ethereal"]);
	BH::config->ReadToggle("Show Sockets", "None", true, Toggles["Show Sockets"]);
	BH::config->ReadToggle("Show ILvl", "None", true, Toggles["Show iLvl"]);
	BH::config->ReadToggle("Show Rune Numbers", "None", true, Toggles["Show Rune Numbers"]);
	BH::config->ReadToggle("Alt Item Style", "None", true, Toggles["Alt Item Style"]);
	BH::config->ReadToggle("Color Mod", "None", false, Toggles["Color Mod"]);
	BH::config->ReadToggle("Shorten Item Names", "None", false, Toggles["Shorten Item Names"]);
	BH::config->ReadToggle("Always Show Items", "None", false, Toggles["Always Show Items"]);
	BH::config->ReadToggle("Advanced Item Display", "None", false, Toggles["Advanced Item Display"]);
	BH::config->ReadToggle("Item Drop Notifications", "None", false, Toggles["Item Drop Notifications"]);
	BH::config->ReadToggle("Item Close Notifications", "None", false, Toggles["Item Close Notifications"]);
	BH::config->ReadToggle("Item Detailed Notifications", "None", false, Toggles["Item Detailed Notifications"]);
	BH::config->ReadToggle("Verbose Notifications", "None", false, Toggles["Verbose Notifications"]);
	BH::config->ReadToggle("Allow Unknown Items", "None", false, Toggles["Allow Unknown Items"]);
	BH::config->ReadToggle("Always Show Item Stat Ranges", "None", true, Toggles["Always Show Item Stat Ranges"]);

	ItemDisplay::UninitializeItemRules();

	//InitializeMPQData();

	BH::config->ReadInt("Filter Level", filterLevelSetting, 1);
}

void Item::DrawSettings() {
	Drawing::Texthook* colored_text;
	settingsTab = new UITab("Item", BH::settingsUI);
	unsigned int x = 8;
	unsigned int y = 7;

	// Settings
	new Drawing::Texthook(settingsTab, x, (y), "Settings");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Always Show Items"].state, "Always Show Items");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Always Show Items"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Always Show Item Stat Ranges"].state, "Always Show Item Stat Ranges");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Always Show Item Stat Ranges"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Show iLvl"].state, "Show Item Level");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Show iLvl"].toggle, "");
	y += 20;

	// Display Style
	new Drawing::Texthook(settingsTab, x, (y), "Display Style (only without loot filter)");
	y += 15;

	Checkhook* etheral = new Checkhook(settingsTab, x, y, &Toggles["Show Ethereal"].state, "Show Ethereal");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Show Ethereal"].toggle, "");
	y += 15;

	Checkhook* sockets = new Checkhook(settingsTab, x, y, &Toggles["Show Sockets"].state, "Show Sockets");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Show Sockets"].toggle, "");
	y += 15;

	Checkhook* runes = new Checkhook(settingsTab, x, y, &Toggles["Show Rune Numbers"].state, "Show Rune Numbers");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Show Rune Numbers"].toggle, "");
	y += 15;

	Checkhook* alt = new Checkhook(settingsTab, x, y, &Toggles["Alt Item Style"].state, "Alt Style");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Alt Item Style"].toggle, "");
	y += 15;

	Checkhook* color = new Checkhook(settingsTab, x, y, &Toggles["Color Mod"].state, "Color Mod");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Color Mod"].toggle, "");
	y += 15;

	Checkhook* shorten = new Checkhook(settingsTab, x, y, &Toggles["Shorten Item Names"].state, "Shorten Names");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Shorten Item Names"].toggle, "");
	y += 20;

	// Loot Filter
	new Drawing::Texthook(settingsTab, x, (y), "Loot Filter");

	y += 15;
	new Checkhook(settingsTab, x, y, &Toggles["Advanced Item Display"].state, "Enable Loot Filter");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Advanced Item Display"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Item Drop Notifications"].state, "Drop Notifications");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Item Drop Notifications"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Item Close Notifications"].state, "Close Notifications");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Item Close Notifications"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Item Detailed Notifications"].state, "Detailed Notifications");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Item Detailed Notifications"].toggle, "");
	y += 15;

	new Checkhook(settingsTab, x, y, &Toggles["Verbose Notifications"].state, "Verbose Notifications");
	new Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &Toggles["Verbose Notifications"].toggle, "");
	y += 15;

	colored_text = new Texthook(settingsTab, x, y + 2, "Filter Level:");
	colored_text->SetColor(Gold);
	// Just a default as this is called first time around, not used
	if (ItemFilterNames.size() == 0) {
		ItemFilterNames.clear();
		ItemFilterNames.push_back(string("0 - Show All Items"));
		ItemFilterNames.push_back("1 - Normal");
	}

	new Combohook(settingsTab, 120, y, 200, &filterLevelSetting, ItemFilterNames);
}

void Item::ReplaceItemFilters(vector<string> itemFilterNames) {
	Hook* prev = NULL;
	for (auto it = settingsTab->Hooks.begin(); it != settingsTab->Hooks.end(); it++) {
		Hook* h = *it;
		Combohook* dropDown = dynamic_cast<Combohook*> (h);
		if (dropDown != NULL) {
			if (dropDown->GetOptions()[0] == "0 - Show All Items") {
				dropDown->ClearOptions();

				for each (string option in ItemFilterNames)
				{
					dropDown->NewOption(option);
				}

				break;
			}
		}
	}

	if (filterLevelSetting >= ItemFilterNames.size()) {
		filterLevelSetting = 1;
	}
}

void Item::OnUnload() {
	itemNamePatch->Remove();
	itemPropertiesPatch->Remove();
	itemPropertyStringDamagePatch->Remove();
	itemPropertyStringPatch->Remove();
	viewInvPatch1->Remove();
	viewInvPatch2->Remove();
	viewInvPatch3->Remove();
	permShowItems1->Remove();
	permShowItems2->Remove();
	permShowItems3->Remove();
	permShowItems4->Remove();
	permShowItems5->Remove();
	itemDropSoundIntercept1->Remove();
	newGroundIntercept->Remove();
	oldGroundIntercept->Remove();
	ItemDisplay::UninitializeItemRules();
}

void Item::OnLoop() {
	static unsigned int localFilterLevel = 9999;

	// This is a bit of a hack to reset the cache when the user changes the item filter level
	if (localFilterLevel != filterLevelSetting) {
		ResetCaches();
		localFilterLevel = filterLevelSetting;
	}

	if (!D2CLIENT_GetUIState(0x01))
		viewingUnit = NULL;

	if (Toggles["Advanced Item Display"].state) {
		ItemDisplay::InitializeItemRules();
	}

	if (viewingUnit && viewingUnit->dwUnitId) {
		if (!viewingUnit->pInventory) {
			viewingUnit = NULL;
			D2CLIENT_SetUIVar(0x01, 1, 0);
		}
		else if (!D2CLIENT_FindServerSideUnit(viewingUnit->dwUnitId, viewingUnit->dwType)) {
			viewingUnit = NULL;
			D2CLIENT_SetUIVar(0x01, 1, 0);
		}
	}
}

void Item::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	for (map<string, Toggle>::iterator it = Toggles.begin(); it != Toggles.end(); it++) {
		if (key == (*it).second.toggle) {
			*block = true;
			if (up) {
				(*it).second.state = !(*it).second.state;
			}
			return;
		}
	}
}

void Item::OnLeftClick(bool up, int x, int y, bool* block) {
	if (up)
		return;
	if (D2CLIENT_GetUIState(0x01) && viewingUnit != NULL && x >= 400)
		*block = true;
}

int CreateUnitItemInfo(UnitItemInfo* uInfo, UnitAny* item) {
	char* code = D2COMMON_GetItemText(item->dwTxtFileNo)->szCode;
	// If the item code is less than 4 characters, it will have space characters instead of null character
	uInfo->itemCode[0] = code[0];
	uInfo->itemCode[1] = code[1] != ' ' ? code[1] : 0;
	uInfo->itemCode[2] = code[2] != ' ' ? code[2] : 0;
	uInfo->itemCode[3] = code[3] != ' ' ? code[3] : 0;
	uInfo->itemCode[4] = 0;
	uInfo->item = item;
	if (ItemAttributeMap.find(std::string(uInfo->itemCode)) != ItemAttributeMap.end()) {
		uInfo->attrs = ItemAttributeMap[std::string(uInfo->itemCode)];
		return 0;
	}
	else {
		return -1;
	}
}

void __fastcall Item::ItemNamePatch(wchar_t* name, UnitAny* item)
{
	char* szName = UnicodeToAnsi(name);
	string itemName = szName;
	char* code = D2COMMON_GetItemText(item->dwTxtFileNo)->szCode;

	if (Toggles["Advanced Item Display"].state) {
		UnitItemInfo uInfo;
		if (!CreateUnitItemInfo(&uInfo, item)) {
			GetItemName(&uInfo, itemName);
		}
		else {
			HandleUnknownItemCode(uInfo.itemCode, "name");
		}
	}
	else {
		OrigGetItemName(item, itemName, code);
	}

	// Some common color codes for text strings (see TextColor enum):
	// ÿc; (purple)
	// ÿc0 (white)
	// ÿc1 (red)
	// ÿc2 (green)
	// ÿc3 (blue)
	// ÿc4 (gold)
	// ÿc5 (gray)
	// ÿc6 (black)
	// ÿc7 (tan)
	// ÿc8 (orange)
	// ÿc9 (yellow)

	/* Test code to display item codes */
	//string test3 = test_code;
	//itemName += " {" + test3 + "}";

	MultiByteToWideChar(CODE_PAGE, MB_PRECOMPOSED, itemName.c_str(), itemName.length(), name, itemName.length());
	name[itemName.length()] = 0;  // null-terminate the string since MultiByteToWideChar doesn't
	delete[] szName;
}

// Path when an item first drops from a monster, chest, etc.
void __stdcall GetItemFromPacket_NewGround(px9c* pPacket)
{
	D2CLIENT_GetItemFromPacket_NewGround_STUB(pPacket);
	UnitAny* pItem = D2CLIENT_FindServerSideUnit(pPacket->nItemId, UNIT_ITEM);
	UnitItemInfo uInfo;
	if (!CreateUnitItemInfo(&uInfo, pItem))
	{
		Item::ProcessItemPacketFilterRules(&uInfo, pPacket);
		if (!(pItem->dwFlags2 & UNITFLAGEX_INVISIBLE))
		{
			D2CLIENT_PlayItemDropSounds_STUB(pItem);
		}
	}
	else
	{
		HandleUnknownItemCode(uInfo.itemCode, "from packet");
	}

	return;
}

// Path when an item that was previously dropped comes into view
void __stdcall GetItemFromPacket_OldGround(px9c* pPacket)
{
	D2CLIENT_ItemPacketBuildAction3_OldGround(pPacket);
	UnitAny* pItem = D2CLIENT_FindServerSideUnit(pPacket->nItemId, UNIT_ITEM);
	UnitItemInfo uInfo;
	if (!CreateUnitItemInfo(&uInfo, pItem))
	{
		Item::ProcessItemPacketFilterRules(&uInfo, pPacket);
	}
	else
	{
		HandleUnknownItemCode(uInfo.itemCode, "from packet");
	}

	return;
}

void Item::ProcessItemPacketFilterRules(UnitItemInfo* uInfo, px9c* pPacket)
{
	if (Toggles["Advanced Item Display"].state) {
		bool showOnMap = false;
		auto color = UNDEFINED_COLOR;

		// TODO: This loop appears to only be used by legacy "Item Drop" & "Item Close" notifications.
		// Can delete after we get rid of them
		for (vector<Rule*>::iterator it = MapRuleList.begin(); it != MapRuleList.end(); it++) {
			// skip map and notification if ping level requirement is not met
			int filterLevel = GetFilterLevel();
			if (filterLevel != 0 && (*it)->action.pingLevel < filterLevel && (*it)->action.pingLevel != -1) continue;

			if ((*it)->Evaluate(uInfo)) {
				auto action_color = (*it)->action.notifyColor;
				// never overwrite color with an undefined color. never overwrite a defined color with dead color.
				if (action_color != UNDEFINED_COLOR && (action_color != DEAD_COLOR || color == UNDEFINED_COLOR))
					color = action_color;
				showOnMap = true;
				// break unless %CONTINUE% is used
				if ((*it)->action.stopProcessing) break;
			}
		}
		//PrintText(1, "Item on ground: %s, %s, %s, %X", item.name.c_str(), item.code, item.attrs->category.c_str(), item.attrs->flags);
		if (showOnMap && !(*BH::MiscToggles2)["Item Detailed Notifications"].state) {
			if (color == UNDEFINED_COLOR) {
				color = ItemColorFromQuality(uInfo->item->pItemData->dwQuality);
			}
			if ((*BH::MiscToggles2)["Item Drop Notifications"].state &&
				pPacket->nAction == ITEM_ACTION_NEW_GROUND &&
				color != DEAD_COLOR
				) {
				PrintText(color, "%s%s Dropped",
					uInfo->attrs->name.c_str(),
					(*BH::MiscToggles2)["Verbose Notifications"].state ? " \377c5drop" : ""
				);
			}
			if ((*BH::MiscToggles2)["Item Close Notifications"].state &&
				pPacket->nAction == ITEM_ACTION_OLD_GROUND &&
				color != DEAD_COLOR
				) {
				PrintText(color, "%s%s",
					uInfo->attrs->name.c_str(),
					(*BH::MiscToggles2)["Verbose Notifications"].state ? " \377c5close" : ""
				);
			}
		}
		else if (!showOnMap) {
			for (vector<Rule*>::iterator it = RuleList.begin(); it != RuleList.end(); it++) {
				if ((*it)->Evaluate(uInfo)) {
					if ((*it)->action.name.length() == 0 && Item::GetFilterLevel() > 0) {
						uInfo->item->dwFlags2 |= UNITFLAGEX_INVISIBLE;
					}
					if ((*it)->action.stopProcessing) break;
				}
			}
		}
	}
}

void Item::OrigGetItemName(UnitAny* item, string& itemName, char* code)
{
	bool displayItemLevel = Toggles["Show iLvl"].state;
	if (Toggles["Shorten Item Names"].state)
	{
		// We will also strip ilvls from these items
		if (code[0] == 't' && code[1] == 's' && code[2] == 'c')  // town portal scroll
		{
			itemName = "ÿc2**ÿc0TP";
			displayItemLevel = false;
		}
		else if (code[0] == 'i' && code[1] == 's' && code[2] == 'c')  // identify scroll
		{
			itemName = "ÿc2**ÿc0ID";
			displayItemLevel = false;
		}
		else if (code[0] == 'v' && code[1] == 'p' && code[2] == 's')  // stamina potion
		{
			itemName = "Stam";
			displayItemLevel = false;
		}
		else if (code[0] == 'y' && code[1] == 'p' && code[2] == 's')  // antidote potion
		{
			itemName = "Anti";
			displayItemLevel = false;
		}
		else if (code[0] == 'w' && code[1] == 'm' && code[2] == 's')  // thawing potion
		{
			itemName = "Thaw";
			displayItemLevel = false;
		}
		else if (code[0] == 'g' && code[1] == 'p' && code[2] == 's')  // rancid gas potion
		{
			itemName = "Ranc";
			displayItemLevel = false;
		}
		else if (code[0] == 'o' && code[1] == 'p' && code[2] == 's')  // oil potion
		{
			itemName = "Oil";
			displayItemLevel = false;
		}
		else if (code[0] == 'g' && code[1] == 'p' && code[2] == 'm')  // choking gas potion
		{
			itemName = "Chok";
			displayItemLevel = false;
		}
		else if (code[0] == 'o' && code[1] == 'p' && code[2] == 'm')  // exploding potion
		{
			itemName = "Expl";
			displayItemLevel = false;
		}
		else if (code[0] == 'g' && code[1] == 'p' && code[2] == 'l')  // strangling gas potion
		{
			itemName = "Stra";
			displayItemLevel = false;
		}
		else if (code[0] == 'o' && code[1] == 'p' && code[2] == 'l')  // fulminating potion
		{
			itemName = "Fulm";
			displayItemLevel = false;
		}
		else if (code[0] == 'h' && code[1] == 'p')  // healing potions
		{
			if (code[2] == '1')
			{
				itemName = "ÿc1**ÿc0Min Heal";
				displayItemLevel = false;
			}
			else if (code[2] == '2')
			{
				itemName = "ÿc1**ÿc0Lt Heal";
				displayItemLevel = false;
			}
			else if (code[2] == '3')
			{
				itemName = "ÿc1**ÿc0Heal";
				displayItemLevel = false;
			}
			else if (code[2] == '4')
			{
				itemName = "ÿc1**ÿc0Gt Heal";
				displayItemLevel = false;
			}
			else if (code[2] == '5')
			{
				itemName = "ÿc1**ÿc0Sup Heal";
				displayItemLevel = false;
			}
		}
		else if (code[0] == 'm' && code[1] == 'p')  // mana potions
		{
			if (code[2] == '1')
			{
				itemName = "ÿc3**ÿc0Min Mana";
				displayItemLevel = false;
			}
			else if (code[2] == '2')
			{
				itemName = "ÿc3**ÿc0Lt Mana";
				displayItemLevel = false;
			}
			else if (code[2] == '3')
			{
				itemName = "ÿc3**ÿc0Mana";
				displayItemLevel = false;
			}
			else if (code[2] == '4')
			{
				itemName = "ÿc3**ÿc0Gt Mana";
				displayItemLevel = false;
			}
			else if (code[2] == '5')
			{
				itemName = "ÿc3**ÿc0Sup Mana";
				displayItemLevel = false;
			}
		}
		else if (code[0] == 'r' && code[1] == 'v')  // rejuv potions
		{
			if (code[2] == 's')
			{
				itemName = "ÿc;**ÿc0Rejuv";
				displayItemLevel = false;
			}
			else if (code[2] == 'l')
			{
				itemName = "ÿc;**ÿc0Full";
				displayItemLevel = false;
			}
		}
		else if (code[1] == 'q' && code[2] == 'v')
		{
			if (code[0] == 'a')  // arrows
			{
				displayItemLevel = false;
			}
			else if (code[0] == 'c')  // bolts
			{
				displayItemLevel = false;
			}
		}
		else if (code[0] == 'k' && code[1] == 'e' && code[2] == 'y')  // key
		{
			displayItemLevel = false;
		}
	}

	/*Suffix Color Mod*/
	if (Toggles["Color Mod"].state)
	{
		/*Essences*/
		if (code[0] == 't' && code[1] == 'e' && code[2] == 's')
		{
			itemName = itemName + " (Andariel/Duriel)";
		}
		if (code[0] == 'c' && code[1] == 'e' && code[2] == 'h')
		{
			itemName = itemName + " (Mephtisto)";
		}
		if (code[0] == 'b' && code[1] == 'e' && code[2] == 't')
		{
			itemName = itemName + " (Diablo)";
		}
		if (code[0] == 'f' && code[1] == 'e' && code[2] == 'd')
		{
			itemName = itemName + " (Baal)";
		}
	}

	if (Toggles["Alt Item Style"].state)
	{
		if (Toggles["Show Rune Numbers"].state && D2COMMON_GetItemText(item->dwTxtFileNo)->nType == 74)
		{
			itemName = to_string(item->dwTxtFileNo - 609) + " - " + itemName;
		}
		else
		{
			if (Toggles["Show Sockets"].state)
			{
				int sockets = D2COMMON_GetUnitStat(item, STAT_SOCKETS, 0);
				if (sockets > 0)
				{
					itemName += "(" + to_string(sockets) + ")";
				}
			}

			if (Toggles["Show Ethereal"].state && item->pItemData->dwFlags & ITEM_ETHEREAL)
			{
				itemName = "Eth " + itemName;
			}

			/*show iLvl unless it is equal to 1*/
			if (displayItemLevel && item->pItemData->dwItemLevel != 1)
			{
				itemName += " L" + to_string(item->pItemData->dwItemLevel);
			}
		}
	}
	else
	{
		if (Toggles["Show Sockets"].state) {
			int sockets = D2COMMON_GetUnitStat(item, STAT_SOCKETS, 0);
			if (sockets > 0)
				itemName += "(" + to_string(sockets) + ")";
		}
		if (Toggles["Show Ethereal"].state && item->pItemData->dwFlags & ITEM_ETHEREAL)
			itemName += "(Eth)";

		if (displayItemLevel)
			itemName += "(L" + to_string(item->pItemData->dwItemLevel) + ")";

		if (Toggles["Show Rune Numbers"].state && D2COMMON_GetItemText(item->dwTxtFileNo)->nType == 74)
			itemName = "[" + to_string(item->dwTxtFileNo - 609) + "]" + itemName;
	}

	/*Affix (Colors) Color Mod*/
	if (Toggles["Color Mod"].state)
	{
		///*Flawless Gems*/
		//if( (code[0] == 'g' && code[1] == 'l'					) ||
		//	(code[0] == 's' && code[1] == 'k' && code[2] == 'l' ) )
		//{
		//	itemName = "ÿc:" + itemName;
		//}
		///*Perfect Gems*/
		//if( (code[0] == 'g' && code[1] == 'p'                   ) ||
		//	(code[0] == 's' && code[1] == 'k' && code[2] == 'p' ) )
		//{
		//	itemName = "ÿc<" + itemName;
		//}
		/*Ethereal*/
		if (item->pItemData->dwFlags & 0x400000)
		{
			/*Turn ethereal elite armors (and paladin shields) purple*/
			if ((code[0] == 'u') ||
				(code[0] == 'p' && code[1] == 'a' && code[2] >= 'b'))
			{
				itemName = "ÿc;" + itemName;
			}
		}
		/*Runes*/
		if (code[0] == 'r')
		{
			if (code[1] == '0')
			{
				itemName = "ÿc0" + itemName;
			}
			else if (code[1] == '1')
			{
				if (code[2] <= '6')
				{
					itemName = "ÿc4" + itemName;
				}
				else
				{
					itemName = "ÿc8" + itemName;
				}
			}
			else if (code[1] == '2')
			{
				if (code[2] <= '2')
				{
					itemName = "ÿc8" + itemName;
				}
				else
				{
					itemName = "ÿc1" + itemName;
				}
			}
			else if (code[1] == '3')
			{
				itemName = "ÿc1" + itemName;
			}
		}
	}
}

static ItemsTxt* GetArmorText(UnitAny* pItem) {
	ItemsTxt* itemTxt = D2COMMON_GetItemText(pItem->dwTxtFileNo);
	int armorTxtRecords = *p_D2COMMON_ArmorTxtRecords;
	for (int i = 0; i < armorTxtRecords; i++) {
		ItemsTxt* armorTxt = &(*p_D2COMMON_ArmorTxt)[i];
		if (strcmp(armorTxt->szCode, itemTxt->szCode) == 0) {
			return armorTxt;
		}
	}
	return NULL;
}

static UnitAny* lastItem;
static DWORD previousFlags;

void __stdcall Item::OnProperties(wchar_t* wTxt)
{
	const int MAXLEN = 1024;
	const int MAXDESCRIPTION = 512;
	UnitAny* pItem = *p_D2CLIENT_SelectedInvItem;
	UnitItemInfo uInfo;
	if (!pItem || pItem->dwType != UNIT_ITEM || CreateUnitItemInfo(&uInfo, pItem)) {
		return; // unknown item code
	}

	if (Toggles["Advanced Item Display"].state)
	{
		if (lastItem == nullptr)
		{
			lastItem = pItem;
			previousFlags = uInfo.item->pItemData->dwFlags;
		}

		if (lastItem != pItem)
		{
			lastItem = pItem;
			previousFlags = uInfo.item->pItemData->dwFlags;
			item_desc_cache.Clear(&uInfo);
			item_name_cache.Clear(&uInfo);
		}
		else
		{
			if (previousFlags != pItem->dwFlags)
			{
				previousFlags = uInfo.item->pItemData->dwFlags;
				item_desc_cache.Clear(&uInfo);
				item_name_cache.Clear(&uInfo);
			}
		}

		string desc = item_desc_cache.Get(&uInfo);
		if (desc != "") {
			static wchar_t wDesc[MAXDESCRIPTION];
			auto chars_written = MultiByteToWideChar(CODE_PAGE, MB_PRECOMPOSED, desc.c_str(), -1, wDesc, MAXDESCRIPTION);

			int aLen = wcslen(wTxt);
			swprintf_s(wTxt + aLen, MAXLEN - aLen,
				L"%s%s\n",
				(chars_written > 0) ? wDesc : L"\377c1Item Description is too long.",
				GetColorCode(TextColor::White).c_str()
			);
		}
	}

	if (!(Toggles["Always Show Item Stat Ranges"].state ||
		GetKeyState(VK_CONTROL) & 0x8000) ||
		pItem == nullptr ||
		pItem->dwType != UNIT_ITEM) {
		/* skip armor range */
	}
	//Any Armor ItemTypes.txt
	else if (D2COMMON_IsMatchingType(pItem, ITEM_TYPE_ALLARMOR)) {
		int aLen = 0;
		bool ebugged = false;
		bool spawned_with_ed = false;
		aLen = wcslen(wTxt);
		ItemsTxt* armorTxt = GetArmorText(pItem);
		DWORD base = D2COMMON_GetBaseStatSigned(pItem, STAT_DEFENSE, 0); // includes eth bonus if applicable
		DWORD min = armorTxt->dwminac; // min of non-eth base
		DWORD max_no_ed = armorTxt->dwmaxac; // max of non-eth base
		bool is_eth = pItem->pItemData->dwFlags & ITEM_ETHEREAL;
		if (((base == max_no_ed + 1) && !is_eth) || ((base == 3 * (max_no_ed + 1) / 2) && is_eth)) { // means item spawned with ED
			spawned_with_ed = true;
		}
		if (is_eth) {
			min = (DWORD)(min * 1.50);
			max_no_ed = (DWORD)(max_no_ed * 1.50);
			if (base > max_no_ed && !spawned_with_ed) { // must be ebugged
				min = (DWORD)(min * 1.50);
				max_no_ed = (DWORD)(max_no_ed * 1.50);
				ebugged = true;
			}
		}

		// Items with enhanced def mod will spawn with base def as max +1.
		// Don't show range if item spawned with edef and hasn't been upgraded.
		//if (!spawned_with_ed) {
		//	swprintf_s(wTxt + aLen, 1024 - aLen,
		//			L"%sBase Defense: %d %s[%d - %d]%s%s\n",
		//			GetColorCode(TextColor::White).c_str(),
		//			base,
		//			GetColorCode(TextColor::DarkGreen).c_str(),
		//			min, max_no_ed,
		//			ebugged ? L"\377c5 Ebug" : L"",
		//			GetColorCode(TextColor::White).c_str()
		//			);
		//}
	}

	int ilvl = pItem->pItemData->dwItemLevel;
	int alvl = GetAffixLevel((BYTE)pItem->pItemData->dwItemLevel, (BYTE)uInfo.attrs->qualityLevel, uInfo.attrs->magicLevel);
	int quality = pItem->pItemData->dwQuality;
	// Add alvl
	if (Toggles["Show iLvl"].state)
	{
		if (ilvl != alvl && (quality == ITEM_QUALITY_MAGIC || quality == ITEM_QUALITY_RARE || quality == ITEM_QUALITY_CRAFT)) {
			int aLen = wcslen(wTxt);
			swprintf_s(wTxt + aLen, MAXLEN - aLen,
				L"%sAffix Level: %d\n",
				GetColorCode(TextColor::White).c_str(),
				GetAffixLevel((BYTE)pItem->pItemData->dwItemLevel, (BYTE)uInfo.attrs->qualityLevel, uInfo.attrs->magicLevel));
		}
	}

	// Add ilvl
	if (Toggles["Show iLvl"].state)
	{
		int aLen = wcslen(wTxt);
		swprintf_s(wTxt + aLen, MAXLEN - aLen,
			L"%sItem Level: %d\n",
			GetColorCode(TextColor::White).c_str(),
			pItem->pItemData->dwItemLevel);
	}
}

BOOL __stdcall Item::OnDamagePropertyBuild(UnitAny* pItem, DamageStats* pDmgStats, int nStat, wchar_t* wOut) {
	wchar_t newDesc[128];

	// Ignore a max stat, use just a min dmg prop to gen the property string
	if (nStat == STAT_MAXIMUMFIREDAMAGE || nStat == STAT_MAXIMUMCOLDDAMAGE || nStat == STAT_MAXIMUMLIGHTNINGDAMAGE || nStat == STAT_MAXIMUMMAGICALDAMAGE ||
		nStat == STAT_MAXIMUMPOISONDAMAGE || nStat == STAT_POISONDAMAGELENGTH || nStat == STAT_ENHANCEDMAXIMUMDAMAGE)
		return TRUE;

	int stat_min, stat_max;
	wchar_t* szProp = nullptr;
	bool ranged = true;
	if (nStat == STAT_MINIMUMFIREDAMAGE) {
		if (pDmgStats->nFireDmgRange == 0)
			return FALSE;
		stat_min = pDmgStats->nMinFireDmg;
		stat_max = pDmgStats->nMaxFireDmg;
		if (stat_min >= stat_max) {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODFIREDAMAGE);
			ranged = false;
		}
		else {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODFIREDAMAGERANGE);
		}
	}
	else if (nStat == STAT_MINIMUMCOLDDAMAGE) {
		if (pDmgStats->nColdDmgRange == 0)
			return FALSE;
		stat_min = pDmgStats->nMinColdDmg;
		stat_max = pDmgStats->nMaxColdDmg;
		if (stat_min >= stat_max) {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODCOLDDAMAGE);
			ranged = false;
		}
		else {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODCOLDDAMAGERANGE);
		}
	}
	else if (nStat == STAT_MINIMUMLIGHTNINGDAMAGE) {
		if (pDmgStats->nLightDmgRange == 0)
			return FALSE;
		stat_min = pDmgStats->nMinLightDmg;
		stat_max = pDmgStats->nMaxLightDmg;
		if (stat_min >= stat_max) {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODLIGHTNINGDAMAGE);
			ranged = false;
		}
		else {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODLIGHTNINGDAMAGERANGE);
		}
	}
	else if (nStat == STAT_MINIMUMMAGICALDAMAGE) {
		if (pDmgStats->nMagicDmgRange == 0)
			return FALSE;
		stat_min = pDmgStats->nMinMagicDmg;
		stat_max = pDmgStats->nMaxMagicDmg;
		if (stat_min >= stat_max) {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODMAGICDAMAGE);
			ranged = false;
		}
		else {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODMAGICDAMAGERANGE);
		}
	}
	else if (nStat == STAT_MINIMUMPOISONDAMAGE) {
		if (pDmgStats->nPsnDmgRange == 0)
			return FALSE;
		if (pDmgStats->nPsnCount <= 0)
			pDmgStats->nPsnCount = 1;

		pDmgStats->nPsnLen = pDmgStats->nPsnLen / pDmgStats->nPsnCount;

		pDmgStats->nMinPsnDmg = stat_min = ((pDmgStats->nMinPsnDmg * pDmgStats->nPsnLen) + 128) / 256;
		pDmgStats->nMaxPsnDmg = stat_max = ((pDmgStats->nMaxPsnDmg * pDmgStats->nPsnLen) + 128) / 256;

		if (stat_min >= stat_max) {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODPOISONDAMAGE);
			swprintf_s(newDesc, 128, szProp, stat_max, pDmgStats->nPsnLen / 25); // Per frame
		}
		else {
			szProp = D2LANG_GetLocaleText(D2STR_STRMODPOISONDAMAGERANGE);
			swprintf_s(newDesc, 128, szProp, stat_min, stat_max, pDmgStats->nPsnLen / 25);
		}
		wcscat_s(wOut, 1024, newDesc);
		return TRUE;
	}
	else if (nStat == STAT_SECONDARYMAXIMUMDAMAGE) {
		if (pDmgStats->dword14)
			return TRUE;
		return pDmgStats->nDmgRange != 0;
	}
	else if (nStat == STAT_MINIMUMDAMAGE || nStat == STAT_MAXIMUMDAMAGE || nStat == STAT_SECONDARYMINIMUMDAMAGE) {
		if (pDmgStats->dword14)
			return TRUE;
		if (!pDmgStats->nDmgRange)
			return FALSE;

		stat_min = pDmgStats->nMinDmg;
		stat_max = pDmgStats->nMaxDmg;

		if (stat_min >= stat_max) {
			return FALSE;
		}
		else {
			pDmgStats->dword14 = TRUE;
			szProp = D2LANG_GetLocaleText(D2STR_STRMODMINDAMAGERANGE);

		}
	}
	else if (nStat == STAT_ENHANCEDMINIMUMDAMAGE) {
		if (!pDmgStats->nDmgPercentRange)
			return FALSE;
		stat_min = pDmgStats->nMinDmgPercent;
		stat_max = (int)(D2LANG_GetLocaleText(10023)); // "Enhanced damage"
		szProp = L"+%d%% %s\n";
	}

	if (szProp == nullptr) {
		return FALSE;
	}

	if (ranged) {
		swprintf_s(newDesc, 128, szProp, stat_min, stat_max);
	}
	else {
		swprintf_s(newDesc, 128, szProp, stat_max);
	}

	// <!--
	if (newDesc[wcslen(newDesc) - 1] == L'\n')
		newDesc[wcslen(newDesc) - 1] = L'\0';
	if (newDesc[wcslen(newDesc) - 1] == L'\n')
		newDesc[wcslen(newDesc) - 1] = L'\0';

	OnPropertyBuild(newDesc, nStat, pItem, 0);
	// Beside this add-on the function is almost 1:1 copy of Blizzard's one -->
	wcscat_s(wOut, 1024, newDesc);
	wcscat_s(wOut, 1024, L"\n");

	return TRUE;
}

void __stdcall Item::OnPropertyBuild(wchar_t* wOut, int nStat, UnitAny* pItem, int nStatParam) {
	if (!(Toggles["Always Show Item Stat Ranges"].state || GetKeyState(VK_CONTROL) & 0x8000) || pItem == nullptr || pItem->dwType != UNIT_ITEM) {
		return;
	}

	ItemsTxtStat* stat = nullptr;
	ItemsTxtStat* all_stat = nullptr; // Stat for common modifer like all-res, all-stats
	ItemsTxtStat* max_elem_stat = nullptr; // Stat for ranged min/max stats

	int nCorruptor = ItemGetCorruptor(pItem);
	BOOL isCorrupted = StatIsCorrupted(nStat, nCorruptor);
	if (isCorrupted) {
		// Add a red star to corruption mods
		int	aLen = wcslen(wOut);
		int leftSpace = 128 - aLen > 0 ? 128 - aLen : 0;
		if (leftSpace) {
			swprintf_s(wOut + aLen, leftSpace,
				L"%s\*%s",
				GetColorCode(TextColor::Red).c_str(),
				GetColorCode(TextColor::Blue).c_str());
		}
	}


	TextColor statColor = TextColor::Blue;
	switch (pItem->pItemData->dwQuality) {
	case ITEM_QUALITY_SET:
	{
		SetItemsTxt* pTxt = &(*p_D2COMMON_sgptDataTable)->pSetItemsTxt[pItem->pItemData->dwFileIndex];
		if (!pTxt)
			break;
		stat = GetItemsTxtStatByMod(pTxt->hStats, 9 + 10, nStat, nStatParam);
		if (stat) {
			all_stat = GetAllStatModifier(pTxt->hStats, 9 + 10, nStat, stat);
		}
		// At some point we should fix set stats with ranges
		//statColor = TextColor::Green;
	}
	case ITEM_QUALITY_UNIQUE:
	{
		if (pItem->pItemData->dwQuality == ITEM_QUALITY_UNIQUE) {
			UniqueItemsTxt* pTxt = &(*p_D2COMMON_sgptDataTable)->pUniqueItemsTxt[pItem->pItemData->dwFileIndex];
			if (!pTxt) {
				break;
			}
			stat = GetItemsTxtStatByMod(pTxt->hStats, 12, nStat, nStatParam);

			if (stat != nullptr) {
				all_stat = GetAllStatModifier(pTxt->hStats, 12, nStat, stat);
			}

			if (stat != nullptr) {
				max_elem_stat = GetMaxElemStatFromMin(pTxt->hStats, 12, nStat, stat);
			}
		}

		if (stat != nullptr) {
			int statMin = stat->dwMin;
			int statMax = stat->dwMax;

			if (all_stat != nullptr) {
				statMin += all_stat->dwMin;
				statMax += all_stat->dwMax;
			}

			if (statMin < statMax || max_elem_stat != nullptr) {
				int	aLen = wcslen(wOut);
				int leftSpace = 128 - aLen > 0 ? 128 - aLen : 0;

				if (nStat >= STAT_DEFENSEPERLEVEL && nStat <= STAT_FINDGEMSPERLEVEL)
				{
					//statMin = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMin >> 3;
					//statMax = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMax >> 3;
					double fStatMin = statMin / 8.0;
					double fStatMax = statMax / 8.0;
					wchar_t fStatMinStr[20];
					wchar_t fStatMaxStr[20];
					swprintf_s(fStatMinStr, L"%.20g", fStatMin);
					swprintf_s(fStatMaxStr, L"%.20g", fStatMax);
					FixDecimalString(fStatMinStr, 3);
					FixDecimalString(fStatMaxStr, 3);
					if (leftSpace) {
						swprintf_s(wOut + aLen, leftSpace,
							L" %s[%s - %s]%s",
							GetColorCode(TextColor::DarkGreen).c_str(),
							fStatMinStr,
							fStatMaxStr,
							GetColorCode(statColor).c_str());
					}
				}
				else
				{
					if (leftSpace) {
						if (max_elem_stat == nullptr)
						{
							swprintf_s(wOut + aLen, leftSpace,
								L" %s[%d - %d]%s",
								GetColorCode(TextColor::DarkGreen).c_str(),
								statMin,
								statMax,
								GetColorCode(statColor).c_str());
						}
						else
						{
							int maxStatMin = max_elem_stat->dwMin;
							int maxStatMax = max_elem_stat->dwMax;
							if (statMin < statMax && maxStatMin < maxStatMax)
							{
								swprintf_s(wOut + aLen, leftSpace,
									L" %s[%d - %d to %d - %d]%s",
									GetColorCode(TextColor::DarkGreen).c_str(),
									statMin,
									statMax,
									maxStatMin,
									maxStatMax,
									GetColorCode(statColor).c_str());
							}
							else if (maxStatMin < maxStatMax)
							{
								swprintf_s(wOut + aLen, leftSpace,
									L" %s[%d to %d - %d]%s",
									GetColorCode(TextColor::DarkGreen).c_str(),
									statMin,
									maxStatMin,
									maxStatMax,
									GetColorCode(statColor).c_str());
							}
						}
					}
				}
			}
		}
	} break;
	default:
	{
		if (pItem->pItemData->dwFlags & ITEM_RUNEWORD) {
			RunesTxt* pTxt = GetRunewordTxtById(pItem->pItemData->wPrefix[0]);
			if (!pTxt)
				break;
			stat = GetItemsTxtStatByMod(pTxt->hStats, 7, nStat, nStatParam);

			if (stat) {
				int statMin = stat->dwMin;
				int statMax = stat->dwMax;

				all_stat = GetAllStatModifier(pTxt->hStats, 7, nStat, stat);
				max_elem_stat = GetMaxElemStatFromMin(pTxt->hStats, 7, nStat, stat);

				if (all_stat) {
					statMin += all_stat->dwMin;
					statMax += all_stat->dwMax;
				}

				if (statMin < statMax || max_elem_stat != nullptr) {
					int	aLen = wcslen(wOut);
					int leftSpace = 128 - aLen > 0 ? 128 - aLen : 0;

					if (nStat >= STAT_DEFENSEPERLEVEL && nStat <= STAT_FINDGEMSPERLEVEL)
					{
						//statMin = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMin >> 3;
						//statMax = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMax >> 3;
						double fStatMin = statMin / 8.0;
						double fStatMax = statMax / 8.0;
						wchar_t fStatMinStr[20];
						wchar_t fStatMaxStr[20];
						swprintf_s(fStatMinStr, L"%.20g", fStatMin);
						swprintf_s(fStatMaxStr, L"%.20g", fStatMax);
						FixDecimalString(fStatMinStr, 3);
						FixDecimalString(fStatMaxStr, 3);
						if (leftSpace) {
							swprintf_s(wOut + aLen, leftSpace,
								L" %s[%s - %s]%s",
								GetColorCode(TextColor::DarkGreen).c_str(),
								fStatMinStr,
								fStatMaxStr,
								GetColorCode(statColor).c_str());
						}
					}
					else
					{
						if (leftSpace) {
							if (max_elem_stat == nullptr)
							{
								swprintf_s(wOut + aLen, leftSpace,
									L" %s[%d - %d]%s",
									GetColorCode(TextColor::DarkGreen).c_str(),
									statMin,
									statMax,
									GetColorCode(statColor).c_str());
							}
							else
							{
								int maxStatMin = max_elem_stat->dwMin;
								int maxStatMax = max_elem_stat->dwMax;
								if (statMin < statMax && maxStatMin < maxStatMax)
								{
									swprintf_s(wOut + aLen, leftSpace,
										L" %s[%d - %d to %d - %d]%s",
										GetColorCode(TextColor::DarkGreen).c_str(),
										statMin,
										statMax,
										maxStatMin,
										maxStatMax,
										GetColorCode(statColor).c_str());
								}
								else if (maxStatMin < maxStatMax)
								{
									swprintf_s(wOut + aLen, leftSpace,
										L" %s[%d to %d - %d]%s",
										GetColorCode(TextColor::DarkGreen).c_str(),
										statMin,
										maxStatMin,
										maxStatMax,
										GetColorCode(statColor).c_str());
								}
							}
						}
					}
				}
			}
		}
		else if (pItem->pItemData->dwQuality == ITEM_QUALITY_MAGIC || pItem->pItemData->dwQuality == ITEM_QUALITY_RARE || pItem->pItemData->dwQuality == ITEM_QUALITY_CRAFT)
		{
			// Check for all res
			// I know this is a mess but there's a lot of edge cases to handle
			bool hasFireRes = false;
			bool hasColdRes = false;
			bool hasLightRes = false;
			bool hasPoisRes = false;
			bool isSingleRes = false;
			int allResVal = 0;
			int minResVal = MAXINT;
			int currentResVal = 0;
			for (int i = 0; i < pItem->pStats->wSetStatCount; i++) {
				int resStat = pItem->pStats->pSetStat[i].wStatIndex;
				int resVal = pItem->pStats->pSetStat[i].dwStatValue;

				if (resStat == STAT_FIRERESIST) {
					hasFireRes = true;
					if (allResVal == 0) {
						allResVal = resVal;
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
					}
					else if (resVal > allResVal) {
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
						else {
							isSingleRes = false;
						}
					}
					else if (resVal < minResVal) {
						minResVal = resVal;
					}
				}
				else if (resStat == STAT_COLDRESIST) {
					hasColdRes = true;
					if (allResVal == 0) {
						allResVal = resVal;
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
					}
					else if (resVal > allResVal) {
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
						else {
							isSingleRes = false;
						}
					}
					else if (resVal < minResVal) {
						minResVal = resVal;
					}
				}
				else if (resStat == STAT_LIGHTNINGRESIST) {
					hasLightRes = true;
					if (allResVal == 0) {
						allResVal = resVal;
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
					}
					else if (resVal > allResVal) {
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
						else {
							isSingleRes = false;
						}
					}
					else if (resVal < minResVal) {
						minResVal = resVal;
					}
				}
				else if (resStat == STAT_POISONRESIST) {
					hasPoisRes = true;
					if (allResVal == 0) {
						allResVal = resVal;
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
					}
					else if (resVal > allResVal) {
						if (resStat == nStat) {
							isSingleRes = true;
							currentResVal = resVal;
						}
						else {
							isSingleRes = false;
						}
					}
					else if (resVal < minResVal) {
						minResVal = resVal;
					}
				}
			}

			bool hasAllRes = (hasFireRes && hasColdRes && hasLightRes && hasPoisRes);
			bool statIsRes = (nStat == STAT_FIRERESIST || nStat == STAT_COLDRESIST || nStat == STAT_LIGHTNINGRESIST || nStat == STAT_POISONRESIST);
			if (isSingleRes && currentResVal <= minResVal) {
				isSingleRes = false;
			}

			int nAffixes = *p_D2COMMON_AutoMagicTxt - D2COMMON_GetItemMagicalMods(1); // Number of affixes without Automagic
			int min = 0, max = 0;
			int allResMin = 0, allResMax = 0;
			int type = D2COMMON_GetItemType(pItem);
			int iLvl = pItem->pItemData->dwItemLevel;
			BnetData* pData = (*p_D2LAUNCH_BnData);
			int is_expansion = pData->nCharFlags & PLAYER_TYPE_EXPANSION;
			for (int i = 1;; ++i) {
				if (!pItem->pItemData->wAutoPrefix && i > nAffixes) // Don't include Automagic.txt affixes if item doesn't use them
					break;
				AutoMagicTxt* pTxt = D2COMMON_GetItemMagicalMods(i);
				if (!pTxt)
					break;
				bool is_classic_affix = pTxt->wVersion == 1;
				bool is_expansion_affix = pTxt->wVersion != 0;
				// skip affixes that are not valid for expansion when using expansion stat ranges
				if (is_expansion && !is_expansion_affix) continue;
				// skip non-classic affixes when using classic stat ranges
				if (!is_expansion && !is_classic_affix) continue;
				//Skip if stat level is > 99 or iLvl of the item
				if (pTxt->dwLevel > 99 || pTxt->dwLevel > iLvl)
					continue;
				//Skip if stat is not spawnable
				if (pItem->pItemData->dwQuality < ITEM_QUALITY_CRAFT && !pTxt->wSpawnable)
					continue;
				//Skip for rares+
				if (pItem->pItemData->dwQuality >= ITEM_QUALITY_RARE && !pTxt->nRare)
					continue;
				//Firstly check Itemtype
				bool found_itype = false;
				bool found_etype = false;

				for (int j = 0; j < 5; ++j)
				{
					if (!pTxt->wEType[j] || pTxt->wEType[j] == 0xFFFF)
						break;
					if (D2COMMON_IsMatchingType(pItem, pTxt->wEType[j])) {
						found_etype = true;
						break;
					}
				}
				if (found_etype) // next if excluded type
					continue;

				for (int j = 0; j < 7; ++j)
				{
					if (!pTxt->wIType[j] || pTxt->wIType[j] == 0xFFFF)
						break;
					if (D2COMMON_IsMatchingType(pItem, pTxt->wIType[j])) {
						found_itype = true;
						break;
					}
				}
				if (!found_itype)
					continue;

				stat = GetItemsTxtStatByMod(pTxt->hMods, 3, nStat, nStatParam);
				if (!stat)
					continue;
				max_elem_stat = GetMaxElemStatFromMin(pTxt->hMods, 3, nStat, stat);
				if (stat->dwProp == 41) {
					allResMin = allResMin == 0 ? stat->dwMin : ((stat->dwMin < allResMin) ? stat->dwMin : allResMin);
					allResMax = (stat->dwMax > allResMax) ? stat->dwMax : allResMax;
				}
				if (statIsRes && !isSingleRes && hasAllRes && nStat && stat->dwProp != 41)
					continue;
				min = min == 0 ? stat->dwMin : ((stat->dwMin < min) ? stat->dwMin : min);
				max = (stat->dwMax > max) ? stat->dwMax : max;
				//break;
				//DEBUGMSG(L"%s: update min to %d, and max to %d (record #%d)", wOut, min, max, i)
			}

			if (isSingleRes && hasAllRes) {
				min += allResMin;
				max += allResMax;
			}

			if (min < max || max_elem_stat != nullptr) {
				int	aLen = wcslen(wOut);
				int leftSpace = 128 - aLen > 0 ? 128 - aLen : 0;
				if (nStat >= STAT_DEFENSEPERLEVEL && nStat <= STAT_FINDGEMSPERLEVEL)
				{
					//statMin = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMin >> 3;
					//statMax = D2COMMON_GetBaseStatSigned(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0) * statMax >> 3;
					double fStatMin = min / 8.0;
					double fStatMax = max / 8.0;
					wchar_t fStatMinStr[20];
					wchar_t fStatMaxStr[20];
					swprintf_s(fStatMinStr, L"%.20g", fStatMin);
					swprintf_s(fStatMaxStr, L"%.20g", fStatMax);
					FixDecimalString(fStatMinStr, 3);
					FixDecimalString(fStatMaxStr, 3);
					if (leftSpace) {
						swprintf_s(wOut + aLen, leftSpace,
							L" %s[%s - %s]%s",
							GetColorCode(TextColor::DarkGreen).c_str(),
							fStatMinStr,
							fStatMaxStr,
							GetColorCode(statColor).c_str());
					}
				}
				else
				{
					if (leftSpace) {
						if (max_elem_stat == nullptr)
						{
							swprintf_s(wOut + aLen, leftSpace,
								L" %s[%d - %d]%s",
								GetColorCode(TextColor::DarkGreen).c_str(),
								min,
								max,
								GetColorCode(statColor).c_str());
						}
						else
						{
							int maxStatMin = max_elem_stat->dwMin;
							int maxStatMax = max_elem_stat->dwMax;
							if (min < max && maxStatMin < maxStatMax)
							{
								swprintf_s(wOut + aLen, leftSpace,
									L" %s[%d - %d to %d - %d]%s",
									GetColorCode(TextColor::DarkGreen).c_str(),
									min,
									max,
									maxStatMin,
									maxStatMax,
									GetColorCode(statColor).c_str());
							}
							else if (maxStatMin < maxStatMax)
							{
								swprintf_s(wOut + aLen, leftSpace,
									L" %s[%d to %d - %d]%s",
									GetColorCode(TextColor::DarkGreen).c_str(),
									min,
									maxStatMin,
									maxStatMax,
									GetColorCode(statColor).c_str());
							}
						}
					}
				}
			}
		}

	} break;

	}
}

/*
	Search mod used in MagicPrefix.txt, UniqueItemsTxt, RunesTxt, etc. (index from Properties.txt) by ItemStatCost.txt stat index
	@param nStatParam - param column for property (skill id etc)
	@param nStat - ItemStatCost.txt record id
	@param nStats - number of pStats
	@param pStats - pointer to ItemsTxtStat* array [PropertiesTxt Id, min, max val)
*/
ItemsTxtStat* GetItemsTxtStatByMod(ItemsTxtStat* pStats, int nStats, int nStat, int nStatParam)
{
	if (nStat == STAT_SKILLONKILL || nStat == STAT_SKILLONHIT || nStat == STAT_SKILLONSTRIKING || nStat == STAT_SKILLONDEATH ||
		nStat == STAT_SKILLONLEVELUP || nStat == STAT_SKILLWHENSTRUCK || nStat == STAT_CHARGED)
	{
		return nullptr;
	}
	for (int i = 0; i < nStats; ++i) {
		try {
			IsBadReadPtr(pStats, sizeof(ItemsTxtStat));
		}
		catch (...) {
			return nullptr;
		}

		if (pStats[i].dwProp == 0xffffffff || pStats[i].dwProp > (*p_D2COMMON_sgptDataTable)->dwProportiesRecs) {
			break;
		}
		PropertiesTxt* pProp = &(*p_D2COMMON_sgptDataTable)->pPropertiesTxt[pStats[i].dwProp];
		if (pProp == nullptr) {
			break;
		}

		if (pProp->wStat[0] == 0xFFFF && pProp->nFunc[0] == 7 && (nStat == STAT_ENHANCEDDAMAGE || nStat == STAT_ENHANCEDMINIMUMDAMAGE || nStat == STAT_ENHANCEDMAXIMUMDAMAGE ||
			nStat == STAT_MAXENHANCEDDMGPERTIME || nStat == STAT_MAXENHANCEDDMGPERLEVEL)) {
			return &pStats[i];
		}
		else if (pProp->wStat[0] == 0xFFFF && pProp->nFunc[0] == 6 && (nStat == STAT_MAXIMUMDAMAGE || nStat == STAT_SECONDARYMAXIMUMDAMAGE ||
			nStat == STAT_MAXDAMAGEPERTIME || nStat == STAT_MAXDAMAGEPERLEVEL)) {
			return &pStats[i];
		}
		else if (pProp->wStat[0] == 0xFFFF && pProp->nFunc[0] == 5 && (nStat == STAT_MINIMUMDAMAGE || nStat == STAT_SECONDARYMINIMUMDAMAGE)) {
			return &pStats[i];
		}
		else if (pProp->nFunc[0] == 17 && (nStat == STAT_MAXENHANCEDDMGPERLEVEL)) {
			return &pStats[i];
		}
		for (int j = 0; j < 7; ++j)
		{
			if (pProp->wStat[j] == 0xFFFF) {
				break;
			}
			if (nStat == STAT_SKILLTAB || nStat == STAT_CLASSSKILLS) {
				// func 36 is the choose random skill func
				if (pProp->wStat[j] == nStat && pProp->nFunc[j] != 36) {
					return &pStats[i];
				}
			}
			if (pProp->wStat[j] == nStat && pStats[i].dwPar == nStatParam) {
				// These functions only use 1 param
				if (pProp->nFunc[j] != 15 && pProp->nFunc[j] != 16) {
					return &pStats[i];
				}
			}
		}
	}
	return nullptr;
}

/*
	Find other mod that inflates the original
	@param pOrigin  - original stat
	@param nStat - ItemStatCost.txt record id
	@param nStats - number of pStats
	@param pStats - pointer to ItemsTxtStat* array [PropertiesTxt Id, min, max val)
*/
ItemsTxtStat* GetAllStatModifier(ItemsTxtStat* pStats, int nStats, int nStat, ItemsTxtStat* pOrigin)
{
	for (int i = 0; i < nStats; ++i) {
		if (pStats[i].dwProp == 0xffffffff)
			break;
		if (pStats[i].dwProp == pOrigin->dwProp)
			continue;

		PropertiesTxt* pProp = &(*p_D2COMMON_sgptDataTable)->pPropertiesTxt[pStats[i].dwProp];
		if (pProp == nullptr) {
			break;
		}

		for (int j = 0; j < 7; ++j) {
			if (pProp->wStat[j] == 0xFFFF) {
				break;
			}
			if (pProp->wStat[j] == nStat) {
				return &pStats[i];
			}
		}
	}
	return nullptr;
}

ItemsTxtStat* GetMaxElemStatFromMin(ItemsTxtStat* pStats, int nStats, int nStat, ItemsTxtStat* pOrigin)
{
	if (nStat != STAT_MINIMUMFIREDAMAGE && nStat != STAT_MINIMUMCOLDDAMAGE && nStat != STAT_MINIMUMLIGHTNINGDAMAGE
		&& nStat != STAT_MINIMUMPOISONDAMAGE && nStat != STAT_MINIMUMMAGICALDAMAGE && nStat != STAT_MINIMUMDAMAGE && nStat != STAT_SECONDARYMINIMUMDAMAGE) {
		return nullptr;
	}

	for (int i = 0; i < nStats; ++i) {
		if (pStats[i].dwProp == 0xffffffff)
			break;
		if (pStats[i].dwProp == pOrigin->dwProp)
			continue;

		PropertiesTxt* pProp = &(*p_D2COMMON_sgptDataTable)->pPropertiesTxt[pStats[i].dwProp];
		if (pProp == nullptr) {
			break;
		}

		for (int j = 0; j < 7; ++j) {
			if (pProp->wProp != 28 && pProp->wStat[j] == 0xFFFF) {
				break;
			}

			switch (nStat)
			{
			case STAT_MINIMUMFIREDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMFIREDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_MINIMUMCOLDDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMCOLDDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_MINIMUMLIGHTNINGDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMLIGHTNINGDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_MINIMUMPOISONDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMPOISONDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_MINIMUMMAGICALDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMMAGICALDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_MINIMUMDAMAGE:
				if (pProp->wStat[j] == STAT_MAXIMUMDAMAGE) {
					return &pStats[i];
				}
				break;
			case STAT_SECONDARYMINIMUMDAMAGE:
				// min/max damage have no stats on the property
				if (pProp->wProp == 28) {
					return &pStats[i];
				}
				break;
			default:
				break;
			}
		}
	}
	return nullptr;
}

int ItemGetCorruptor(UnitAny* pItem)
{
	int corruptor = -1;

	for (int i = 0; i < pItem->pStats->wSetStatCount; i++) {
		Stat* pStat = &pItem->pStats->pSetStat[i];
		if (pStat->wStatIndex == STAT_CORRUPTED) {
			corruptor = pStat->dwStatValue - 2;
			break;
		}
	}

	return corruptor;
}

BOOL StatIsCorrupted(int nStat, int nCorruptor)
{
	if (nCorruptor < 0 || nCorruptor >= NUM_CORRUPTIONS)
		return false;

	for (int j = 0; j < 6; j++) {
		if (CorruptionMods[nCorruptor][j] < 0) {
			continue;
		}
		else if (CorruptionMods[nCorruptor][j] == nStat) {
			return true;
		}
	}
	return false;
}

RunesTxt* GetRunewordTxtById(int rwId)
{
	int n = *(D2COMMON_GetRunesTxtRecords());
	for (int i = 1; i < n; ++i)
	{
		RunesTxt* pTxt = D2COMMON_GetRunesTxt(i);
		if (!pTxt)
			break;
		if (pTxt->dwRwId == rwId)
			return pTxt;
	}
	return 0;
}

UnitAny* Item::GetViewUnit()
{
	UnitAny* view = (viewingUnit) ? viewingUnit : D2CLIENT_GetPlayerUnit();
	if (view != NULL && view->dwUnitId == D2CLIENT_GetPlayerUnit()->dwUnitId)
		return D2CLIENT_GetPlayerUnit();

	return viewingUnit;
}

UnitAny* Item::GetViewUnitAndDrawBox()
{
	UnitAny* view = (viewingUnit) ? viewingUnit : D2CLIENT_GetPlayerUnit();
	if (view->dwUnitId == D2CLIENT_GetPlayerUnit()->dwUnitId)
		return D2CLIENT_GetPlayerUnit();

	// Black out the player inventory when inspecting
	Drawing::Boxhook::Draw(*p_D2CLIENT_PanelOffsetX + 320 + 14, 250, 289, 231, White, Drawing::BTBlack);
	if (viewingUnit != NULL && viewingUnit->dwType == 0) {
		Drawing::Texthook::Draw(*p_D2CLIENT_PanelOffsetX + 160 + 320, 300, Drawing::Center, 0, White, "%s", viewingUnit->pPlayerData->szName);
	}
	return viewingUnit;
}

void FixDecimalString(wchar_t* s, int n)
{
	wchar_t* p;
	int count;

	p = wcschr(s, L'.');         // Find decimal point, if any.
	if (p != NULL) {
		count = n;              // Adjust for more or less decimals.
		while (count >= 0) {    // Maximum decimals allowed.
			count--;
			if (*p == '\0')    // If there's less than desired.
				break;
			p++;               // Next character.
		}

		*p-- = '\0';            // Truncate string.
		while (*p == '0')       // Remove trailing zeros.
			*p-- = '\0';

		if (*p == '.') {        // If all decimals were zeros, remove ".".
			*p = '\0';
		}
	}
}

void __declspec(naked) ItemName_Interception()
{
	__asm {
		mov ecx, edi
		mov edx, ebx
		call Item::ItemNamePatch
		mov al, [ebp + 0x12a]
		ret
	}
}


__declspec(naked) void __fastcall GetProperties_Interception()
{
	__asm
	{
		push eax
		call Item::OnProperties
		add esp, 0x808
		ret 12
	}
}

/*	Wrapper over D2CLIENT.0x2E04B (1.13d)
	BOOL __userpurge ITEMS_BuildDamagePropertyDesc@<eax>(DamageStats *pStats@<eax>, int nStat, wchar_t *wOut)
	Function is pretty simple so I decided to rewrite it.
	@esp-0x20:	pItem
*/
void __declspec(naked) GetItemPropertyStringDamage_Interception()
{
	__asm {
		push[esp + 8]			// wOut
		push[esp + 8]			// nStat
		push eax				// pStats
		push[esp - 0x20 + 12]	// pItem

		call Item::OnDamagePropertyBuild

		ret 8
	}
}

/* Wrapper over D2CLIENT.0x2E06D (1.13d)
	As far I know this: int __userpurge ITEMS_ParseStats_6FADCE40<eax>(signed __int32 nStat<eax>, wchar_t *wOut<esi>, UnitAny *pItem, StatListEx *pStatList, DWORD nStatParam, DWORD nStatValue, int a7)
	Warning: wOut is 128 words length only!
	@ebx the nStat value
	@edi pStatListEx
	@esp-0x10 seems to always keep pItem *careful*
*/
void __declspec(naked) GetItemPropertyString_Interception()
{
	static DWORD rtn = 0; // if something is stupid but works then it's not stupid!
	__asm
	{
		pop rtn
		// Firstly generate string using old function
		call D2CLIENT_ParseStats_J
		push rtn

		push[esp - 4] // preserve nStatParam

		push eax // Store result
		mov eax, [esp - 0x10 + 8 + 4] // pItem
		push ecx
		push edx

		// Then pass the output to our func
		push[esp + 12] // nStatParam
		push eax // pItem
		push ebx // nStat
		push esi // wOut

		call Item::OnPropertyBuild

		pop edx
		pop ecx
		pop eax

		add esp, 4 // clean nStatParam

		ret
	}
}

void __declspec(naked) ViewInventoryPatch1_ASM()
{
	__asm {
		push eax;
		call Item::GetViewUnitAndDrawBox;
		mov esi, eax;
		pop eax;
		ret;
	}
}
void __declspec(naked) ViewInventoryPatch2_ASM()
{
	__asm {
		push eax;
		call Item::GetViewUnit;
		mov ebx, eax;
		pop eax;
		ret;
	}
}
void __declspec(naked) ViewInventoryPatch3_ASM()
{
	__asm
	{
		push eax;
		push ebx;
		call Item::GetViewUnit;

		mov ebx, [edi];
		cmp ebx, 1;
		je OldCode;

		mov edi, eax;

	OldCode:
		pop ebx;
		pop eax;
		test eax, eax;
		mov ecx, dword ptr[edi + 0x60];

		ret;
	}
}

//seems to force alt to be down
BOOL Item::PermShowItemsPatch1()
{
	return Toggles["Always Show Items"].state || D2CLIENT_GetUIState(UI_GROUND_ITEMS);
	//return Toggles["Always Show Items"].state;
}

//these two seem to deal w/ fixing the inv/waypoints when alt is down
BOOL Item::PermShowItemsPatch2() {
	//return Toggles["Always Show Items"].state || D2CLIENT_GetUIState(UI_GROUND_ITEMS);
	return Toggles["Always Show Items"].state && !D2CLIENT_GetUIState(UI_GROUND_ITEMS);
}

BOOL Item::PermShowItemsPatch3() {
	return Toggles["Always Show Items"].state || D2CLIENT_GetUIState(UI_GROUND_ITEMS);
	//return Toggles["Always Show Items"].state;
}


void __declspec(naked) PermShowItemsPatch1_ASM()
{
	__asm {
		call Item::PermShowItemsPatch1
		test eax, eax
		ret
	}
}


void __declspec(naked) PermShowItemsPatch2_ASM()
{
	__asm {
		call Item::PermShowItemsPatch2
		test eax, eax
		je orgcode
		ret
		orgcode :
		mov eax, dword ptr[esp + 0x20]
			test eax, eax
			ret
	}
}


void __declspec(naked) PermShowItemsPatch3_ASM()
{
	__asm {
		push ebp
		push esi
		call Item::PermShowItemsPatch3
		test eax, eax
		pop esi
		pop ebp
		jz 	outcode
		cmp ebp, 0x20
		jge outcode
		ret
		outcode :
		add dword ptr[esp], 0x38A  //to 6FB0DD89
			ret
	}
}


void __declspec(naked) PermShowItemsPatch4_ASM()
{
	__asm {
		push eax
		call Item::PermShowItemsPatch1
		mov ecx, eax
		pop eax
		ret
	}
}
