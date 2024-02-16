#pragma once

#include <algorithm>
#include <list>
#include <string>
#include <Windows.h>

#include "../Hook.h"

#include "../../Config.h"
#include "../../Drawing.h"


struct DisplayedStat
{
	std::string name;
	int			id;
	int         value;
	bool        useValue;
};

enum class WeaponType
{
	kUnknown = -1,
	kAxe = 300,
	kAxe2H,
	kWand,
	kClub,
	kScepter,
	kMace,
	kHammer,
	kHammer2H,
	kSword,
	kSword2H,
	kKnife,
	kThrowing,
	kJavelin,
	kSpear,
	kPole,
	kStaff,
	kBow,
	kCrossbow,
	kThrowingPot,
	kClaw1,
	kClaw2,
	kOrb,
	kAmaBow,
	kAmaSpear,
	kAmaJav,
};

namespace Drawing
{
	class StatsDisplay;

	class StatsDisplay : public HookGroup
	{
	private:
		static StatsDisplay* display;
		std::string                   name;
		unsigned int                  x, y, xSize, ySize;
		bool                          active, minimized;
		CRITICAL_SECTION              crit;
		std::vector<DisplayedStat*>   customStats;
	public:
		StatsDisplay(std::string name);
		~StatsDisplay();

		void LoadConfig();

		void Lock() { EnterCriticalSection(&crit); };
		void Unlock() { LeaveCriticalSection(&crit); };

		std::string  GetName() { return name; };
		unsigned int GetX() { return x; };
		unsigned int GetY() { return y; };
		unsigned int GetXSize() { return xSize; };
		unsigned int GetYSize() { return ySize; };
		bool         IsActive() { return active; };
		bool         IsMinimized() { return minimized; };

		bool InRange(unsigned int x,
			unsigned int y);

		void SetX(unsigned int newX);
		void SetY(unsigned int newY);
		void SetXSize(unsigned int newXSize);
		void SetYSize(unsigned int newYSize);

		void SetName(std::string newName)
		{
			Lock();
			name = newName;
			Unlock();
		};

		void SetActive(bool newState)
		{
			Lock();
			active = newState;
			Unlock();
		};

		void SetMinimized(bool newState)
		{
			Lock();
			minimized = newState;
			Unlock();
		};

		void        OnDraw();
		static void Draw();

		bool OnClick(bool         up,
			unsigned int mouseX,
			unsigned int mouseY);
		static bool Click(bool         up,
			unsigned int mouseX,
			unsigned int mouseY);

		bool OnKey(bool   up,
			BYTE   key,
			LPARAM lParam);
		static bool KeyClick(bool   bUp,
			BYTE   bKey,
			LPARAM lParam);

		void StatsDisplay::GetBreakpointString(UnitAny* unit,
			int              nStat,
			std::vector<int> bps,
			char* bpCharString);

		void StatsDisplay::GetIASBreakpointString(UnitAny* pUnit, char* bpCharString, int x, int* y);
		static std::vector<int> StatsDisplay::GetRollbackSkillFrames(std::vector<int> nAnimFrames,
			int nActionFrame,
			int nTotalFrames,
			int nFrameBonus,
			int nAnimRate,
			int nSkillRollback);
		static int GetActIndex(int map_number, int difficulty);
		static WeaponType GetCurrentWeaponType(int dw_text_file_number);
	};
};
