#include "StashExport.h"
#include "../../D2Helpers.h"
#include "../../D2Ptrs.h"
#include "../../BH.h"
#include "../../D2Stubs.h"
#include "../Item/ItemDisplay.h"
#include "../Item/Item.h"
#include "../../Constants.h"
#include "../GameSettings/GameSettings.h"
#include <algorithm>

map<std::string, std::unique_ptr<Mustache::AMustacheTemplate>> StashExport::MustacheTemplates;
UnitAny* StashExport::viewingUnit;

#define NAMEOF(statid) (AllStatList[statid]->name)

using namespace Drawing;

void StashExport::OnLoad() {
	/*
	LoadConfig();

	Drawing::Texthook* colored_text;
	settingsTab = new UITab("StashExport", BH::settingsUI);

	unsigned int x = 8;
	unsigned int y = 7;

	colored_text = new Drawing::Texthook(settingsTab, x, y, "Stash");

	y += 15;
	colored_text = new Drawing::Texthook(settingsTab, x, (y), "Export Gear");
	colored_text->SetColor(Gold);
	new Drawing::Keyhook(settingsTab, GameSettings::KeyHookOffset, y + 2, &App.stash.exportGear.hotkey, "");
	new Checkhook(settingsTab, x, (y += 15), &App.stash.includeEquipment.toggle.isEnabled, "Include Equipment");
	new Checkhook(settingsTab, x, (y += 15), &App.stash.exportOnMenu.toggle.isEnabled, "Export On Menu");

	// the MustacheTemplates will not be reloaded
	options.clear();
	options.push_back("json");

	int idx = 0;

	for (auto it = App.stash.mustacheFormat.values.cbegin(); it != App.stash.mustacheFormat.values.cend(); it++) {
		auto t = Mustache::parse(it->second);
		if (t) {
			idx++;
			if (App.stash.mustacheDefault.value.compare(it->first) == 0) {
				exportType = idx;
			}
			MustacheTemplates[it->first] = std::unique_ptr<Mustache::AMustacheTemplate>(t);
			// Only add "top level" options to the Export Type dropdown
			// Otherwise it quickly gets cluttered with mustacheFormat lines
			if (std::find(
				App.stash.mustacheOptions.values.begin(),
				App.stash.mustacheOptions.values.end(),
				it->first) != App.stash.mustacheOptions.values.end()
				)
			{
				options.push_back(it->first);
			}
		}
	}

	y += 20;
	colored_text = new Drawing::Texthook(settingsTab, x, y, "Export Type");
	new Combohook(settingsTab, x, (y += 15), 150, &exportType, options);
	*/
}

void StashExport::LoadConfig() {

}

void StashExport::OnUnload() {

}

void StashExport::OnLoop() {
}

bool checkFlag(UnitAny* pItem, DWORD flag) {
	return (pItem->pItemData->dwFlags & flag) > 0;
}

JSONObject* StashExport::getStatEntry(WORD statId, WORD statId2, DWORD statVal, DWORD min, DWORD max) {
	if (!statVal)
		return nullptr;
	int hi = statVal >> 8;
	int lo = statVal & 0xff;
	auto entry = new JSONObject();
	if (min != max) {
		auto range = new JSONObject();
		range->set("min", (int)min);
		range->set("max", (int)max);
		entry->set("range", range);
	}

	switch (statId) {
	case STAT_SINGLESKILL:
	case STAT_NONCLASSSKILL: {
		SkillsTxt* pSkillsTxt = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[statId2];
		SkillDescTxt* pSkillDescTxt = &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[pSkillsTxt->wSkillDesc];
		if (pSkillsTxt && pSkillDescTxt)
		{
			std::string skillName = UnicodeToAnsi(GetTblEntryByIndex(pSkillDescTxt->wStrName, TBLOFFSET_STRING));
			// Format skill bonus and skill name onto stat
			std::string name = string_format("+%d to %s", (int)statVal, skillName.c_str());
			entry->set("name", name);
			entry->set("skill", skillName);
			entry->set("value", (int)statVal);
		}
	}
						   break;
	case STAT_AURA: {
		SkillsTxt* pSkillsTxt = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[statId2];
		SkillDescTxt* pSkillDescTxt = &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[pSkillsTxt->wSkillDesc];
		if (pSkillsTxt && pSkillDescTxt)
		{
			std::string skillName = UnicodeToAnsi(GetTblEntryByIndex(pSkillDescTxt->wStrName, TBLOFFSET_STRING));
			// Format aura level and skill name onto stat
			std::string name = string_format(NAMEOF(statId), (int)statVal, skillName.c_str());
			entry->set("name", name);
		}
	}
				  break;
	case STAT_SKILLTAB:
		for (int j = 0; j < 21; j++) {
			if (SKILL_TABS[j].id == statId2) {
				entry->set("name", NAMEOF(statId));
				entry->set("skill", SKILL_TABS[j].name);
				entry->set("value", (int)statVal);
				break;
			}
		}
		break;
	case STAT_CLASSSKILLS:
		for (int j = 0; j < 7; j++) {
			if (CLASS_NAMES[j].id == statId2) {
				entry->set("name", NAMEOF(statId));
				entry->set("skill", CLASS_NAMES[j].name);
				entry->set("value", (int)statVal);
			}
		}
		break;
	case STAT_CHARGED: {
		int level = statId2 & 0x3F;
		statId2 >>= 6;	// skill Id needs to be bit-shifted
		SkillsTxt* pSkillsTxt = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[statId2];
		SkillDescTxt* pSkillDescTxt = &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[pSkillsTxt->wSkillDesc];
		if (pSkillsTxt && pSkillDescTxt)
		{
			std::string skillName = UnicodeToAnsi(GetTblEntryByIndex(pSkillDescTxt->wStrName, TBLOFFSET_STRING));
			// Format current/max charges onto stat
			std::string charges = string_format(NAMEOF(statId), lo, hi);
			entry->set("skill", skillName);
			entry->set("charges", charges);
			entry->set("level", level);
		}
	}
					 break;
	case STAT_SKILLONDEATH:
	case STAT_SKILLONHIT:
	case STAT_SKILLONKILL:
	case STAT_SKILLONLEVELUP:
	case STAT_SKILLONCAST:
	case STAT_SKILLONSTRIKING:
	case STAT_SKILLWHENSTRUCK: {
		int level = statId2 & 0x3F;
		statId2 >>= 6;	// skill Id needs to be bit-shifted
		SkillsTxt* pSkillsTxt = &(*p_D2COMMON_sgptDataTable)->pSkillsTxt[statId2];
		SkillDescTxt* pSkillDescTxt = &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[pSkillsTxt->wSkillDesc];
		if (pSkillsTxt && pSkillDescTxt)
		{
			std::string skillName = UnicodeToAnsi(GetTblEntryByIndex(pSkillDescTxt->wStrName, TBLOFFSET_STRING));
			// Format skill chance, skill level, and skill name onto stat
			std::string name = string_format(NAMEOF(statId), lo, level, skillName.c_str());
			entry->set("name", name);
			entry->set("chance", lo);
			entry->set("level", level);
		}
	}
							 break;
	default:
		switch (statId) {
		case STAT_HP:
		case STAT_MAXHP:
		case STAT_MANA:
		case STAT_MAXMANA:
		case STAT_STAMINA:
		case STAT_MAXSTAMINA:
		case STAT_LIFEPERLEVEL:
		case STAT_MANAPERLEVEL:
			statVal = hi;
			break;
		default:
			break;
		}
		entry->set("name", NAMEOF(statId));
		entry->set("stat_id", statId);
		entry->set("value", (int)statVal);
		break;
	}

	return entry;
}

void StashExport::fillStats(JSONArray* statsArray, ItemsTxtStat* itemDef, UnitAny* pItem, int maxProps)
{
	for (int prop = 0; prop < maxProps; prop++)
	{
		if (itemDef[prop].dwProp == -1) { break; }

		ItemsTxtStat itemStats = itemDef[prop];
		PropertiesTxt* pPropertiesTxt = &(*p_D2COMMON_sgptDataTable)->pPropertiesTxt[itemDef[prop].dwProp];
		if (!pPropertiesTxt) { break; }

		for (int stat = 0; stat < 8; stat++)
		{
			int func = pPropertiesTxt->nFunc[stat];
			// 'dmg%' doesn't have a stat because.. reasons
			if (func != 7 && pPropertiesTxt->wStat[stat] == 0xFFFF) { break; }

			auto statFunc = STAT_FUNCTIONS[1];
			if (func < MAX_FUNCTION) {
				statFunc = STAT_FUNCTIONS[func];
			}

			ItemStatCostTxt* pItemStatCostTxt = &(*p_D2COMMON_sgptDataTable)->pItemStatCostTxt[pPropertiesTxt->wStat[stat]];
			if (!pItemStatCostTxt) { break; }

			int statParam = 0;
			switch (func)
			{
			case 21:
				statParam = pPropertiesTxt->wVal[prop];
				break;
			default:
				statParam = itemStats.dwPar;
				break;
			}

			auto entry = statFunc(pItem, itemStats, pItemStatCostTxt, statParam);
			statsArray->add(entry);
		}
	}
}

void StashExport::GetItemInfo(UnitAny* pItem, JSONObject* pBuffer) {
	ItemsTxt* txt = D2COMMON_GetItemText(pItem->dwTxtFileNo);
	std::string type = UnicodeToAnsi(D2LANG_GetLocaleText(txt->wnamestr));
	// Remove hardcoded color
	type = MaybeStripColorPrefix(type);
	pBuffer->set("type", type);
	pBuffer->set("quality", std::string(QUALITY_NAMES[pItem->pItemData->dwQuality]));
	pBuffer->set("iLevel", (int)pItem->pItemData->dwItemLevel);

	for (int j = 0; j < 6; j++)
	{
		const auto location = ITEM_LOCATIONS[j];
		if (location.id != pItem->pItemData->ItemLocation) {
			continue;
		}

		if (location.id == STORAGE_NULL) {
			if (pItem->pItemData->NodePage == NODEPAGE_EQUIP) {
				pBuffer->set("location", std::string("Equip"));
			}

			break;
		}
		else {
			pBuffer->set("location", location.name);
		}

		break;
	}

	char cCode[] = {
		txt->szCode[0],
		txt->szCode[1],
		txt->szCode[2],
		txt->szCode[3],
		0
	};
	std::string code = cCode;
	code = Trim(code);

	int nCorruptor = ItemGetCorruptor(pItem, STAT_CORRUPTED);

	BYTE nType = txt->nType;
	if (nType >= 96 && nType <= 102) { // is gem?
		pBuffer->set("isGem", true);
	}
	else if (nType == 74) {	// is rune?
		pBuffer->set("isRune", true);
	}
	else {
		pBuffer->set("isEthereal", checkFlag(pItem, ITEM_ETHEREAL));
		pBuffer->set("unidentified", !checkFlag(pItem, ITEM_IDENTIFIED));
		pBuffer->set("sockets", (int)D2COMMON_GetUnitStat(pItem, STAT_SOCKETS, 0));
		pBuffer->set("defense", (int)D2COMMON_GetUnitStat(pItem, STAT_DEFENSE, 0));
		pBuffer->set("quantity", (int)D2COMMON_GetUnitStat(pItem, STAT_AMMOQUANTITY, 0));

		auto statsObject = new JSONArray();
		pBuffer->set("stats", statsObject);
		switch (pItem->pItemData->dwQuality) {
		case ITEM_QUALITY_MAGIC: {
			auto name = type;
			for (int i = 0; i < 3; i++) {
				const char* prefix = D2COMMON_GetItemMagicalMods(pItem->pItemData->wPrefix[i])->szName;
				const char* suffix = D2COMMON_GetItemMagicalMods(pItem->pItemData->wSuffix[i])->szName;
				if (prefix) {
					name = std::string(prefix) + " " + name;
				}
				if (suffix) {
					name = name + " " + suffix;
				}
			}

			// Remove hardcoded color
			name = MaybeStripColorPrefix(name);
			pBuffer->set("name", name);
		}
							   break;
		case ITEM_QUALITY_UNIQUE: {
			UniqueItemsTxt* pUniqueItemsTxt = &(*p_D2COMMON_sgptDataTable)->pUniqueItemsTxt[pItem->pItemData->dwFileIndex];
			if (pUniqueItemsTxt)
			{
				std::string name = pUniqueItemsTxt->szName;
				// Remove hardcoded color
				name = MaybeStripColorPrefix(name);
				pBuffer->set("name", name);
				fillStats(statsObject, pUniqueItemsTxt->hStats, pItem, 12);
			}
		}
								break;
		case ITEM_QUALITY_SET: {
			SetItemsTxt* pSetItemsTxt = &(*p_D2COMMON_sgptDataTable)->pSetItemsTxt[pItem->pItemData->dwFileIndex];
			SetsTxt* pSetsTxt = &(*p_D2COMMON_sgptDataTable)->pSetsTxt[pSetItemsTxt->nSetId];
			if (pSetItemsTxt && pSetsTxt)
			{
				std::string itemName = pSetItemsTxt->szName;
				std::string setName = UnicodeToAnsi(GetTblEntryByIndex(pSetsTxt->wStringId, TBLOFFSET_STRING));
				// Remove hardcoded color
				itemName = MaybeStripColorPrefix(itemName);
				pBuffer->set("set", setName);
				pBuffer->set("name", itemName);
				fillStats(statsObject, pSetItemsTxt->hStats, pItem, 9);
			}
		}
							 break;
		case ITEM_QUALITY_CRAFT:
		case ITEM_QUALITY_RARE: {
			RareAffixTxt* rarePrefix = D2COMMON_10365_DATATBLS_GetRareAffixTxtRecord(pItem->pItemData->wRarePrefix);
			RareAffixTxt* rareSuffix = D2COMMON_10365_DATATBLS_GetRareAffixTxtRecord(pItem->pItemData->wRareSuffix);

			if (rarePrefix && rareSuffix)
			{
				std::string rarePrefixName = UnicodeToAnsi(GetTblEntryByIndex(rarePrefix->wStringId, TBLOFFSET_STRING));
				std::string rareSuffixName = UnicodeToAnsi(GetTblEntryByIndex(rareSuffix->wStringId, TBLOFFSET_STRING));
				pBuffer->set("name", rarePrefixName + " " + rareSuffixName);
			}
		}
							  break;
		default:
			break;
		}

		if (checkFlag(pItem, ITEM_RUNEWORD)) {
			pBuffer->set("isRuneword", true);
			std::string rwName = UnicodeToAnsi(D2LANG_GetLocaleText(pItem->pItemData->wPrefix[0]));
			pBuffer->set("runeword", rwName);

			RunesTxt* pRune = D2COMMON_ITEMS_GetRunesTxtRecordFromItem(pItem);
			if (pRune)
			{
				fillStats(statsObject, pRune->hStats, pItem, 7);
			}
		}
		//else { //if (pItem->pItemData->dwQuality != ITEM_QUALITY_UNIQUE &&
			//pItem->pItemData->dwQuality != ITEM_QUALITY_SET) {
		DWORD value = 0;
		Stat* aStatList = new Stat[STAT_MAX];
		StatList* pStatList = D2COMMON_GetStatList(pItem, NULL, STAT_MAX);
		if (pStatList) {
			DWORD dwStats = D2COMMON_CopyStatList(pStatList, aStatList, STAT_MAX);
			for (UINT i = 0; i < dwStats; i++) {
				auto entry = getStatEntry(
					aStatList[i].wStatIndex,
					aStatList[i].wSubIndex,
					aStatList[i].dwStatValue,
					0, 0);
				if (entry->get("name")->toString() == "corruptor") {
					continue;
				}
				BOOL hasStat = FALSE;
				BOOL isCorrupted = StatIsCorrupted(aStatList[i].wStatIndex, nCorruptor);
				for (int i = 0; i < statsObject->length(); i++) {
					JSONObject* ent = statsObject->getObject(i);
					if (ent->equals(entry)) {
						ent->set("corrupted", isCorrupted);
						hasStat = TRUE;
						break;
					}
					else if (ent->get("name")->equals(entry->get("name")))
					{
						if (ent->find("skill")->toString() == "null" || ent->get("skill")->equals(entry->get("skill")))
						{
							int val1 = ent->get("value")->toInt();
							int val2 = entry->get("value")->toInt();
							if (val1 >= val2)
							{
								hasStat = TRUE;
								break;
							}
							else
							{
								hasStat = TRUE;
								ent->set("value", val2);
								ent->set("corrupted", isCorrupted);
								break;
							}
						}
					}
					else if (ent->get("name")->toString() == "dmg%" && (entry->get("name")->toString() == "item_maxdamage_percent" || entry->get("name")->toString() == "item_mindamage_percent"))
					{
						ent->set("corrupted", isCorrupted);
						hasStat = TRUE;
						break;
					}
				}
				if (!hasStat) {
					entry->set("corrupted", isCorrupted);
					statsObject->add(entry);
				}
			}
		}
		delete[] aStatList;
		//}

		// Get items in the sockets
		if (pBuffer->getNumber("sockets") > 0) {
			JSONArray* sockets = new JSONArray();
			for (UnitAny* sItem = pItem->pInventory->pFirstItem; sItem; sItem = sItem->pItemData->pNextInvItem) {
				JSONObject* sock = new JSONObject();
				GetItemInfo(sItem, sock);
				sockets->add(sock);
			}
			pBuffer->set("socketed", sockets);
		}
	}
}

void StashExport::WriteStash() {
	BnetData* pInfo = (*p_D2LAUNCH_BnData);

	if (!IsInitialized()) {
		return;
	}

	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit) return;

	// Make sure the directory exists
	CreateDirectory((BH::path + "\\stash\\").c_str(), NULL);

	std::string account = pInfo->szAccountName;
	std::string path = BH::path + "\\stash\\" + (account == "" ? "SinglePlayer" : account) + "_" + unit->pPlayerData->szName + ".txt";
	fstream file(path, std::ofstream::out | std::ofstream::trunc);
	if (!file.is_open()) {
		PrintText(1, "Failed to open %s for writing", path.c_str());
		return;
	}

	auto data = new JSONArray;
	std::map<JSONObject*, unsigned int> objCounts;
	for (UnitAny* pItem = unit->pInventory->pFirstItem; pItem; pItem = pItem->pItemData->pNextInvItem) {
		if (pItem->pItemData->NodePage == NODEPAGE_EQUIP &&
			!App.stash.includeEquipment.toggle.isEnabled) {
			continue;
		}
		// TODO: Add another switch here for "includeStash" (and maybe even includeInventory)
		// So we can export equipped items only

		auto stash = new JSONObject();
		GetItemInfo(pItem, stash);
		auto name = stash->getString("type");
		if (name.find("Potion") == std::string::npos
			&& name.find("Scroll") == std::string::npos
			&& name.find("Tome") == std::string::npos) { // skip potions & scrolls in the output

			auto item = (JSONObject*)data->contains(stash);
			if (item) {
				// Item already found, track the count
				if (objCounts[item]) {
					objCounts[item]++;
				}
				else {
					objCounts[item] = 2u;
				}
			}
			else {
				data->add(stash);
			}
		}
	}
	for (auto it = objCounts.begin(); it != objCounts.end(); it++) {
		it->first->set("count", (int)it->second);
	}

	std::string buffer;
	if (exportType == 0 /* text */) {
		JSONWriter writer(buffer, SER_OPT_FORMATTED);
		data->serialize(writer);
	}
	else { /* text */
		std::string tmpltName = options.at(exportType);
		if (MustacheTemplates.find(tmpltName) != MustacheTemplates.end()) {
			buffer = Mustache::renderTemplate(MustacheTemplates[tmpltName].get(), Mustache::Context(data, [](std::string name) -> Mustache::AMustacheTemplate* {
				auto tmpl = MustacheTemplates.find(name);
				if (tmpl != MustacheTemplates.end()) {
					return tmpl->second.get();
				}

				return nullptr;
				}));
		}
	}

	file.write(buffer.c_str(), buffer.length());
	delete data;
	PrintText(White, "Exported stash to: %s", path.c_str());
}

void StashExport::CopyItemJSONToClipboard() {
	if (!IsInitialized()) {
		return;
	}

	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit) return;

	//UnitAny* pItem = D2CLIENT_GetCursorItem();
	UnitAny* pItem = (*p_D2CLIENT_HoverItem);
	if (!pItem) return;

	auto stash = new JSONObject();
	bool bIncludeEquipment = App.stash.includeEquipment.toggle.isEnabled;

	App.stash.includeEquipment.toggle.isEnabled = true;
	GetItemInfo(pItem, stash);
	App.stash.includeEquipment.toggle.isEnabled = bIncludeEquipment;

	std::string buffer;
	JSONWriter writer(buffer, SER_OPT_FORMATTED);
	stash->serialize(writer);

	const char* clipBuffer = buffer.c_str();
	int len = buffer.length() + 1;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), clipBuffer, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();

	delete stash;
	PrintText(White, "Copied item to clipboard");
}

void StashExport::OnKey(bool up, BYTE key, LPARAM lParam, bool* block) {
	bool ctrlState = ((GetKeyState(VK_LCONTROL) & 0x80) || (GetKeyState(VK_RCONTROL) & 0x80));
	if (key == App.stash.exportGear.hotkey) {
		*block = true;
		if (up)
			return;
		WriteStash();
	}
	else if (key == VK_ESCAPE) {
		if (up &&
			App.stash.exportOnMenu.toggle.isEnabled &&
			D2CLIENT_GetUIState(UI_ESCMENU_MAIN)) {
			WriteStash();
		}
	}
	// Control + C
	else if (key == 67 && ctrlState) {
		*block = true;
		if (!up)
			return;
		CopyItemJSONToClipboard();
	}
}

static JSONObject* INVALID_FUNCTION(UnitAny* pItem, ItemsTxtStat itemStats, ItemStatCostTxt* pItemStatCostTxt, int param)
{
	return nullptr;
}

static JSONObject* DEFAULT_FUNCTION(UnitAny* pItem, ItemsTxtStat itemStats, ItemStatCostTxt* pItemStatCostTxt, int param)
{
	auto statLayer = param ? param : 0;
	int statId = pItemStatCostTxt->wStat;
	StatList* pStatList = D2COMMON_GetStatList(pItem, 0, STAT_MAX);
	int statVal = D2COMMON_GetStatValueFromStatList(pStatList, statId, statLayer);

	if (checkFlag(pItem, ITEM_RUNEWORD))
	{
		StatList* pStatList = D2COMMON_GetStateStatList(pItem, STATE_RUNEWORD);
		statVal += D2COMMON_GetStatValueFromStatList(pStatList, statId, statLayer);
	}

	if (statVal) {
		auto entry = StashExport::getStatEntry(
			statId,
			statLayer,
			statVal,
			itemStats.dwMin,
			itemStats.dwMax);
		return entry;
	}

	return nullptr;
}

static JSONObject* DAMAGE_PERCENT_FUNCTION(UnitAny* pItem, ItemsTxtStat itemStats, ItemStatCostTxt* pItemStatCostTxt, int param)
{
	auto entry = new JSONObject();
	std::string name = "% Enhanced Damage";
	StatList* pStatList = D2COMMON_GetStatList(pItem, 0x00, STAT_MAX);
	int statVal = D2COMMON_GetStatValueFromStatList(pStatList, STAT_ENHANCEDMINIMUMDAMAGE, 0);
	if (checkFlag(pItem, ITEM_RUNEWORD))
	{
		StatList* pStatList = D2COMMON_GetStateStatList(pItem, STATE_RUNEWORD);
		statVal += D2COMMON_GetStatValueFromStatList(pStatList, STAT_ENHANCEDMINIMUMDAMAGE, param);
	}

	entry->set("name", name);
	entry->set("value", statVal);
	auto range = new JSONObject();
	range->set("min", (int)itemStats.dwMin);
	range->set("max", (int)itemStats.dwMax);
	entry->set("range", range);
	return entry;
}

std::function<JSONObject* (UnitAny*, ItemsTxtStat, ItemStatCostTxt*, int)> STAT_FUNCTIONS[] = {
	&INVALID_FUNCTION,			// Invalid function
	&DEFAULT_FUNCTION,			// Func 1
	&DEFAULT_FUNCTION,			// Func 2
	&DEFAULT_FUNCTION,			// Func 3
	&DEFAULT_FUNCTION,			// Func 4
	&DEFAULT_FUNCTION,			// Func 5
	&DEFAULT_FUNCTION,			// Func 6
	&DAMAGE_PERCENT_FUNCTION,	// Func 7
	&DEFAULT_FUNCTION,			// Func 8
	&DEFAULT_FUNCTION,			// Func 9
	&DEFAULT_FUNCTION,			// Func 10  // was SKILL_TAB_FUNCTION
	&DEFAULT_FUNCTION,			// Func 11  // was SKILL_ON_X_FUNCTION
	&DEFAULT_FUNCTION,			// Func 12
	&DEFAULT_FUNCTION,			// Func 13
	&DEFAULT_FUNCTION,			// Func 14
	&DEFAULT_FUNCTION,			// Func 15  // DAMAGE_RANGE_FUNCTION
	&DEFAULT_FUNCTION,			// Func 16
	&DEFAULT_FUNCTION,			// Func 17
	&DEFAULT_FUNCTION,			// Func 18
	&DEFAULT_FUNCTION,			// Func 19  // was CHARGED_FUNCTION
	&DEFAULT_FUNCTION,			// Func 20
	&DEFAULT_FUNCTION,			// Func 21  // was CLASS_SKILL_FUNCTION
	&DEFAULT_FUNCTION
};

char* QUALITY_NAMES[] = {
	"None",
	"Inferior",
	"Normal",
	"Superior",
	"Magic",
	"Set",
	"Rare",
	"Unique",
	"Crafted"
};

IdNamePair ITEM_LOCATIONS[6] = {
	{ STORAGE_INVENTORY, "Inventory" },
	{ STORAGE_EQUIP, "Equip" },
	{ STORAGE_BELT, "Belt" },
	{ STORAGE_CUBE, "Cube" },
	{ STORAGE_STASH, "Stash" },
	{ STORAGE_NULL, "Null" }
};

IdNamePair NODE_PAGES[4] = {
	{ 0, "Unknown" },
	{ NODEPAGE_STORAGE, "Storage" },
	{ NODEPAGE_BELTSLOTS, "Beltslots" },
	{ NODEPAGE_EQUIP, "Equiped" }
};

IdNamePair CLASS_NAMES[7] = {
	{ 0, "Amazon Skills" },
	{ 1, "Sorceress Skills" },
	{ 2, "Necromancer Skills" },
	{ 3, "Paladin Skills" },
	{ 4, "Barbarian Skills" },
	{ 5, "Druid Skills" },
	{ 6, "Assassin Skills" }
};

IdNamePair SKILL_TABS[21] = {
	{ 0, "Bow and Crossbow Skills (Amazon)" },
	{ 1, "Passive and Magic Skills (Amazon)" },
	{ 2, "Javalin and Spear Skills (Amazon)" },
	{ 8, "Fire Skills (Sorceress)" },
	{ 9, "Lightning Skills (Sorceress)" },
	{ 10, "Cold Skills (Sorceress)" },
	{ 16, "Curses Skills (Necromancer)" },
	{ 17, "Poison and Bone Skills (Necromancer)" },
	{ 18, "Summoning Skills (Necromancer)" },
	{ 24, "Combat Skills (Paladin)" },
	{ 25, "Offensive Auras (Paladin)" },
	{ 26, "Defensive Auras (Paladin)" },
	{ 32, "Combat Skills (Barbarian)" },
	{ 33, "Masteries (Barbarian)" },
	{ 34, "Warcries (Barbarian)" },
	{ 40, "Summoning (Druid)" },
	{ 41, "Shapeshifting (Druid)" },
	{ 42, "Elemental (Druid)" },
	{ 48, "Traps (Assassin)" },
	{ 49, "Shadow Desciplines (Assassin)" },
	{ 50, "Martial Arts (Assassin)" },
};
