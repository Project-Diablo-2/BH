#include "StatsDisplay.h"

#include "../../BH.h"
#include "../../D2Helpers.h"
#include "../../D2Ptrs.h"

#include "../../Modules/Item/Item.h"

#include "../Basic/Boxhook/Boxhook.h"
#include "../Basic/Framehook/Framehook.h"
#include "../Basic/Texthook/Texthook.h"
#include "../../Modules/GameSettings/GameSettings.h"

#include <set>

using namespace Drawing;

const int kWolfTxtFileNoAlias = 139;
const int kBearTxtFileNoAlias = 140;
const int kDruidOneHandFHRAlias = 141;
const int kPaladinSpearStaveFHRAlias = 142;
const int kSorceressLightningFCRAlias = 143;

std::map<DWORD, std::vector<int>> faster_hit_recovery_frames = {
	{ CLASS_AMA, { 6, 13, 20, 32, 52, 86, 174, 600 } },
	{ CLASS_SOR, { 5, 9, 14, 20, 30, 42, 60, 86, 142, 280 } },
	{ CLASS_NEC, { 5, 10, 16, 26, 39, 56, 86, 152, 377 } },
	{ CLASS_DRU, { 5, 10, 16, 26, 39, 56, 86, 152, 377 } },
	{ CLASS_BAR, { 7, 15, 27, 48, 86, 200 } },
	{ CLASS_ASN, { 7, 15, 27, 48, 86, 200 } },
	{ CLASS_PAL, { 7, 15, 27, 48, 86, 200 } },
	{ kWolfTxtFileNoAlias, { 9, 20, 42, 86, 280 } },
	{ kBearTxtFileNoAlias, { 5, 10, 16, 24, 37, 54, 86, 152, 360 } },
	{ kDruidOneHandFHRAlias, { 3, 7, 13, 19, 29, 42, 63, 99, 174, 456 } },
	{ kPaladinSpearStaveFHRAlias, { 3, 7, 13, 20, 32, 48, 75, 129, 280 } },
	{ MERC_A1, { 6, 13, 20, 32, 52, 86, 174, 600 } },
	{ MERC_A2, { 5, 9, 14, 20, 30, 42, 60, 86, 142, 280 } },
	{ MERC_A3, { 5, 8, 13, 18, 24, 32, 46, 63, 86, 133, 232, 600 } },
	{ MERC_A4, { 9, 20, 42, 86, 280 } },
	{ MERC_A5, { 7, 15, 27, 48, 86, 200 } },
};

std::map<DWORD, std::vector<int>> faster_cast_rate_frames = {
	{ CLASS_AMA, { 7, 14, 22, 32, 48, 68, 99, 152 } },
	{ CLASS_SOR, { 9, 20, 37, 63, 105, 200 } },
	{ CLASS_BAR, { 9, 20, 37, 63, 105, 200 } },
	{ CLASS_NEC, { 9, 18, 30, 48, 75, 125 } },
	{ CLASS_PAL, { 9, 18, 30, 48, 75, 125 } },
	{ CLASS_ASN, { 8, 16, 27, 42, 65, 102, 174 } },
	{ CLASS_DRU, { 4, 10, 19, 30, 46, 68, 99, 163 } },
	{ kWolfTxtFileNoAlias, { 6, 14, 26, 40, 60, 95, 157 } },
	{ kBearTxtFileNoAlias, { 7, 15, 26, 40, 63, 99, 163 } },
	{ kSorceressLightningFCRAlias, { 7, 15, 23, 35, 52, 78, 117, 194 } },
	{ MERC_A1, { 0 } },
	{ MERC_A2, { 0 } },
	{ MERC_A3, { 8, 15, 26, 39, 58, 86, 138 } },
	{ MERC_A4, { 9, 18, 30, 48, 75, 125 } },
	{ MERC_A5, { 0 } },
};

std::map<DWORD, std::set<int>> merc_attack_skills = {
	{ MERC_A1, { 385, 386, 545 } },

	{ MERC_A2, { 10 } },

	{ MERC_A3, {  } },

	{ MERC_A4, {  } },

	{ MERC_A5, { 144, 425 } },
};

std::set<int> rollback_skills = {
	26, 30, 106, 248, 255
};

std::map<DWORD, std::string> merc_skill_names = {
	{ MERC_A1, "Fire/Cold/Magic Arrow" },

	{ MERC_A2, "Jab" },

	{ MERC_A3, "Fireball/Ice Blast/Lightning" },

	{ MERC_A4, "Holy Nova/Bone Spear" },

	{ MERC_A5, "Concentrate/Whirlwind" },

};

const double experience_penalty[][16] = {
	{ },
	{ 67, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 77, 9, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 85, 11, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 91, 13, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 96, 16, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 98, 22, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 98, 29, 10, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 96, 39, 12, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 93, 48, 13, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 89, 58, 14, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 82, 67, 15, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 74, 77, 17, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 68, 83, 23, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 58, 89, 34, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 48, 92, 51, 8, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 38, 94, 68, 13, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 29, 94, 83, 23, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 20, 93, 91, 34, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 14, 92, 95, 47, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 10, 87, 95, 61, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 8, 82, 94, 72, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 6, 75, 92, 82, 6, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 6, 65, 91, 89, 7, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 5, 55, 89, 95, 10, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 100 },
	{ 5, 44, 90, 92, 67, 64, 54, 49, 44, 42, 36, 32, 31, 30, 30, 100 },
	{ 5, 35, 89, 95, 70, 67, 56, 51, 46, 43, 37, 33, 32, 31, 32, 100 },
	{ 5, 27, 86, 97, 72, 70, 58, 53, 48, 45, 38, 35, 33, 32, 33, 100 },
	{ 5, 20, 78, 98, 75, 72, 61, 55, 49, 47, 40, 36, 35, 33, 34, 100 },
	{ 5, 14, 67, 99, 78, 75, 63, 57, 51, 48, 41, 37, 36, 35, 35, 100 },
	{ 5, 11, 52, 95, 80, 77, 65, 59, 53, 50, 43, 38, 37, 36, 37, 100 },
	{ 5, 8, 37, 90, 83, 80, 67, 61, 55, 52, 44, 40, 38, 37, 38, 100 },
	{ 5, 7, 24, 83, 86, 82, 69, 63, 56, 53, 45, 41, 40, 38, 39, 100 },
	{ 5, 6, 15, 69, 88, 85, 71, 65, 58, 55, 47, 42, 41, 39, 40, 100 },
	{ 5, 5, 10, 55, 90, 88, 73, 67, 60, 57, 48, 44, 42, 41, 41, 100 },
	{ 5, 5, 8, 47, 92, 90, 76, 69, 62, 58, 50, 45, 43, 42, 43, 100 },
	{ 5, 5, 8, 34, 94, 93, 78, 71, 64, 60, 51, 46, 44, 43, 44, 100 },
	{ 5, 5, 8, 22, 94, 95, 80, 73, 65, 62, 53, 47, 46, 44, 45, 100 },
	{ 5, 5, 8, 17, 94, 97, 82, 75, 67, 63, 54, 49, 47, 45, 46, 100 },
	{ 5, 5, 8, 12, 92, 98, 84, 77, 69, 65, 55, 50, 48, 47, 47, 100 },
	{ 5, 5, 8, 6, 88, 99, 86, 79, 71, 67, 57, 51, 49, 48, 49, 100 },
	{ 5, 5, 8, 6, 82, 99, 89, 81, 72, 68, 58, 52, 51, 49, 50, 100 },
	{ 5, 5, 8, 5, 75, 97, 91, 83, 74, 70, 60, 54, 52, 50, 51, 100 },
	{ 5, 5, 8, 5, 67, 93, 93, 85, 76, 72, 61, 55, 53, 51, 52, 100 },
	{ 5, 5, 8, 5, 58, 84, 94, 87, 78, 73, 63, 56, 54, 53, 54, 100 },
	{ 5, 5, 8, 5, 50, 72, 96, 88, 79, 75, 64, 58, 56, 54, 55, 100 },
	{ 5, 5, 7, 5, 42, 56, 97, 90, 81, 77, 65, 59, 57, 55, 56, 100 },
	{ 5, 5, 7, 5, 34, 42, 97, 92, 83, 78, 67, 60, 58, 56, 57, 100 },
	{ 5, 5, 6, 5, 26, 29, 97, 94, 85, 80, 68, 61, 59, 57, 58, 100 },
	{ 5, 5, 6, 5, 20, 19, 94, 95, 86, 82, 70, 63, 60, 59, 60, 100 },
	{ 5, 5, 5, 5, 14, 13, 88, 95, 88, 83, 71, 64, 62, 60, 61, 100 },
	{ 5, 5, 5, 5, 11, 10, 79, 95, 90, 85, 73, 65, 63, 61, 62, 100 },
	{ 5, 5, 5, 5, 9, 7, 69, 94, 92, 87, 74, 67, 64, 62, 63, 100 },
	{ 5, 5, 5, 5, 8, 6, 58, 93, 94, 88, 75, 68, 65, 63, 65, 100 },
	{ 5, 5, 5, 5, 8, 5, 47, 91, 95, 90, 77, 69, 67, 65, 66, 100 },
	{ 5, 5, 5, 5, 8, 5, 37, 87, 97, 91, 78, 70, 68, 66, 67, 100 },
	{ 5, 5, 5, 5, 9, 5, 29, 80, 99, 92, 80, 72, 69, 67, 68, 100 },
	{ 5, 5, 5, 5, 9, 5, 22, 71, 100, 93, 81, 73, 70, 68, 69, 100 },
	{ 5, 5, 5, 5, 9, 5, 16, 61, 100, 95, 82, 74, 72, 69, 71, 100 },
	{ 5, 5, 5, 5, 9, 5, 13, 48, 100, 95, 84, 76, 73, 70, 72, 100 },
	{ 5, 5, 5, 5, 9, 5, 10, 37, 100, 95, 85, 77, 74, 72, 73, 100 },
	{ 5, 5, 5, 5, 8, 5, 9, 27, 99, 94, 87, 78, 75, 73, 74, 100 },
	{ 5, 5, 5, 5, 8, 5, 8, 19, 93, 92, 88, 79, 77, 74, 76, 100 },
	{ 5, 5, 5, 5, 8, 5, 8, 12, 75, 89, 90, 81, 78, 75, 77, 100 },
	{ 5, 5, 5, 5, 7, 5, 7, 8, 56, 86, 91, 82, 79, 76, 78, 100 },
	{ 5, 5, 5, 5, 7, 5, 7, 6, 37, 80, 92, 83, 80, 78, 79, 100 },
	{ 5, 5, 5, 5, 6, 5, 7, 5, 19, 74, 94, 84, 81, 79, 80, 100 },
	{ 5, 5, 5, 5, 6, 5, 6, 5, 6, 63, 95, 86, 83, 80, 82, 100 },
	{ 5, 5, 5, 5, 5, 5, 6, 5, 2, 52, 97, 87, 84, 81, 83, 100 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 41, 98, 88, 85, 82, 84, 100 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 30, 98, 90, 86, 84, 85, 95.3 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 21, 99, 91, 88, 85, 86, 90.6 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 15, 99, 92, 89, 86, 88, 85.9 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 11, 98, 93, 90, 87, 89, 81.3 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 92, 95, 91, 88, 90, 76.6 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 82, 96, 93, 90, 91, 71.9 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 70, 97, 94, 91, 92, 67.2 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 56, 98, 95, 92, 93, 62.5 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 42, 99, 96, 93, 93, 57.8 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 31, 99, 97, 94, 94, 53.1 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 24, 99, 98, 96, 94, 48.4 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 18, 95, 99, 97, 95, 43.8 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 15, 89, 99, 98, 96, 39.1 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 13, 81, 99, 99, 96, 34.9 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 12, 72, 96, 100, 96, 29.7 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 11, 59, 91, 100, 95, 25 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 10, 46, 83, 100, 93, 18.8 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 35, 71, 100, 87, 14.1 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 9, 26, 58, 95, 78, 10.6 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 8, 18, 45, 88, 66, 7.91 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 8, 13, 33, 76, 52, 5.96 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 10, 23, 57, 36, 4.49 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 8, 16, 38, 24, 3.42 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 11, 24, 15, 2.54 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 12, 10, 1.95 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 7, 1.46 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 5, 7, 1.07 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 0.78 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 0.59 },
	{ 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 7, 0.49 },
};


DWORD syncCooldown = 0;

StatsDisplay* StatsDisplay::display;

StatsDisplay::StatsDisplay(std::string name)
{
	int yPos = 10;
	int width = 300;

	InitializeCriticalSection(&crit);
	SetY(yPos);
	SetXSize(width);

	LoadConfig();

	SetName(name);
	SetActive(true);
	SetMinimized(true);

	display = this;
}

StatsDisplay::~StatsDisplay()
{
	Lock();
	// Remove all hooks associated with the display
	while (Hooks.size() > 0) { delete (*Hooks.begin()); }
	Unlock();
	DeleteCriticalSection(&crit);
}

void StatsDisplay::LoadConfig()
{
	int height = 342 + 8 * 7 + 16 * 10;
	customStats.clear();

	for (map<string, string>::iterator it = App.screen.additionalStats.values.begin(); it != App.screen.additionalStats.values.end(); it++)
	{
		int statId = -1;
		stringstream ss(Trim((*it).first));
		if (!(ss >> statId).fail() && statId < STAT_MAX)
		{
			DisplayedStat* customStat = new DisplayedStat();
			customStat->name = AllStatList[statId]->name;
			customStat->id = statId;
			customStat->useValue = false;
			// Getting rid of the check for sp->saveParamBits > 0 to display weapon mastery values
			// if a param is supplied it will be used
			int          num = -1;
			stringstream ss(Trim((*it).second));
			if (!(ss >> num).fail() && num > 0)
			{
				customStat->useValue = true;
				customStat->value = num;
			}
			customStats.push_back(customStat);
		}
	}
	if (customStats.size() > 0) { height += (customStats.size() * 16) + 8; }

	int xPos = App.general.statsOnRight.value ? *p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
	SetX(xPos);
	SetYSize(height);
}

void StatsDisplay::SetX(unsigned int newX)
{
	if (newX >= 0 && newX <= Hook::GetScreenWidth())
	{
		Lock();
		x = newX;
		Unlock();
	}
}

void StatsDisplay::SetY(unsigned int newY)
{
	if (newY >= 0 && newY <= Hook::GetScreenHeight())
	{
		Lock();
		y = newY;
		Unlock();
	}
}

void StatsDisplay::SetXSize(unsigned int newXSize)
{
	if (newXSize >= 0 && newXSize <= (Hook::GetScreenWidth() - GetX()))
	{
		Lock();
		xSize = newXSize;
		Unlock();
	}
}

void StatsDisplay::SetYSize(unsigned int newYSize)
{
	if (newYSize >= 0 && newYSize <= (Hook::GetScreenHeight() - GetY()))
	{
		Lock();
		ySize = newYSize;
		Unlock();
	}
}

bool StatsDisplay::InRange(unsigned int x,
	unsigned int y)
{
	return IsActive() &&
		x >= GetX() && y >= GetY() &&
		x <= GetX() + GetXSize() && y <= GetY() + GetYSize();
}

void StatsDisplay::Draw()
{
	display->Lock();
	display->OnDraw();
	display->Unlock();
}

void StatsDisplay::OnDraw()
{
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	bool     isMerc = false;
	if (!unit)
		return;
	int column1 = GetX() + 5;
	int column2 = column1 + GetXSize() / 2;

	if (!IsMinimized())
	{
		int xPos = App.general.statsOnRight.value ? *p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
		SetX(xPos);

		if (D2CLIENT_GetUIState(UI_MERC))
		{
			unit = D2CLIENT_GetMercUnit();
			isMerc = true;
		}
		for (std::list<Hook*>::iterator it = Hooks.begin(); it != Hooks.end(); ++it)
			(*it)->OnDraw();

		int  y = GetY();
		RECT pRect;
		pRect.left = GetX();
		pRect.top = y;
		pRect.right = x + GetXSize();
		pRect.bottom = y + GetYSize();
		Boxhook::Draw(GetX(), GetY(), GetXSize(), GetYSize(), White, Drawing::BTBlack);

		Framehook::DrawRectStub(&pRect);


		Texthook::Draw(column1,
			(y += 8),
			None,
			6,
			Gold,
			"Name:ÿc0 %s",
			isMerc ? "ÿc;Mercenary" : unit->pPlayerData->szName);

		auto player_level = D2COMMON_GetUnitStat(unit, STAT_LEVEL, 0);
		Texthook::Draw(pRect.right - 5,
			y,
			Right,
			6,
			Gold,
			L"Level:ÿc0 %d",
			static_cast<int>(player_level));

		auto map_id = **Var_D2CLIENT_MapId();
		auto difficulty = D2CLIENT_GetDifficulty();
		auto act_id = StatsDisplay::GetActIndex(map_id, difficulty);
		auto xp_percentage = 0.0;
		if (act_id >= 0 && act_id < 15)
		{
			xp_percentage = experience_penalty[player_level][act_id] * experience_penalty[player_level][15] / 100.0;
		}

		Texthook::Draw(pRect.right - 5,
			y + 12,
			Right,
			6,
			Gold,
			L"XP: %.2f%% / Additional XP:ÿc: %d%%",
			xp_percentage,
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ADDEXPERIENCE, 0)));


		y += 8;

		BnetData* pData = (*p_D2LAUNCH_BnData);
		int       xPacMultiplier = pData->nCharFlags & PLAYER_TYPE_EXPANSION ? 2 : 1;
		int       resPenalty[3] = { RES_PENALTY_CLS_NORM, RES_PENALTY_CLS_NM, RES_PENALTY_CLS_HELL };
		int       penalty = resPenalty[D2CLIENT_GetDifficulty()] * xPacMultiplier;
		int       fMax = min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXFIRERESIST, 0)) + 75, MAX_PLAYER_RESISTANCE);
		int       cMax = min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXCOLDRESIST, 0)) + 75, MAX_PLAYER_RESISTANCE);
		int       lMax = min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXLIGHTNINGRESIST, 0)) + 75, MAX_PLAYER_RESISTANCE);
		int       pMax = min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXPOISONRESIST, 0)) + 75, MAX_PLAYER_RESISTANCE);
		int       pLengthReduce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONLENGTHREDUCTION, 0));
		int       hfd = min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_HALFFREEZEDURATION, 0)), 2);
		int       cbf = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_CANNOTBEFROZEN, 0));

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Red,
			L"ÿc4Fire Resist:ÿc1 %d ÿc0/ %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIRERESIST, 0)) + penalty,
			fMax);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Blue,
			L"ÿc4Cold Resist:ÿc3 %d ÿc0/ %d  ÿc4Length:ÿc3 %d%%",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDRESIST, 0)) + penalty,
			cMax,
			(cbf > 0) ? 0 : (100 - 50 * hfd)
		);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Yellow,
			L"ÿc4Lightning Resist:ÿc9 %d ÿc0/ %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGRESIST, 0)) + penalty,
			lMax);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Poison Resist:ÿc2 %d ÿc0/ %d  ÿc4Length:ÿc: %d%%",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONRESIST, 0)) + penalty,
			pMax,
			(100 - penalty - pLengthReduce)
		);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Curse Resist:ÿc8 %d /ÿc0 75 ÿc4Length:ÿc0 %d%%",
			static_cast<int>(min(D2COMMON_GetUnitStat(unit, STAT_CURSE_EFFECTIVENESS, 0), 75)),
			static_cast<int>(max(100 - D2COMMON_GetUnitStat(unit, STAT_CURSERESISTANCE, 0), 25)));
		y += 8;

		int fAbsorb = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREABSORB, 0));
		int fAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREABSORBPERCENT, 0));
		int cAbsorb = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDABSORB, 0));
		int cAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDABSORBPERCENT, 0));
		int lAbsorb = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORB, 0));
		int lAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORBPERCENT, 0));
		int mAbsorb = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICABSORB, 0));
		int mAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICABSORBPERCENT, 0));
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Red,
			L"ÿc4Absorption: ÿc1%dÿc0/ÿc1%d%c ÿc3%dÿc0/ÿc3%d%c ÿc9%dÿc0/ÿc9%d%c ÿc8%dÿc0/ÿc8%d%c",
			fAbsorb,
			fAbsorbPct,
			'%',
			cAbsorb,
			cAbsorbPct,
			'%',
			lAbsorb,
			lAbsorbPct,
			'%',
			mAbsorb,
			mAbsorbPct,
			'%');

		int dmgReduction = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTION, 0));
		int dmgReductionPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTIONPCT, 0));
		int magReduction = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTION, 0));
		int magReductionPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTIONPCT, 0));
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Tan,
			L"ÿc4Damage Reduction: ÿc7%dÿc0/ÿc7%d%c ÿc8%dÿc0/ÿc8%d%c",
			dmgReduction,
			dmgReductionPct,
			'%',
			magReduction,
			magReductionPct,
			'%');
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Attacker Takes Damage:ÿc0 %d ÿc9 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ATTACKERTAKESDAMAGE, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ATTACKERTAKESLTNGDMG, 0)));
		y += 8;

		int fMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREMASTERY, 0));
		int cMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDMASTERY, 0));
		int lMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGMASTERY, 0));
		int pMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONMASTERY, 0));
		int mMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICDMGMASTERY, 0));

		int fPierce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PSENEMYFIRERESREDUC, 0));
		int cPierce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PSENEMYCOLDRESREDUC, 0));
		int lPierce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PSENEMYLIGHTNRESREDUC, 0));
		int pPierce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PSENEMYPSNRESREDUC, 0));
		int mPierce = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICRESREDUC, 0));
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Elemental Mastery:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
			fMastery,
			cMastery,
			lMastery,
			pMastery,
			mMastery);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Elemental Pierce:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
			fPierce,
			cPierce,
			lPierce,
			pPierce,
			mPierce);
		int  classNum = pData->nCharClass;
		auto classArMod = CharList[classNum]->toHitFactor - 35;
		int  dexAR = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0)) * 5 + classArMod;
		int  gearAR = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ATTACKRATING, 0));

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Base AR:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0% dÿc5 total:ÿc0 %d",
			dexAR,
			gearAR,
			dexAR + gearAR);

		int gearDef = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEFENSE, 0));
		int dexDef = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0)) / 4;

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Base Def:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0 %dÿc5 total:ÿc0 %d",
			dexDef,
			gearDef,
			dexDef + gearDef);

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Base Damage:ÿc5 1h:ÿc0 %d-%dÿc5 2h:ÿc0 %d-%d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMDAMAGE, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMDAMAGE, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_SECONDARYMINIMUMDAMAGE, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_SECONDARYMAXIMUMDAMAGE, 0)));

		y += 8;

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Cast Rate:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FASTERCAST, 0))
		);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Block Rate:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FASTERBLOCK, 0))
		);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Hit Recovery:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FASTERHITRECOVERY, 0))
		);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Run/Walk:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FASTERRUNWALK, 0))
		);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Attack Rate:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ATTACKRATE, 0)));
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"IAS:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_IAS, 0)));
		y += 8;


		char szSkillText[255] = "";
		if (!isMerc)
		{
			char rightSkillName[64] = "";
			Skill* pRightSkill = D2COMMON_10507_UNITS_GetRightSkill(unit);
			SkillDescTxt* pSkillDescTxt = &(*p_D2COMMON_sgptDataTable)->pSkillDescTxt[pRightSkill->pSkillInfo->wSkillDesc];
			std::string skillName = UnicodeToAnsi(GetTblEntryByIndex(pSkillDescTxt->wStrName, TBLOFFSET_STRING));
			sprintf(rightSkillName, "%s", skillName.c_str());

			Texthook::Draw(column1,
				(y += 16),
				None,
				6,
				Gold,
				"Breakpoints (ÿc0%sÿc4):",
				rightSkillName);
		}
		else
		{
			std::string sSkillText = "";
			if (merc_skill_names.find(unit->dwTxtFileNo) != merc_skill_names.end())
			{
				sSkillText = merc_skill_names.at(unit->dwTxtFileNo);
			}

			sprintf(szSkillText, "%.255s", sSkillText.c_str());

			Texthook::Draw(column1,
				(y += 16),
				None,
				6,
				Gold,
				"Breakpoints (ÿc0%sÿc4):",
				szSkillText);
		}


		auto fcr_key = unit->dwTxtFileNo;
		fcr_key = D2COMMON_GetUnitState(unit, 139) ? kWolfTxtFileNoAlias : fcr_key;
		fcr_key = D2COMMON_GetUnitState(unit, 140) ? kBearTxtFileNoAlias : fcr_key;

		auto skillId = !isMerc ? unit->pInfo->pRightSkill->pSkillInfo->wSkillId : 0;

		// Set Lighting frames FCR
		switch (skillId) {
		case 53: // Chain Lighting
		case 64: // Frozen Orb
			fcr_key = kSorceressLightningFCRAlias;
			break;
		}

		char bp_fcr_string[255] = "";
		GetBreakpointString(unit, STAT_FASTERCAST, faster_cast_rate_frames[fcr_key], (char*)&bp_fcr_string);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			"FCR:ÿc0 %s",
			bp_fcr_string);


		auto fhr_key = unit->dwTxtFileNo;
		fhr_key = D2COMMON_GetUnitState(unit, 139) ? kWolfTxtFileNoAlias : fhr_key;
		fhr_key = D2COMMON_GetUnitState(unit, 140) ? kBearTxtFileNoAlias : fhr_key;

		UnitAny* pLeftWeapon = NULL;
		pLeftWeapon = D2COMMON_GetLeftHandWeapon(unit->pInventory);
		auto weapon_type = pLeftWeapon ? GetCurrentWeaponType(pLeftWeapon->dwTxtFileNo) : WeaponType::kAxe;

		if (fhr_key == CLASS_PAL && (weapon_type == WeaponType::kSpear || weapon_type == WeaponType::kStaff))
		{
			fhr_key = kPaladinSpearStaveFHRAlias;
		}
		else if (fhr_key == CLASS_DRU && (
			weapon_type != WeaponType::kStaff && weapon_type != WeaponType::kSpear &&
			weapon_type != WeaponType::kHammer2H && weapon_type != WeaponType::kAxe2H &&
			weapon_type != WeaponType::kPole))
		{
			fhr_key = kDruidOneHandFHRAlias;
		}

		char bp_string[255] = "";
		GetBreakpointString(unit, STAT_FASTERHITRECOVERY, faster_hit_recovery_frames[fhr_key], (char*)&bp_string);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			"FHR:ÿc0 %s",
			bp_string);


		char ias_bp_string[255] = "IAS (Frames):ÿc0 ";
		y += 16;
		GetIASBreakpointString(unit, ias_bp_string, column1, &y);

		y += 8;
		int vLevelThreshold[5] = { 9, 18, 27, 36, 45 };
		int nOWFrameDamage = D2GAME_CalcOpenWoundsDamage(vLevelThreshold, player_level) + 25;
		int nOWCharDPS = (nOWFrameDamage * 25) / 256;

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Crushing Blow:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_CRUSHINGBLOW, 0)));
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Open Wounds: ÿc0%d%%/+%d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_OPENWOUNDS, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEEP_WOUNDS, 0)) + nOWCharDPS);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Deadly Strike:ÿc0 %d / %d",
			min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEADLYSTRIKE, 0)), min(75 + D2COMMON_GetUnitStat(unit, STAT_MAXDEADLYSTRIKE, 0), 100)),
			min(75 + D2COMMON_GetUnitStat(unit, STAT_MAXDEADLYSTRIKE, 0), 100));
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Critical Strike: ÿc0%d",
			min(static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_CRITICALSTRIKE, 0)), 75));
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Life Leech:ÿc1 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIFELEECH, 0)));
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Mana Leech:ÿc3 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MANALEECH, 0)));
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Projectile Pierce:ÿc0 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PIERCINGATTACK, 0)) +
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PIERCE, 0)));
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"HP/MP per Kill:ÿc1 %d ÿc0/ÿc3 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIFEAFTEREACHKILL, 0)),
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MANAAFTEREACHKILL, 0)));

		int minFire = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMFIREDAMAGE, 0));
		int maxFire = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMFIREDAMAGE, 0));
		int fireMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREMASTERY, 0));
		minFire = minFire + UTILITY_CalcPercent(minFire, fireMastery, 100);
		maxFire = maxFire + UTILITY_CalcPercent(maxFire, fireMastery, 100);
		int minLight = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMLIGHTNINGDAMAGE, 0));
		int maxLight = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMLIGHTNINGDAMAGE, 0));
		int lightMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGMASTERY, 0));
		minLight = minLight + UTILITY_CalcPercent(minLight, lightMastery, 100);
		maxLight = maxLight + UTILITY_CalcPercent(maxLight, lightMastery, 100);
		int minCold = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMCOLDDAMAGE, 0));
		int maxCold = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMCOLDDAMAGE, 0));
		int coldMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDMASTERY, 0));
		minCold = minCold + UTILITY_CalcPercent(minCold, coldMastery, 100);
		maxCold = maxCold + UTILITY_CalcPercent(maxCold, coldMastery, 100);
		int minPoison = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMPOISONDAMAGE, 0));
		int maxPoison = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMPOISONDAMAGE, 0));
		int poisonMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONMASTERY, 0));
		minPoison = minPoison + UTILITY_CalcPercent(minPoison, poisonMastery, 100);
		maxPoison = maxPoison + UTILITY_CalcPercent(maxPoison, poisonMastery, 100);
		int poisonLength = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONDAMAGELENGTH, 0));
		int poisonLengthOverride = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_SKILLPOISONOVERRIDELEN, 0));
		if (poisonLengthOverride > 0) { poisonLength = poisonLengthOverride; }
		int minMagic = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMMAGICALDAMAGE, 0));
		int maxMagic = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMMAGICALDAMAGE, 0));
		int magicMastery = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PASSIVEMAGICDMGMASTERY, 0));
		minMagic = minMagic + UTILITY_CalcPercent(minMagic, magicMastery, 100);
		maxMagic = maxMagic + UTILITY_CalcPercent(maxMagic, magicMastery, 100);
		int addedPhys = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ADDSDAMAGE, 0));
		y += 8;
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Added Damage:ÿc0 %d",
			addedPhys);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Orange,
			"%d-%d",
			minMagic,
			maxMagic);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Red,
			"%d-%d",
			minFire,
			maxFire);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Blue,
			"%d-%d",
			minCold,
			maxCold);
		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Yellow,
			"%d-%d",
			minLight,
			maxLight);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Green,
			"%d-%d over %.1fs",
			static_cast<int>(minPoison / 256.0 * poisonLength),
			static_cast<int>(maxPoison / 256.0 * poisonLength),
			poisonLength / 25.0);

		Texthook::Draw(column1,
			(y += 16),
			None,
			6,
			Gold,
			L"Magic Find:ÿc3 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICFIND, 0))
		);
		Texthook::Draw(column2,
			y,
			None,
			6,
			Gold,
			L"Gold Find:ÿc9 %d",
			static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_GOLDFIND, 0)));

		if (!isMerc)
		{
			Texthook::Draw(column1,
				(y += 16),
				None,
				6,
				Gold,
				L"Stash Gold:ÿc9 %d",
				static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_GOLDBANK, 0)));
		}

		//int cowKingKilled = D2COMMON_GetQuestFlag(D2CLIENT_GetQuestInfo(), 4, 10);
		//Texthook::Draw(column2, y, None, 6, Gold,
		//		L"Cow King:ÿc0 %s", cowKingKilled ? L"killed" : L"alive");

		if (customStats.size() > 0)
		{
			y += 8;
			for (unsigned int i = 0; i < customStats.size(); i++)
			{
				int secondary = customStats[i]->useValue ? customStats[i]->value : 0;
				int stat = static_cast<int>(D2COMMON_GetUnitStat(unit, customStats[i]->id, secondary));
				if (secondary > 0)
				{
					Texthook::Draw(column1,
						(y += 16),
						None,
						6,
						Gold,
						"%s[%d]:ÿc0 %d",
						customStats[i]->name.c_str(),
						secondary,
						stat);
				}
				else
				{
					Texthook::Draw(column1,
						(y += 16),
						None,
						6,
						Gold,
						"%s:ÿc0 %d",
						customStats[i]->name.c_str(),
						stat);
				}
			}
		}
	}
}

bool StatsDisplay::KeyClick(bool   bUp,
	BYTE   bKey,
	LPARAM lParam)
{
	display->Lock();
	bool block = display->OnKey(bUp, bKey, lParam);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnKey(bool   up,
	BYTE   kkey,
	LPARAM lParam)
{
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;
	if (IsMinimized())
	{
		//if (!up && kkey == App.game.characterStats.hotkey)
		//{
		//	LoadConfig();
		//	SetMinimized(false);
		//	return true;
		//}
	}
	else
	{
		//if (!up && (kkey == App.game.characterStats.hotkey || kkey == VK_ESCAPE))
		if (!up && kkey == VK_ESCAPE)
		{
			SetMinimized(true);
			return true;
		}
	}
	return false;
}

bool StatsDisplay::Click(bool         up,
	unsigned int mouseX,
	unsigned int mouseY)
{
	display->Lock();
	bool block = display->OnClick(up, mouseX, mouseY);
	display->Unlock();
	return block;
}

bool StatsDisplay::OnClick(bool         up,
	unsigned int x,
	unsigned int y)
{
	UnitAny* unit = D2CLIENT_GetPlayerUnit();
	if (!unit)
		return false;

	if (!IsMinimized() && InRange(x, y))
	{
		SetMinimized(true);
		return true;
	}
	return false;
}

void StatsDisplay::GetBreakpointString(UnitAny* unit,
	int              nStat,
	std::vector<int> bps,
	char* bpCharString)
{
	std::string bpString = "";
	int         bp = static_cast<int>(D2COMMON_GetUnitStat(unit, nStat, 0));
	for (int i = 0; i < bps.size() - 1; i++)
	{
		if (bp >= bps[i] && bp < bps[i + 1])
		{
			bpString += "ÿc8";
			bpString += std::to_string(bps[i]);
			bpString += "ÿc0 / ";
		}
		else { bpString += std::to_string(bps[i]) + " / "; }
	}
	if (bp >= bps[bps.size() - 1]) { bpString += "ÿc8"; }
	bpString += std::to_string(bps[bps.size() - 1]);

	sprintf(bpCharString, "%.255s", bpString.c_str());
}

void StatsDisplay::GetIASBreakpointString(UnitAny* pUnit,
	char* bpCharString, int x, int* pY)
{
	Skill* pRightSkill = D2COMMON_10507_UNITS_GetRightSkill(pUnit);
	if (!pRightSkill && pUnit->dwType == UNIT_MONSTER)
	{
		// Mercenaries
		if (pUnit->pInfo)
		{
			std::set<int> vMercSkills = merc_attack_skills.at(pUnit->dwTxtFileNo);
			pRightSkill = pUnit->pInfo->pFirstSkill;

			if (pUnit->dwTxtFileNo == MERC_A5 &&
				pRightSkill &&
				pRightSkill->pSkillInfo &&
				pRightSkill->pSkillInfo->wSkillId == 425 &&
				pRightSkill->pNextSkill &&
				pRightSkill->pNextSkill->pSkillInfo &&
				pRightSkill->pNextSkill->pSkillInfo->wSkillId != 149)
			{
				pRightSkill = pRightSkill->pNextSkill;
			}

			while (pRightSkill && pRightSkill->pSkillInfo && vMercSkills.find(pRightSkill->pSkillInfo->wSkillId) == vMercSkills.end())
			{
				pRightSkill = pRightSkill->pNextSkill;
			}
		}
	}

	if (pRightSkill)
	{
		int nFrameBonus = 0;
		int nFrames = 0;
		int nMinFrames = 0;
		int nMaxFrames = 0;
		int nAnimFrames = 0;
		int nAnimSpeed = 256;
		int nAnimAcceleration = 100;
		int nMinAnimAcceleration = 15;
		int nMaxAnimAcceleration = 175;
		int nFrameMinAccr = 0;
		int nAttackRate = 0;
		int nAttackRateBonus = 0;
		int nAnimRate = 0;
		int nMinAnimRate = 0;
		int nMaxAnimRate = 0;

		int nSkillRollback = 0;
		int nTotalHits = 0;
		int nActionFrame = 0;

		int nSkillId = pRightSkill->pSkillInfo->wSkillId;
		int nMode = 0;
		int nAnimType = pUnit->dwType;
		int nUnitID = pUnit->dwTxtFileNo;
		AnimDataRecord* pAnimData = NULL;

		// Whirlwind has hardcoded frames
		if (nSkillId == 151 || nSkillId == 380 || nSkillId == 425)
		{
			int WWNextHitDelay = pRightSkill->pSkillInfo->dwParam3;
			if (D2COMMON_UnitCanDualWield(pUnit))
			{
				Inventory* pInventory = pUnit->pInventory;
				UnitAny* pLeftWeapon = NULL;
				UnitAny* pRightWeapon = NULL;
				// In this case, pLeftWeapon == main hand weapon
				if (pInventory != NULL)
				{
					pLeftWeapon = D2COMMON_GetLeftHandWeapon(pInventory);
					if (pLeftWeapon != NULL)
					{
						int nBodyLoc = D2COMMON_ItemGetBodyLocation(pLeftWeapon);
						pRightWeapon = D2COMMON_GetItemByBodyLoc(pInventory, ((nBodyLoc & 0xFF) == 4) + 4);
					}
				}

				if (pLeftWeapon && pRightWeapon)
				{
					BOOL bIsPvp = FALSE;
					if (pUnit && pUnit->pPath && pUnit->pPath->pRoom1 && pUnit->pPath->pRoom1->pRoom2 && pUnit->pPath->pRoom1->pRoom2->pLevel)
					{
						int nLevelId = pUnit->pPath->pRoom1->pRoom2->pLevel->dwLevelNo;
						bIsPvp = (nLevelId == 157 || nLevelId == 159 || nLevelId == 166);
					}

					// Remove DW penalty in pvp
					if (!bIsPvp)
					{
						WWNextHitDelay += 1;
					}
				}
			}

			Texthook::Draw(x,
				*pY,
				None,
				6,
				Gold,
				"IAS (Frames):ÿc8 0 (%d)",
				WWNextHitDelay);
			return;
		}
		// Double swing
		else if (nSkillId == 133 && pUnit->dwMode != PLAYER_MODE_SEQUENCE)
		{
			nAttackRateBonus = pRightSkill->pSkillInfo->dwParam5;
		}
		// Dragon tail
		else if (nSkillId == 270 && pUnit->dwMode != PLAYER_MODE_KICK)
		{
			nAttackRateBonus = pRightSkill->pSkillInfo->dwParam4;
		}

		if (pRightSkill->mode == PLAYER_MODE_SEQUENCE)
		{
			int nSeqNum = D2COMMON_10030_SKILLS_GetSeqNumFromSkill(pUnit, pRightSkill);
			int nSeqIndex = D2COMMON_GetSequenceIndex_STUB(pUnit);
			if (&(p_D2COMMON_PlayerSequenceDataTable[nSeqNum]) == NULL)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A");
				return;
			}

			SequenceInfo* pSeqInfo = &(p_D2COMMON_PlayerSequenceDataTable[nSeqNum])[nSeqIndex];
			nFrames = pSeqInfo->nFrameCount;
			// This skill can't be used with current setup
			if (nFrames == 0)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A");
				return;
			}
			if (pSeqInfo->pSeqData->bMode == PLAYER_MODE_CAST)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A");
				return;
			}

			// Charge only uses last 7 frames for attack
			if (nSkillId == 107)
			{
				nFrames = 8;
				int nAnimSpeedBonus = pRightSkill->pSkillInfo->dwParam6;
				// Charge is special, the animation does not use attack rate at all, so instead it's baked into the anim speed
				// This makes the formula the same, removes anim speed and use normal attack rate portion like other skills for visual only
				nAnimSpeed = 100;

				nFrameMinAccr = D2COMMON_GetFrameMinAccr_STUB(FRAMES_IAS, pUnit);
				nAttackRate = D2COMMON_GetUnitStat(pUnit, STAT_ATTACKRATE, 0) + nAnimSpeedBonus;
				nAnimAcceleration = nFrameMinAccr + nAttackRate;
				nMinAnimAcceleration = nAttackRate;
				nMaxAnimAcceleration = 256;
			}
			else
			{
				nFrameMinAccr = D2COMMON_GetFrameMinAccr_STUB(FRAMES_IAS, pUnit);
				nAttackRate = D2COMMON_GetUnitStat(pUnit, STAT_ATTACKRATE, 0) + nAttackRateBonus;
				nAnimAcceleration = nFrameMinAccr + nAttackRate - 30;
				nMinAnimAcceleration = nAttackRate - 30;
			}

		}
		else if (pUnit->dwType == UNIT_MONSTER && pRightSkill->mode == NPC_MODE_SEQUENCE)
		{
			int nSeqNum = D2COMMON_10030_SKILLS_GetSeqNumFromSkill(pUnit, pRightSkill);
			SequenceInfo* pMonSeqInfo = D2COMMON_10194_DATATBLS_GetMonSeqTableRecord(nSeqNum);
			nFrames = pMonSeqInfo->nFrameCount;

			nFrameMinAccr = D2COMMON_GetFrameMinAccr_STUB(FRAMES_IAS, pUnit);
			nAttackRate = D2COMMON_GetUnitStat(pUnit, STAT_ATTACKRATE, 0) + nAttackRateBonus;
			nAnimAcceleration = nFrameMinAccr + nAttackRate;
			nMinAnimAcceleration = nAttackRate;
		}
		else
		{
			nMode = pRightSkill->mode;
			// Ignore spells. Ignore auras/Force Move
			if (nMode == PLAYER_MODE_CAST || nMode == PLAYER_MODE_RUN)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A");
				return;
			}

			D2COMMON_10350_ConvertMode(pUnit, &nAnimType, &nUnitID, &nMode, (char*)"StatsDisplay.cpp", 1009);
			pAnimData = D2COMMON_GetAnimDataRecord(pUnit, nUnitID, nMode, nAnimType, pUnit->pInventory);
			if (!pAnimData)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A");
				return;
			}
			// Hide Werewolf & Werebear until we have their frames worked out
			if (nUnitID == 430 || nUnitID == 431)
			{
				Texthook::Draw(x,
					*pY,
					None,
					6,
					Gold,
					"IAS (Frames): N/A (Work in Progress)");
				return;
			}

			nFrames = pAnimData->dwFrames;
			nAnimSpeed = pAnimData->dwAnimSpeed;
			// This calculates EIAS from IAS
			nFrameMinAccr = D2COMMON_GetFrameMinAccr_STUB(FRAMES_IAS, pUnit);
			// This is 100 + WSM + SIAS
			nAttackRate = D2COMMON_GetUnitStat(pUnit, STAT_ATTACKRATE, 0) + nAttackRateBonus;
			nAnimAcceleration = nFrameMinAccr + nAttackRate;
			nMinAnimAcceleration = nAttackRate;
		}


		if (rollback_skills.find(nSkillId) != rollback_skills.end())
		{
			switch (pRightSkill->pSkillInfo->wSkillId)
			{
			case 26: // Strafe
			{
				nSkillRollback = pRightSkill->pSkillInfo->dwParam6;
				break;
			}
			case 30:  // Fend
			case 106: // Zeal
			case 248: // Fury
			{
				nSkillRollback = pRightSkill->pSkillInfo->dwParam2;
				break;
			}
			case 255: // Dragon Talon
			{
				nSkillRollback = 100; // Hardcoded, not in skillstxt
				break;
			}
			default:
			{
				break;
			}
			}
			nTotalHits = D2COMMON_10786_SKILLS_EvaluateSkillFormula(pUnit, pRightSkill->pSkillInfo->dwCalc1, pRightSkill->pSkillInfo->wSkillId, pRightSkill->skillLevel);

			if (pAnimData)
			{
				for (int i = 0; i < nFrames; ++i)
				{
					if (pAnimData->dwAnimData[i])
					{
						nActionFrame = i;
						break;
					}
				}
			}
		}

		if (D2COMMON_UnitCanDualWield(pUnit))
		{
			Inventory* pInventory = pUnit->pInventory;
			UnitAny* pLeftWeapon = NULL;
			UnitAny* pRightWeapon = NULL;
			// In this case, pLeftWeapon == main hand weapon
			if (pInventory != NULL)
			{
				pLeftWeapon = D2COMMON_GetLeftHandWeapon(pInventory);
				if (pLeftWeapon != NULL)
				{
					int nBodyLoc = D2COMMON_ItemGetBodyLocation(pLeftWeapon);
					pRightWeapon = D2COMMON_GetItemByBodyLoc(pInventory, ((nBodyLoc & 0xFF) == 4) + 4);
				}

				if (pLeftWeapon != NULL &&
					D2COMMON_IsMatchingType(pLeftWeapon, 45) && // ITEM_TYPE_WEAPON
					D2COMMON_ItemCanBeEquipped(pLeftWeapon) &&
					D2COMMON_StatListGetOwner(pLeftWeapon, NULL) != NULL &&
					pRightWeapon != NULL &&
					D2COMMON_IsMatchingType(pRightWeapon, 45) && // ITEM_TYPE_WEAPON
					D2COMMON_ItemCanBeEquipped(pRightWeapon) &&
					D2COMMON_StatListGetOwner(pRightWeapon, NULL) != NULL)
				{
					int nLeftWeaponAttackRate = D2COMMON_GetUnitStat(pLeftWeapon, STAT_ATTACKRATE, 0);
					int nRightWeaponAttackRate = D2COMMON_GetUnitStat(pRightWeapon, STAT_ATTACKRATE, 0);

					if (pUnit && pUnit->dwType == UNIT_PLAYER)
					{
						nLeftWeaponAttackRate += D2COMMON_GetUnitStat(pLeftWeapon, STAT_IAS, 0);
						nRightWeaponAttackRate += D2COMMON_GetUnitStat(pRightWeapon, STAT_IAS, 0);

						D2COMMON_MergeStatList(pUnit, pLeftWeapon, 0);
						D2COMMON_MergeStatList(pUnit, pRightWeapon, 0);

						if (nLeftWeaponAttackRate > nRightWeaponAttackRate)
						{
							D2COMMON_MergeStatList(pUnit, pLeftWeapon, 1);
						}
						else
						{
							D2COMMON_MergeStatList(pUnit, pRightWeapon, 1);
						}

						int nMinAccrIAS = D2COMMON_GetFrameMinAccr_STUB(FRAMES_IAS, pUnit);
						int nBaseIAS = D2COMMON_GetUnitStat(pUnit, STAT_ATTACKRATE, 0);

						nAnimAcceleration = nBaseIAS + nMinAccrIAS;
						nMinAnimAcceleration = nBaseIAS;
						if (pRightSkill->mode == PLAYER_MODE_SEQUENCE)
						{
							nAnimAcceleration -= 30;
							nMinAnimAcceleration -= 30;
						}
					}
				}
			}
		}

		// Temp set skill mode to calc frame bonus
		int nOldMode = pUnit->dwMode;
		pUnit->dwMode = pRightSkill->mode;
		nFrameBonus = D2COMMON_10031_UNITS_GetFrameBonus(pUnit);
		pUnit->dwMode = nOldMode;

		if (nMinAnimAcceleration > nMaxAnimAcceleration) nMinAnimAcceleration = nMaxAnimAcceleration - 1;  // 174 so we go through the for loop once
		if (nAnimAcceleration < 15) nAnimAcceleration = 15;
		if (nAnimAcceleration > nMaxAnimAcceleration) nAnimAcceleration = nMaxAnimAcceleration;
		if (nAnimSpeed == 0) nAnimSpeed = 256;

		nMinAnimRate = ((nAnimSpeed * nMaxAnimAcceleration) / 100);
		nMaxAnimRate = ((nAnimSpeed * nMinAnimAcceleration) / 100);
		nAnimRate = ((nAnimSpeed * nAnimAcceleration) / 100);

		nMinFrames = ceil(((((double)nFrames - (double)nFrameBonus) * 256) / (double)nMinAnimRate)) - 1;
		nMaxFrames = ceil(((((double)nFrames - (double)nFrameBonus) * 256) / (double)nMaxAnimRate)) - 1;
		nAnimFrames = ceil(((((double)nFrames - (double)nFrameBonus) * 256) / (double)nAnimRate)) - 1;

		std::vector<int> nMinActionFrames(nTotalHits);
		std::vector<int> nMaxActionFrames(nTotalHits);
		std::vector<int> nAnimActionFrames(nTotalHits);

		std::vector<int> nCurrentActionFrame(nTotalHits);
		std::vector<int> nPreviousActionFrame(nTotalHits);

		if (rollback_skills.find(nSkillId) != rollback_skills.end())
		{
			nMinActionFrames = GetRollbackSkillFrames(nMinActionFrames, nActionFrame, nFrames, nFrameBonus, nMinAnimRate, nSkillRollback);
			nMaxActionFrames = GetRollbackSkillFrames(nMaxActionFrames, nActionFrame, nFrames, nFrameBonus, nMaxAnimRate, nSkillRollback);
			nAnimActionFrames = GetRollbackSkillFrames(nAnimActionFrames, nActionFrame, nFrames, nFrameBonus, nAnimRate, nSkillRollback);
		}

		std::string bpString = bpCharString;
		int nIASBP = 0;
		double nBaseIAS = 0;
		int nPreviousFrameBP = 0;
		int nCurrentFrames = 0;

		for (int i = nMinAnimAcceleration; i < nMaxAnimAcceleration + 1; i++)
		{
			nBaseIAS = (double)i - (double)nMinAnimAcceleration;
			nIASBP = ceil((120 * nBaseIAS) / (120 - nBaseIAS));
			nAnimRate = (nAnimSpeed * i) / 100;

			if (rollback_skills.find(nSkillId) != rollback_skills.end())
			{
				nCurrentActionFrame = GetRollbackSkillFrames(nCurrentActionFrame, nActionFrame, nFrames, nFrameBonus, nAnimRate, nSkillRollback);
				if (nCurrentActionFrame == nPreviousActionFrame) continue;
				nPreviousActionFrame = nCurrentActionFrame;
			}
			else
			{
				nCurrentFrames = ceil(((((double)nFrames - (double)nFrameBonus) * 256) / (double)nAnimRate)) - 1;
				if (nCurrentFrames == nPreviousFrameBP) continue;
				nPreviousFrameBP = nCurrentFrames;
			}

			if (rollback_skills.find(nSkillId) != rollback_skills.end() && nCurrentActionFrame == nAnimActionFrames)
			{
				bpString += "ÿc8";
				bpString += std::to_string(nIASBP) + " (";
				for (int i = 0; i < nCurrentActionFrame.size() - 1; i++)
				{
					bpString += std::to_string(nCurrentActionFrame[i]) + "|";
				}
				bpString += std::to_string(nCurrentActionFrame.back()) + ")";
				bpString += "ÿc0";
			}
			else if (nCurrentFrames == nAnimFrames)
			{
				bpString += "ÿc8";
				bpString += std::to_string(nIASBP) + " (" + std::to_string(nCurrentFrames) + ")";
				bpString += "ÿc0";
			}
			else
			{
				bpString += std::to_string(nIASBP);// +"(" + std::to_string(i) + ")";
			}
			bpString += " / ";
		}

		nBaseIAS = (double)nMaxAnimAcceleration - (double)nMinAnimAcceleration;
		nIASBP = ceil((120 * nBaseIAS) / (120 - nBaseIAS));
		// TODO: nCurrentFrames here appears to be identical to nMinFrames. Is this a mistake? Should this be something else?
		nCurrentFrames = ceil(((((double)nFrames - (double)nFrameBonus) * 256) / (double)((nAnimSpeed * nMaxAnimAcceleration) / 100))) - 1;

		if (rollback_skills.find(nSkillId) != rollback_skills.end() && nMinActionFrames != nPreviousActionFrame)
		{
			if (nMinActionFrames == nAnimActionFrames) bpString += "ÿc8";
			bpString += std::to_string(nIASBP);
			if (nMinActionFrames == nAnimActionFrames)
			{
				bpString += " (";
				for (int i = 0; i < nMinActionFrames.size() - 1; i++)
				{
					bpString += std::to_string(nMinActionFrames[i]) + "|";
				}
				bpString += std::to_string(nMinActionFrames.back()) + ")";
			}
		}
		else if (nCurrentFrames != nPreviousFrameBP && nPreviousFrameBP != 0)
		{
			if (nMinFrames == nAnimFrames) bpString += "ÿc8";
			bpString += std::to_string(nIASBP);
			if (nCurrentFrames == nAnimFrames)
			{
				bpString += " (" + std::to_string(nMinFrames) + ")";
			}
		}
		else
		{
			if (bpString.length() > 3)
			{
				// Remove the trailing slash if there was no final BP
				for (int j = 0; j < 3; j++) bpString.pop_back();
			}
		}

		int nMaxLength = GetXSize() / 4 - 8;
		int nLastSlash = bpString.substr(0, nMaxLength).find_last_of('/');
		if (bpString.length() > nMaxLength && nLastSlash != -1)
		{
			sprintf(bpCharString, "%.255s", bpString.substr(0, nLastSlash + 1).c_str());
			Texthook::Draw(x,
				*pY,
				None,
				6,
				Gold,
				"%s",
				bpCharString);

			bpString = bpString.substr(nLastSlash + 1, bpString.length());
			nLastSlash = bpString.substr(0, (nMaxLength - 16)).find_last_of('/');

			// - 16 to account for spacing
			while (bpString.length() > (nMaxLength - 16) && nLastSlash != -1)
			{

				*pY += 8;

				sprintf(bpCharString, "%.255s", bpString.substr(0, nLastSlash + 1).c_str());
				Texthook::Draw(x,
					*pY,
					None,
					6,
					White,
					"             %s",
					bpCharString);

				bpString = bpString.substr(nLastSlash + 1, bpString.length());
				nLastSlash = bpString.substr(0, (nMaxLength - 16)).find_last_of('/');
			}

			*pY += 8;
			sprintf(bpCharString, "%.255s", bpString.c_str());
			Texthook::Draw(x,
				*pY,
				None,
				6,
				White,
				"             %s",
				bpCharString);
		}
		else
		{
			sprintf(bpCharString, "%.255s", bpString.c_str());
			Texthook::Draw(x,
				*pY,
				None,
				6,
				Gold,
				"%s",
				bpCharString);
		}
	}
	else
	{
		Texthook::Draw(x,
			*pY,
			None,
			6,
			Gold,
			"IAS (Frames): N/A");
	}
}

std::vector<int> StatsDisplay::GetRollbackSkillFrames(std::vector<int> nAnimFrames, int nActionFrame, int nTotalFrames, int nFrameBonus, int nAnimRate, int nSkillRollback)
{
	int nRollbackBase = 0;
	int nRollbackStartFrame = 0;
	int nTotalHits = nAnimFrames.size();
	std::vector<int> nActionFrames256(nTotalHits);

	// Calculate the first action frame
	nActionFrames256[0] = (nFrameBonus * 256) + ceil((((double)nActionFrame - (double)nFrameBonus) * 256) / (double)nAnimRate) * nAnimRate;
	for (int i = 1; i <= nTotalHits - 1; i++)
	{
		nRollbackBase = ((nActionFrames256[i - 1]) >> 8) * (100 - nSkillRollback) / 100;  // Rollback the skill to n frame number
		nRollbackStartFrame = (nRollbackBase * 256) + nAnimRate;  // The starting frame for the next action
		nActionFrames256[i] = (nRollbackBase * 256) + ceil((((double)nActionFrame - (double)nRollbackBase) * 256) / (double)nAnimRate) * nAnimRate;  // The following action frame

		// Get frame values for middle frame actions
		nAnimFrames[i] = 1;
		while (nRollbackStartFrame < nActionFrames256[i])
		{
			nRollbackStartFrame += nAnimRate;
			nAnimFrames[i] += 1;
		}
	}
	// Calculate the last frame:
	nActionFrames256.back() = ((nRollbackBase * 256) + ceil((((double)nTotalFrames - (double)nRollbackBase) * 256) / (double)nAnimRate) * nAnimRate) - (nAnimRate * 2);

	// Get frame value for last frame action
	while (nRollbackStartFrame < nActionFrames256.back())
	{
		nRollbackStartFrame += nAnimRate;
		nAnimFrames.back() += 1;
	}
	// Get frame value for the first frame action
	nAnimFrames[0] = ceil((nActionFrames256[0] - (nFrameBonus * 256)) / (double)nAnimRate) + 1;

	return nAnimFrames;
}

int StatsDisplay::GetActIndex(const int map_number,
	const int difficulty)
{
	auto act = -1;
	if (map_number != MAP_UNKNOWN)
	{
		if (map_number < MAP_A2_LUT_GHOLEIN) { act = (1 + difficulty * 5) - 1; }
		else if (map_number < MAP_A3_KURAST_DOCKS) { act = (2 + difficulty * 5) - 1; }
		else if (map_number < MAP_A4_THE_PANDEMONIUM_FORTRESS) { act = (3 + difficulty * 5) - 1; }
		else if (map_number < MAP_A5_HARROGATH) { act = (4 + difficulty * 5) - 1; }
		else if (map_number <= MAP_A5_WORLDSTONE_KEEP) { act = (5 + difficulty * 5) - 1; }
		else if (map_number <= MAP_A5_TRISTRAM) { act = (5 + difficulty * 5) - 1; } // ubers + organ bosses will need later confirmation
	}

	return act;
}

WeaponType StatsDisplay::GetCurrentWeaponType(const int weapon_number)
{
	switch (weapon_number)
	{
	case 0: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe;
	case 1:
		return WeaponType::kAxe;
	case 2:
		return WeaponType::kAxe;
	case 3:
		return WeaponType::kAxe;
	case 4:
		return WeaponType::kAxe;
	case 5: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe2H;
	case 6:
		return WeaponType::kAxe2H;
	case 7:
		return WeaponType::kAxe2H;
	case 8:
		return WeaponType::kAxe2H;
	case 9:
		return WeaponType::kAxe2H;
	case 10: // NOLINT(bugprone-branch-clone)
		return WeaponType::kWand;
	case 11:
		return WeaponType::kWand;
	case 12:
		return WeaponType::kWand;
	case 13:
		return WeaponType::kWand;
	case 14:
		return WeaponType::kClub;
	case 15: // NOLINT(bugprone-branch-clone)
		return WeaponType::kScepter;
	case 16:
		return WeaponType::kScepter;
	case 17:
		return WeaponType::kScepter;
	case 18:
		return WeaponType::kClub;
	case 19: // NOLINT(bugprone-branch-clone)
		return WeaponType::kMace;
	case 20:
		return WeaponType::kMace;
	case 21:
		return WeaponType::kMace;
	case 22:
		return WeaponType::kHammer;
	case 23: // NOLINT(bugprone-branch-clone)
		return WeaponType::kHammer2H;
	case 24:
		return WeaponType::kHammer2H;
	case 25: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword;
	case 26:
		return WeaponType::kSword;
	case 27:
		return WeaponType::kSword;
	case 28:
		return WeaponType::kSword;
	case 29:
		return WeaponType::kSword;
	case 30:
		return WeaponType::kSword;
	case 31:
		return WeaponType::kSword;
	case 32:
		return WeaponType::kSword;
	case 33: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword2H;
	case 34:
		return WeaponType::kSword2H;
	case 35:
		return WeaponType::kSword2H;
	case 36:
		return WeaponType::kSword2H;
	case 37:
		return WeaponType::kSword2H;
	case 38:
		return WeaponType::kSword2H;
	case 39: // NOLINT(bugprone-branch-clone)
		return WeaponType::kKnife;
	case 40:
		return WeaponType::kKnife;
	case 41:
		return WeaponType::kKnife;
	case 42:
		return WeaponType::kKnife;
	case 43: // NOLINT(bugprone-branch-clone)
		return WeaponType::kThrowing;
	case 44:
		return WeaponType::kThrowing;
	case 45:
		return WeaponType::kThrowing;
	case 46:
		return WeaponType::kThrowing;
	case 47: // NOLINT(bugprone-branch-clone)
		return WeaponType::kJavelin;
	case 48:
		return WeaponType::kJavelin;
	case 49:
		return WeaponType::kJavelin;
	case 50:
		return WeaponType::kJavelin;
	case 51:
		return WeaponType::kJavelin;
	case 52: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSpear;
	case 53:
		return WeaponType::kSpear;
	case 54:
		return WeaponType::kSpear;
	case 55:
		return WeaponType::kSpear;
	case 56:
		return WeaponType::kSpear;
	case 57: // NOLINT(bugprone-branch-clone)
		return WeaponType::kPole;
	case 58:
		return WeaponType::kPole;
	case 59:
		return WeaponType::kPole;
	case 60:
		return WeaponType::kPole;
	case 61:
		return WeaponType::kPole;
	case 62:
		return WeaponType::kPole;
	case 63: // NOLINT(bugprone-branch-clone)
		return WeaponType::kStaff;
	case 64:
		return WeaponType::kStaff;
	case 65:
		return WeaponType::kStaff;
	case 66:
		return WeaponType::kStaff;
	case 67:
		return WeaponType::kStaff;
	case 68: // NOLINT(bugprone-branch-clone)
		return WeaponType::kBow;
	case 69:
		return WeaponType::kBow;
	case 70:
		return WeaponType::kBow;
	case 71:
		return WeaponType::kBow;
	case 72:
		return WeaponType::kBow;
	case 73:
		return WeaponType::kBow;
	case 74:
		return WeaponType::kBow;
	case 75:
		return WeaponType::kBow;
	case 76: // NOLINT(bugprone-branch-clone)
		return WeaponType::kCrossbow;
	case 77:
		return WeaponType::kCrossbow;
	case 78:
		return WeaponType::kCrossbow;
	case 79:
		return WeaponType::kCrossbow;
	case 80: // NOLINT(bugprone-branch-clone)
		return WeaponType::kThrowingPot;
	case 81:
		return WeaponType::kThrowingPot;
	case 82:
		return WeaponType::kThrowingPot;
	case 83:
		return WeaponType::kThrowingPot;
	case 84:
		return WeaponType::kThrowingPot;
	case 85:
		return WeaponType::kThrowingPot;
	case 86: // NOLINT(bugprone-branch-clone)
		return WeaponType::kKnife;
	case 87:
		return WeaponType::kKnife;
	case 88:
		return WeaponType::kClub;
	case 89: // NOLINT(bugprone-branch-clone)
		return WeaponType::kHammer;
	case 90:
		return WeaponType::kHammer;
	case 91: // NOLINT(bugprone-branch-clone)
		return WeaponType::kStaff;
	case 92:
		return WeaponType::kStaff;
	case 93: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe;
	case 94:
		return WeaponType::kAxe;
	case 95:
		return WeaponType::kAxe;
	case 96:
		return WeaponType::kAxe;
	case 97:
		return WeaponType::kAxe;
	case 98: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe2H;
	case 99:
		return WeaponType::kAxe2H;
	case 100:
		return WeaponType::kAxe2H;
	case 101:
		return WeaponType::kAxe2H;
	case 102:
		return WeaponType::kAxe2H;
	case 103: // NOLINT(bugprone-branch-clone)
		return WeaponType::kWand;
	case 104:
		return WeaponType::kWand;
	case 105:
		return WeaponType::kWand;
	case 106:
		return WeaponType::kWand;
	case 107:
		return WeaponType::kClub;
	case 108: // NOLINT(bugprone-branch-clone)
		return WeaponType::kScepter;
	case 109:
		return WeaponType::kScepter;
	case 110:
		return WeaponType::kScepter;
	case 111:
		return WeaponType::kClub;
	case 112: // NOLINT(bugprone-branch-clone)
		return WeaponType::kMace;
	case 113:
		return WeaponType::kMace;
	case 114:
		return WeaponType::kMace;
	case 115:
		return WeaponType::kHammer;
	case 116: // NOLINT(bugprone-branch-clone)
		return WeaponType::kHammer2H;
	case 117:
		return WeaponType::kHammer2H;
	case 118: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword;
	case 119:
		return WeaponType::kSword;
	case 120:
		return WeaponType::kSword;
	case 121:
		return WeaponType::kSword;
	case 122:
		return WeaponType::kSword;
	case 123:
		return WeaponType::kSword;
	case 124:
		return WeaponType::kSword;
	case 125:
		return WeaponType::kSword;
	case 126: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword2H;
	case 127:
		return WeaponType::kSword2H;
	case 128:
		return WeaponType::kSword2H;
	case 129:
		return WeaponType::kSword2H;
	case 130:
		return WeaponType::kSword2H;
	case 131:
		return WeaponType::kSword2H;
	case 132: // NOLINT(bugprone-branch-clone)
		return WeaponType::kKnife;
	case 133:
		return WeaponType::kKnife;
	case 134:
		return WeaponType::kKnife;
	case 135:
		return WeaponType::kKnife;
	case 136: // NOLINT(bugprone-branch-clone)
		return WeaponType::kThrowing;
	case 137:
		return WeaponType::kThrowing;
	case 138:
		return WeaponType::kThrowing;
	case 139:
		return WeaponType::kThrowing;
	case 140: // NOLINT(bugprone-branch-clone)
		return WeaponType::kJavelin;
	case 141:
		return WeaponType::kJavelin;
	case 142:
		return WeaponType::kJavelin;
	case 143:
		return WeaponType::kJavelin;
	case 144:
		return WeaponType::kJavelin;
	case 145: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSpear;
	case 146:
		return WeaponType::kSpear;
	case 147:
		return WeaponType::kSpear;
	case 148:
		return WeaponType::kSpear;
	case 149:
		return WeaponType::kSpear;
	case 150: // NOLINT(bugprone-branch-clone)
		return WeaponType::kPole;
	case 151:
		return WeaponType::kPole;
	case 152:
		return WeaponType::kPole;
	case 153:
		return WeaponType::kPole;
	case 154:
		return WeaponType::kPole;
	case 155:
		return WeaponType::kPole;
	case 156: // NOLINT(bugprone-branch-clone)
		return WeaponType::kStaff;
	case 157:
		return WeaponType::kStaff;
	case 158:
		return WeaponType::kStaff;
	case 159:
		return WeaponType::kStaff;
	case 160:
		return WeaponType::kStaff;
	case 161: // NOLINT(bugprone-branch-clone)
		return WeaponType::kBow;
	case 162:
		return WeaponType::kBow;
	case 163:
		return WeaponType::kBow;
	case 164:
		return WeaponType::kBow;
	case 165:
		return WeaponType::kBow;
	case 166:
		return WeaponType::kBow;
	case 167:
		return WeaponType::kBow;
	case 168:
		return WeaponType::kBow;
	case 169: // NOLINT(bugprone-branch-clone)
		return WeaponType::kCrossbow;
	case 170:
		return WeaponType::kCrossbow;
	case 171:
		return WeaponType::kCrossbow;
	case 172:
		return WeaponType::kCrossbow;
	case 173: // NOLINT(bugprone-branch-clone)
		return WeaponType::kMace;
	case 174:
		return WeaponType::kMace;

	case 175: // NOLINT(bugprone-branch-clone)
		return WeaponType::kClaw1;
	case 176:
		return WeaponType::kClaw1;
	case 177:
		return WeaponType::kClaw1;
	case 178:
		return WeaponType::kClaw1;
	case 179:
		return WeaponType::kClaw1;
	case 180:
		return WeaponType::kClaw1;
	case 181:
		return WeaponType::kClaw1;
	case 182:
		return WeaponType::kClaw1;
	case 183:
		return WeaponType::kClaw1;
	case 184:
		return WeaponType::kClaw1;
	case 185: // NOLINT(bugprone-branch-clone)
		return WeaponType::kClaw2;
	case 186:
		return WeaponType::kClaw2;
	case 187:
		return WeaponType::kClaw2;
	case 188:
		return WeaponType::kClaw2;
	case 189:
		return WeaponType::kClaw2;
	case 190:
		return WeaponType::kClaw2;
	case 191:
		return WeaponType::kClaw2;
	case 192:
		return WeaponType::kClaw2;
	case 193:
		return WeaponType::kClaw2;
	case 194:
		return WeaponType::kClaw2;
	case 195:
		return WeaponType::kClaw2;
	case 196: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe;
	case 197:
		return WeaponType::kAxe;
	case 198:
		return WeaponType::kAxe;
	case 199:
		return WeaponType::kAxe;
	case 200:
		return WeaponType::kAxe;
	case 201: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAxe2H;
	case 202:
		return WeaponType::kAxe2H;
	case 203:
		return WeaponType::kAxe2H;
	case 204:
		return WeaponType::kAxe2H;
	case 205:
		return WeaponType::kAxe2H;
	case 206: // NOLINT(bugprone-branch-clone)
		return WeaponType::kWand;
	case 207:
		return WeaponType::kWand;
	case 208:
		return WeaponType::kWand;
	case 209:
		return WeaponType::kWand;
	case 210:
		return WeaponType::kClub;
	case 211: // NOLINT(bugprone-branch-clone)
		return WeaponType::kScepter;
	case 212:
		return WeaponType::kScepter;
	case 213:
		return WeaponType::kScepter;
	case 214:
		return WeaponType::kClub;
	case 215: // NOLINT(bugprone-branch-clone)
		return WeaponType::kMace;
	case 216:
		return WeaponType::kMace;
	case 217:
		return WeaponType::kMace;
	case 218:
		return WeaponType::kHammer;
	case 219: // NOLINT(bugprone-branch-clone)
		return WeaponType::kHammer2H;
	case 220:
		return WeaponType::kHammer2H;
	case 221: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword;
	case 222:
		return WeaponType::kSword;
	case 223:
		return WeaponType::kSword;
	case 224:
		return WeaponType::kSword;
	case 225:
		return WeaponType::kSword;
	case 226:
		return WeaponType::kSword;
	case 227:
		return WeaponType::kSword;
	case 228:
		return WeaponType::kSword;
	case 229: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSword2H;
	case 230:
		return WeaponType::kSword2H;
	case 231:
		return WeaponType::kSword2H;
	case 232:
		return WeaponType::kSword2H;
	case 233:
		return WeaponType::kSword2H;
	case 234:
		return WeaponType::kSword2H;
	case 235: // NOLINT(bugprone-branch-clone)
		return WeaponType::kKnife;
	case 236:
		return WeaponType::kKnife;
	case 237:
		return WeaponType::kKnife;
	case 238:
		return WeaponType::kKnife;
	case 239: // NOLINT(bugprone-branch-clone)
		return WeaponType::kThrowing;
	case 240:
		return WeaponType::kThrowing;
	case 241:
		return WeaponType::kThrowing;
	case 242:
		return WeaponType::kThrowing;
	case 243: // NOLINT(bugprone-branch-clone)
		return WeaponType::kJavelin;
	case 244:
		return WeaponType::kJavelin;
	case 245:
		return WeaponType::kJavelin;
	case 246:
		return WeaponType::kJavelin;
	case 247:
		return WeaponType::kJavelin;
	case 248: // NOLINT(bugprone-branch-clone)
		return WeaponType::kSpear;
	case 249:
		return WeaponType::kSpear;
	case 250:
		return WeaponType::kSpear;
	case 251:
		return WeaponType::kSpear;
	case 252:
		return WeaponType::kSpear;
	case 253: // NOLINT(bugprone-branch-clone)
		return WeaponType::kPole;
	case 254:
		return WeaponType::kPole;
	case 255:
		return WeaponType::kPole;
	case 256:
		return WeaponType::kPole;
	case 257:
		return WeaponType::kPole;
	case 258:
		return WeaponType::kPole;
	case 259: // NOLINT(bugprone-branch-clone)
		return WeaponType::kStaff;
	case 260:
		return WeaponType::kStaff;
	case 261:
		return WeaponType::kStaff;
	case 262:
		return WeaponType::kStaff;
	case 263:
		return WeaponType::kStaff;
	case 264: // NOLINT(bugprone-branch-clone)
		return WeaponType::kBow;
	case 265:
		return WeaponType::kBow;
	case 266:
		return WeaponType::kBow;
	case 267:
		return WeaponType::kBow;
	case 268:
		return WeaponType::kBow;
	case 269:
		return WeaponType::kBow;
	case 270:
		return WeaponType::kBow;
	case 271:
		return WeaponType::kBow;
	case 272: // NOLINT(bugprone-branch-clone)
		return WeaponType::kCrossbow;
	case 273:
		return WeaponType::kCrossbow;
	case 274:
		return WeaponType::kCrossbow;
	case 275:
		return WeaponType::kCrossbow;
	case 276: // NOLINT(bugprone-branch-clone)
		return WeaponType::kOrb;
	case 277:
		return WeaponType::kOrb;
	case 278:
		return WeaponType::kOrb;
	case 279:
		return WeaponType::kOrb;
	case 280:
		return WeaponType::kOrb;
	case 281: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaBow;
	case 282:
		return WeaponType::kAmaBow;
	case 283: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaSpear;
	case 284:
		return WeaponType::kAmaSpear;
	case 285:
		return WeaponType::kAmaJav;
	case 286: // NOLINT(bugprone-branch-clone)
		return WeaponType::kOrb;
	case 287:
		return WeaponType::kOrb;
	case 288:
		return WeaponType::kOrb;
	case 289:
		return WeaponType::kOrb;
	case 290:
		return WeaponType::kOrb;
	case 291: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaBow;
	case 292:
		return WeaponType::kAmaBow;
	case 293: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaSpear;
	case 294:
		return WeaponType::kAmaSpear;
	case 295:
		return WeaponType::kAmaJav;
	case 296: // NOLINT(bugprone-branch-clone)
		return WeaponType::kOrb;
	case 297:
		return WeaponType::kOrb;
	case 298:
		return WeaponType::kOrb;
	case 299:
		return WeaponType::kOrb;
	case 300:
		return WeaponType::kOrb;
	case 301: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaBow;
	case 302:
		return WeaponType::kAmaBow;
	case 303: // NOLINT(bugprone-branch-clone)
		return WeaponType::kAmaSpear;
	case 304:
		return WeaponType::kAmaSpear;
	case 305:
		return WeaponType::kAmaJav;
	default:
		return WeaponType::kUnknown;
	}
}
