#include "StatsDisplay.h"

#include "../../BH.h"
#include "../../D2Helpers.h"
#include "../../D2Ptrs.h"

#include "../../Modules/Item/Item.h"

#include "../Basic/Boxhook/Boxhook.h"
#include "../Basic/Framehook/Framehook.h"
#include "../Basic/Texthook/Texthook.h"


using namespace Drawing;

const int kWolfTxtFileNoAlias         = 139;
const int kBearTxtFileNoAlias         = 140;
const int kDruidOneHandFHRAlias       = 141;
const int kPaladinSpearStaveFHRAlias  = 142;
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
    { MERC_A5, { 0 } },
};

std::map<DWORD, std::vector<int>> increased_attack_speed_frames = {
    { CLASS_PAL, { 13 } },
    { CLASS_AMA, { 11 } },
    { CLASS_BAR, { 11 } },
    { CLASS_NEC, { 14 } },
    { CLASS_SOR, { 14 } },
    { CLASS_DRU, { 15 } },
    { CLASS_ASN, { 10 } },

    { MERC_A1, { 10 } },

    { MERC_A2, { 10 } },

    { MERC_A3, { 10 } },

    { MERC_A5, { 10 } },

};


DWORD syncCooldown = 0;

StatsDisplay* StatsDisplay::display;

StatsDisplay::StatsDisplay( std::string name )
{
    int yPos  = 10;
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

StatsDisplay::~StatsDisplay()
{
    Lock();
    // Remove all hooks associated with the display
    while ( Hooks.size() > 0 ) { delete ( *Hooks.begin() ); }
    Unlock();
    DeleteCriticalSection(&crit);
}

void StatsDisplay::LoadConfig()
{
    int height = 342 + 8 * 5 + 16 * 10;
    customStats.clear();

    BH::config->ReadToggle("Stats on Right", "None", false, Toggles["Stats on Right"]);

    vector<pair<string, string>> stats;
    BH::config->ReadMapList("Stat Screen", stats);
    for ( unsigned int i = 0; i < stats.size(); i++ )
    {
        std::transform(stats[i].first.begin(), stats[i].first.end(), stats[i].first.begin(), ::tolower);
        if ( StatMap.count(stats[i].first) > 0 )
        {
            StatProperties* sp         = StatMap[stats[i].first];
            DisplayedStat*  customStat = new DisplayedStat();
            customStat->name           = stats[i].first;
            customStat->useValue       = false;
            std::transform(customStat->name.begin(), customStat->name.end(), customStat->name.begin(), ::tolower);
            // Getting rid of the check for sp->saveParamBits > 0 to display weapon mastery values
            // if a param is supplied it will be used
            int          num = -1;
            stringstream ss(Trim(stats[i].second));
            if ( !( ss >> num ).fail() && num > 0 )
            {
                customStat->useValue = true;
                customStat->value    = num;
            }
            customStats.push_back(customStat);
        }
    }
    if ( customStats.size() > 0 ) { height += ( customStats.size() * 16 ) + 8; }

    int xPos = Toggles["Stats on Right"].state ? *p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
    SetX(xPos);
    SetYSize(height);
}

void StatsDisplay::SetX( unsigned int newX )
{
    if ( newX >= 0 && newX <= Hook::GetScreenWidth() )
    {
        Lock();
        x = newX;
        Unlock();
    }
}

void StatsDisplay::SetY( unsigned int newY )
{
    if ( newY >= 0 && newY <= Hook::GetScreenHeight() )
    {
        Lock();
        y = newY;
        Unlock();
    }
}

void StatsDisplay::SetXSize( unsigned int newXSize )
{
    if ( newXSize >= 0 && newXSize <= ( Hook::GetScreenWidth() - GetX() ) )
    {
        Lock();
        xSize = newXSize;
        Unlock();
    }
}

void StatsDisplay::SetYSize( unsigned int newYSize )
{
    if ( newYSize >= 0 && newYSize <= ( Hook::GetScreenHeight() - GetY() ) )
    {
        Lock();
        ySize = newYSize;
        Unlock();
    }
}

bool StatsDisplay::InRange( unsigned int x,
                            unsigned int y )
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
    UnitAny* unit   = D2CLIENT_GetPlayerUnit();
    bool     isMerc = false;
    if ( !unit )
        return;
    int column1 = GetX() + 5;
    int column2 = column1 + GetXSize() / 2;

    if ( !IsMinimized() )
    {
        int xPos = Toggles["Stats on Right"].state ? *p_D2CLIENT_ScreenSizeX - 10 - GetXSize() : 10;
        SetX(xPos);

        if ( D2CLIENT_GetUIState(UI_MERC) )
        {
            unit   = D2CLIENT_GetMercUnit();
            isMerc = true;
        }
        for ( std::list<Hook*>::iterator it = Hooks.begin(); it != Hooks.end(); ++it )
            ( *it )->OnDraw();

        int  y = GetY();
        RECT pRect;
        pRect.left   = GetX();
        pRect.top    = y;
        pRect.right  = x + GetXSize();
        pRect.bottom = y + GetYSize();
        Boxhook::Draw(GetX(), GetY(), GetXSize(), GetYSize(), White, Drawing::BTBlack);

        Framehook::DrawRectStub(&pRect);

        Texthook::Draw(column1,
                       ( y += 8 ),
                       None,
                       6,
                       Gold,
                       "Name:ÿc0 %s",
                       isMerc ? "ÿc;Mercenary" : unit->pPlayerData->szName);
        Texthook::Draw(pRect.right - 5,
                       y,
                       Right,
                       6,
                       Gold,
                       L"Level:ÿc0 %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LEVEL, 0)));
        Texthook::Draw(pRect.right - 5,
                       y + 12,
                       Right,
                       6,
                       Gold,
                       L"Additional XP:ÿc: %d%%",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ADDEXPERIENCE, 0)));

        y += 8;

        BnetData* pData          = ( *p_D2LAUNCH_BnData );
        int       xPacMultiplier = pData->nCharFlags & PLAYER_TYPE_EXPANSION ? 2 : 1;
        int       resPenalty[3]  = { RES_PENALTY_CLS_NORM, RES_PENALTY_CLS_NM, RES_PENALTY_CLS_HELL };
        int       penalty        = resPenalty[D2CLIENT_GetDifficulty()] * xPacMultiplier;
        int       fMax           = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXFIRERESIST, 0)) + 75;
        int       cMax           = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXCOLDRESIST, 0)) + 75;
        int       lMax           = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXLIGHTNINGRESIST, 0)) + 75;
        int       pMax           = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXPOISONRESIST, 0)) + 75;
        int       pLengthReduce  = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONLENGTHREDUCTION, 0));

        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Red,
                       L"ÿc4Fire Resist:ÿc1 %d ÿc0/ %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIRERESIST, 0)) + penalty,
                       fMax);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Blue,
                       L"ÿc4Cold Resist:ÿc3 %d ÿc0/ %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDRESIST, 0)) + penalty,
                       cMax);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Yellow,
                       L"ÿc4Lightning Resist:ÿc9 %d ÿc0/ %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGRESIST, 0)) + penalty,
                       lMax);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Poison Resist:ÿc2 %d ÿc0/ %d  ÿc4Length:ÿc: %d%%",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONRESIST, 0)) + penalty,
                       pMax,
                       ( 100 - penalty - pLengthReduce )
                      );
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Curse Resist: %d%%",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_CURSERESISTANCE, 0)) + penalty);
        y += 8;

        int fAbsorb    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREABSORB, 0));
        int fAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_FIREABSORBPERCENT, 0));
        int cAbsorb    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDABSORB, 0));
        int cAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_COLDABSORBPERCENT, 0));
        int lAbsorb    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORB, 0));
        int lAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_LIGHTNINGABSORBPERCENT, 0));
        int mAbsorb    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICABSORB, 0));
        int mAbsorbPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICABSORBPERCENT, 0));
        Texthook::Draw(column1,
                       ( y += 16 ),
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

        int dmgReduction    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTION, 0));
        int dmgReductionPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DMGREDUCTIONPCT, 0));
        int magReduction    = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTION, 0));
        int magReductionPct = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAGICDMGREDUCTIONPCT, 0));
        Texthook::Draw(column1,
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Elemental Pierce:ÿc1 %d%%ÿc3 %d%%ÿc9 %d%%ÿc2 %d%%ÿc8 %d%%",
                       fPierce,
                       cPierce,
                       lPierce,
                       pPierce,
                       mPierce);
        int  classNum   = pData->nCharClass;
        auto classArMod = CharList[classNum]->toHitFactor - 35;
        int  dexAR      = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0)) * 5 + classArMod;
        int  gearAR     = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ATTACKRATING, 0));

        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Base AR:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0% dÿc5 total:ÿc0 %d",
                       dexAR,
                       gearAR,
                       dexAR + gearAR);

        int gearDef = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEFENSE, 0));
        int dexDef  = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEXTERITY, 0)) / 4;

        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Base Def:ÿc5 dex:ÿc0 %dÿc5 equip:ÿc0 %dÿc5 total:ÿc0 %d",
                       dexDef,
                       gearDef,
                       dexDef + gearDef);

        Texthook::Draw(column1,
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
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


        auto weapon_type = GetCurrentWeaponType(unit->pInventory);

        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Breakpoints (FCR relative to Right Click Skill):");


        auto fcr_key = unit->dwTxtFileNo;
        fcr_key      = D2COMMON_GetUnitState(unit, 139) ? kWolfTxtFileNoAlias : fcr_key;
        fcr_key      = D2COMMON_GetUnitState(unit, 140) ? kBearTxtFileNoAlias : fcr_key;

        auto skillId = !isMerc ? unit->pInfo->pRightSkill->pSkillInfo->wSkillId : 0;
        fcr_key      = skillId == 54
                           ? kSorceressLightningFCRAlias
                           : skillId == 53
                           ? kSorceressLightningFCRAlias
                           : skillId == 49
                           ? kSorceressLightningFCRAlias
                           : skillId == 64
                           ? kSorceressLightningFCRAlias
                           : fcr_key;

        char bp_fcr_string[255] = "";
        GetBreakpointString(unit, STAT_FASTERCAST, faster_cast_rate_frames[fcr_key], (char*)&bp_fcr_string);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       "FCR:ÿc0 %s",
                       bp_fcr_string);


        auto fhr_key = unit->dwTxtFileNo;
        fhr_key      = D2COMMON_GetUnitState(unit, 139) ? kWolfTxtFileNoAlias : fhr_key;
        fhr_key      = D2COMMON_GetUnitState(unit, 140) ? kBearTxtFileNoAlias : fhr_key;

        fhr_key = fhr_key == CLASS_PAL
                      ? ( weapon_type == WeaponType::kSpear || weapon_type == WeaponType::kStaff )
                            ? kPaladinSpearStaveFHRAlias
                            : fhr_key
                      : fhr_key == CLASS_DRU
                      ? ( weapon_type != WeaponType::kStaff && weapon_type != WeaponType::kSpear &&
                          weapon_type != WeaponType::kHammer2H && weapon_type != WeaponType::kAxe2H &&
                          weapon_type != WeaponType::kPole )
                            ? kDruidOneHandFHRAlias
                            : fhr_key
                      : fhr_key;

        char bp_string[255] = "";
        GetBreakpointString(unit, STAT_FASTERHITRECOVERY, faster_hit_recovery_frames[fhr_key], (char*)&bp_string);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       "FHR:ÿc0 %s",
                       bp_string);


        auto ias_key = unit->dwTxtFileNo;

        char bp_ias_string[255] = "";
        GetBreakpointString(unit, STAT_ATTACKRATE, increased_attack_speed_frames[ias_key], (char*)&bp_ias_string);
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       "IAS:ÿc0 %s",
                       bp_ias_string);


        y += 8;

        Texthook::Draw(column1,
                       ( y += 16 ),
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
                       L"Open Wounds: ÿc0%d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_OPENWOUNDS, 0)));
        Texthook::Draw(column1,
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Deadly Strike:ÿc0 %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_DEADLYSTRIKE, 0)));
        Texthook::Draw(column2,
                       y,
                       None,
                       6,
                       Gold,
                       L"Critical Strike: ÿc0%d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_CRITICALSTRIKE, 0)));
        Texthook::Draw(column1,
                       ( y += 16 ),
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
                       ( y += 16 ),
                       None,
                       6,
                       Gold,
                       L"Projectile Pierce:ÿc0 %d",
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PIERCINGATTACK, 0)) +
                       static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_PIERCE, 0)));

        int minFire              = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMFIREDAMAGE, 0));
        int maxFire              = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMFIREDAMAGE, 0));
        int minLight             = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMLIGHTNINGDAMAGE, 0));
        int maxLight             = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMLIGHTNINGDAMAGE, 0));
        int minCold              = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMCOLDDAMAGE, 0));
        int maxCold              = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMCOLDDAMAGE, 0));
        int minPoison            = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMPOISONDAMAGE, 0));
        int maxPoison            = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMPOISONDAMAGE, 0));
        int poisonLength         = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_POISONDAMAGELENGTH, 0));
        int poisonLengthOverride = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_SKILLPOISONOVERRIDELEN, 0));
        if ( poisonLengthOverride > 0 ) { poisonLength = poisonLengthOverride; }
        int minMagic  = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MINIMUMMAGICALDAMAGE, 0));
        int maxMagic  = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_MAXIMUMMAGICALDAMAGE, 0));
        int addedPhys = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_ADDSDAMAGE, 0));
        y += 8;
        Texthook::Draw(column1,
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
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
                       ( y += 16 ),
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

        if ( !isMerc )
        {
            Texthook::Draw(column1,
                           ( y += 16 ),
                           None,
                           6,
                           Gold,
                           L"Stash Gold:ÿc9 %d",
                           static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_GOLDBANK, 0)));
        }

        //int cowKingKilled = D2COMMON_GetQuestFlag(D2CLIENT_GetQuestInfo(), 4, 10);
        //Texthook::Draw(column2, y, None, 6, Gold,
        //		L"Cow King:ÿc0 %s", cowKingKilled ? L"killed" : L"alive");

        if ( customStats.size() > 0 )
        {
            y += 8;
            for ( unsigned int i = 0; i < customStats.size(); i++ )
            {
                int secondary = customStats[i]->useValue ? customStats[i]->value : 0;
                int stat      = static_cast<int>(D2COMMON_GetUnitStat(unit, STAT_NUMBER(customStats[i]->name), secondary));
                if ( secondary > 0 )
                {
                    Texthook::Draw(column1,
                                   ( y += 16 ),
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
                                   ( y += 16 ),
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

bool StatsDisplay::KeyClick( bool   bUp,
                             BYTE   bKey,
                             LPARAM lParam )
{
    display->Lock();
    bool block = display->OnKey(bUp, bKey, lParam);
    display->Unlock();
    return block;
}

bool StatsDisplay::OnKey( bool   up,
                          BYTE   kkey,
                          LPARAM lParam )
{
    UnitAny* unit = D2CLIENT_GetPlayerUnit();
    if ( !unit )
        return false;
    Item* item = static_cast<Item*>(BH::moduleManager->Get("item"));
    //Resync
    if ( !up && kkey == item->resyncKey )
    {
        DWORD curTime = GetTickCount64();
        if ( curTime >= syncCooldown )
        {
            SendSyncMsg();
            syncCooldown = curTime + 1000;
        }
    }
    if ( IsMinimized() )
    {
        if ( !up && kkey == item->advStatMenuKey )
        {
            LoadConfig();
            SetMinimized(false);
            return true;
        }
    }
    else
    {
        if ( !up && ( kkey == item->advStatMenuKey || kkey == VK_ESCAPE ) )
        {
            SetMinimized(true);
            return true;
        }
    }
    return false;
}

bool StatsDisplay::Click( bool         up,
                          unsigned int mouseX,
                          unsigned int mouseY )
{
    display->Lock();
    bool block = display->OnClick(up, mouseX, mouseY);
    display->Unlock();
    return block;
}

bool StatsDisplay::OnClick( bool         up,
                            unsigned int x,
                            unsigned int y )
{
    UnitAny* unit = D2CLIENT_GetPlayerUnit();
    if ( !unit )
        return false;

    if ( !IsMinimized() && InRange(x, y) )
    {
        SetMinimized(true);
        return true;
    }
    return false;
}

void StatsDisplay::GetBreakpointString( UnitAny*         unit,
                                        int              nStat,
                                        std::vector<int> bps,
                                        char*            bpCharString )
{
    std::string bpString = "";
    int         bp       = static_cast<int>(D2COMMON_GetUnitStat(unit, nStat, 0));
    for ( int i = 0; i < bps.size() - 1; i++ )
    {
        if ( bp >= bps[i] && bp < bps[i + 1] )
        {
            bpString += "ÿc8";
            bpString += std::to_string(bps[i]);
            bpString += "ÿc0 / ";
        }
        else { bpString += std::to_string(bps[i]) + " / "; }
    }
    if ( bp >= bps[bps.size() - 1] ) { bpString += "ÿc8"; }
    bpString += std::to_string(bps[bps.size() - 1]);

    sprintf(bpCharString, "%.255s", bpString.c_str());
}

WeaponType StatsDisplay::GetCurrentWeaponType( Inventory* inventory )
{
    if ( inventory == nullptr ) return WeaponType::kUnknown;
    if ( inventory->pFirstItem == nullptr ) return WeaponType::kUnknown;

    auto* current_item = inventory->pFirstItem;
    do
    {
        if ( current_item->pItemData->BodyLocation == 4 || current_item->pItemData->BodyLocation == 5 ) { return GetCurrentWeaponType(current_item->dwTxtFileNo); }
        current_item = current_item->pItemData->pNextInvItem;
    }
    while ( current_item != nullptr );

    return WeaponType::kUnknown;
}

WeaponType StatsDisplay::GetCurrentWeaponType( const int weapon_number )
{
    if ( weapon_number >= 306 ) return WeaponType::kUnknown;
    if ( weapon_number <= 4 ||
         weapon_number >= 93 && weapon_number <= 97 ||
         weapon_number >= 196 && weapon_number <= 200 ) { return WeaponType::kAxe; }
    if ( weapon_number >= 5 && weapon_number <= 9 ||
         weapon_number >= 98 && weapon_number <= 102 ||
         weapon_number >= 201 && weapon_number <= 205 ) { return WeaponType::kAxe2H; }
    if ( weapon_number >= 10 && weapon_number <= 13 ||
         weapon_number >= 103 && weapon_number <= 106 ||
         weapon_number >= 206 && weapon_number <= 209 ) { return WeaponType::kWand; }
    if ( weapon_number == 14 || weapon_number == 18 ||
         weapon_number == 88 ||
         weapon_number == 107 || weapon_number == 111 ||
         weapon_number == 210 || weapon_number == 214 ) { return WeaponType::kClub; }
    if ( weapon_number >= 15 && weapon_number <= 17 ||
         weapon_number >= 108 && weapon_number <= 110 ||
         weapon_number >= 211 && weapon_number <= 213 ) { return WeaponType::kScepter; }
    if ( weapon_number >= 19 && weapon_number <= 21 ||
         weapon_number >= 112 && weapon_number <= 114 ||
         weapon_number >= 173 && weapon_number <= 174 ||
         weapon_number >= 215 && weapon_number <= 217 ) { return WeaponType::kMace; }
    if ( weapon_number == 22 ||
         weapon_number >= 89 && weapon_number <= 90 ||
         weapon_number == 115 ||
         weapon_number == 218 ) { return WeaponType::kHammer; }
    if ( weapon_number >= 23 && weapon_number <= 24 ||
         weapon_number >= 116 && weapon_number <= 117 ||
         weapon_number >= 219 && weapon_number <= 220 ) { return WeaponType::kHammer2H; }
    if ( weapon_number >= 25 && weapon_number <= 32 ||
         weapon_number >= 118 && weapon_number <= 125 ||
         weapon_number >= 211 && weapon_number <= 228 ) { return WeaponType::kSword; }
    if ( weapon_number >= 33 && weapon_number <= 38 ||
         weapon_number >= 126 && weapon_number <= 131 ||
         weapon_number >= 229 && weapon_number <= 234 ) { return WeaponType::kSword2H; }
    if ( weapon_number >= 39 && weapon_number <= 42 ||
         weapon_number >= 86 && weapon_number <= 87 ||
         weapon_number >= 132 && weapon_number <= 135 ||
         weapon_number >= 235 && weapon_number <= 238 ) { return WeaponType::kKnife; }
    if ( weapon_number >= 43 && weapon_number <= 46 ||
         weapon_number >= 136 && weapon_number <= 139 ||
         weapon_number >= 239 && weapon_number <= 242 ) { return WeaponType::kThrowing; }
    if ( weapon_number >= 47 && weapon_number <= 51 ||
         weapon_number >= 140 && weapon_number <= 144 ||
         weapon_number >= 243 && weapon_number <= 247 ) { return WeaponType::kJavelin; }
    if ( weapon_number >= 52 && weapon_number <= 56 ||
         weapon_number >= 145 && weapon_number <= 149 ||
         weapon_number >= 248 && weapon_number <= 252 ) { return WeaponType::kSpear; }
    if ( weapon_number >= 57 && weapon_number <= 62 ||
         weapon_number >= 150 && weapon_number <= 155 ||
         weapon_number >= 253 && weapon_number <= 258 ) { return WeaponType::kPole; }
    if ( weapon_number >= 63 && weapon_number <= 67 ||
         weapon_number >= 91 && weapon_number <= 92 ||
         weapon_number >= 156 && weapon_number <= 160 ||
         weapon_number >= 259 && weapon_number <= 263 ) { return WeaponType::kStaff; }
    if ( weapon_number >= 68 && weapon_number <= 75 ||
         weapon_number >= 161 && weapon_number <= 168 ||
         weapon_number >= 264 && weapon_number <= 271 ) { return WeaponType::kBow; }
    if ( weapon_number >= 76 && weapon_number <= 79 ||
         weapon_number >= 169 && weapon_number <= 172 ||
         weapon_number >= 272 && weapon_number <= 275 ) { return WeaponType::kBow; }
    if ( weapon_number >= 80 && weapon_number <= 85 ) { return WeaponType::kThrowingPot; }
    if ( weapon_number >= 175 && weapon_number <= 184 ) { return WeaponType::kClaw1; }
    if ( weapon_number >= 185 && weapon_number <= 195 ) { return WeaponType::kClaw2; }
    if ( weapon_number >= 276 && weapon_number <= 280 ||
         weapon_number >= 286 && weapon_number <= 290 ||
         weapon_number >= 296 && weapon_number <= 300 ) { return WeaponType::kOrb; }
    if ( weapon_number >= 281 && weapon_number <= 282 ||
         weapon_number >= 291 && weapon_number <= 292 ||
         weapon_number >= 301 && weapon_number <= 302 ) { return WeaponType::kAmaBow; }
    if ( weapon_number >= 283 && weapon_number <= 284 ||
         weapon_number >= 293 && weapon_number <= 294 ||
         weapon_number >= 303 && weapon_number <= 304 ) { return WeaponType::kAmaSpear; }
    if ( weapon_number == 285 ||
         weapon_number == 295 ||
         weapon_number == 305 ) { return WeaponType::kAmaJav; }

    return WeaponType::kUnknown;
}
