#include "StatsDisplay.h"
#include "../Basic/Texthook/Texthook.h"
#include "../Basic/Framehook/Framehook.h"
#include "../Basic/Boxhook/Boxhook.h"
#include "../../D2Ptrs.h"
#include "../../BH.h"
#include "../../D2Helpers.h"
#include "../../Modules/Item/Item.h"

using namespace Drawing;

std::vector<int> FHRBreakpoints[] = {
	{6 , 13 , 20 , 32 , 52 , 86 , 174 , 600},
	{5 , 9 , 14 , 20 , 30 , 42 , 60 , 86 , 142 , 280},
	{5 , 10 , 16 , 26 , 39 , 56 , 86 , 152 , 377},
	{7 , 15 , 27 , 48 , 86 , 200 , 4680},
	{7 , 15 , 27 , 48 , 86 , 200 , 4680},
	{5 , 10 , 16 , 26 , 39 , 56 , 86 , 152 , 377},
	{7 , 15 , 27 , 48 , 86 , 200 , 4680}
};

std::vector<int> FCRBreakpoints[] = {
	{7 , 14 , 22 , 32 , 48 , 68 , 99 , 152},
	{9 , 20 , 37 , 63 , 105 , 200},
	{9 , 18 , 30 , 48 , 75 , 125},
	{9 , 18 , 30 , 48 , 75 , 125},
	{9 , 20 , 37 , 63 , 105 , 200},
	{4 , 10 , 19 , 30 , 46 , 68 , 99 , 163},
	{8 , 16 , 27 , 42 , 65 , 102 , 174}
};

std::vector<int> SorcLightningFCRBreakpoints = { 7 , 15 , 23 , 35 , 52 , 78 , 117 , 194 };
std::vector<int> WolfFHRBreakpoints = { 9 , 20 , 42 , 86 , 280 };
std::vector<int> BearFHRBreakpoints = { 5 , 10 , 16 , 24 , 37 , 54 , 86 , 152, 360 };

DWORD syncCooldown = 0;

StatsDisplay* StatsDisplay::display;

StatsDisplay::StatsDisplay(std::string name) {
	int yPos = 10;
	int width = 274;

	InitializeCriticalSection(&crit);
	SetY(yPos);
	SetXSize(width);

	LoadConfig();

	SetName(name);
	SetActive(true);
	SetMinimized(true);

	//BH::config->ReadKey("Character Stats", "VK_8", statsKey);
	display = this;
}

StatsDisplay::~StatsDisplay() {
	Lock();
	// Remove all hooks associated with the display
	while (Hooks.size() > 0) {
		delete (*Hooks.begin());
	}
	Unlock();
	DeleteCriticalSection(&crit);
}

void StatsDisplay::LoadConfig() {
	int height = 342 + 8 * 5 + 16 * 10;
	customStats.clear();

	BH::config->ReadToggle("Stats on Right", "None", false, Toggles["Stats on Right"]);

	vector<pair<string, string>> stats;
	BH::config->ReadMapList("Stat Screen", stats);
	for (unsigned int i = 0; i < stats.size(); i++) {
		std::transform(stats[i].first.begin(), stats[i].first.end(), stats[i].first.begin(), ::tolower);
		if (StatMap.count(stats[i].first) > 0) {
			StatProperties* sp = StatMap[stats[i].first];
			DisplayedStat* customStat = new DisplayedStat();
			customStat->name = stats[i].first;
			customStat->useValue = false;
			std::transform(customStat->name.begin(), customStat->name.end(), customStat->name.begin(), ::tolower);
			// Getting rid of the check for sp->saveParamBits > 0 to display weapon mastery values
			// if a param is supplied it will be used
			int num = -1;
			stringstream ss(Trim(stats[i].second));
			if (!(ss >> num).fail() && num > 0) {
				customStat->useValue = true;
				customStat->value = num;
			}
			customStats.push_back(customStat);
		}
	}
	if (customStats.size() > 0) {
		height += (customStats.size() * 16) + 8;
	}

	int xPos = Toggles["Stats on Right"].state ?
		*p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
	SetX(xPos);
	SetYSize(height);
}

void StatsDisplay::SetX(unsigned int newX) {
	if (newX >= 0 && newX <= Hook::GetScreenWidth()) {
		Lock();
		x = newX;
		Unlock();
	}
}

void StatsDisplay::SetY(unsigned int newY) {
	if (newY >= 0 && newY <= Hook::GetScreenHeight()) {
		Lock();
		y = newY;
		Unlock();
	}
}

void StatsDisplay::SetXSize(unsigned int newXSize) {
	if (newXSize >= 0 && newXSize <= (Hook::GetScreenWidth() - GetX())) {
		Lock();
		xSize = newXSize;
		Unlock();
	}
}

void StatsDisplay::SetYSize(unsigned int newYSize) {
	if (newYSize >= 0 && newYSize <= (Hook::GetScreenHeight() - GetY())) {
		Lock();
		ySize = newYSize;
		Unlock();
	}
}

bool StatsDisplay::InRange(unsigned int x, unsigned int y) {
	return IsActive() &&
		x >= GetX() && y >= GetY() &&
		x <= GetX() + GetXSize() && y <= GetY() + GetYSize();
}

void StatsDisplay::Draw() {
	display->Lock();
	display->OnDraw();
	display->Unlock();
}

void StatsDisplay::OnDraw() {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	bool isMerc = false;
	if (!unit)
		return;
	int column1 = GetX() + 5;
	int column2 = column1 + GetXSize() / 2;

	if (!IsMinimized()) {
		if (D2CLIENT_GetUIState(UI_MERC)) {
			unit = D2CLIENT_GetMercUnit();
			isMerc = true;
		}
		for (std::list<Hook*>::iterator it = Hooks.begin(); it != Hooks.end(); it++)
			(*it)->OnDraw();

		int y = GetY();
		RECT pRect;
		pRect.left = GetX();
		pRect.top = y;
		pRect.right = x + GetXSize();
		pRect.bottom = y + GetYSize();
		Drawing::Boxhook::Draw(GetX(), GetY(), GetXSize(), GetYSize(), White, Drawing::BTBlack);

		Drawing::Framehook::DrawRectStub(&pRect);

		Texthook::Draw(column1, (y += 8), None, 6, Gold,
			"Name:ÿc0 %s",
			isMerc ? "ÿc;Mercenary" : unit->pPlayerData->szName);
		Texthook::Draw(pRect.right - 5, y, Right, 6, Gold,
			L"Level:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_LEVEL, 0));
		Texthook::Draw(pRect.right - 5, y + 12, Right, 6, Gold,
			L"Additional XP:ÿc: %d%%",
			(int)D2COMMON_GetUnitStat(unit, STAT_ADDEXPERIENCE, 0));

		y += 8;

		BnetData* pData = (*p_D2LAUNCH_BnData);
		int xPacMultiplier = pData->nCharFlags & PLAYER_TYPE_EXPANSION ? 2 : 1;
		int resPenalty[3] = { RES_PENALTY_CLS_NORM, RES_PENALTY_CLS_NM, RES_PENALTY_CLS_HELL };
		int penalty = resPenalty[D2CLIENT_GetDifficulty()] * xPacMultiplier;
		int fMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXFIRERESIST, 0) + 75;
		int cMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXCOLDRESIST, 0) + 75;
		int lMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXLIGHTNINGRESIST, 0) + 75;
		int pMax = (int)D2COMMON_GetUnitStat(unit, STAT_MAXPOISONRESIST, 0) + 75;
		int pLengthReduce = (int)D2COMMON_GetUnitStat(unit, STAT_POISONLENGTHREDUCTION, 0);

		Texthook::Draw(column1, (y += 16), None, 6, Red, L"ÿc4Fire Resist:ÿc1 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_FIRERESIST, 0) + penalty, fMax);
		Texthook::Draw(column1, (y += 16), None, 6, Blue, L"ÿc4Cold Resist:ÿc3 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_COLDRESIST, 0) + penalty, cMax);
		Texthook::Draw(column1, (y += 16), None, 6, Yellow, L"ÿc4Lightning Resist:ÿc9 %d ÿc0/ %d", (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGRESIST, 0) + penalty, lMax);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Poison Resist:ÿc2 %d ÿc0/ %d  ÿc4Length:ÿc: %d%%",
			(int)D2COMMON_GetUnitStat(unit, STAT_POISONRESIST, 0) + penalty,
			pMax,
			(100 - penalty - pLengthReduce)
		);
		Texthook::Draw(column1, (y += 16), None, 6, Gold, L"Curse Resist: %d%%", (int)D2COMMON_GetUnitStat(unit, STAT_CURSERESISTANCE, 0) + penalty);
		y += 8;

		int fAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_FIREABSORB, 0);
		int fAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_FIREABSORBPERCENT, 0);
		int cAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_COLDABSORB, 0);
		int cAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_COLDABSORBPERCENT, 0);
		int lAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORB, 0);
		int lAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORBPERCENT, 0);
		int mAbsorb = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICABSORB, 0);
		int mAbsorbPct = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICABSORBPERCENT, 0);
		Texthook::Draw(column1, (y += 16), None, 6, Red, L"ÿc4Absorption: ÿc1%dÿc0/ÿc1%d%c ÿc3%dÿc0/ÿc3%d%c ÿc9%dÿc0/ÿc9%d%c ÿc8%dÿc0/ÿc8%d%c", fAbsorb, fAbsorbPct, '%', cAbsorb, cAbsorbPct, '%', lAbsorb, lAbsorbPct, '%', mAbsorb, mAbsorbPct, '%');

		int dmgReduction = (int)D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTION, 0);
		int dmgReductionPct = (int)D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTIONPCT, 0);
		int magReduction = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTION, 0);
		int magReductionPct = (int)D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTIONPCT, 0);
		Texthook::Draw(column1, (y += 16), None, 6, Tan, L"ÿc4Damage Reduction: ÿc7%dÿc0/ÿc7%d%c ÿc8%dÿc0/ÿc8%d%c", dmgReduction, dmgReductionPct, '%', magReduction, magReductionPct, '%');
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Attacker Takes Damage:ÿc0 %d ÿc9 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_ATTACKERTAKESDAMAGE, 0), (int)D2COMMON_GetUnitStat(unit, STAT_ATTACKERTAKESLTNGDMG, 0));
		y += 8;

		int fMastery = (int)D2COMMON_GetUnitStat(unit, STAT_FIREMASTERY, 0);
		int cMastery = (int)D2COMMON_GetUnitStat(unit, STAT_COLDMASTERY, 0);
		int lMastery = (int)D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGMASTERY, 0);
		int pMastery = (int)D2COMMON_GetUnitStat(unit, STAT_POISONMASTERY, 0);
		int mMastery = (int)D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICDMGMASTERY, 0);

		int fPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYFIRERESREDUC, 0);
		int cPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYCOLDRESREDUC, 0);
		int lPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYLIGHTNRESREDUC, 0);
		int pPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PSENEMYPSNRESREDUC, 0);
		int mPierce = (int)D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICRESREDUC, 0);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Elemental Mastery:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
			fMastery, cMastery, lMastery, pMastery, mMastery);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Elemental Pierce:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
			fPierce, cPierce, lPierce, pPierce, mPierce);
		int classNum = pData->nCharClass;
		auto classArMod = CharList[classNum]->toHitFactor - 35;
		int dexAR = (int)D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0) * 5 + classArMod;
		int gearAR = (int)D2COMMON_GetUnitStat(unit, STAT_ATTACKRATING, 0);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Base AR:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0% dÿc5 total:ÿc0 %d",
			dexAR, gearAR, dexAR + gearAR);

		int gearDef = (int)D2COMMON_GetUnitStat(unit, STAT_DEFENSE, 0);
		int dexDef = (int)D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0) / 4;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Base Def:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0 %dÿc5 total:ÿc0 %d",
			dexDef, gearDef, dexDef + gearDef);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Base Damage:ÿc5 1h:ÿc0 %d-%dÿc5 2h:ÿc0 %d-%d",
			(int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMDAMAGE, 0),
			(int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMDAMAGE, 0),
			(int)D2COMMON_GetUnitStat(unit, STAT_SECONDARYMINIMUMDAMAGE, 0),
			(int)D2COMMON_GetUnitStat(unit, STAT_SECONDARYMAXIMUMDAMAGE, 0));

		y += 8;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Cast Rate:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_FASTERCAST, 0)
		);
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Block Rate:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_FASTERBLOCK, 0)
		);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Hit Recovery:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_FASTERHITRECOVERY, 0)
		);
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Run/Walk:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_FASTERRUNWALK, 0)
		);
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Attack Rate:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_ATTACKRATE, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
			L"IAS:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_IAS, 0));
		y += 8;

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			"Breakpoints:");

		if (!isMerc)
		{
			char bpString[255] = "";
			GetBreakpointString(unit, STAT_FASTERHITRECOVERY, FHRBreakpoints[unit->dwTxtFileNo], (char*)&bpString);
			Texthook::Draw(column1, (y += 16), None, 6, Gold,
				"FHR:ÿc0 %s",
				bpString);

			// Wolf form
			if (D2COMMON_GetUnitState(unit, 139)) {
				GetBreakpointString(unit, STAT_FASTERHITRECOVERY, WolfFHRBreakpoints, (char*)&bpString);
				Texthook::Draw(column1, (y += 16), None, 6, Gold,
					"Wolf FHR:ÿc0 %s",
					bpString);
			}

			// Bear form
			if (D2COMMON_GetUnitState(unit, 140)) {
				GetBreakpointString(unit, STAT_FASTERHITRECOVERY, BearFHRBreakpoints, (char*)&bpString);
				Texthook::Draw(column1, (y += 16), None, 6, Gold,
					"Bear FHR:ÿc0 %s",
					bpString);
			}

			GetBreakpointString(unit, STAT_FASTERCAST, FCRBreakpoints[unit->dwTxtFileNo], (char*)&bpString);
			Texthook::Draw(column1, (y += 16), None, 6, Gold,
				"FCR:ÿc0 %s",
				bpString);
			if (unit->dwTxtFileNo == CLASS_SOR) {
				GetBreakpointString(unit, STAT_FASTERCAST, SorcLightningFCRBreakpoints, (char*)&bpString);
				Texthook::Draw(column1, (y += 16), None, 6, Gold,
					"Lightning FCR:ÿc0 %s",
					bpString);
			}
			y += 8;
		}

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Crushing Blow:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_CRUSHINGBLOW, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Open Wounds: ÿc0%d",
			(int)D2COMMON_GetUnitStat(unit, STAT_OPENWOUNDS, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Deadly Strike:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_DEADLYSTRIKE, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Critical Strike: ÿc0%d",
			(int)D2COMMON_GetUnitStat(unit, STAT_CRITICALSTRIKE, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Life Leech:ÿc1 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_LIFELEECH, 0));
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Mana Leech:ÿc3 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_MANALEECH, 0));
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Projectile Pierce:ÿc0 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_PIERCINGATTACK, 0) +
			(int)D2COMMON_GetUnitStat(unit, STAT_PIERCE, 0));

		int minFire = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMFIREDAMAGE, 0);
		int maxFire = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMFIREDAMAGE, 0);
		int minLight = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMLIGHTNINGDAMAGE, 0);
		int maxLight = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMLIGHTNINGDAMAGE, 0);
		int minCold = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMCOLDDAMAGE, 0);
		int maxCold = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMCOLDDAMAGE, 0);
		int minPoison = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMPOISONDAMAGE, 0);
		int maxPoison = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMPOISONDAMAGE, 0);
		int poisonLength = (int)D2COMMON_GetUnitStat(unit, STAT_POISONDAMAGELENGTH, 0);
		int poisonLengthOverride = (int)D2COMMON_GetUnitStat(unit, STAT_SKILLPOISONOVERRIDELEN, 0);
		if (poisonLengthOverride > 0) {
			poisonLength = poisonLengthOverride;
		}
		int minMagic = (int)D2COMMON_GetUnitStat(unit, STAT_MINIMUMMAGICALDAMAGE, 0);
		int maxMagic = (int)D2COMMON_GetUnitStat(unit, STAT_MAXIMUMMAGICALDAMAGE, 0);
		int addedPhys = (int)D2COMMON_GetUnitStat(unit, STAT_ADDSDAMAGE, 0);
		y += 8;
		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Added Damage:ÿc0 %d",
			addedPhys);
		Texthook::Draw(column2, y, None, 6, Orange,
			"%d-%d",
			minMagic, maxMagic);
		Texthook::Draw(column1, (y += 16), None, 6, Red,
			"%d-%d",
			minFire, maxFire);
		Texthook::Draw(column2, y, None, 6, Blue,
			"%d-%d",
			minCold, maxCold);
		Texthook::Draw(column1, (y += 16), None, 6, Yellow,
			"%d-%d",
			minLight, maxLight);
		Texthook::Draw(column2, y, None, 6, Green,
			"%d-%d over %.1fs",
			(int)(minPoison / 256.0 * poisonLength),
			(int)(maxPoison / 256.0 * poisonLength),
			poisonLength / 25.0);

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Magic Find:ÿc3 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_MAGICFIND, 0)
		);
		Texthook::Draw(column2, y, None, 6, Gold,
			L"Gold Find:ÿc9 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_GOLDFIND, 0));

		Texthook::Draw(column1, (y += 16), None, 6, Gold,
			L"Stash Gold:ÿc9 %d",
			(int)D2COMMON_GetUnitStat(unit, STAT_GOLDBANK, 0));

		//int cowKingKilled = D2COMMON_GetQuestFlag(D2CLIENT_GetQuestInfo(), 4, 10);
		//Texthook::Draw(column2, y, None, 6, Gold,
		//		L"Cow King:ÿc0 %s", cowKingKilled ? L"killed" : L"alive");

		if (customStats.size() > 0) {
			y += 8;
			for (unsigned int i = 0; i < customStats.size(); i++) {
				int secondary = customStats[i]->useValue ? customStats[i]->value : 0;
				int stat = (int)D2COMMON_GetUnitStat(unit, STAT_NUMBER(customStats[i]->name), secondary);
				if (secondary > 0) {
					Texthook::Draw(column1, (y += 16), None, 6, Gold, "%s[%d]:ÿc0 %d",
						customStats[i]->name.c_str(), secondary, stat);
				}
				else {
					Texthook::Draw(column1, (y += 16), None, 6, Gold, "%s:ÿc0 %d",
						customStats[i]->name.c_str(), stat);
				}
			}
		}
	}
}

bool StatsDisplay::KeyClick(bool bUp, BYTE bKey, LPARAM lParam) {
	display->Lock();
	bool block = display->OnKey(bUp, bKey, lParam);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnKey(bool up, BYTE kkey, LPARAM lParam) {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;
	Item* item = (Item*)BH::moduleManager->Get("item");
	//Resync
	if (!up && kkey == item->resyncKey) {
		DWORD curTime = GetTickCount64();
		if (curTime >= syncCooldown) {
			SendSyncMsg();
			syncCooldown = curTime + 1000;
		}
	}
	if (IsMinimized()) {
		if (!up && kkey == item->advStatMenuKey) {
			LoadConfig();
			SetMinimized(false);
			return true;
		}
	}
	else {
		if (!up && (kkey == item->advStatMenuKey || kkey == VK_ESCAPE)) {
			SetMinimized(true);
			return true;
		}
	}
	return false;
}

bool StatsDisplay::Click(bool up, unsigned int mouseX, unsigned int mouseY) {
	display->Lock();
	bool block = display->OnClick(up, mouseX, mouseY);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnClick(bool up, unsigned int x, unsigned int y) {
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;

	if (!IsMinimized() && InRange(x, y)) {
		SetMinimized(true);
		return true;
	}
	return false;
}

void StatsDisplay::GetBreakpointString(UnitAny* unit, int nStat, std::vector<int> bps, char* bpCharString)
{
	std::string bpString = "";
	int bp = (int)D2COMMON_GetUnitStat(unit, nStat, 0);
	for (int i = 0; i < bps.size() - 1; i++) {
		if (bp >= bps[i] && bp < bps[i + 1]) {
			bpString += "ÿc8";
			bpString += std::to_string(bps[i]);
			bpString += "ÿc0 / ";
		}
		else {
			bpString += std::to_string(bps[i]) + " / ";
		}
	}
	if (bp >= bps[bps.size() - 1]) {
		bpString += "ÿc8";
	}
	bpString += std::to_string(bps[bps.size() - 1]);

	sprintf(bpCharString, "%.255s", bpString.c_str());
}
