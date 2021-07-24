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
    switch ( weapon_number )
    {
        case 1: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe;
        case 2:
            return WeaponType::kAxe;
        case 3:
            return WeaponType::kAxe;
        case 4:
            return WeaponType::kAxe;
        case 5:
            return WeaponType::kAxe;
        case 6: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe2H;
        case 7:
            return WeaponType::kAxe2H;
        case 8:
            return WeaponType::kAxe2H;
        case 9:
            return WeaponType::kAxe2H;
        case 10:
            return WeaponType::kAxe2H;
        case 11: // NOLINT(bugprone-branch-clone)
            return WeaponType::kWand;
        case 12:
            return WeaponType::kWand;
        case 13:
            return WeaponType::kWand;
        case 14:
            return WeaponType::kWand;
        case 15:
            return WeaponType::kClub;
        case 16: // NOLINT(bugprone-branch-clone)
            return WeaponType::kScepter;
        case 17:
            return WeaponType::kScepter;
        case 18:
            return WeaponType::kScepter;
        case 19:
            return WeaponType::kClub;
        case 20: // NOLINT(bugprone-branch-clone)
            return WeaponType::kMace;
        case 21:
            return WeaponType::kMace;
        case 22:
            return WeaponType::kMace;
        case 23:
            return WeaponType::kHammer;
        case 24: // NOLINT(bugprone-branch-clone)
            return WeaponType::kHammer2H;
        case 25:
            return WeaponType::kHammer2H;
        case 26: // NOLINT(bugprone-branch-clone)
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
        case 33:
            return WeaponType::kSword;
        case 34: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSword2H;
        case 35:
            return WeaponType::kSword2H;
        case 36:
            return WeaponType::kSword2H;
        case 37:
            return WeaponType::kSword2H;
        case 38:
            return WeaponType::kSword2H;
        case 39:
            return WeaponType::kSword2H;
        case 40: // NOLINT(bugprone-branch-clone)
            return WeaponType::kKnife;
        case 41:
            return WeaponType::kKnife;
        case 42:
            return WeaponType::kKnife;
        case 43:
            return WeaponType::kKnife;
        case 44: // NOLINT(bugprone-branch-clone)
            return WeaponType::kThrowing;
        case 45:
            return WeaponType::kThrowing;
        case 46:
            return WeaponType::kThrowing;
        case 47:
            return WeaponType::kThrowing;
        case 48: // NOLINT(bugprone-branch-clone)
            return WeaponType::kJavelin;
        case 49:
            return WeaponType::kJavelin;
        case 50:
            return WeaponType::kJavelin;
        case 51:
            return WeaponType::kJavelin;
        case 52:
            return WeaponType::kJavelin;
        case 53: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSpear;
        case 54:
            return WeaponType::kSpear;
        case 55:
            return WeaponType::kSpear;
        case 56:
            return WeaponType::kSpear;
        case 57:
            return WeaponType::kSpear;
        case 58: // NOLINT(bugprone-branch-clone)
            return WeaponType::kPole;
        case 59:
            return WeaponType::kPole;
        case 60:
            return WeaponType::kPole;
        case 61:
            return WeaponType::kPole;
        case 62:
            return WeaponType::kPole;
        case 63:
            return WeaponType::kPole;
        case 64: // NOLINT(bugprone-branch-clone)
            return WeaponType::kStaff;
        case 65:
            return WeaponType::kStaff;
        case 66:
            return WeaponType::kStaff;
        case 67:
            return WeaponType::kStaff;
        case 68:
            return WeaponType::kStaff;
        case 69: // NOLINT(bugprone-branch-clone)
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
        case 76:
            return WeaponType::kBow;
        case 77: // NOLINT(bugprone-branch-clone)
            return WeaponType::kCrossbow;
        case 78:
            return WeaponType::kCrossbow;
        case 79:
            return WeaponType::kCrossbow;
        case 80:
            return WeaponType::kCrossbow;
        case 81: // NOLINT(bugprone-branch-clone)
            return WeaponType::kThrowingPot;
        case 82:
            return WeaponType::kThrowingPot;
        case 83:
            return WeaponType::kThrowingPot;
        case 84:
            return WeaponType::kThrowingPot;
        case 85:
            return WeaponType::kThrowingPot;
        case 86:
            return WeaponType::kThrowingPot;
        case 87: // NOLINT(bugprone-branch-clone)
            return WeaponType::kKnife;
        case 88:
            return WeaponType::kKnife;
        case 89:
            return WeaponType::kClub;
        case 90: // NOLINT(bugprone-branch-clone)
            return WeaponType::kHammer;
        case 91:
            return WeaponType::kHammer;
        case 92: // NOLINT(bugprone-branch-clone)
            return WeaponType::kStaff;
        case 93:
            return WeaponType::kStaff;
        case 94: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe;
        case 95:
            return WeaponType::kAxe;
        case 96:
            return WeaponType::kAxe;
        case 97:
            return WeaponType::kAxe;
        case 98:
            return WeaponType::kAxe;
        case 99: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe2H;
        case 100:
            return WeaponType::kAxe2H;
        case 101:
            return WeaponType::kAxe2H;
        case 102:
            return WeaponType::kAxe2H;
        case 103:
            return WeaponType::kAxe2H;
        case 104: // NOLINT(bugprone-branch-clone)
            return WeaponType::kWand;
        case 105:
            return WeaponType::kWand;
        case 106:
            return WeaponType::kWand;
        case 107:
            return WeaponType::kWand;
        case 108:
            return WeaponType::kClub;
        case 109: // NOLINT(bugprone-branch-clone)
            return WeaponType::kScepter;
        case 110:
            return WeaponType::kScepter;
        case 111:
            return WeaponType::kScepter;
        case 112:
            return WeaponType::kClub;
        case 113: // NOLINT(bugprone-branch-clone)
            return WeaponType::kMace;
        case 114:
            return WeaponType::kMace;
        case 115:
            return WeaponType::kMace;
        case 116:
            return WeaponType::kHammer;
        case 117: // NOLINT(bugprone-branch-clone)
            return WeaponType::kHammer2H;
        case 118:
            return WeaponType::kHammer2H;
        case 119: // NOLINT(bugprone-branch-clone)
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
        case 126:
            return WeaponType::kSword;
        case 127: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSword2H;
        case 128:
            return WeaponType::kSword2H;
        case 129:
            return WeaponType::kSword2H;
        case 130:
            return WeaponType::kSword2H;
        case 131:
            return WeaponType::kSword2H;
        case 132:
            return WeaponType::kSword2H;
        case 133: // NOLINT(bugprone-branch-clone)
            return WeaponType::kKnife;
        case 134:
            return WeaponType::kKnife;
        case 135:
            return WeaponType::kKnife;
        case 136:
            return WeaponType::kKnife;
        case 137: // NOLINT(bugprone-branch-clone)
            return WeaponType::kThrowing;
        case 138:
            return WeaponType::kThrowing;
        case 139:
            return WeaponType::kThrowing;
        case 140:
            return WeaponType::kThrowing;
        case 141: // NOLINT(bugprone-branch-clone)
            return WeaponType::kJavelin;
        case 142:
            return WeaponType::kJavelin;
        case 143:
            return WeaponType::kJavelin;
        case 144:
            return WeaponType::kJavelin;
        case 145:
            return WeaponType::kJavelin;
        case 146: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSpear;
        case 147:
            return WeaponType::kSpear;
        case 148:
            return WeaponType::kSpear;
        case 149:
            return WeaponType::kSpear;
        case 150:
            return WeaponType::kSpear;
        case 151: // NOLINT(bugprone-branch-clone)
            return WeaponType::kPole;
        case 152:
            return WeaponType::kPole;
        case 153:
            return WeaponType::kPole;
        case 154:
            return WeaponType::kPole;
        case 155:
            return WeaponType::kPole;
        case 156:
            return WeaponType::kPole;
        case 157: // NOLINT(bugprone-branch-clone)
            return WeaponType::kStaff;
        case 158:
            return WeaponType::kStaff;
        case 159:
            return WeaponType::kStaff;
        case 160:
            return WeaponType::kStaff;
        case 161:
            return WeaponType::kStaff;
        case 162: // NOLINT(bugprone-branch-clone)
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
        case 169:
            return WeaponType::kBow;
        case 170: // NOLINT(bugprone-branch-clone)
            return WeaponType::kCrossbow;
        case 171:
            return WeaponType::kCrossbow;
        case 172:
            return WeaponType::kCrossbow;
        case 173:
            return WeaponType::kCrossbow;
        case 174: // NOLINT(bugprone-branch-clone)
            return WeaponType::kMace;
        case 175:
            return WeaponType::kMace;
        case 176:
            return WeaponType::kUnknown;
        case 177: // NOLINT(bugprone-branch-clone)
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
        case 185:
            return WeaponType::kClaw1;
        case 186:
            return WeaponType::kClaw1;
        case 187: // NOLINT(bugprone-branch-clone)
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
        case 196:
            return WeaponType::kClaw2;
        case 197:
            return WeaponType::kClaw2;
        case 198: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe;
        case 199:
            return WeaponType::kAxe;
        case 200:
            return WeaponType::kAxe;
        case 201:
            return WeaponType::kAxe;
        case 202:
            return WeaponType::kAxe;
        case 203: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAxe2H;
        case 204:
            return WeaponType::kAxe2H;
        case 205:
            return WeaponType::kAxe2H;
        case 206:
            return WeaponType::kAxe2H;
        case 207:
            return WeaponType::kAxe2H;
        case 208: // NOLINT(bugprone-branch-clone)
            return WeaponType::kWand;
        case 209:
            return WeaponType::kWand;
        case 210:
            return WeaponType::kWand;
        case 211:
            return WeaponType::kWand;
        case 212:
            return WeaponType::kClub;
        case 213: // NOLINT(bugprone-branch-clone)
            return WeaponType::kScepter;
        case 214:
            return WeaponType::kScepter;
        case 215:
            return WeaponType::kScepter;
        case 216:
            return WeaponType::kClub;
        case 217: // NOLINT(bugprone-branch-clone)
            return WeaponType::kMace;
        case 218:
            return WeaponType::kMace;
        case 219:
            return WeaponType::kMace;
        case 220:
            return WeaponType::kHammer;
        case 221: // NOLINT(bugprone-branch-clone)
            return WeaponType::kHammer2H;
        case 222:
            return WeaponType::kHammer2H;
        case 223: // NOLINT(bugprone-branch-clone)
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
        case 229:
            return WeaponType::kSword;
        case 230:
            return WeaponType::kSword;
        case 231: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSword2H;
        case 232:
            return WeaponType::kSword2H;
        case 233:
            return WeaponType::kSword2H;
        case 234:
            return WeaponType::kSword2H;
        case 235:
            return WeaponType::kSword2H;
        case 236:
            return WeaponType::kSword2H;
        case 237: // NOLINT(bugprone-branch-clone)
            return WeaponType::kKnife;
        case 238:
            return WeaponType::kKnife;
        case 239:
            return WeaponType::kKnife;
        case 240:
            return WeaponType::kKnife;
        case 241: // NOLINT(bugprone-branch-clone)
            return WeaponType::kThrowing;
        case 242:
            return WeaponType::kThrowing;
        case 243:
            return WeaponType::kThrowing;
        case 244:
            return WeaponType::kThrowing;
        case 245: // NOLINT(bugprone-branch-clone)
            return WeaponType::kJavelin;
        case 246:
            return WeaponType::kJavelin;
        case 247:
            return WeaponType::kJavelin;
        case 248:
            return WeaponType::kJavelin;
        case 249:
            return WeaponType::kJavelin;
        case 250: // NOLINT(bugprone-branch-clone)
            return WeaponType::kSpear;
        case 251:
            return WeaponType::kSpear;
        case 252:
            return WeaponType::kSpear;
        case 253:
            return WeaponType::kSpear;
        case 254:
            return WeaponType::kSpear;
        case 255: // NOLINT(bugprone-branch-clone)
            return WeaponType::kPole;
        case 256:
            return WeaponType::kPole;
        case 257:
            return WeaponType::kPole;
        case 258:
            return WeaponType::kPole;
        case 259:
            return WeaponType::kPole;
        case 260:
            return WeaponType::kPole;
        case 261: // NOLINT(bugprone-branch-clone)
            return WeaponType::kStaff;
        case 262:
            return WeaponType::kStaff;
        case 263:
            return WeaponType::kStaff;
        case 264:
            return WeaponType::kStaff;
        case 265:
            return WeaponType::kStaff;
        case 266: // NOLINT(bugprone-branch-clone)
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
        case 272:
            return WeaponType::kBow;
        case 273:
            return WeaponType::kBow;
        case 274: // NOLINT(bugprone-branch-clone)
            return WeaponType::kCrossbow;
        case 275:
            return WeaponType::kCrossbow;
        case 276:
            return WeaponType::kCrossbow;
        case 277:
            return WeaponType::kCrossbow;
        case 278: // NOLINT(bugprone-branch-clone)
            return WeaponType::kOrb;
        case 279:
            return WeaponType::kOrb;
        case 280:
            return WeaponType::kOrb;
        case 281:
            return WeaponType::kOrb;
        case 282:
            return WeaponType::kOrb;
        case 283: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaBow;
        case 284:
            return WeaponType::kAmaBow;
        case 285: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaSpear;
        case 286:
            return WeaponType::kAmaSpear;
        case 287:
            return WeaponType::kAmaJav;
        case 288: // NOLINT(bugprone-branch-clone)
            return WeaponType::kOrb;
        case 289:
            return WeaponType::kOrb;
        case 290:
            return WeaponType::kOrb;
        case 291:
            return WeaponType::kOrb;
        case 292:
            return WeaponType::kOrb;
        case 293: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaBow;
        case 294:
            return WeaponType::kAmaBow;
        case 295: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaSpear;
        case 296:
            return WeaponType::kAmaSpear;
        case 297:
            return WeaponType::kAmaJav;
        case 298: // NOLINT(bugprone-branch-clone)
            return WeaponType::kOrb;
        case 299:
            return WeaponType::kOrb;
        case 300:
            return WeaponType::kOrb;
        case 301:
            return WeaponType::kOrb;
        case 302:
            return WeaponType::kOrb;
        case 303: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaBow;
        case 304:
            return WeaponType::kAmaBow;
        case 305: // NOLINT(bugprone-branch-clone)
            return WeaponType::kAmaSpear;
        case 306:
            return WeaponType::kAmaSpear;
        case 307:
            return WeaponType::kAmaJav;
        default:
            return WeaponType::kUnknown;
    }
}
