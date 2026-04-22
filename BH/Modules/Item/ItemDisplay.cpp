#include "ItemDisplay.h"
#include "Item.h"
#include "../../Drawing/Stats/StatsDisplay.h"
#include "../../D2Helpers.h"
#include "../../Common.h"
#include <cctype>
#include <vector>
#include <string>
#include <sstream>

#define MAP_COLOR_WHITE     0x20

// All colors here must also be defined in ReplacementMap
#define MAP_COLOR_REPLACEMENTS	\
	{L"WHITE", 0x20},		\
	{L"RED", 0x0A},			\
	{L"GREEN", 0x84},		\
	{L"BLUE", 0x97},			\
	{L"GOLD", 0x0D},			\
	{L"GRAY", 0xD0},			\
	{L"BLACK", 0x00},		\
	{L"TAN", 0x5A},			\
	{L"ORANGE", 0x60},		\
	{L"YELLOW", 0x0C},		\
	{L"PURPLE", 0x9B},		\
	{L"DARK_GREEN", 0x76}, \
	{L"CORAL", 0x66}, \
	{L"SAGE", 0x82}, \
	{L"TEAL", 0xCB}, \
	{L"LIGHT_GRAY", 0xD6}, \
	{L"FULL_TRANS", 0xCB}, \
	{L"THREE_FOURTHS_TRANS", 0xCB}, \
	{L"HALF_TRANS", 0xCB}, \
	{L"QUARTER_TRANS", 0xCB}

std::map<std::string, int> code_to_dwtxtfileno = {
		{"hax", 0},
		{"axe", 1},
		{"2ax", 2},
		{"mpi", 3},
		{"wax", 4},
		{"lax", 5},
		{"bax", 6},
		{"btx", 7},
		{"gax", 8},
		{"gix", 9},
		{"wnd", 10},
		{"ywn", 11},
		{"bwn", 12},
		{"gwn", 13},
		{"clb", 14},
		{"scp", 15},
		{"gsc", 16},
		{"wsp", 17},
		{"spc", 18},
		{"mac", 19},
		{"mst", 20},
		{"fla", 21},
		{"whm", 22},
		{"mau", 23},
		{"gma", 24},
		{"ssd", 25},
		{"scm", 26},
		{"sbr", 27},
		{"flc", 28},
		{"crs", 29},
		{"bsd", 30},
		{"lsd", 31},
		{"wsd", 32},
		{"2hs", 33},
		{"clm", 34},
		{"gis", 35},
		{"bsw", 36},
		{"flb", 37},
		{"gsd", 38},
		{"dgr", 39},
		{"dir", 40},
		{"kri", 41},
		{"bld", 42},
		{"tkf", 43},
		{"tax", 44},
		{"bkf", 45},
		{"bal", 46},
		{"jav", 47},
		{"pil", 48},
		{"ssp", 49},
		{"glv", 50},
		{"tsp", 51},
		{"spr", 52},
		{"tri", 53},
		{"brn", 54},
		{"spt", 55},
		{"pik", 56},
		{"bar", 57},
		{"vou", 58},
		{"scy", 59},
		{"pax", 60},
		{"hal", 61},
		{"wsc", 62},
		{"sst", 63},
		{"lst", 64},
		{"cst", 65},
		{"bst", 66},
		{"wst", 67},
		{"sbw", 68},
		{"hbw", 69},
		{"lbw", 70},
		{"cbw", 71},
		{"sbb", 72},
		{"lbb", 73},
		{"swb", 74},
		{"lwb", 75},
		{"lxb", 76},
		{"mxb", 77},
		{"hxb", 78},
		{"rxb", 79},
		{"gps", 80},
		{"ops", 81},
		{"gpm", 82},
		{"opm", 83},
		{"gpl", 84},
		{"opl", 85},
		{"d33", 86},
		{"g33", 87},
		{"leg", 88},
		{"hdm", 89},
		{"hfh", 90},
		{"hst", 91},
		{"msf", 92},
		{"9ha", 93},
		{"9ax", 94},
		{"92a", 95},
		{"9mp", 96},
		{"9wa", 97},
		{"9la", 98},
		{"9ba", 99},
		{"9bt", 100},
		{"9ga", 101},
		{"9gi", 102},
		{"9wn", 103},
		{"9yw", 104},
		{"9bw", 105},
		{"9gw", 106},
		{"9cl", 107},
		{"9sc", 108},
		{"9qs", 109},
		{"9ws", 110},
		{"9sp", 111},
		{"9ma", 112},
		{"9mt", 113},
		{"9fl", 114},
		{"9wh", 115},
		{"9m9", 116},
		{"9gm", 117},
		{"9ss", 118},
		{"9sm", 119},
		{"9sb", 120},
		{"9fc", 121},
		{"9cr", 122},
		{"9bs", 123},
		{"9ls", 124},
		{"9wd", 125},
		{"92h", 126},
		{"9cm", 127},
		{"9gs", 128},
		{"9b9", 129},
		{"9fb", 130},
		{"9gd", 131},
		{"9dg", 132},
		{"9di", 133},
		{"9kr", 134},
		{"9bl", 135},
		{"9tk", 136},
		{"9ta", 137},
		{"9bk", 138},
		{"9b8", 139},
		{"9ja", 140},
		{"9pi", 141},
		{"9s9", 142},
		{"9gl", 143},
		{"9ts", 144},
		{"9sr", 145},
		{"9tr", 146},
		{"9br", 147},
		{"9st", 148},
		{"9p9", 149},
		{"9b7", 150},
		{"9vo", 151},
		{"9s8", 152},
		{"9pa", 153},
		{"9h9", 154},
		{"9wc", 155},
		{"8ss", 156},
		{"8ls", 157},
		{"8cs", 158},
		{"8bs", 159},
		{"8ws", 160},
		{"8sb", 161},
		{"8hb", 162},
		{"8lb", 163},
		{"8cb", 164},
		{"8s8", 165},
		{"8l8", 166},
		{"8sw", 167},
		{"8lw", 168},
		{"8lx", 169},
		{"8mx", 170},
		{"8hx", 171},
		{"8rx", 172},
		{"qf1", 173},
		{"qf2", 174},
		{"ktr", 175},
		{"wrb", 176},
		{"axf", 177},
		{"ces", 178},
		{"clw", 179},
		{"btl", 180},
		{"skr", 181},
		{"9ar", 182},
		{"9wb", 183},
		{"9xf", 184},
		{"9cs", 185},
		{"9lw", 186},
		{"9tw", 187},
		{"9qr", 188},
		{"7ar", 189},
		{"7wb", 190},
		{"7xf", 191},
		{"7cs", 192},
		{"7lw", 193},
		{"7tw", 194},
		{"7qr", 195},
		{"7ha", 196},
		{"7ax", 197},
		{"72a", 198},
		{"7mp", 199},
		{"7wa", 200},
		{"7la", 201},
		{"7ba", 202},
		{"7bt", 203},
		{"7ga", 204},
		{"7gi", 205},
		{"7wn", 206},
		{"7yw", 207},
		{"7bw", 208},
		{"7gw", 209},
		{"7cl", 210},
		{"7sc", 211},
		{"7qs", 212},
		{"7ws", 213},
		{"7sp", 214},
		{"7ma", 215},
		{"7mt", 216},
		{"7fl", 217},
		{"7wh", 218},
		{"7m7", 219},
		{"7gm", 220},
		{"7ss", 221},
		{"7sm", 222},
		{"7sb", 223},
		{"7fc", 224},
		{"7cr", 225},
		{"7bs", 226},
		{"7ls", 227},
		{"7wd", 228},
		{"72h", 229},
		{"7cm", 230},
		{"7gs", 231},
		{"7b7", 232},
		{"7fb", 233},
		{"7gd", 234},
		{"7dg", 235},
		{"7di", 236},
		{"7kr", 237},
		{"7bl", 238},
		{"7tk", 239},
		{"7ta", 240},
		{"7bk", 241},
		{"7b8", 242},
		{"7ja", 243},
		{"7pi", 244},
		{"7s7", 245},
		{"7gl", 246},
		{"7ts", 247},
		{"7sr", 248},
		{"7tr", 249},
		{"7br", 250},
		{"7st", 251},
		{"7p7", 252},
		{"7o7", 253},
		{"7vo", 254},
		{"7s8", 255},
		{"7pa", 256},
		{"7h7", 257},
		{"7wc", 258},
		{"6ss", 259},
		{"6ls", 260},
		{"6cs", 261},
		{"6bs", 262},
		{"6ws", 263},
		{"6sb", 264},
		{"6hb", 265},
		{"6lb", 266},
		{"6cb", 267},
		{"6s7", 268},
		{"6l7", 269},
		{"6sw", 270},
		{"6lw", 271},
		{"6lx", 272},
		{"6mx", 273},
		{"6hx", 274},
		{"6rx", 275},
		{"ob1", 276},
		{"ob2", 277},
		{"ob3", 278},
		{"ob4", 279},
		{"ob5", 280},
		{"am1", 281},
		{"am2", 282},
		{"am3", 283},
		{"am4", 284},
		{"am5", 285},
		{"ob6", 286},
		{"ob7", 287},
		{"ob8", 288},
		{"ob9", 289},
		{"oba", 290},
		{"am6", 291},
		{"am7", 292},
		{"am8", 293},
		{"am9", 294},
		{"ama", 295},
		{"obb", 296},
		{"obc", 297},
		{"obd", 298},
		{"obe", 299},
		{"obf", 300},
		{"amb", 301},
		{"amc", 302},
		{"amd", 303},
		{"ame", 304},
		{"amf", 305},
};

std::map<int, int> maptiers = {
	{ITEM_TYPE_PVP_MAP_MOOR, 0},
	{ITEM_TYPE_PVP_MAP_DESERT, 0},
	{ITEM_TYPE_T1_MAP, 1},
	{ITEM_TYPE_T2_MAP, 2},
	{ITEM_TYPE_T3_MAP, 3},
	{ITEM_TYPE_T4_MAP, 4},
	{ITEM_TYPE_T5_MAP, 5},
};

int GetAdjustedUnitStat(UnitItemInfo* uInfo, DWORD stat, DWORD layer)
{
	int tmpVal = D2COMMON_GetUnitStat(uInfo->item, stat, layer);
	if (stat == STAT_MAXHP || stat == STAT_MAXMANA) {
		tmpVal /= 256;
	}
	else if (
		stat == STAT_ENHANCEDDEFENSE ||				// return 0
		stat == STAT_ENHANCEDMAXIMUMDAMAGE ||		// return 0
		stat == STAT_ENHANCEDMINIMUMDAMAGE ||		// return 0
		stat == STAT_MINIMUMDAMAGE ||				// return base min 1h weapon damage
		stat == STAT_MAXIMUMDAMAGE ||				// return base max 1h weapon damage
		stat == STAT_SECONDARYMINIMUMDAMAGE ||		// return base min 2h weapon damage
		stat == STAT_SECONDARYMAXIMUMDAMAGE	||		// return base max 2h weapon damage
		stat == STAT_MINIMUMTHROWINGDAMAGE ||		// return min throw weapon damage
		stat == STAT_MAXIMUMTHROWINGDAMAGE		    // return max throw weapon damage
		) {
		tmpVal = GetStatFromList(uInfo, stat);
	}
	return tmpVal;
}

enum AttributeFlagTypes
{
	ITEMFLAG_BASE,
	ITEMFLAG_WEAPON,
	ITEMFLAG_ARMOR,
	ITEMFLAG_MISC
};

enum LocationFlagTypes
{
	LOCATIONFLAG_EQUIPPED,
	LOCATIONFLAG_MERCEQUIPPED,
	LOCATIONFLAG_INVENTORY,
	LOCATIONFLAG_CUBE,
	LOCATIONFLAG_STASH,
	LOCATIONFLAG_GROUND
};

enum Operation
{
	EQUAL,
	GREATER_THAN,
	LESS_THAN,
	BETWEEN,
	NONE
};

enum FilterCondition
{
	COND_AND,
	COND_OR,
	COND_ETH,
	COND_SOCK,
	COND_SET,
	COND_MAG,
	COND_RARE,
	COND_UNI,
	COND_AMAZON,
	COND_SORCERESS,
	COND_NECROMANCER,
	COND_PALADIN,
	COND_BARBARIAN,
	COND_DRUID,
	COND_ASSASSIN,
	COND_CRAFTALVL,
	COND_REROLLALVL,
	COND_PREFIX,
	COND_SUFFIX,
	COND_AUTOMOD,
	COND_MAPID,
	COND_MAPTIER,
	COND_CRAFT,
	COND_RW,
	COND_NMAG,
	COND_SUP,
	COND_INF,
	COND_NORM,
	COND_EXC,
	COND_ELT,
	COND_CLASS,
	COND_ID,
	COND_ILVL,
	COND_QLVL,
	COND_ALVL,
	COND_CLVL,
	COND_FILTLVL,
	COND_DIFF,
	COND_RUNE,
	COND_GOLD,
	COND_GEMMED,
	COND_GEMTYPE,
	COND_GEM,
	COND_ED,
	COND_EDEF,
	COND_EDAM,
	COND_DEF,
	COND_MAXDUR,
	COND_RES,
	COND_FRES,
	COND_CRES,
	COND_LRES,
	COND_PRES,
	COND_IAS,
	COND_FCR,
	COND_FHR,
	COND_FBR,
	COND_LIFE,
	COND_MANA,
	COND_QTY,
	COND_GOODSK,
	COND_GOODTBSK,
	COND_FOOLS,
	COND_LVLREQ,
	COND_ARPER,
	COND_MFIND,
	COND_GFIND,
	COND_STR,
	COND_DEX,
	COND_FRW,
	COND_MINDMG,
	COND_MAXDMG,
	COND_AR,
	COND_DTM,
	COND_MAEK,
	COND_REPLIFE,
	COND_REPQUANT,
	COND_REPAIR,
	COND_ARMOR,
	COND_BELT,
	COND_CHEST,
	COND_HELM,
	COND_SHIELD,
	COND_GLOVES,
	COND_BOOTS,
	COND_CIRC,
	COND_DRU,
	COND_BAR,
	COND_DIN,
	COND_NEC,
	COND_SIN,
	COND_SOR,
	COND_ZON,
	COND_MISC,
	COND_JEWELRY,
	COND_CHARM,
	COND_QUIVER,
	COND_SHOP,
	COND_EQUIPPED,
	COND_MERC,
	COND_INVENTORY,
	COND_CUBE,
	COND_STASH,
	COND_GROUND,
	COND_1H,
	COND_2H,
	COND_AXE,
	COND_MACE,
	COND_CLUB,
	COND_TMACE,
	COND_HAMMER,
	COND_SWORD,
	COND_DAGGER,
	COND_THROWING,
	COND_JAV,
	COND_SPEAR,
	COND_POLEARM,
	COND_BOW,
	COND_XBOW,
	COND_STAFF,
	COND_WAND,
	COND_SCEPTER,
	COND_WEAPON,
	COND_SK,
	COND_OS,
	COND_CHSK,
	COND_CLSK,
	COND_ALLSK,
	COND_TABSK,
	COND_STAT,
	COND_CHARSTAT,
	COND_MULTI,
	COND_BUYPRICE,
	COND_SELLPRICE,
	COND_PRICE,
	COND_WIDTH,
	COND_HEIGHT,
	COND_AREA,
	COND_ITEMCODE,
	COND_ADD,
	COND_TRUE,
	COND_FALSE,
	COND_REQSTAT,
	COND_BASEDAMAGEMIN1H,
	COND_BASEDAMAGEMIN2H,
	COND_BASEDAMAGEMINTHROW,
	COND_BASEDAMAGEMINKICK,
	COND_BASEDAMAGEMINSMITE,
	COND_BASEDAMAGEMAX1H,
	COND_BASEDAMAGEMAX2H,
	COND_BASEDAMAGEMAXTHROW,
	COND_BASEDAMAGEMAXKICK,
	COND_BASEDAMAGEMAXSMITE,
	COND_BASEBLOCK,
	COND_ALLATTRIB,
	COND_MAXRES,
	COND_UPSTAT,
	COND_MAXSOCKETS,
	COND_FORMULA,
	COND_BELT_1,
	COND_BELT_2,
	COND_BELT_3,
	COND_BELT_4,

	COND_NULL
};

std::map<std::wstring, FilterCondition> condition_map =
{
	{L"AND", COND_AND},
	{L"&&", COND_AND},
	{L"OR", COND_OR},
	{L"||", COND_OR},
	{L"TRUE", COND_TRUE},
	{L"FALSE", COND_FALSE},
	{L"ETH", COND_ETH},
	{L"SOCK", COND_SOCK},
	{L"SOCKETS", COND_SOCK},
	{L"SET", COND_SET},
	{L"MAG", COND_MAG},
	{L"RARE", COND_RARE},
	{L"UNI", COND_UNI},
	{L"AMAZON", COND_AMAZON},
	{L"SORCERESS", COND_SORCERESS},
	{L"NECROMANCER", COND_NECROMANCER},
	{L"PALADIN", COND_PALADIN},
	{L"BARBARIAN", COND_BARBARIAN},
	{L"DRUID", COND_DRUID},
	{L"ASSASSIN", COND_ASSASSIN},
	{L"CRAFTALVL", COND_CRAFTALVL},
	{L"REROLLALVL", COND_REROLLALVL},
	{L"PREFIX", COND_PREFIX},
	{L"SUFFIX", COND_SUFFIX},
	{L"AUTOMOD", COND_AUTOMOD},
	{L"MAPID", COND_MAPID},
	{L"MAPTIER", COND_MAPTIER},
	{L"CRAFT", COND_CRAFT},
	{L"RW", COND_RW},
	{L"NMAG", COND_NMAG},
	{L"SUP", COND_SUP},
	{L"INF", COND_INF},
	{L"NORM", COND_NORM},
	{L"EXC", COND_EXC},
	{L"ELT", COND_ELT},
	{L"CLASS", COND_CLASS},
	{L"ID", COND_ID},
	{L"ILVL", COND_ILVL},
	{L"QLVL", COND_QLVL},
	{L"ALVL", COND_ALVL},
	{L"CLVL", COND_CLVL},
	{L"FILTLVL", COND_FILTLVL},
	{L"DIFF", COND_DIFF},
	{L"RUNE", COND_RUNE},
	{L"GOLD", COND_GOLD},
	{L"GEMMED", COND_GEMMED},
	{L"GEMTYPE", COND_GEMTYPE},
	{L"GEM", COND_GEM},
	{L"GEMLEVEL", COND_GEM},
	{L"ED", COND_ED},
	{L"EDEF", COND_EDEF},
	{L"EDAM", COND_EDAM},
	{L"DEF", COND_DEF},
	{L"MAXDUR", COND_MAXDUR},
	{L"RES", COND_RES},
	{L"FRES", COND_FRES},
	{L"CRES", COND_CRES},
	{L"LRES", COND_LRES},
	{L"PRES", COND_PRES},
	{L"IAS", COND_IAS},
	{L"FCR", COND_FCR},
	{L"FHR", COND_FHR},
	{L"FBR", COND_FBR},
	{L"LIFE", COND_LIFE},
	{L"MANA", COND_MANA},
	{L"QTY", COND_QTY},
	{L"GOODSK", COND_GOODSK},
	{L"GOODTBSK", COND_GOODTBSK},
	{L"FOOLS", COND_FOOLS},
	{L"LVLREQ", COND_LVLREQ},
	{L"ARPER", COND_ARPER},
	{L"MFIND", COND_MFIND},
	{L"GFIND", COND_GFIND},
	{L"STR", COND_STR},
	{L"DEX", COND_DEX},
	{L"FRW", COND_FRW},
	{L"MINDMG", COND_MINDMG},
	{L"MAXDMG", COND_MAXDMG},
	{L"AR", COND_AR},
	{L"DTM", COND_DTM},
	{L"MAEK", COND_MAEK},
	{L"REPLIFE", COND_REPLIFE},
	{L"REPQUANT", COND_REPQUANT},
	{L"REPAIR", COND_REPAIR},
	{L"ARMOR", COND_ARMOR},
	{L"BELT", COND_BELT},
	{L"CHEST", COND_CHEST},
	{L"HELM", COND_HELM},
	{L"SHIELD", COND_SHIELD},
	{L"GLOVES", COND_GLOVES},
	{L"BOOTS", COND_BOOTS},
	{L"CIRC", COND_CIRC},
	{L"DRU", COND_DRU},
	{L"BAR", COND_BAR},
	{L"DIN", COND_DIN},
	{L"NEC", COND_NEC},
	{L"SIN", COND_SIN},
	{L"SOR", COND_SOR},
	{L"ZON", COND_ZON},
	{L"MISC", COND_MISC},
	{L"JEWELRY", COND_JEWELRY},
	{L"CHARM", COND_CHARM},
	{L"QUIVER", COND_QUIVER},
	{L"SHOP", COND_SHOP},
	{L"EQUIPPED", COND_EQUIPPED},
	{L"MERC", COND_MERC},
	{L"CUBE", COND_CUBE},
	{L"INVENTORY", COND_INVENTORY},
	{L"STASH", COND_STASH},
	{L"GROUND", COND_GROUND},
	{L"1H", COND_1H},
	{L"2H", COND_2H},
	{L"AXE", COND_AXE},
	{L"MACE", COND_MACE},
	{L"CLUB", COND_CLUB},
	{L"TMACE", COND_TMACE},
	{L"HAMMER", COND_HAMMER},
	{L"SWORD", COND_SWORD},
	{L"DAGGER", COND_DAGGER},
	{L"THROWING", COND_THROWING},
	{L"JAV", COND_JAV},
	{L"SPEAR", COND_SPEAR},
	{L"POLEARM", COND_POLEARM},
	{L"BOW", COND_BOW},
	{L"XBOW", COND_XBOW},
	{L"STAFF", COND_STAFF},
	{L"WAND", COND_WAND},
	{L"SCEPTER", COND_SCEPTER},
	{L"EQ1", COND_HELM},
	{L"EQ2", COND_CHEST},
	{L"EQ3", COND_SHIELD},
	{L"EQ4", COND_GLOVES},
	{L"EQ5", COND_BOOTS},
	{L"EQ6", COND_BELT},
	{L"EQ7", COND_CIRC},
	{L"CL1", COND_DRU},
	{L"CL2", COND_BAR},
	{L"CL3", COND_DIN},
	{L"CL4", COND_NEC},
	{L"CL5", COND_SIN},
	{L"CL6", COND_SOR},
	{L"CL7", COND_ZON},
	{L"WEAPON", COND_WEAPON},
	{L"WP1", COND_AXE},
	{L"WP2", COND_MACE},
	{L"WP3", COND_SWORD},
	{L"WP4", COND_DAGGER},
	{L"WP5", COND_THROWING},
	{L"WP6", COND_JAV},
	{L"WP7", COND_SPEAR},
	{L"WP8", COND_POLEARM},
	{L"WP9", COND_BOW},
	{L"WP10", COND_XBOW},
	{L"WP11", COND_STAFF},
	{L"WP12", COND_WAND},
	{L"WP13", COND_SCEPTER},
	{L"ALLSK", COND_ALLSK},
	{L"BUYPRICE", COND_BUYPRICE},
	{L"SELLPRICE", COND_PRICE},
	{L"PRICE", COND_PRICE},
	{L"REQSTR", COND_REQSTAT},
	{L"REQDEX", COND_REQSTAT},
	{L"REQLVL", COND_REQSTAT},
	{L"BASEMINONEH", COND_BASEDAMAGEMIN1H},
	{L"BASEMINTWOH", COND_BASEDAMAGEMIN2H},
	{L"BASEMINTHROW", COND_BASEDAMAGEMINTHROW},
	{L"BASEMINKICK", COND_BASEDAMAGEMINKICK},
	{L"BASEMINSMITE", COND_BASEDAMAGEMINSMITE},
	{L"BASEMAXONEH", COND_BASEDAMAGEMAX1H},
	{L"BASEMAXTWOH", COND_BASEDAMAGEMAX2H},
	{L"BASEMAXTHROW", COND_BASEDAMAGEMAXTHROW},
	{L"BASEMAXKICK", COND_BASEDAMAGEMAXKICK},
	{L"BASEMAXSMITE", COND_BASEDAMAGEMAXSMITE},
	{L"BASEBLOCK", COND_BASEBLOCK},
	{L"ALLATTRIB", COND_ALLATTRIB},
	{L"MAXRES", COND_MAXRES},
	{L"UPSTR", COND_UPSTAT},
	{L"UPDEX", COND_UPSTAT},
	{L"UPLVL", COND_UPSTAT},
	{L"MAXSOCKETS", COND_MAXSOCKETS},
	{L"WIDTH", COND_WIDTH},
	{L"HEIGHT", COND_HEIGHT},
	{L"AREA", COND_AREA},
	{L"BELTCOLUMN1", COND_BELT_1},
	{L"BELTCOLUMN2", COND_BELT_2},
	{L"BELTCOLUMN3", COND_BELT_3},
	{L"BELTCOLUMN4", COND_BELT_4},
	// These have a number as part of the key, handled separately
	//{"SK", COND_SK},
	//{"OS", COND_OS},
	//{"CHSK", COND_CHSK},
	//{"CLSK", COND_CLSK},
	//{"TABSK", COND_TABSK},
	//{"STAT", COND_STAT},
	//{"CHARSTAT", COND_CHARSTAT},
	//{"MULTI", COND_MULTI},

};

struct SkillReplace {
	DWORD id;
	DWORD params;

	SkillReplace(DWORD id, DWORD params) :
		id(id),
		params(params) {}
};

// case-sensitive searches for AddCondition
const unordered_map<wstring, SkillReplace> skills = {
	{{L"LIFE"}, { STAT_MAXHP, 0}},
	{{L"MANA"}, { STAT_MAXMANA, 0}},
	{{L"STR"}, { STAT_STRENGTH, 0}},
	{{L"DEX"}, { STAT_DEXTERITY, 0}},
	{{L"CRES"}, { STAT_COLDRESIST, 0}},
	{{L"FRES"}, { STAT_FIRERESIST, 0}},
	{{L"LRES"}, { STAT_LIGHTNINGRESIST, 0}},
	{{L"PRES"}, { STAT_POISONRESIST, 0}},
	{{L"MINDMG"}, { STAT_MINIMUMDAMAGE, 0}},
	{{L"MAXDMG"}, { STAT_MAXIMUMDAMAGE, 0}},
	{{L"EDEF"}, { STAT_ENHANCEDDEFENSE, 0}},
	{{L"EDAM"}, { STAT_ENHANCEDMAXIMUMDAMAGE, 0}},
	{{L"FCR"}, { STAT_FASTERCAST, 0}},
	{{L"AR"}, { STAT_ATTACKRATING, 0}},
	{{L"REPLIFE"}, { STAT_REPLENISHLIFE, 0}},
	{{L"STAT"}, { ~0UL, 1}},
	{{L"MULTI"}, { ~0UL, 2}},
};

unordered_map<wstring, std::shared_ptr<Formula<FormulaContext>>> formulaMap;

std::map<std::string, int>   UnknownItemCodes;
vector<pair<string, string>> formulas;
vector<pair<wstring, wstring>> aliases;
vector<pair<wstring, wstring>> rules;
vector<Rule*>                RuleList;
vector<Rule*>                MapRuleList;
vector<Rule*>                IgnoreRuleList;
BYTE                         LastConditionType;

TrueCondition* trueCondition = new TrueCondition();
FalseCondition* falseCondition = new FalseCondition();

// Helper function to get a list of strings
//vector<string> split(const string& s,
//	char          delim)
//{
//	vector<string> result;
//	stringstream   ss(s);
//	string         item;
//	while (getline(ss, item, delim)) { result.push_back(item); }
//	return result;
//}

// Helper function to get a list of wide strings
vector<wstring> wsplit(const wstring& s,
	wchar_t          delim)
{
	vector<wstring> result;
	wstringstream   ss(s);
	wstring         item;
	while (getline(ss, item, delim)) { result.push_back(item); }
	return result;
}

// Helper function to join a list of string
//template <typename C>
//std::string join(C const& strings,
//	std::string const& delim)
//{
//	std::ostringstream ostr;
//	auto               last = std::prev(std::end(strings));
//	std::copy(
//		std::begin(strings),
//		last,
//		std::ostream_iterator<std::string>(ostr, delim.c_str())
//	);
//	ostr << *last;
//	return ostr.str();
//}

int ShopNPCs[] = {
	NPCID_Akara,	// Act 1
	NPCID_Gheed,	// Act 1
	NPCID_Charsi,	// Act 1
	NPCID_Elzix,	// Act 2
	NPCID_Drognan,	// Act 2
	NPCID_Fara,		// Act 2
	NPCID_Lysander,	// Act 2
	NPCID_Hratli,	// Act 3
	NPCID_Alkor,	// Act 3
	NPCID_Ormus,	// Act 3
	NPCID_Asheara,	// Act 3
	NPCID_Jamella,	// Act 4
	NPCID_Halbu,	// Act 4
	NPCID_Larzuk,	// Act 5
	NPCID_Malah,	// Act 5
	NPCID_Anya		// Act 5
};

int GetShopPrice(UnitAny* pPlayer, UnitAny* pItem, int nTransactionType)
{
	int nNpcId = NPCID_Malah;
	if (nTransactionType == TRANSACTIONTYPE_BUY)
	{
		UnitAny* pVendor = D2CLIENT_GetCurrentInteractingNPC();
		if (pVendor)
		{
			if (find(begin(ShopNPCs), end(ShopNPCs), pVendor->dwTxtFileNo) != end(ShopNPCs))
			{
				nNpcId = pVendor->dwTxtFileNo;
			}
		}
	}

	return D2COMMON_GetItemPrice(pPlayer, pItem, D2CLIENT_GetDifficulty(), (DWORD)D2CLIENT_GetQuestInfo(), nNpcId, nTransactionType);
}

const wchar_t* GemLevels[] = {
	L"NONE",
	L"Chipped",
	L"Flawed",
	L"Normal",
	L"Flawless",
	L"Perfect"
};

const wchar_t* GemTypes[] = {
	L"NONE",
	L"Amethyst",
	L"Diamond",
	L"Emerald",
	L"Ruby",
	L"Sapphire",
	L"Topaz",
	L"Skull"
};

struct ReplaceContext {
	UnitItemInfo* info;
	wstring name;
	ItemsTxt* text;
	// no newlines allowed
	bool limit;
	bool inShop;
	bool nlAllowed;
	bool nmagStaffmod;
	bool blockedNL;

	ReplaceContext(UnitItemInfo* info, wstring name, bool limit);
};

struct ReplacementValue {
	const function<wstring(ReplaceContext&, const ReplacementValue&)> fn;
	wstring str = L"";
	int param1 = 0;
	int param2 = 0;

	ReplacementValue(const wstring& str, const int param1, const int param2, function<wstring(ReplaceContext&, const ReplacementValue&)> fn) :
		str(str),
		param1(param1),
		param2(param2),
		fn(fn) {
	}

	wstring Replace(ReplaceContext& ctx) const;
};

struct ReplacementSpec {
	uint16_t params;
	function<wstring(ReplaceContext&, const ReplacementValue&)> fn;

	ReplacementSpec(uint16_t params, function<wstring(ReplaceContext&, const ReplacementValue&)> fn) :
		params(params),
		fn(fn) {
	}

	static ReplacementValue MakeReplacementValue(const wstring& str);
	static ReplacementValue MakeReplacementValue(const wsmatch& match, bool& fail);

	// STATIC keywords
	// no-op
	static wstring ReplaceNone(ReplaceContext& ctx, const ReplacementValue& val);
	// string between keywords
	static wstring ReplaceConst(ReplaceContext& ctx, const ReplacementValue& val);
	// %NAME%
	static wstring ReplaceName(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASENAME%
	static wstring ReplaceBaseName(ReplaceContext& ctx, const ReplacementValue& val);
	// %SOCKETS%
	static wstring ReplaceSockets(ReplaceContext& ctx, const ReplacementValue& val);
	// %RUNENUM%
	static wstring ReplaceRuneNumber(ReplaceContext& ctx, const ReplacementValue& val);
	// %RUNNAME%
	static wstring ReplaceRuneName(ReplaceContext& ctx, const ReplacementValue& val);
	// %GEMLEVEL%
	static wstring ReplaceGemLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %GEMTYPE%
	static wstring ReplaceGemType(ReplaceContext& ctx, const ReplacementValue& val);
	// %ILVL%
	static wstring ReplaceItemLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %ALVL%
	static wstring ReplaceAffixLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %CRAFTALVL%
	static wstring ReplaceCraftLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %REROLLALVL%
	static wstring ReplaceRerollLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %LVLREQ%
	static wstring ReplaceLevelRequirement(ReplaceContext& ctx, const ReplacementValue& val);
	// %WPNSPD%
	static wstring ReplaceWeaponSpeed(ReplaceContext& ctx, const ReplacementValue& val);
	// %RANGE%
	static wstring ReplaceRange(ReplaceContext& ctx, const ReplacementValue& val);
	// %CODE%
	static wstring ReplaceCode(ReplaceContext& ctx, const ReplacementValue& val);
	// %BUYPRICE%
	static wstring ReplaceBuyPrice(ReplaceContext& ctx, const ReplacementValue& val);
	// %PRICE%
	static wstring ReplacePrice(ReplaceContext& ctx, const ReplacementValue& val);
	// %QTY%
	static wstring ReplaceQuantity(ReplaceContext& ctx, const ReplacementValue& val);
	// %RES%
	static wstring ReplaceAllResist(ReplaceContext& ctx, const ReplacementValue& val);
	// enhance damage or enhance defense %ED%
	static wstring ReplaceEnhancedD(ReplaceContext& ctx, const ReplacementValue& val);
	// %CS%
	static wstring ReplaceConditionalSpace(ReplaceContext& ctx, const ReplacementValue& val);
	// %CL%
	static wstring ReplaceConditionalLine(ReplaceContext& ctx, const ReplacementValue& val);
	// %NL%
	static wstring ReplaceNewLine(ReplaceContext& ctx, const ReplacementValue& val);
	// %REQDEX%
	static wstring ReplaceReqDex(ReplaceContext& ctx, const ReplacementValue& val);
	// %REQSTR%
	static wstring ReplaceReqStr(ReplaceContext& ctx, const ReplacementValue& val);
	// %REQLVL%
	static wstring ReplaceReqLvl(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMINONEH%
	static wstring ReplaceBaseMin1h(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMAXONEH%
	static wstring ReplaceBaseMax1h(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMINTWOH%
	static wstring ReplaceBaseMin2h(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMAXTWOH%
	static wstring ReplaceBaseMax2h(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMINTHROW%
	static wstring ReplaceBaseMinThrow(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMAXTHROW%
	static wstring ReplaceBaseMaxThrow(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMINKICK%
	static wstring ReplaceBaseMinKick(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMAXKICK%
	static wstring ReplaceBaseMaxKick(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMINSMITE%
	static wstring ReplaceBaseMinSmite(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEMAXSMITE%
	static wstring ReplaceBaseMaxSmite(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASEBLOCK%
	static wstring ReplaceBaseBlock(ReplaceContext& ctx, const ReplacementValue& val);
	// %ALLATTRIB%
	static wstring ReplaceAllAttributes(ReplaceContext& ctx, const ReplacementValue& val);
	// %MAXRES%
	static wstring ReplaceMaxRes(ReplaceContext& ctx, const ReplacementValue& val);
	// %UPDEX%
	static wstring ReplaceUpDex(ReplaceContext& ctx, const ReplacementValue& val);
	// %UPSTR%
	static wstring ReplaceUpStr(ReplaceContext& ctx, const ReplacementValue& val);
	// %UPLVL%
	static wstring ReplaceUpLvl(ReplaceContext& ctx, const ReplacementValue& val);
	// %MAXSOCKETS%
	static wstring ReplaceMaxSockets(ReplaceContext& ctx, const ReplacementValue& val);
	// %MINDMG%
	static wstring ReplaceMinDamage(ReplaceContext& ctx, const ReplacementValue& val);
	// %MAXDMG%
	static wstring ReplaceMaxDamage(ReplaceContext& ctx, const ReplacementValue& val);

	// DYNAMIC keywords
	// %STAT%
	static wstring ReplaceStat(ReplaceContext& ctx, const ReplacementValue& val);
	// %SK%
	static wstring ReplaceSingleSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %OS%
	static wstring ReplaceNonClassSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %CLSK%
	static wstring ReplaceClassSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %TABSK%
	static wstring ReplaceSkillTab(ReplaceContext& ctx, const ReplacementValue& val);
	// %CHARSTAT%
	static wstring ReplaceCharStat(ReplaceContext& ctx, const ReplacementValue& val);
	// %MULTI%
	static wstring ReplaceMulti(ReplaceContext& ctx, const ReplacementValue& val);

	// COLOR keywords
	static function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceGlideDependentColor(const wstring& primary, const wstring& secondary);
	static function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceHDTextDependentColor(const wstring& primary, const wstring& secondary);
	static function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceBindString(const wstring& str);
	static function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceNamedStat(int id);
	static function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceBindFormula(shared_ptr<Formula<FormulaContext>> f);
};

unordered_map<wstring, ReplacementSpec> ReplacementMap = {
	// STATIC
	{ L"NAME", { 0, ReplacementSpec::ReplaceName } },
	{ L"BASENAME", { 0, ReplacementSpec::ReplaceBaseName } },
	{ L"SOCKETS", { 0, ReplacementSpec::ReplaceSockets } },
	{ L"RUNENUM", { 0, ReplacementSpec::ReplaceRuneNumber } },
	{ L"RUNENAME", { 0, ReplacementSpec::ReplaceRuneName } },
	{ L"GEMLEVEL", { 0, ReplacementSpec::ReplaceGemLevel } },
	{ L"GEMTYPE", { 0, ReplacementSpec::ReplaceGemType } },
	{ L"ILVL", { 0, ReplacementSpec::ReplaceItemLevel } },
	{ L"ALVL", { 0, ReplacementSpec::ReplaceAffixLevel } },
	{ L"CRAFTALVL", { 0, ReplacementSpec::ReplaceCraftLevel } },
	{ L"REROLLALVL", {0, ReplacementSpec::ReplaceRerollLevel }},
	{ L"LVLREQ", { 0, ReplacementSpec::ReplaceLevelRequirement } },
	{ L"WPNSPD", { 0, ReplacementSpec::ReplaceWeaponSpeed } },
	{ L"RANGE", { 0, ReplacementSpec::ReplaceRange } },
	{ L"CODE", { 0, ReplacementSpec::ReplaceCode } },
	// %LBRACE%
	{ L"LBRACE", { 0, ReplacementSpec::ReplaceBindString(L"{") } },
	// %RBRACE%
	{ L"RBRACE", { 0, ReplacementSpec::ReplaceBindString(L"}") } },
	{ L"BUYPRICE", { 0, ReplacementSpec::ReplaceBuyPrice } },
	{ L"SELLPRICE", { 0, ReplacementSpec::ReplacePrice } },
	{ L"PRICE", { 0, ReplacementSpec::ReplacePrice } },
	{ L"QTY", { 0, ReplacementSpec::ReplaceQuantity } },
	{ L"RES", { 0, ReplacementSpec::ReplaceAllResist } },
	{ L"ED", { 0, ReplacementSpec::ReplaceEnhancedD } },
	{ L"CS", { 0, ReplacementSpec::ReplaceConditionalSpace } },
	{ L"CL", { 0, ReplacementSpec::ReplaceConditionalLine } },
	{ L"NL", { 0, ReplacementSpec::ReplaceNewLine } },
	{ L"REQSTR", { 0, ReplacementSpec::ReplaceReqStr } },
	{ L"REQDEX", { 0, ReplacementSpec::ReplaceReqDex } },
	{ L"REQLVL", { 0, ReplacementSpec::ReplaceReqLvl } },
	{ L"BASEMINONEH", { 0, ReplacementSpec::ReplaceBaseMin1h } },
	{ L"BASEMAXONEH", { 0, ReplacementSpec::ReplaceBaseMax1h } },
	{ L"BASEMINTWOH", { 0, ReplacementSpec::ReplaceBaseMin2h } },
	{ L"BASEMAXTWOH", { 0, ReplacementSpec::ReplaceBaseMax2h } },
	{ L"BASEMINTHROW", { 0, ReplacementSpec::ReplaceBaseMinThrow } },
	{ L"BASEMAXTHROW", { 0, ReplacementSpec::ReplaceBaseMaxThrow } },
	{ L"BASEMINKICK", { 0, ReplacementSpec::ReplaceBaseMinKick } },
	{ L"BASEMAXKICK", { 0, ReplacementSpec::ReplaceBaseMaxKick } },
	{ L"BASEMINSMITE", { 0, ReplacementSpec::ReplaceBaseMinSmite } },
	{ L"BASEMAXSMITE", { 0, ReplacementSpec::ReplaceBaseMaxSmite } },
	{ L"BASEBLOCK", { 0, ReplacementSpec::ReplaceBaseBlock } },
	{ L"ALLATTRIB", { 0, ReplacementSpec::ReplaceAllAttributes } },
	{ L"MAXRES", { 0, ReplacementSpec::ReplaceMaxRes } },
	{ L"UPSTR", { 0, ReplacementSpec::ReplaceUpStr } },
	{ L"UPDEX", { 0, ReplacementSpec::ReplaceUpDex } },
	{ L"UPLVL", { 0, ReplacementSpec::ReplaceUpLvl } },
	{ L"MAXSOCKETS", { 0, ReplacementSpec::ReplaceMaxSockets } },
	{ L"MINDMG", { 0, ReplacementSpec::ReplaceMinDamage } },
	{ L"MAXDMG", { 0, ReplacementSpec::ReplaceMaxDamage } },
	// named stats
	{ L"EDEF", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ENHANCEDDEFENSE) } },
	{ L"EDAM", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ENHANCEDMAXIMUMDAMAGE) } },
	{ L"DEF", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DEFENSE) } },
	{ L"FRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FIRERESIST) } },
	{ L"CRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_COLDRESIST) } },
	{ L"LRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_LIGHTNINGRESIST) } },
	{ L"PRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_POISONRESIST) } },
	{ L"IAS", { 0, ReplacementSpec::ReplaceNamedStat(STAT_IAS) } },
	{ L"FCR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERCAST) } },
	{ L"FHR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERHITRECOVERY) } },
	{ L"FBR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERBLOCK) } },
	{ L"LIFE", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAXHP) } },
	{ L"MANA", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAXMANA) } },
	{ L"ARPER", { 0, ReplacementSpec::ReplaceNamedStat(STAT_TOHITPERCENT) } },
	{ L"MFIND", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAGICFIND) } },
	{ L"GFIND", { 0, ReplacementSpec::ReplaceNamedStat(STAT_GOLDFIND) } },
	{ L"STR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_STRENGTH) } },
	{ L"DEX", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DEXTERITY) } },
	{ L"FRW", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERRUNWALK) } },
	{ L"AR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ATTACKRATING) } },
	{ L"DTM", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DAMAGETOMANA) } },
	{ L"MAEK", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MANAAFTEREACHKILL) } },
	{ L"REPLIFE", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPLENISHLIFE) } },
	{ L"REPQUANT", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPLENISHESQUANTITY) } },
	{ L"REPAIR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPAIRSDURABILITY) } },
	// DYNAMIC
	{ L"STAT", { 1, ReplacementSpec::ReplaceStat } },
	{ L"SK", { 1, ReplacementSpec::ReplaceSingleSkill } },
	{ L"OS", { 1, ReplacementSpec::ReplaceNonClassSkill } },
	{ L"CLSK", { 1, ReplacementSpec::ReplaceClassSkill } },
	{ L"TABSK", { 1, ReplacementSpec::ReplaceSkillTab } },
	{ L"CHARSTAT", { 1, ReplacementSpec::ReplaceCharStat } },
	{ L"MULTI", { 2, ReplacementSpec::ReplaceMulti } },
	// COLORS
	{ L"BLACK", { 0, ReplacementSpec::ReplaceGlideDependentColor(L"\xFF" L"c\x02", L"\xFF" L"c6") }},
	{ L"CORAL", { 0, ReplacementSpec::ReplaceGlideDependentColor(L"\xFF" L"c\x06", L"\xFF" L"c1") }},
	{ L"SAGE", { 0, ReplacementSpec::ReplaceGlideDependentColor(L"\xFF" L"c\x07", L"\xFF" L"c2") }},
	{ L"TEAL", { 0, ReplacementSpec::ReplaceGlideDependentColor(L"\xFF" L"c\x09", L"\xFF" L"c3") }},
	{ L"LIGHT_GRAY", { 0, ReplacementSpec::ReplaceGlideDependentColor(L"\xFF" L"c\x0C", L"\xFF" L"c5") }},
	{ L"FULL_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor(L"\xFF" L"c\x40", L"") }},
	{ L"THREE_FOURTHS_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor(L"\xFF" L"c\x41", L"") }},
	{ L"HALF_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor(L"\xFF" L"c\x42", L"") }},
	{ L"QUARTER_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor(L"\xFF" L"c\x43", L"") }},
	{ L"WHITE", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c0") } },
	{ L"RED", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c1") } },
	{ L"GREEN", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c2") } },
	{ L"BLUE", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c3") } },
	{ L"GOLD", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c4") } },
	{ L"GRAY", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c5") } },
	{ L"TAN", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c7") } },
	{ L"ORANGE", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c8") } },
	{ L"YELLOW", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c9") } },
	{ L"PURPLE", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c;") } },
	{ L"DARK_GREEN", { 0, ReplacementSpec::ReplaceBindString(L"\xFF" L"c:") } },
};

unordered_map<wstring, ReplacementSpec> FormulaReplacementMap;

wregex ReplacementRegex(L"%([A-Z_]+)(?:(\\d{1,9})(?:,(\\d{1,9}))?)?%", wregex::ECMAScript);
vector<ReplacementValue> BuildReplacementActions(const wstring& action)
{
	vector<ReplacementValue> res;
	wsmatch match;

	const auto begin = action.begin();
	size_t lastIndex = 0;
	while (lastIndex < action.size() && regex_search(begin + lastIndex, action.end(), match, ReplacementRegex)) {
		const auto position = distance(begin, match[0].first);
		const auto length = distance(match[0].first, match[0].second);
		if (lastIndex != position) {
			const auto str = action.substr(lastIndex, position - lastIndex);
			res.push_back(ReplacementSpec::MakeReplacementValue(str));
		}
		bool failed = false;
		auto val = ReplacementSpec::MakeReplacementValue(match, failed);
		// If no matching keyword was found, we must append the start of the failed keyword
		// then start a match at the second %.
		if (failed) {
			const auto str = action.substr(position, length - 1);
			res.push_back(ReplacementSpec::MakeReplacementValue(str));
			lastIndex = position + length - 1;
			continue;
		}
		lastIndex = position + length;
		res.push_back(val);
	}

	if (lastIndex != action.length()) {
		const auto str = action.substr(lastIndex);
		res.push_back(ReplacementSpec::MakeReplacementValue(str));
	}

	return res;
}

ReplacementValue ReplacementSpec::MakeReplacementValue(const wstring& str)
{
	return ReplacementValue(str, 0, 0, ReplacementSpec::ReplaceConst);
}

ReplacementValue ReplacementSpec::MakeReplacementValue(const wsmatch& match, bool& fail)
{
	auto spec = ReplacementMap.find(match[1]);
	if (spec == ReplacementMap.end()) {
		spec = FormulaReplacementMap.find(match[1]);
		if (spec == FormulaReplacementMap.end()) {
			fail = true;
			return ReplacementValue(match.str(), 0, 0, ReplacementSpec::ReplaceNone);
		}
	}
	const auto& replacer = spec->second;
	const int count = (match[2].length() != 0) + (match[3].length() != 0);
	if (count != replacer.params) {
		return ReplacementValue(match.str(), 0, 0, ReplacementSpec::ReplaceConst);
	}
	int param1 = match[2].length() > 0 ? stoi(match[2].str()) : 0;
	int param2 = match[3].length() > 0 ? stoi(match[3].str()) : 0;
	return ReplacementValue(L"", param1, param2, replacer.fn);
}

function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceNamedStat(int id)
{
	ReplacementValue rep(L"", id, 0, ReplacementSpec::ReplaceNone);
	return [rep](ReplaceContext& ctx, const ReplacementValue& val) -> wstring {
		return ReplacementSpec::ReplaceStat(ctx, rep);
	};
}

function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceBindString(const std::wstring& str)
{
	return [str](ReplaceContext& ctx, const ReplacementValue& val) -> wstring {
		return str;
	};
}

function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceBindFormula(shared_ptr<Formula<FormulaContext>> f)
{
	return [f](ReplaceContext& ctx, const ReplacementValue& val) -> wstring {
		float out = 0.0f;
		if (f->execute(ctx.info, out) != FormulaStatus::OK || !std::isfinite(out) || std::abs(out) > 2147483648.0f)
		{
			return L"f_err";
		}
		wchar_t buffer[64];
		int len = swprintf(buffer, 64, L"%.2f", out);
		if (len < 3) {
			return L"f_err";
		}
		// remove .00
		if (buffer[len - 1] == L'0' && buffer[len - 2] == L'0') {
			buffer[len - 3] = L'\0';
		}
		// remove a single trailing 0
		else if (buffer[len - 1] == L'0') {
			buffer[len - 1] = L'\0';
		}
		return buffer;
	};
}

wstring ReplacementSpec::ReplaceNone(ReplaceContext& ctx, const ReplacementValue& val)
{
	return L"";
}

wstring ReplacementSpec::ReplaceConst(ReplaceContext& ctx, const ReplacementValue& val)
{
	return val.str;
}

wstring ReplacementSpec::ReplaceName(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.name.size() > 1023) {
		ctx.name.resize(1023);
	}
	return ctx.name;
}

wstring ReplacementSpec::ReplaceBaseName(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.info == nullptr || ctx.info->attrs == nullptr) {
		return L"";
	}

	return MaybeStripColorPrefixW(ctx.info->attrs->name);
}

wstring ReplacementSpec::ReplaceSockets(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarSockets(ctx.info);
}

wstring ReplacementSpec::ReplaceRuneNumber(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRuneNum(ctx.info);
}

wstring ReplacementSpec::ReplaceRuneName(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRuneName(ctx.info);
}

wstring ReplacementSpec::ReplaceGemLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarGemLevel(ctx.info);
}

wstring ReplacementSpec::ReplaceGemType(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarGemType(ctx.info);
}

wstring ReplacementSpec::ReplaceItemLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarIlvl(ctx.info);
}

wstring ReplacementSpec::ReplaceAffixLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarAlvl(ctx.info);
}

wstring ReplacementSpec::ReplaceCraftLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarCraftAlvl(ctx.info);
}

wstring ReplacementSpec::ReplaceRerollLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRerollAlvl(ctx.info);
}

wstring ReplacementSpec::ReplaceLevelRequirement(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarLevelReq(ctx.info);
}

wstring ReplacementSpec::ReplaceWeaponSpeed(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarWeaponSpeed(ctx.text);
}

wstring ReplacementSpec::ReplaceRange(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRangeAdder(ctx.text);
}

wstring ReplacementSpec::ReplaceCode(ReplaceContext& ctx, const ReplacementValue& val)
{
	return AnsiToWide(ctx.info->itemCode);
}

wstring ReplacementSpec::ReplaceBuyPrice(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarBuyValue(ctx.info, ctx.text);
}

wstring ReplacementSpec::ReplacePrice(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarSellValue(ctx.info, ctx.text);
}

wstring ReplacementSpec::ReplaceQuantity(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarQty(ctx.info);
}

wstring ReplacementSpec::ReplaceAllResist(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarAllRes(ctx.info);
}

wstring ReplacementSpec::ReplaceEnhancedD(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarEd(ctx.info);
}

wstring ReplacementSpec::ReplaceReqStr(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::STRENGTH, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceReqDex(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::DEXTERITY, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceReqLvl(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::LEVEL, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMin1h(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MIN1H, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMax1h(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAX1H, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMin2h(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MIN2H, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMax2h(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAX2H, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMinThrow(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINTHROW, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMaxThrow(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXTHROW, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMinKick(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINKICK, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMaxKick(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXKICK, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMinSmite(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINSMITE, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseMaxSmite(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXSMITE, ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceBaseBlock(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", BaseBlockCondition::GetValue(ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceAllAttributes(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", AllAttributesCondition::GetValue(ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceMaxRes(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", MaxResCondition::GetValue(ctx.info));
	return buffer;
}

wstring ReplacementSpec::ReplaceUpStr(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = UpStatCondition::GetValue(UpStatCondition::UpStatType::STRENGTH, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceUpDex(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = UpStatCondition::GetValue(UpStatCondition::UpStatType::DEXTERITY, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceUpLvl(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto req = UpStatCondition::GetValue(UpStatCondition::UpStatType::LEVEL, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", req);
	return buffer;
}

wstring ReplacementSpec::ReplaceMaxSockets(ReplaceContext& ctx, const ReplacementValue& val)
{
	wchar_t buffer[16];
	BYTE max = MaxSocketsCondition::GetValue(ctx.info);
	swprintf(buffer, 16, L"%d", max);
	return buffer;
}

wstring ReplacementSpec::ReplaceMinDamage(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto dmg = MinMaxDamageCondition::GetValue(MinMaxDamageCondition::DamageType::MIN, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", dmg);
	return buffer;
}

wstring ReplacementSpec::ReplaceMaxDamage(ReplaceContext& ctx, const ReplacementValue& val)
{
	auto dmg = MinMaxDamageCondition::GetValue(MinMaxDamageCondition::DamageType::MAX, ctx.info);
	wchar_t buffer[16];
	swprintf(buffer, 16, L"%d", dmg);
	return buffer;
}

wstring ReplacementSpec::ReplaceConditionalSpace(ReplaceContext& ctx, const ReplacementValue& val)
{
	return L"\b";
}

wstring ReplacementSpec::ReplaceConditionalLine(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.blockedNL) {
		return L"";
	}
	if (!ctx.limit || ctx.nlAllowed) {
		return L"\r";
	}
	if (ctx.nmagStaffmod) {
		ctx.blockedNL = true;
		return L"\r";
	}
	return L"";
}

wstring ReplacementSpec::ReplaceNewLine(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.blockedNL) {
		return L"";
	}
	if (!ctx.limit || ctx.nlAllowed) {
		return L"\n";
	}
	if (ctx.nmagStaffmod) {
		ctx.blockedNL = true;
		return L"\n";
	}
	return L"";
}

wstring ReplacementSpec::ReplaceStat(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat = val.param1;
	if (stat > STAT_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, stat, 0);
	// Hp and mana need adjusting
	if (stat == STAT_MAXHP || stat == STAT_MAXMANA)
	{
		value /= 256;
	}
	// These stat values need to be grabbed differently otherwise they just:
	else if (
		stat == STAT_ENHANCEDDEFENSE ||				// return 0
		stat == STAT_ENHANCEDMAXIMUMDAMAGE ||		// return 0
		stat == STAT_ENHANCEDMINIMUMDAMAGE ||		// return 0
		stat == STAT_MINIMUMDAMAGE ||				// return base min 1h weapon damage
		stat == STAT_MAXIMUMDAMAGE ||				// return base max 1h weapon damage
		stat == STAT_SECONDARYMINIMUMDAMAGE ||		// return base min 2h weapon damage
		stat == STAT_SECONDARYMAXIMUMDAMAGE	||		// return base max 2h weapon damage
		stat == STAT_MINIMUMTHROWINGDAMAGE ||		// return min throw weapon damage
		stat == STAT_MAXIMUMTHROWINGDAMAGE		    // return max throw weapon damage
		)
	{
		value = GetStatFromList(ctx.info, stat);
	}
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceSingleSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_SINGLESKILL, skill);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceNonClassSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_NONCLASSSKILL, skill);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceClassSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_CLASSSKILLS, skill);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceSkillTab(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_SKILLTAB, skill);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceCharStat(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat = val.param1;
	if (stat > STAT_MAX) {
		return L"";
	}
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), stat, 0);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

wstring ReplacementSpec::ReplaceMulti(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat1 = val.param1;
	if (stat1 > STAT_MAX) {
		return L"";
	}
	const auto stat2 = val.param2;
	wchar_t buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, stat1, stat2);
	swprintf(buffer, 16, L"%d", value);
	return buffer;
}

function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceGlideDependentColor(const wstring& primary, const wstring& secondary)
{
	return [primary, secondary](ReplaceContext& ctx, const ReplacementValue& val) {
		return *p_D2GFX_RenderMode != 4 ? secondary : primary;
		};
}

function<wstring(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceHDTextDependentColor(const wstring& primary, const wstring& secondary)
{
	return [primary, secondary](ReplaceContext& ctx, const ReplacementValue& val) {
		return (!App.d2gl.usingD2GL.value && !App.d2gl.usingHDText.value) ? secondary : primary;
		};
}

ReplaceContext::ReplaceContext(UnitItemInfo* info, wstring name, bool limit) :
	info(info),
	name(name),
	limit(limit),
	blockedNL(false) {
	text = D2COMMON_GetItemText(info->item->dwTxtFileNo);
	inShop = (info->item->pItemData->pOwnerInventory != 0 && // Skip on ground items
		find(begin(ShopNPCs), end(ShopNPCs), info->item->pItemData->pOwnerInventory->pOwner->dwTxtFileNo) != end(ShopNPCs));
	nlAllowed = ((info->item->pItemData->dwFlags & ITEM_IDENTIFIED) > 0 &&
		(info->item->pItemData->dwQuality >= ITEM_QUALITY_MAGIC || (info->item->pItemData->dwFlags & ITEM_RUNEWORD) > 0)) ||
		inShop;

	// Check if non-mag item capable of having staffmods or similar mods
	nmagStaffmod = ((info->item->pItemData->dwQuality == ITEM_QUALITY_INFERIOR ||
		info->item->pItemData->dwQuality == ITEM_QUALITY_NORMAL ||
		info->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR) &&
		(info->attrs->staffmodClass < CLASS_NA ||
		(text->wautoprefix > 0 && text->wautoprefix != 308) ||
		(info->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR)));
}

wstring ReplacementValue::Replace(ReplaceContext& ctx) const
{
	return fn(ctx, *this);
}

bool IsGem(ItemAttributes* attrs) { return (attrs->miscFlags & ITEM_GROUP_GEM) > 0; }

BYTE GetGemLevel(ItemAttributes* attrs)
{
	if (attrs->miscFlags & ITEM_GROUP_CHIPPED) { return 1; }
	else if (attrs->miscFlags & ITEM_GROUP_FLAWED) { return 2; }
	else if (attrs->miscFlags & ITEM_GROUP_REGULAR) { return 3; }
	else if (attrs->miscFlags & ITEM_GROUP_FLAWLESS) { return 4; }
	else if (attrs->miscFlags & ITEM_GROUP_PERFECT) { return 5; }
	return 0;
}

const wchar_t* GetGemLevelString(BYTE level) { return GemLevels[level]; }

BYTE GetGemType(ItemAttributes* attrs)
{
	if (attrs->miscFlags & ITEM_GROUP_AMETHYST) { return 1; }
	else if (attrs->miscFlags & ITEM_GROUP_DIAMOND) { return 2; }
	else if (attrs->miscFlags & ITEM_GROUP_EMERALD) { return 3; }
	else if (attrs->miscFlags & ITEM_GROUP_RUBY) { return 4; }
	else if (attrs->miscFlags & ITEM_GROUP_SAPPHIRE) { return 5; }
	else if (attrs->miscFlags & ITEM_GROUP_TOPAZ) { return 6; }
	else if (attrs->miscFlags & ITEM_GROUP_SKULL) { return 7; }
	return 0;
}

const wchar_t* GetGemTypeString(BYTE type) { return GemTypes[type]; }

bool IsRune(ItemAttributes* attrs)
{
	return (attrs->miscFlags & ITEM_GROUP_RUNE) > 0;
	//char* code = attrs->code;
	//return (code[0] == 'r' && std::isdigit(code[1]) && std::isdigit(code[2]));
}

BYTE RuneNumberFromItemCode(char* code) { return (BYTE)(((code[1] - '0') * 10) + code[2] - '0'); }

// must be lowercase
std::unordered_map<std::wstring, FormulaVarDefinition<FormulaContext>> formulaVarDefs = {
	{ L"stat", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto stat = ids[0];
			return GetAdjustedUnitStat(uInfo, stat, 0);
		}
	} },
	{ L"multi", { 2, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto stat = ids[0];
			auto layer = ids[1];
			return GetAdjustedUnitStat(uInfo, stat, layer);
		}
	} },
	{ L"charstat", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto stat = ids[0];
			return D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), stat, 0);
		}
	} },
	{ L"onehand", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (code_to_dwtxtfileno.find(uInfo->itemCode) != code_to_dwtxtfileno.end()) {
				int weapon_number = code_to_dwtxtfileno[uInfo->itemCode];
				WeaponType weapon_type = Drawing::StatsDisplay::GetCurrentWeaponType(weapon_number);

				if (weapon_type == WeaponType::kAxe ||
					weapon_type == WeaponType::kWand ||
					weapon_type == WeaponType::kClub ||
					weapon_type == WeaponType::kScepter ||
					weapon_type == WeaponType::kMace ||
					weapon_type == WeaponType::kHammer ||
					weapon_type == WeaponType::kSword ||
					weapon_type == WeaponType::kKnife ||
					weapon_type == WeaponType::kThrowing ||
					weapon_type == WeaponType::kJavelin ||
					weapon_type == WeaponType::kThrowingPot ||
					weapon_type == WeaponType::kClaw1 ||
					weapon_type == WeaponType::kClaw2 ||
					weapon_type == WeaponType::kOrb ||
					weapon_type == WeaponType::kAmaJav
					) {
					return 1;
				}
			}
			return 0;
		}
	} },
	{ L"twohand", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (code_to_dwtxtfileno.find(uInfo->itemCode) != code_to_dwtxtfileno.end()) {
				int weapon_number = code_to_dwtxtfileno[uInfo->itemCode];
				WeaponType weapon_type = Drawing::StatsDisplay::GetCurrentWeaponType(weapon_number);

				if (weapon_type == WeaponType::kAxe2H ||
					weapon_type == WeaponType::kHammer2H ||
					weapon_type == WeaponType::kSword2H ||
					weapon_type == WeaponType::kSpear ||
					weapon_type == WeaponType::kPole ||
					weapon_type == WeaponType::kStaff ||
					weapon_type == WeaponType::kBow ||
					weapon_type == WeaponType::kCrossbow ||
					weapon_type == WeaponType::kAmaBow ||
					weapon_type == WeaponType::kAmaSpear
					) {
					return 1;
				}
			}
			return 0;
		}
	} },
	{ L"allsk", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_ALLSKILLS, 0);
		}
	} },
	{ L"alvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			BYTE qlvl = uInfo->attrs->qualityLevel;
			return GetAffixLevel((BYTE)uInfo->item->pItemData->dwItemLevel, (BYTE)uInfo->attrs->qualityLevel, uInfo->attrs->magicLevel);
		}
	} },
	{ L"amazon", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 0;
		}
	} },
	{ L"ar", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_ATTACKRATING, 0);
		}
	} },
	{ L"armor", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_ALLARMOR) != 0;
		}
	} },
	{ L"arper", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_TOHITPERCENT, 0);
		}
	} },
	{ L"assassin", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 6;
		}
	} },
	{ L"automod", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto itemData = uInfo->item->pItemData;
			if ((itemData->dwQuality == ITEM_QUALITY_MAGIC || itemData->dwQuality == ITEM_QUALITY_RARE) && !(itemData->dwFlags & ITEM_IDENTIFIED)) {
				return false;
			}
			return itemData->wAutoPrefix - AUTOMOD_OFFSET;
		}
	} },
	{ L"axe", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_AXE) != 0;
		}
	} },
	{ L"bar", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_BARBARIAN_HELM) != 0;
		}
	} },
	{ L"barbarian", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 4;
		}
	} },
	{ L"belt", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_BELT) != 0;
		}
	} },
	{ L"boots", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_BOOTS) != 0;
		}
	} },
	{ L"bow", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_BOW) != 0;
		}
	} },
	{ L"buyprice", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetShopPrice(D2CLIENT_GetPlayerUnit(), uInfo->item, TRANSACTIONTYPE_BUY);
		}
	} },
	{ L"charm", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->miscFlags & ITEM_GROUP_CHARM) != 0;
		}
	} },
	{ L"chest", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_BODY_ARMOR) != 0;
		}
	} },
	{ L"chsk", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto skill = ids[0];
			if (skill < 0 || skill > SKILL_MAX) {
				err = FormulaStatus::MATH_ERROR;
				return 0;
			}
			DWORD     value = 0;
			Stat      aStatList[256] = { NULL };
			StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
			if (pStatList) {
				DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
				for (UINT i = 0; i < dwStats; i++) {
					//if (aStatList[i].wStatIndex == STAT_CHARGED)
					//	PrintText(1, "ChargedCondition::EvaluateInternal: Index=%hx, SubIndex=%hx, Value=%x", aStatList[i].wStatIndex, aStatList[i].wSubIndex, aStatList[i].dwStatValue);
					if (aStatList[i].wStatIndex == STAT_CHARGED && (aStatList[i].wSubIndex >> 6) == skill) {
						// 10 MSBs of subindex is the skill ID
						unsigned int level = aStatList[i].wSubIndex & 0x3F;     // 6 LSBs are the skill level
						value = (level > value) ? level : value; // use highest level
					}
				}
			}
			return value;
		}
	} },
	{ L"circ", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_CIRCLET) != 0;
		}
	} },
	{ L"cl", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto classId = ids[0];
			switch (classId) {
				case 1:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_DRUID_PELT) != 0;
				}
				case 2:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_BARBARIAN_HELM) != 0;
				}
				case 3:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_PALADIN_SHIELD) != 0;
				}
				case 4:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_NECROMANCER_SHIELD) != 0;
				}
				case 5:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_ASSASSIN_KATAR) != 0;
				}
				case 6:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_SORCERESS_ORB) != 0;
				}
				case 7:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_AMAZON_WEAPON) != 0;
				}
				default:
				{
					err = FormulaStatus::MATH_ERROR;
					return 0;
				}
			}
		}
	} },
	{ L"class", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->baseFlags & ITEM_GROUP_CLASS) != 0;
		}
	} },
	{ L"clsk", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto classId = ids[0];
			if (classId < 0 || classId >= CLASS_NA) {
				err = FormulaStatus::MATH_ERROR;
				return 0;
			}
			return GetAdjustedUnitStat(uInfo, STAT_CLASSSKILLS, classId);
		}
	} },
	{ L"club", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_CLUB) != 0;
		}
	} },
	{ L"clvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);
		}
	} },
	{ L"craft", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_CRAFT;
		}
	} },
	{ L"craftalvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			int clvl = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);
			int craft_alvl = GetAffixLevel(
				(int)(0.5 * clvl) + (int)(0.5 * uInfo->item->pItemData->dwItemLevel),
				uInfo->attrs->qualityLevel,
				uInfo->attrs->magicLevel
			);
			return craft_alvl;
		}
	} },
	{ L"cres", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_COLDRESIST, 0);
		}
	} },
	{ L"cube", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (!uInfo->item || !uInfo->item->pItemData) {
				return 0;
			}
			const auto pItemData = uInfo->item->pItemData;
			return pItemData->ItemLocation == STORAGE_CUBE
				&& pItemData->pOwnerInventory
				&& pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit();
		}
	} },
	{ L"dagger", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_DAGGER) != 0;
		}
	} },
	{ L"def", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_DEFENSE, 0);
		}
	} },
	{ L"dex", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_DEXTERITY, 0);
		}
	} },
	{ L"diff", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetDifficulty();
		}
	} },
	{ L"din", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_PALADIN_SHIELD) != 0;
		}
	} },
	{ L"dru", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_DRUID_PELT) != 0;
		}
	} },
	{ L"druid", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 5;
		}
	} },
	{ L"dtm", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_DAMAGETOMANA, 0);
		}
	} },
	{ L"ed", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			WORD stat;
			if (uInfo->attrs->armorFlags & ITEM_GROUP_ALLARMOR) { stat = STAT_ENHANCEDDEFENSE; }
			else {
				// Normal %ED will have the same value for STAT_ENHANCEDMAXIMUMDAMAGE and STAT_ENHANCEDMINIMUMDAMAGE
				stat = STAT_ENHANCEDMAXIMUMDAMAGE;
			}
			DWORD     value = 0;
			StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
			if (pStatList) {
				value += D2COMMON_GetStatValueFromStatList(pStatList, stat, 0);
			}
			return value;
		}
	} },
	{ L"edam", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_ENHANCEDMAXIMUMDAMAGE, 0);
		}
	} },
	{ L"edef", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_ENHANCEDDEFENSE, 0);
		}
	} },
	{ L"elt", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->baseFlags & ITEM_GROUP_ELITE) != 0;
		}
	} },
	{ L"eq", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			const auto id = ids[0];
			switch (id) {
				case 1:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_HELM) != 0;
				}
				case 2:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_BODY_ARMOR) != 0;
				}
				case 3:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_SHIELD) != 0;
				}
				case 4:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_GLOVES) != 0;
				}
				case 5:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_BOOTS) != 0;
				}
				case 6:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_BELT) != 0;
				}
				case 7:
				{
					return (uInfo->attrs->armorFlags & ITEM_GROUP_CIRCLET) != 0;
				}
				default:
				{
					err = FormulaStatus::MATH_ERROR;
					return 0;
				}
			}
		}
	} },
	{ L"equipped", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (!uInfo->item || !uInfo->item->pItemData) {
				return 0;
			}
			const auto pItemData = uInfo->item->pItemData;
			return pItemData->ItemLocation == STORAGE_NULL
				&& pItemData->BodyLocation > 0
				&& pItemData->pOwnerInventory
				&& pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit();
		}
	} },
	{ L"eth", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) != 0;
		}
	} },
	{ L"exc", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->baseFlags & ITEM_GROUP_EXCEPTIONAL) != 0;
		}
	} },
	{ L"false", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return 0;
		}
	} },
	{ L"fbr", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_FASTERBLOCK, 0);
		}
	} },
	{ L"fcr", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_FASTERCAST, 0);
		}
	} },
	{ L"fhr", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_FASTERHITRECOVERY, 0);
		}
	} },
	{ L"filtlvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return Item::GetFilterLevel();
		}
	} },
	{ L"fools", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			unsigned int value = 0;
			Stat         aStatList[256] = { NULL };
			StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
			if (pStatList) {
				DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
				for (UINT i = 0; i < dwStats; i++) {
					if (aStatList[i].wStatIndex == STAT_MAXDAMAGEPERLEVEL && aStatList[i].wSubIndex == 0) { value += 1; }
					if (aStatList[i].wStatIndex == STAT_ATTACKRATINGPERLEVEL && aStatList[i].wSubIndex == 0) { value += 2; }
				}
			}
			return value == 3;
		}
	} },
	{ L"fres", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_FIRERESIST, 0);
		}
	} },
	{ L"frw", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_FASTERRUNWALK, 0);
		}
	} },
	{ L"gem", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (IsGem(uInfo->attrs)) {
				return GetGemLevel(uInfo->attrs);
			}
			return 0;
		}
	} },
	{ L"gemlevel", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (IsGem(uInfo->attrs)) {
				return GetGemLevel(uInfo->attrs);
			}
			return 0;
		}
	} },
	{ L"gemmed", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (uInfo->item->pInventory) {
				return uInfo->item->pInventory->dwItemCount > 0;
			}
			return 0;
		}
	} },
	{ L"gemtype", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (IsGem(uInfo->attrs)) {
				return GetGemType(uInfo->attrs);
			}
			return 0;
		}
	} },
	{ L"gfind", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_GOLDFIND, 0);
		}
	} },
	{ L"gloves", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_GLOVES) != 0;
		}
	} },
	{ L"gold", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (uInfo->itemCode[0] == 'g' && uInfo->itemCode[1] == 'l' && uInfo->itemCode[2] == 'd') {
				return D2COMMON_GetUnitStat(uInfo->item, STAT_GOLD, 0);
			}
			return 0;
		}
	} },
	{ L"ground", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->dwMode == ITEM_MODE_ON_GROUND || uInfo->item->dwMode == ITEM_MODE_BEING_DROPPED;
		}
	} },
	{ L"hammer", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_HAMMER) != 0;
		}
	} },
	{ L"helm", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_HELM) != 0;
		}
	} },
	{ L"ias", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_IAS, 0);
		}
	} },
	{ L"id", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->item->pItemData->dwFlags & ITEM_IDENTIFIED) != 0;
		}
	} },
	{ L"ilvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwItemLevel;
		}
	} },
	{ L"inf", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_INFERIOR;
		}
	} },
	{ L"inventory", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (!uInfo->item || !uInfo->item->pItemData) {
				return 0;
			}
			const auto pItemData = uInfo->item->pItemData;
			return pItemData->ItemLocation == STORAGE_INVENTORY
				&& uInfo->item->dwMode == ITEM_MODE_INV_STASH_CUBE_STORE
				&& pItemData->pOwnerInventory
				&& pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit();
		}
	} },
	{ L"jav", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_JAVELIN) != 0;
		}
	} },
	{ L"jewelry", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->miscFlags & ITEM_GROUP_JEWELRY) != 0;
		}
	} },
	{ L"life", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_MAXHP, 0);
		}
	} },
	{ L"lres", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_LIGHTNINGRESIST, 0);
		}
	} },
	{ L"lvlreq", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetRequiredLevel(uInfo->item);
		}
	} },
	{ L"mace", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_ALLMACE) != 0;
		}
	} },
	{ L"maek", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_MANAAFTEREACHKILL, 0);
		}
	} },
	{ L"mag", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_MAGIC;
		}
	} },
	{ L"mana", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_MAXMANA, 0);
		}
	} },
	{ L"mapid", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			UnitAny* player = D2CLIENT_GetPlayerUnit();
			if (player) {
				int map_id = D2COMMON_GetLevelIdFromRoom(D2COMMON_GetRoomFromUnit(player));
				return map_id;
			}
			return 0;
		}
	} },
	{ L"maptier", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (maptiers.find(uInfo->attrs->category) != maptiers.end()) {
				return maptiers.at(uInfo->attrs->category);
			}
			return -1;
		}
	} },
	{ L"maxdmg", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return MinMaxDamageCondition::GetValue(MinMaxDamageCondition::DamageType::MAX, uInfo);
		}
	} },
	{ L"maxdur", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			DWORD     value = 0;
			Stat      aStatList[256] = { NULL };
			StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
			if (pStatList) {
				DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
				for (UINT i = 0; i < dwStats; i++) { if (aStatList[i].wStatIndex == STAT_ENHANCEDMAXDURABILITY && aStatList[i].wSubIndex == 0) { value += aStatList[i].dwStatValue; } }
			}
			return value;
		}
	} },
	{ L"merc", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto pMerc = GetClientMercUnit();
			auto pItemData = uInfo->item->pItemData;
			return pMerc
				&& pItemData->ItemLocation == STORAGE_NULL
				&& pItemData->BodyLocation > 0
				&& pItemData->pOwnerInventory
				&& pItemData->pOwnerInventory->pOwner == pMerc;
		}
	} },
	{ L"mfind", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_MAGICFIND, 0);
		}
	} },
	{ L"mindmg", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return MinMaxDamageCondition::GetValue(MinMaxDamageCondition::DamageType::MIN, uInfo);
		}
	} },
	{ L"misc", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->miscFlags & ITEM_GROUP_ALLMISC) != 0;
		}
	} },
	{ L"nec", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_NECROMANCER_SHIELD) != 0;
		}
	} },
	{ L"necromancer", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 2;
		}
	} },
	{ L"nmag", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_INFERIOR
				|| uInfo->item->pItemData->dwQuality == ITEM_QUALITY_NORMAL
				|| uInfo->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR;
		}
	} },
	{ L"norm", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->baseFlags & ITEM_GROUP_NORMAL) != 0;
		}
	} },
	{ L"os", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto skill = ids[0];
			if (skill < 0 || skill > SKILL_MAX) {
				err = FormulaStatus::MATH_ERROR;
				return 0;
			}
			return GetAdjustedUnitStat(uInfo, STAT_NONCLASSSKILL, skill);
		}
	} },
	{ L"paladin", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 3;
		}
	} },
	{ L"polearm", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_POLEARM) != 0;
		}
	} },
	{ L"pres", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_POISONRESIST, 0);
		}
	} },
	{ L"price", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetShopPrice(D2CLIENT_GetPlayerUnit(), uInfo->item, TRANSACTIONTYPE_SELL);
		}
	} },
	{ L"qlvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->attrs->qualityLevel;
		}
	} },
	{ L"qty", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_AMMOQUANTITY, 0);
		}
	} },
	{ L"quiver", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->miscFlags & ITEM_GROUP_QUIVER) != 0;
		}
	} },
	{ L"rare", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_RARE;
		}
	} },
	{ L"repair", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_REPAIRSDURABILITY, 0);
		}
	} },
	{ L"replife", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_REPLENISHLIFE, 0);
		}
	} },
	{ L"repquant", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_REPLENISHESQUANTITY, 0);
		}
	} },
	{ L"rerollalvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return ComputeRerollAffixLevel(uInfo);
		}
	} },
	{ L"res", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			int fRes = D2COMMON_GetUnitStat(uInfo->item, STAT_FIRERESIST, 0);
			int lRes = D2COMMON_GetUnitStat(uInfo->item, STAT_LIGHTNINGRESIST, 0);
			int cRes = D2COMMON_GetUnitStat(uInfo->item, STAT_COLDRESIST, 0);
			int pRes = D2COMMON_GetUnitStat(uInfo->item, STAT_POISONRESIST, 0);
			if (fRes && lRes && cRes && pRes) {
				return min(min(fRes, lRes), min(cRes, pRes));
			}
			return 0;
		}
	} },
	{ L"rune", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (IsRune(uInfo->attrs)) {
				return RuneNumberFromItemCode(uInfo->itemCode);
			}
			return 0;
		}
	} },
	{ L"rw", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->item->pItemData->dwFlags & ITEM_RUNEWORD) != 0;
		}
	} },
	{ L"scepter", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_SCEPTER) != 0;
		}
	} },
	{ L"sellprice", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetShopPrice(D2CLIENT_GetPlayerUnit(), uInfo->item, TRANSACTIONTYPE_SELL);
		}
	} },
	{ L"set", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_SET;
		}
	} },
	{ L"shield", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->armorFlags & ITEM_GROUP_SHIELD) != 0;
		}
	} },
	{ L"shop", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (uInfo->item->pItemData &&
				uInfo->item->pItemData->pOwnerInventory &&
				uInfo->item->pItemData->pOwnerInventory->pOwner) {
				auto wNpcId = uInfo->item->pItemData->pOwnerInventory->pOwner->dwTxtFileNo;
				if (find(begin(ShopNPCs), end(ShopNPCs), wNpcId) != end(ShopNPCs)) {
					return 1;
				}
			}
			return 0;
		}
	} },
	{ L"sin", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_ASSASSIN_KATAR) != 0;
		}
	} },
	{ L"sk", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto skill = ids[0];
			if (skill < 0 || skill > SKILL_MAX) {
				err = FormulaStatus::MATH_ERROR;
				return 0;
			}
			return GetAdjustedUnitStat(uInfo, STAT_SINGLESKILL, skill);
		}
	} },
	{ L"sock", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_SOCKETS, 0);
		}
	} },
	{ L"sockets", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_SOCKETS, 0);
		}
	} },
	{ L"sor", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_SORCERESS_ORB) != 0;
		}
	} },
	{ L"sorceress", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return D2CLIENT_GetPlayerUnit()->dwTxtFileNo == 1;
		}
	} },
	{ L"spear", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_SPEAR) != 0;
		}
	} },
	{ L"staff", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_STAFF) != 0;
		}
	} },
	{ L"stash", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			if (!uInfo->item || !uInfo->item->pItemData) {
				return 0;
			}
			const auto pItemData = uInfo->item->pItemData;
			return pItemData->ItemLocation == STORAGE_STASH
				&& pItemData->pOwnerInventory
				&& pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit();
		}
	} },
	{ L"str", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return GetAdjustedUnitStat(uInfo, STAT_STRENGTH, 0);
		}
	} },
	{ L"sup", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR;
		}
	} },
	{ L"sword", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_SWORD) != 0;
		}
	} },
	{ L"tabsk", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			auto skill = ids[0];
			if (skill < 0 || skill > SKILL_MAX) {
				err = FormulaStatus::MATH_ERROR;
				return 0;
			}
			return GetAdjustedUnitStat(uInfo, STAT_SKILLTAB, skill);
		}
	} },
	{ L"throwing", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_THROWING) != 0;
		}
	} },
	{ L"tmace", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_TIPPED_MACE) != 0;
		}
	} },
	{ L"true", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return 1;
		}
	} },
	{ L"uni", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->item->pItemData->dwQuality == ITEM_QUALITY_UNIQUE;
		}
	} },
	{ L"wand", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_WAND) != 0;
		}
	} },
	{ L"weapon", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_ALLWEAPON) != 0;
		}
	} },
	{ L"wp", { 1, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			const auto id = ids[0];
			switch (id) {
				case 1:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_AXE) != 0;
				}
				case 2:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_ALLMACE) != 0;
				}
				case 3:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_SWORD) != 0;
				}
				case 4:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_DAGGER) != 0;
				}
				case 5:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_THROWING) != 0;
				}
				case 6:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_JAVELIN) != 0;
				}
				case 7:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_SPEAR) != 0;
				}
				case 8:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_POLEARM) != 0;
				}
				case 9:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_BOW) != 0;
				}
				case 10:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_CROSSBOW) != 0;
				}
				case 11:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_STAFF) != 0;
				}
				case 12:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_WAND) != 0;
				}
				case 13:
				{
					return (uInfo->attrs->weaponFlags & ITEM_GROUP_SCEPTER) != 0;
				}
				default:
				{
					err = FormulaStatus::MATH_ERROR;
					return 0;
				}
			}
		}
	} },
	{ L"xbow", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_CROSSBOW) != 0;
		}
	} },
	{ L"zon", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return (uInfo->attrs->weaponFlags & ITEM_GROUP_AMAZON_WEAPON) != 0;
		}
	} },
	{ L"width", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->attrs->width;
		}
	} },
	{ L"height", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->attrs->height;
		}
	} },
	{ L"area", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return uInfo->attrs->width * uInfo->attrs->height;
		}
	} },
	{ L"maxsockets", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return MaxSocketsCondition::GetValue(uInfo);
		}
	} },
	{ L"uplvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return UpStatCondition::GetValue(UpStatCondition::UpStatType::LEVEL, uInfo);
		}
	} },
	{ L"upstr", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return UpStatCondition::GetValue(UpStatCondition::UpStatType::STRENGTH, uInfo);
		}
	} },
	{ L"updex", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return UpStatCondition::GetValue(UpStatCondition::UpStatType::DEXTERITY, uInfo);
		}
	} },
	{ L"maxres", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return MaxResCondition::GetValue(uInfo);
		}
	} },
	{ L"allattrib", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return AllAttributesCondition::GetValue(uInfo);
		}
	} },
	{ L"baseblock", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseBlockCondition::GetValue(uInfo);
		}
	} },
	{ L"baseminoneh", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MIN1H, uInfo);
		}
	} },
	{ L"basemintwoh", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MIN2H, uInfo);
		}
	} },
	{ L"baseminsmite", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINSMITE, uInfo);
		}
	} },
	{ L"baseminkick", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINKICK, uInfo);
		}
	} },
	{ L"baseminthrow", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MINTHROW, uInfo);
		}
	} },
	{ L"basemaxoneh", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAX1H, uInfo);
		}
	} },
	{ L"basemaxtwoh", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAX2H, uInfo);
		}
	} },
	{ L"basemaxsmite", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXSMITE, uInfo);
		}
	} },
	{ L"basemaxkick", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXKICK, uInfo);
		}
	} },
	{ L"basemaxthrow", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return BaseWeaponDamageCondition::GetValue(BaseWeaponDamageCondition::DamageType::MAXTHROW, uInfo);
		}
	} },
	{ L"reqlvl", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::LEVEL, uInfo);
		}
	} },
	{ L"reqstr", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::STRENGTH, uInfo);
		}
	} },
	{ L"reqdex", { 0, [](FormulaStatus& err, UnitItemInfo* uInfo, const std::vector<int>& ids) -> float {
			return ReqStatCondition::GetValue(ReqStatCondition::ReqStatType::DEXTERITY, uInfo);
		}
	} },
};

// Find the item description. This code is called only when there's a cache miss
wstring ItemDescLookupCache::make_cached_T(UnitItemInfo* uInfo)
{
	ReplaceContext ctx(uInfo, L"", FALSE);
	for (vector<Rule*>::const_iterator it = RuleList.begin(); it != RuleList.end(); it++)
	{
		if ((*it)->Evaluate(uInfo))
		{
			ctx.name = (*it)->ApplyDescription(ctx);
			if ((*it)->action.stopProcessing) { break; }
		}
	}
	if (!ctx.name.empty()) { TrimItemText(uInfo, ctx.name, FALSE); }

	return ctx.name;
}

string ItemDescLookupCache::to_str(const wstring& name)
{
	std::string itemName = WideToAnsi(name);
	size_t      start_pos = 0;
	while ((start_pos = itemName.find('\n', start_pos)) != std::string::npos)
	{
		itemName.replace(start_pos, 1, " - ");
		start_pos += 3;
	}
	return itemName;
}

// Find the item name. This code is called only when there's a cache miss
wstring ItemNameLookupCache::make_cached_T(UnitItemInfo* uInfo,
	const wstring& name)
{
	wstring new_name(name);
	if (!new_name.empty() && new_name.front() == L' ') { new_name.erase(0, 1); }					// removes one leading space (happens on magic items without a prefix)
	if (!new_name.empty() && new_name.back() == L' ') { new_name.erase(new_name.size() - 1, 1); }	// removes one trailing space (happens on magic items without a suffix)
	ReplaceContext ctx(uInfo, new_name.c_str(), TRUE);
	for (vector<Rule*>::const_iterator it = RuleList.begin(); it != RuleList.end(); it++)
	{
		if ((*it)->Evaluate(uInfo))
		{
			ctx.name = (*it)->ApplyName(ctx);
			if ((*it)->action.stopProcessing) { break; }
		}
	}
	if (!ctx.name.empty()) { TrimItemText(uInfo, ctx.name, TRUE); }

	return ctx.name;
}

string ItemNameLookupCache::to_str(const wstring& name)
{
	std::string itemName = WideToAnsi(name);
	size_t      start_pos = 0;
	while ((start_pos = itemName.find('\n', start_pos)) != std::string::npos)
	{
		itemName.replace(start_pos, 1, " - ");
		start_pos += 3;
	}
	return itemName;
}

vector<Action> MapActionLookupCache::make_cached_T(UnitItemInfo* uInfo)
{
	vector<Action> actions;
	for (vector<Rule*>::const_iterator it = RuleList.begin(); it != RuleList.end(); it++) { if ((*it)->Evaluate(uInfo)) { actions.push_back((*it)->action); } }
	return actions;
}

string MapActionLookupCache::to_str(const vector<Action>& actions)
{
	string name;
	for (auto& action : actions) { name += WideToAnsi(action.name) + " "; }
	return name;
}

// least recently used cache for storing a limited number of item names
ItemDescLookupCache  item_desc_cache(RuleList);
ItemNameLookupCache  item_name_cache(RuleList);
MapActionLookupCache map_action_cache(MapRuleList);

void GetItemName(UnitItemInfo* uInfo,
	wstring& name)
{
	wstring new_name = item_name_cache.Get(uInfo, name);
	if (new_name == L"" && Item::GetFilterLevel() == 0) {
		return;
	}
	name.assign(new_name);
}

wstring NameVarSockets(UnitItemInfo* uInfo)
{
	wchar_t sockets[4] = L"";
	swprintf(sockets, 4, L"%d", D2COMMON_GetUnitStat(uInfo->item, STAT_SOCKETS, 0));
	return 	sockets;
}

wstring NameVarRuneNum(UnitItemInfo* uInfo)
{
	wchar_t runenum[4] = L"0";
	if (IsRune(uInfo->attrs))
		swprintf(runenum, 4, L"%d", RuneNumberFromItemCode(uInfo->itemCode));
	return runenum;
}

wstring NameVarRuneName(UnitItemInfo* uInfo)
{
	// TODO: removes " Rune" from the rune name. Pretty likely to break on non-english strings
	// Utilize D2LANG_GetLocaleText(20473) area?
	wstring runename;
	if (IsRune(uInfo->attrs))
	{
		runename = uInfo->attrs->name.substr(0, uInfo->attrs->name.find(L' '));
	}
	return runename;
}

wstring NameVarGemLevel(UnitItemInfo* uInfo)
{
	wchar_t gemlevel[16] = L"";
	if (IsGem(uInfo->attrs))
		swprintf(gemlevel, 16, L"%ls", GetGemLevelString(GetGemLevel(uInfo->attrs)));
	return gemlevel;
}

wstring NameVarGemType(UnitItemInfo* uInfo)
{
	wchar_t gemtype[16] = L"";
	if (IsGem(uInfo->attrs))
		swprintf(gemtype, 16, L"%ls", GetGemTypeString(GetGemType(uInfo->attrs)));
	return gemtype;
}

wstring NameVarIlvl(UnitItemInfo* uInfo)
{
	wchar_t ilvl[4] = L"0";
	swprintf(ilvl, 4, L"%d", uInfo->item->pItemData->dwItemLevel);
	return ilvl;
}

wstring NameVarAlvl(UnitItemInfo* uInfo)
{
	wchar_t alvl[4] = L"0";
	int alvl_int = GetAffixLevel(
		uInfo->item->pItemData->dwItemLevel,
		uInfo->attrs->qualityLevel,
		uInfo->attrs->magicLevel
	);
	swprintf(alvl, 4, L"%d", alvl_int);
	return alvl;
}

wstring NameVarCraftAlvl(UnitItemInfo* uInfo)
{
	wchar_t craftalvl[4] = L"0";
	int clvl = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);

	int craft_alvl = GetAffixLevel(
		(int)(0.5 * clvl) + (int)(0.5 * uInfo->item->pItemData->dwItemLevel),
		uInfo->attrs->qualityLevel,
		uInfo->attrs->magicLevel
	);
	swprintf(craftalvl, 4, L"%d", craft_alvl);
	return craftalvl;
}

wstring NameVarRerollAlvl(UnitItemInfo* uInfo)
{
	wchar_t alvl[4] = L"0";
	int reroll_alvl = ComputeRerollAffixLevel(uInfo);
	swprintf(alvl, 4, L"%d", reroll_alvl);
	return alvl;
}

wstring NameVarLevelReq(UnitItemInfo* uInfo)
{
	wchar_t lvlreq[4] = L"0";
	swprintf(lvlreq, 4, L"%d", GetRequiredLevel(uInfo->item));
	return lvlreq;
}

wstring NameVarWeaponSpeed(ItemsTxt* itemTxt)
{
	wchar_t wpnspd[4] = L"0";
	swprintf(wpnspd, 4, L"%d", itemTxt->dwspeed);
	return wpnspd;
}

wstring NameVarRangeAdder(ItemsTxt* itemTxt)
{
	wchar_t rangeadder[4] = L"0";
	swprintf(rangeadder, 4, L"%d", itemTxt->brangeadder);
	return rangeadder;
}

wstring NameVarBuyValue(UnitItemInfo* uInfo,
	ItemsTxt* itemTxt)
{
	wchar_t sellvalue[16] = L"";
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (pUnit && itemTxt->bquest == 0)
	{
		swprintf(sellvalue, 16, L"%d", GetShopPrice(pUnit, uInfo->item, TRANSACTIONTYPE_BUY));
	}
	return sellvalue;
}

wstring NameVarSellValue(UnitItemInfo* uInfo,
	ItemsTxt* itemTxt)
{
	wchar_t sellvalue[16] = L"";
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (pUnit && itemTxt->bquest == 0)
	{
		swprintf(sellvalue, 16, L"%d", GetShopPrice(pUnit, uInfo->item, TRANSACTIONTYPE_SELL));
	}
	return sellvalue;
}

wstring NameVarQty(UnitItemInfo* uInfo)
{
	wchar_t qty[10] = L"0";
	swprintf(qty, 10, L"%d", D2COMMON_GetUnitStat(uInfo->item, STAT_AMMOQUANTITY, 0));
	return qty;
}

wstring NameVarAllRes(UnitItemInfo* uInfo)
{
	wchar_t allres[4] = L"0";
	int fRes = D2COMMON_GetUnitStat(uInfo->item, STAT_FIRERESIST, 0);
	int lRes = D2COMMON_GetUnitStat(uInfo->item, STAT_LIGHTNINGRESIST, 0);
	int cRes = D2COMMON_GetUnitStat(uInfo->item, STAT_COLDRESIST, 0);
	int pRes = D2COMMON_GetUnitStat(uInfo->item, STAT_POISONRESIST, 0);
	int minres = 0;
	if (fRes && lRes && cRes && pRes)
	{
		minres = min(min(fRes, lRes), min(cRes, pRes));
	}
	swprintf(allres, 4, L"%d", minres);
	return allres;
}

wstring NameVarEd(UnitItemInfo* uInfo)
{
	wchar_t ed[4] = L"0";

	// Either enhanced defense or enhanced damage depending on item type
	WORD stat;
	if (uInfo->attrs->armorFlags & ITEM_GROUP_ALLARMOR) { stat = STAT_ENHANCEDDEFENSE; }
	else
	{
		// Normal %ED will have the same value for STAT_ENHANCEDMAXIMUMDAMAGE and STAT_ENHANCEDMINIMUMDAMAGE
		stat = STAT_ENHANCEDMAXIMUMDAMAGE;
	}
	DWORD     value = 0;
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	if (pStatList)
	{
		value += D2COMMON_GetStatValueFromStatList(pStatList, stat, 0);
	}
	swprintf(ed, 4, L"%d", value);
	return ed;
}

bool IsWhitespaceEquivalent(wchar_t ch)
{
	return iswspace(ch) || ch == L'\b';
}

void TrimItemText(UnitItemInfo* uInfo,
	wstring& name,
	BOOL bLimit)
{
	int offset = 0;
	for (int i = 0; i < name.length(); ++i) {
		const wchar_t ch = name[i];
		switch (ch) {
			case L'\r':
			{
				// only need a newline if
				// 1) the previous character isn't a newline
				// 2) there is non-newline next
				if (offset == 0 || name[offset - 1] == L'\n') {
					break;
				}
				if (i + 1 < name.length() && name[i + 1] != L'\n' && name[i + 1] != L'\r') {
					name[offset++] = L'\n';
				}
				break;
			}
			case L'\b':
			{
				// only need a space if
				// 1) the previous character isn't whitespace
				// 2) there is non-whitespace next
				if (offset == 0 || IsWhitespaceEquivalent(name[offset - 1])) {
					break;
				}
				if (i + 1 < name.length() && !IsWhitespaceEquivalent(name[i + 1])) {
					name[offset++] = L' ';
				}
				break;
			}
			default:
			{
				name[offset++] = ch;
			}
		}
	}
	name.resize(offset);

	int nColorCodesSize = 0;
	int lengthLimit = 0;
	if (bLimit)
	{
		// Calc the extra size from colors
		// In wide strings: color code is \xFF followed by 'c' followed by digit = 3 wchar_t
		std::wregex color_reg(L"\\xFFc[0-9;:\\x01-\\x1F]", std::regex_constants::ECMAScript);
		auto       color_matches = std::wsregex_iterator(name.begin(), name.end(), color_reg);
		auto       color_end = std::wsregex_iterator();
		auto       match_count = std::distance(color_matches, color_end);
		nColorCodesSize += 3 * match_count;

		bool inShop = (uInfo->item->pItemData->pOwnerInventory != 0 && // Skip on ground items
			find(begin(ShopNPCs), end(ShopNPCs), uInfo->item->pItemData->pOwnerInventory->pOwner->dwTxtFileNo) != end(ShopNPCs));

		// Increase limit for shop items
		lengthLimit = inShop ? MAX_ITEM_TEXT_SIZE : MAX_ITEM_NAME_SIZE;

		int nColorsToKeep = 0;
		for (std::wsregex_iterator k = color_matches; k != color_end; ++k)
		{
			std::wsmatch match = *k;
			auto        pos = match.position();
			if (pos - (nColorsToKeep) > lengthLimit) { break; }
			nColorsToKeep += 3;
		}

		// Truncate if too long
		if (name.size() - nColorCodesSize > lengthLimit)
		{
			int max_size = lengthLimit + nColorsToKeep;
			name.resize(max_size);
		}
	}

	// Limit all names/descriptions to a hard cap, regardless of color codes
	lengthLimit = (uInfo->itemCode[0] == 't' || uInfo->itemCode[0] == 'i') &&
		uInfo->itemCode[1] == 'b' &&
		uInfo->itemCode[2] == 'k' ? BOOK_NAME_SIZE_LIMIT : MAX_ITEM_TEXT_SIZE;
	if (name.size() > lengthLimit - 1) { name.resize(lengthLimit - 1); }
}

BYTE GetAffixLevel(BYTE ilvl,
	BYTE qlvl,
	BYTE mlvl)
{
	if (ilvl > 99) { ilvl = 99; }
	if (qlvl > ilvl) { ilvl = qlvl; }
	if (mlvl > 0) { return ilvl + mlvl > 99 ? 99 : ilvl + mlvl; }
	return ((ilvl) < (99 - ((qlvl) / 2)) ? (ilvl)-((qlvl) / 2) : (ilvl) * 2 - 99);
}

// Returns the (lowest) level requirement (for any class) of an item
BYTE GetRequiredLevel(UnitAny* item)
{
	// Some crafted items can supposedly go above 100, but it's practically the same as 100
	BYTE rlvl = 100;

	// The unit for which the required level is calculated
	UnitAny* character = D2CLIENT_GetPlayerUnit();

	// Extra checks for these as they can have charges
	if (item->pItemData->dwQuality == ITEM_QUALITY_RARE || item->pItemData->dwQuality == ITEM_QUALITY_MAGIC)
	{
		// Save the original class of the character (0-6)
		DWORD temp = character->dwTxtFileNo;

		// Pretend to be every class once, use the lowest req lvl (for charged items)
		for (DWORD i = 0; i < 7; i++)
		{
			character->dwTxtFileNo = i;
			BYTE temprlvl = (BYTE)D2COMMON_GetItemLevelRequirement(item, character);

			if (temprlvl < rlvl)
			{
				rlvl = temprlvl;
				//Only one class will have a lower req than the others, so if a lower one is found we can stop
				if (i > 0) { break; }
			}
		}
		// Go back to being original class
		character->dwTxtFileNo = temp;
	}
	else { rlvl = (BYTE)D2COMMON_GetItemLevelRequirement(item, character); }

	return rlvl;
}

BYTE GetOperation(wstring* op)
{
	if (op->length() < 1) { return NONE; }
	else if ((*op)[0] == L'=') { return EQUAL; }
	else if ((*op)[0] == L'<') { return LESS_THAN; }
	else if ((*op)[0] == L'>') { return GREATER_THAN; }
	else if ((*op)[0] == L'~') { return BETWEEN; }
	return NONE;
}

unsigned int GetItemCodeIndex(char codeChar)
{
	// Characters '0'-'9' map to 0-9, and a-z map to 10-35
	return codeChar - (codeChar < 90 ? 48 : 87);
}

bool FloatCompare(float Lvalue,
	BYTE         operation,
	int Rvalue,
	int Bvalue = 0)
{
	switch (operation) {
		case EQUAL:
			return Lvalue == Rvalue;
		case GREATER_THAN:
			return Lvalue > Rvalue;
		case LESS_THAN:
			return Lvalue < Rvalue;
		case BETWEEN:
			return (Rvalue <= Lvalue && Lvalue <= Bvalue);
		default:
			return false;
	}
}

bool IntegerCompare(int Lvalue,
	BYTE         operation,
	int Rvalue,
	int Bvalue = 0)
{
	switch (operation)
	{
	case EQUAL:
		return Lvalue == Rvalue;
	case GREATER_THAN:
		return Lvalue > Rvalue;
	case LESS_THAN:
		return Lvalue < Rvalue;
	case BETWEEN:
		return (Rvalue <= Lvalue && Lvalue <= Bvalue);
	default:
		return false;
	}
}

void RegisterFormula(const std::wstring& ref, std::unique_ptr<Formula<FormulaContext>>& ptr)
{
	formulaMap.insert_or_assign(ref, std::move(ptr));
	FormulaReplacementMap.insert_or_assign(ref, ReplacementSpec{ 0, ReplacementSpec::ReplaceBindFormula(formulaMap.find(ref)->second) });
}

struct IslandReplacementHelper
{
	const wstring IslandIdentifier = L"$f(";
	const wstring IslandPrefix = L"ISLAND_";
	wstring IslandSuffix;

	IslandReplacementHelper()
	{
		reset();
	}

	void reset()
	{
		IslandSuffix.clear();
	}

	wstring GetNextFormulaIslandRef()
	{
		for (wchar_t& c : IslandSuffix) {
			if (c == L'Z') {
				c = L'A';
				continue;
			}
			++c;
			return IslandPrefix + IslandSuffix;
		}

		IslandSuffix.push_back(L'A');
		return IslandPrefix + IslandSuffix;
	}

	size_t MatchParentheses(wstring& text, size_t begin)
	{
		for (int i = begin, count = 0; i < text.length(); ++i) {
			if (text[i] == L'(') {
				count += 1;
			}
			else if (text[i] == L')') {
				count -= 1;
				if (count == 0) {
					return i;
				}
			}
		}
		return text.length();
	}

	void ReplaceFormulaIslands(wstring& text, wstring& pre, wstring& suf)
	{
		wstring result;
		size_t offset = 0;
		while (offset < text.length()) {
			const auto start = text.find(IslandIdentifier, offset);
			if (start == wstring::npos) {
				if (offset == 0) {
					return;
				}
				result.append(text, offset, wstring::npos);
				break;
			}
			result.append(text, offset, start - offset);
			size_t endOfIsland = MatchParentheses(text, start + IslandIdentifier.length() - 1);
			if (endOfIsland < text.length()) {
				std::unique_ptr<Formula<FormulaContext>> out;
				size_t length = endOfIsland - start - IslandIdentifier.length();
				if (Formula<FormulaContext>::Compile(text.substr(start + IslandIdentifier.length(), length), out, formulaVarDefs) == FormulaStatus::OK) {
					const auto ref = GetNextFormulaIslandRef();
					RegisterFormula(ref, out);
					result.append(pre + ref + suf);
				}
				else {
					result.append(text, start, endOfIsland - start + 1);
				}
				offset = endOfIsland + 1;
				continue;
			}
			result.append(text, start, IslandIdentifier.length());
			offset = start + IslandIdentifier.length();
		}
		text = move(result);
	}
} islandReplacementHelper;

namespace ItemDisplay
{
	bool item_display_initialized = false;

	void InitializeItemRules()
	{
		if (item_display_initialized) return;
		if (!IsInitialized()) { return; }


		item_display_initialized = true;
		rules.clear();
		aliases.clear();
		formulas.clear();
		formulaMap.clear();
		FormulaReplacementMap.clear();
		islandReplacementHelper.reset();
		ResetCaches();

		{
			vector<pair<string, string>> rawAliases;
			BH::lootFilter->ReadMapList("Alias", rawAliases);
			for (auto& p : rawAliases) {
				aliases.push_back({AnsiToWide(p.first), AnsiToWide(p.second)});
			}
		}
		BH::lootFilter->ReadMapList("Formula", formulas);
		{
			vector<pair<string, string>> rawRules;
			BH::lootFilter->ReadMapList("ItemDisplay", rawRules);
			for (auto& p : rawRules) {
				rules.push_back({AnsiToWide(p.first), AnsiToWide(p.second)});
			}
		}

		// Limit aliases to single keywords
		for (unsigned int i = 0; i < aliases.size(); i++)
		{
			aliases[i].first = TrimW(aliases[i].first);

			if (aliases[i].first.find(L" ") != wstring::npos)
				aliases[i].first.erase(aliases[i].first.find(L" "));
		}

		for (const auto& f : formulas)
		{
			const auto& key = f.first;
			const auto& text = f.second;

			auto formulaRef = L"FORMULA" + AnsiToWide(key);
			transform(formulaRef.begin(), formulaRef.end(), formulaRef.begin(), [](wchar_t c) { return (c >= L'a' && c <= L'z') ? (c - 32) : c; });

			std::unique_ptr<Formula<FormulaContext>> out;
			if (Formula<FormulaContext>::Compile(AnsiToWide(text), out, formulaVarDefs) != FormulaStatus::OK)
			{
				continue;
			}

			RegisterFormula(formulaRef, out);
		}

		std::wstring percent = L"%";
		std::wstring empty = L"";
		for (unsigned int i = 0; i < rules.size(); i++)
		{
			for (auto alias : aliases)
			{
				if (alias.first.empty())
					continue;

				while (rules[i].first.find(alias.first) != wstring::npos)
					rules[i].first.replace(rules[i].first.find(alias.first), alias.first.length(), alias.second);

				transform(alias.first.begin(), alias.first.end(), alias.first.begin(), [](wchar_t c) { return (c >= L'a' && c <= L'z') ? (c - 32) : c; });
				while (rules[i].second.find(L"%" + alias.first + L"%") != wstring::npos)
					rules[i].second.replace(rules[i].second.find(L"%" + alias.first + L"%"), alias.first.length() + 2, alias.second);
			}

			// find inline formula islands
			islandReplacementHelper.ReplaceFormulaIslands(rules[i].first, empty, empty);
			islandReplacementHelper.ReplaceFormulaIslands(rules[i].second, percent, percent);

			wstring         buf;
			wstringstream   ss(rules[i].first);
			vector<wstring> tokens;
			while (ss >> buf) { tokens.push_back(buf); }

			LastConditionType = CT_None;
			vector<Condition*> RawConditions;
			for (vector<wstring>::iterator tok = tokens.begin(); tok < tokens.end(); tok++) { Condition::BuildConditions(RawConditions, (*tok)); }
			Rule* r = new Rule(RawConditions, &(rules[i].second));

			RuleList.push_back(r);
			if (r->action.colorOnMap != UNDEFINED_COLOR ||
				r->action.borderColor != UNDEFINED_COLOR ||
				r->action.dotColor != UNDEFINED_COLOR ||
				r->action.pxColor != UNDEFINED_COLOR ||
				r->action.lineColor != UNDEFINED_COLOR ||
				r->action.soundID != 0) {
				MapRuleList.push_back(r);
			}
			else if (r->action.name.length() == 0) { IgnoreRuleList.push_back(r); }
		}

		// Setup filter level names based on config file
		Item* item = static_cast<Item*>(BH::moduleManager->Get("item"));

		// Clear and add 0 to the list
		item->ItemFilterNames.clear();
		item->ItemFilterNames.push_back("0 - Show All Items");

		vector<pair<string, string>> filterDefinitions;
		BH::lootFilter->ReadMapList("ItemDisplayFilterName", filterDefinitions);
		for (unsigned int i = 0; i < filterDefinitions.size(); i++) {
			item->ItemFilterNames.push_back(to_string(i + 1) + " - " + filterDefinitions[i].second);

			// Max 9 entries
			if (i >= 11) {
				break;
			}
		}

		// If there is only 1 entry, it means no definitons were made, add standard
		if (item->ItemFilterNames.size() == 1) {
			item->ItemFilterNames.push_back("1 - Standard");
		}

		if (App.lootfilter.filterLevel.uValue >= item->ItemFilterNames.size()) {
			App.lootfilter.filterLevel.uValue = 1;
		}
		//item->ReplaceItemFilters(item->ItemFilterNames);
	}

	void UninitializeItemRules()
	{
		// RuleList contains every created rule. MapRuleList and IgnoreRuleList have a subset of rules.
		// Deleting objects in RuleList is sufficient.
		if (item_display_initialized)
		{
			for (Rule* r : RuleList)
			{
				for (Condition* condition : r->conditions) { delete condition; }
				delete r;
			}
		}
		item_display_initialized = false;
		ResetCaches();
		RuleList.clear();
		MapRuleList.clear();
		IgnoreRuleList.clear();
	}
}

Rule::Rule(vector<Condition*>& inputConditions,
	wstring* str)
{
	Condition::ProcessConditions(inputConditions, conditions);
	BuildAction(str, &action);
	if (!Convert()) {
		root = -1;
	}
	name = BuildReplacementActions(action.name);
	description = BuildReplacementActions(action.description);
}

wstring Rule::ApplyName(ReplaceContext& ctx) {
	wstring res;
	for (const auto& fn : name) {
		res += fn.Replace(ctx);
	}
	return res;
}

wstring Rule::ApplyDescription(ReplaceContext& ctx) {
	wstring res;
	for (const auto& fn : description) {
		res += fn.Replace(ctx);
	}
	return res;
}

bool Rule::Convert() {
	nodes.clear();
	nodes.reserve(conditions.size());
	vector<size_t> stack;
	stack.reserve(nodes.capacity());
	for (Condition* condition : conditions) {
		nodes.emplace_back(condition, -1, -1);
		const size_t id = nodes.size() - 1;
		auto& node = nodes[id];
		switch (condition->conditionType) {
		case CT_Operand: {
			stack.push_back(id);
			break;
		}
		case CT_BinaryOperator: {
			if (stack.size() < 2) {
				return false;
			}
			node.rhs = stack.back();
			stack.pop_back();
			node.lhs = stack.back();
			stack.pop_back();
			stack.push_back(id);
			break;
		}
		case CT_NegationOperator: {
			if (stack.size() < 1) {
				return false;
			}
			node.lhs = stack.back();
			stack.pop_back();
			stack.push_back(id);
			break;
		}
		default:
			return false;
		}
	}
	if (stack.size() != 1) {
		return false;
	}
	root = stack.back();
	const auto& node = nodes[root];
	if (node.condition->conditionType == CT_BinaryOperator) {
		return node.lhs != -1 && node.rhs != -1;
	}
	if (node.condition->conditionType == CT_NegationOperator) {
		return node.lhs != -1;
	}
	return true;
}

bool Rule::EvaluateTree(UnitItemInfo* uInfo) {
	return root != -1 && EvaluateTreeInternal(uInfo, root);
}

bool Rule::EvaluateTreeInternal(UnitItemInfo* uInfo, size_t id) {
	const auto& node = nodes[id];
	switch (node.condition->conditionType) {
	case CT_Operand: {
		return node.condition->Evaluate(uInfo, NULL, NULL);
	}
	case CT_BinaryOperator: {
		if (node.condition->circuitType == Condition::CircuitType::AND) {
			return EvaluateTreeInternal(uInfo, node.lhs) && EvaluateTreeInternal(uInfo, node.rhs);
		}
		if (node.condition->circuitType == Condition::CircuitType::OR) {
			return EvaluateTreeInternal(uInfo, node.lhs) || EvaluateTreeInternal(uInfo, node.rhs);
		}
		// and, or are the only binary ops
		return false;
	}
	case CT_NegationOperator: {
		return !EvaluateTreeInternal(uInfo, node.lhs);
	}
	default:
		return false;
	}
}

void BuildAction(wstring* str,
	Action* act)
{
	act->name = wstring(str->c_str());

	//// upcase all text in a %replacement_string%
	//// for some reason \w wasn't catching _, so I added it to the groups
	try
	{
		std::wregex replace_reg(
			LR"(^(?:(?:%[^%]*%)|[^%])*%((?:\w|_|-)*?[a-z]+?(?:\w|_|-)*?)%)",
			std::regex_constants::ECMAScript);
		std::wsmatch replace_match;
		while (std::regex_search(act->name, replace_match, replace_reg))
		{
			auto offset = replace_match[1].first - act->name.begin();
			std::transform(
				replace_match[1].first,
				replace_match[1].second,
				act->name.begin() + offset,
				[](wchar_t c) { return (c >= L'a' && c <= L'z') ? (c - 32) : c; }
			);
		}
	}
	catch (std::exception e)
	{
		act->name = L"\xFF" L"c1FILTER REGEX ERROR";
	}

	// new stuff:
	act->borderColor = ParseMapColor(act, L"BORDER");
	act->colorOnMap = ParseMapColor(act, L"MAP");
	act->dotColor = ParseMapColor(act, L"DOT");
	act->pxColor = ParseMapColor(act, L"PX");
	act->lineColor = ParseMapColor(act, L"LINE");
	act->notifyColor = ParseMapColor(act, L"NOTIFY");
	act->pingLevel = ParsePingLevel(act, L"TIER");
	act->description = ParseDescription(act);
	act->soundID = ParseSoundID(act, L"SOUNDID");

	// legacy support:
	size_t map = act->name.find(L"%MAP%");
	if (map != wstring::npos)
	{
		int          mapColor = MAP_COLOR_WHITE;
		size_t       lastColorPos = 0;
		ColorReplace colors[] = {
			MAP_COLOR_REPLACEMENTS
		};
		for (int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++)
		{
			size_t pos = act->name.find(L"%" + colors[n].key + L"%");
			if (pos != wstring::npos && pos < map && pos >= lastColorPos)
			{
				mapColor = colors[n].value;
				lastColorPos = pos;
			}
		}

		act->name.replace(map, 5, L"");
		act->colorOnMap = mapColor;
		if (act->borderColor == UNDEFINED_COLOR)
			act->borderColor = act->colorOnMap;
	}

	size_t done = act->name.find(L"%CONTINUE%");
	if (done != wstring::npos)
	{
		act->name.replace(done, 10, L"");
		act->stopProcessing = false;
	}
}

int ParsePingLevel(Action* act, const wstring& key_string) {
	std::wregex pattern(L"%" + key_string + L"-([0-9])%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	int ping_level = -1;
	std::wsmatch the_match;

	if (std::regex_search(act->name, the_match, pattern)) {
		ping_level = stoi(the_match[1].str());
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(), L"");
	}
	return ping_level;
}

// ParseSoundID
// Returns an int ranging from 0 to the MAX_SOUND_ID.
// If the parsed soundID is not found in that range, this will return a 0.
int ParseSoundID(Action* act, const wstring& key_string) {
	std::wregex pattern(L"%" + key_string + L"-([0-9]{1,4})%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	// Default soundID should be 0 incase this is played.
	// 0 is none.wav
	int soundID = 0;
	std::wsmatch the_match;

	if (std::regex_search(act->name, the_match, pattern)) {
		int matchedSoundID = stoi(the_match[1].str());
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(), L"");

		// Do our best to ensure the soundID is valid.
		// Ensure soundID is in the range of 0 and MAX_SOUND_ID.
		if (matchedSoundID < *p_D2CLIENT_SoundRecords) {
			soundID = matchedSoundID;
		}
	}

	return soundID;
}

wstring ParseDescription(Action* act)
{
	size_t l_idx = act->name.find(L"{");
	size_t r_idx = act->name.find(L"}");
	if (l_idx == wstring::npos || r_idx == wstring::npos || l_idx > r_idx) return L"";
	size_t start_idx = l_idx + 1;
	size_t len = r_idx - start_idx;
	wstring desc_string = act->name.substr(start_idx, len);
	act->name.replace(l_idx, len + 2, L"");
	return desc_string;
}

int ParseMapColor(Action* act,
	const wstring& key_string)
{
	std::wregex pattern(L"%" + key_string + L"-([a-f0-9]{1,4})%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	int         color = UNDEFINED_COLOR;
	std::wsmatch the_match;

	if (std::regex_search(act->name, the_match, pattern))
	{
		color = stoi(the_match[1].str(), nullptr, 16);
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(),
			L"");
	}
	return color;
}

const wstring Condition::tokenDelims = L"<=>~";

// Implements the shunting-yard algorithm to evaluate condition expressions
// Returns a vector of conditions in Reverse Polish Notation
void Condition::ProcessConditions(vector<Condition*>& inputConditions,
	vector<Condition*>& processedConditions)
{
	vector<Condition*> conditionStack;
	unsigned int       size = inputConditions.size();
	for (unsigned int c = 0; c < size; c++)
	{
		Condition* input = inputConditions[c];
		if (input->conditionType == CT_Operand) { processedConditions.push_back(input); }
		else if (input->conditionType == CT_BinaryOperator || input->conditionType == CT_NegationOperator)
		{
			bool go = true;
			while (go)
			{
				if (conditionStack.size() > 0)
				{
					Condition* stack = conditionStack.back();
					if ((stack->conditionType == CT_NegationOperator || stack->conditionType == CT_BinaryOperator) &&
						input->conditionType == CT_BinaryOperator)
					{
						conditionStack.pop_back();
						processedConditions.push_back(stack);
					}
					else { go = false; }
				}
				else { go = false; }
			}
			conditionStack.push_back(input);
		}
		else if (input->conditionType == CT_LeftParen) { conditionStack.push_back(input); }
		else if (input->conditionType == CT_RightParen)
		{
			bool foundLeftParen = false;
			while (conditionStack.size() > 0 && !foundLeftParen)
			{
				Condition* stack = conditionStack.back();
				conditionStack.pop_back();
				if (stack->conditionType == CT_LeftParen)
				{
					foundLeftParen = true;
					break;
				}
				else { processedConditions.push_back(stack); }
			}
			if (!foundLeftParen)
			{
				// TODO: find a way to report error
				return;
			}
		}
	}
	while (conditionStack.size() > 0)
	{
		Condition* next = conditionStack.back();
		conditionStack.pop_back();
		if (next->conditionType == CT_LeftParen || next->conditionType == CT_RightParen)
		{
			// TODO: find a way to report error
			break;
		}
		else { processedConditions.push_back(next); }
	}
}

void Condition::BuildConditions(vector<Condition*>& conditions,
	wstring              token)
{
	vector<Condition*> endConditions;
	int                i;

	// Since we don't have a real parser, things will break if [!()] appear in
	// the middle of a token (e.g. "(X AND Y)(A AND B)")

	// Look for syntax characters at the beginning of the token
	for (i = 0; i < (int)token.length(); i++)
	{
		if (token[i] == L'!') { Condition::AddNonOperand(conditions, new NegationOperator()); }
		else if (token[i] == L'(') { Condition::AddNonOperand(conditions, new LeftParen()); }
		else if (token[i] == L')') { Condition::AddNonOperand(conditions, new RightParen()); }
		else { break; }
	}
	token.erase(0, i);

	// Look for syntax characters at the end of the token
	for (i = token.length() - 1; i >= 0; i--)
	{
		if (token[i] == L'!') { endConditions.insert(endConditions.begin(), new NegationOperator()); }
		else if (token[i] == L'(') { endConditions.insert(endConditions.begin(), new LeftParen()); }
		else if (token[i] == L')') { endConditions.insert(endConditions.begin(), new RightParen()); }
		else { break; }
	}
	if (i < (int)(token.length() - 1)) { token.erase(i + 1, wstring::npos); }

	size_t delPos = token.find_first_of(tokenDelims);
	wstring key;
	wstring delim = L"";
	int    value = 0;
	int    value2 = 0;
	if (delPos != wstring::npos)
	{
		key = TrimW(token.substr(0, delPos));
		delim = token.substr(delPos, 1);
		wstring valueStr = TrimW(token.substr(delPos + 1));
		if (valueStr.length() > 0)
		{
			// Get min/max values if a range is given
			if (delim == L"~" && valueStr.find(L"-") != wstring::npos)
			{
				auto rangeDelim = valueStr.find(L"-");
				wstringstream ss1(valueStr.substr(0, rangeDelim));
				valueStr.erase(0, rangeDelim + 1);
				wstringstream ss2(valueStr);
				if ((ss1 >> value).fail() || (ss2 >> value2).fail())
				{
					return; // TODO: returning errors
				}
			}
			else {
				wstringstream ss(valueStr);
				if ((ss >> value).fail())
				{
					return; // TODO: returning errors
				}
			}
		}
	}
	else { key = token; }

	BYTE operation = GetOperation(&delim);
	unsigned int keylen = key.length();
	wstringstream number_ss(L"");
	int cond_num = -1;
	std::wregex multi_reg(L"([0-9]{1,10}),([0-9]{1,10})", std::regex_constants::ECMAScript | std::regex_constants::icase);
	std::wsmatch multi_match;

	FilterCondition condition = COND_NULL;
	if (condition_map.find(key) != condition_map.end())
	{
		condition = condition_map[key];
	}
	else if (keylen >= 3 && !(iswupper(key[0]) || iswupper(key[1]) || iswupper(key[2])))
	{
		condition = COND_ITEMCODE;
	}
	else if (key.find(L'+') != std::wstring::npos)
	{
		condition = COND_ADD;
	}
	else if (key.compare(0, 2, L"SK") == 0)
	{
		condition = COND_SK;
		number_ss = wstringstream(key.substr(2));
	}
	else if (key.compare(0, 2, L"OS") == 0)
	{
		condition = COND_OS;
		number_ss = wstringstream(key.substr(2));
	}
	else if (key.compare(0, 4, L"CHSK") == 0)
	{
		condition = COND_CHSK;
		number_ss = wstringstream(key.substr(4));
	}
	else if (key.compare(0, 4, L"CLSK") == 0)
	{
		condition = COND_CLSK;
		number_ss = wstringstream(key.substr(4));
	}
	else if (key.compare(0, 5, L"TABSK") == 0)
	{
		condition = COND_TABSK;
		number_ss = wstringstream(key.substr(5));
	}
	else if (key.compare(0, 4, L"STAT") == 0)
	{
		condition = COND_STAT;
		number_ss = wstringstream(key.substr(4));
	}
	else if (key.compare(0, 8, L"CHARSTAT") == 0)
	{
		condition = COND_CHARSTAT;
		number_ss = wstringstream(key.substr(8));
	}
	else if (key.compare(0, 5, L"MULTI") == 0)
	{
		condition = COND_MULTI;
	}
	else if (formulaMap.find(key) != formulaMap.end())
	{
		condition = COND_FORMULA;
	}

	switch (condition)
	{
	case COND_AND:
		Condition::AddNonOperand(conditions, new AndOperator());
		break;
	case COND_OR:
		Condition::AddNonOperand(conditions, new OrOperator());
		break;
	case COND_TRUE:
		Condition::AddOperand(conditions, new TrueCondition());
		break;
	case COND_FALSE:
		Condition::AddOperand(conditions, new FalseCondition());
		break;
	case COND_ETH:
		Condition::AddOperand(conditions, new FlagsCondition(ITEM_ETHEREAL));
		break;
	case COND_SOCK:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_SOCKETS, 0, operation, value, value2));
		break;
	case COND_SET:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_SET));
		break;
	case COND_MAG:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_MAGIC));
		break;
	case COND_RARE:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_RARE));
		break;
	case COND_UNI:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_UNIQUE));
		break;
	case COND_AMAZON:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 0));
		break;
	case COND_SORCERESS:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 1));
		break;
	case COND_NECROMANCER:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 2));
		break;
	case COND_PALADIN:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 3));
		break;
	case COND_BARBARIAN:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 4));
		break;
	case COND_DRUID:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 5));
		break;
	case COND_ASSASSIN:
		Condition::AddOperand(conditions, new CharacterClassCondition(EQUAL, 6));
		break;
	case COND_CRAFTALVL:
		Condition::AddOperand(conditions, new CraftLevelCondition(operation, value, value2));
		break;
	case COND_REROLLALVL:
		Condition::AddOperand(conditions, new RerollLevelCondition(operation, value, value2));
		break;
	case COND_PREFIX:
		Condition::AddOperand(conditions, new MagicPrefixCondition(operation, value, value2));
		break;
	case COND_SUFFIX:
		Condition::AddOperand(conditions, new MagicSuffixCondition(operation, value, value2));
		break;
	case COND_AUTOMOD:
		Condition::AddOperand(conditions, new AutomodCondition(operation, value, value2));
		break;
	case COND_MAPID:
		Condition::AddOperand(conditions, new MapIdCondition(operation, value, value2));
		break;
	case COND_MAPTIER:
		Condition::AddOperand(conditions, new MapTierCondition(operation, value, value2));
		break;
	case COND_CRAFT:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_CRAFT));
		break;
	case COND_RW:
		Condition::AddOperand(conditions, new FlagsCondition(ITEM_RUNEWORD));
		break;
	case COND_NMAG:
		Condition::AddOperand(conditions, new NonMagicalCondition());
		break;
	case COND_SUP:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_SUPERIOR));
		break;
	case COND_INF:
		Condition::AddOperand(conditions, new QualityCondition(ITEM_QUALITY_INFERIOR));
		break;
	case COND_NORM:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_NORMAL, ITEMFLAG_BASE));
		break;
	case COND_EXC:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_EXCEPTIONAL, ITEMFLAG_BASE));
		break;
	case COND_ELT:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ELITE, ITEMFLAG_BASE));
		break;
	case COND_CLASS:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_CLASS, ITEMFLAG_BASE));
		break;
	case COND_ID:
		Condition::AddOperand(conditions, new FlagsCondition(ITEM_IDENTIFIED));
		break;
	case COND_ILVL:
		Condition::AddOperand(conditions, new ItemLevelCondition(operation, value, value2));
		break;
	case COND_QLVL:
		Condition::AddOperand(conditions, new QualityLevelCondition(operation, value, value2));
		break;
	case COND_ALVL:
		Condition::AddOperand(conditions, new AffixLevelCondition(operation, value, value2));
		break;
	case COND_CLVL:
		Condition::AddOperand(conditions, new CharStatCondition(STAT_LEVEL, 0, operation, value, value2));
		break;
	case COND_FILTLVL:
		Condition::AddOperand(conditions, new FilterLevelCondition(operation, value, value2));
		break;
	case COND_DIFF:
		Condition::AddOperand(conditions, new DifficultyCondition(operation, value, value2));
		break;
	case COND_RUNE:
		Condition::AddOperand(conditions, new RuneCondition(operation, value, value2));
		break;
	case COND_GOLD:
		Condition::AddOperand(conditions, new GoldCondition(operation, value, value2));
		break;
	case COND_GEMMED:
		Condition::AddOperand(conditions, new GemmedCondition());
		break;
	case COND_GEMTYPE:
		Condition::AddOperand(conditions, new GemTypeCondition(operation, value, value2));
		break;
	case COND_GEM:
		Condition::AddOperand(conditions, new GemLevelCondition(operation, value, value2));
		break;
	case COND_ED:
		Condition::AddOperand(conditions, new EDCondition(operation, value, value2));
		break;
	case COND_EDEF:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_ENHANCEDDEFENSE, 0, operation, value, value2));
		break;
	case COND_EDAM:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_ENHANCEDMAXIMUMDAMAGE, 0, operation, value, value2));
		break;
	case COND_DEF:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_DEFENSE, 0, operation, value, value2));
		break;
	case COND_MAXDUR:
		Condition::AddOperand(conditions, new DurabilityCondition(operation, value, value2));
		break;
	case COND_RES:
		Condition::AddOperand(conditions, new ResistAllCondition(operation, value, value2));
		break;
	case COND_FRES:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_FIRERESIST, 0, operation, value, value2));
		break;
	case COND_CRES:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_COLDRESIST, 0, operation, value, value2));
		break;
	case COND_LRES:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_LIGHTNINGRESIST, 0, operation, value, value2));
		break;
	case COND_PRES:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_POISONRESIST, 0, operation, value, value2));
		break;
	case COND_IAS:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_IAS, 0, operation, value, value2));
		break;
	case COND_FCR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_FASTERCAST, 0, operation, value, value2));
		break;
	case COND_FHR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_FASTERHITRECOVERY, 0, operation, value, value2));
		break;
	case COND_FBR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_FASTERBLOCK, 0, operation, value, value2));
		break;
	case COND_LIFE:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MAXHP, 0, operation, value, value2));
		break;
	case COND_MANA:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MAXMANA, 0, operation, value, value2));
		break;
	case COND_QTY:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_AMMOQUANTITY, 0, operation, value, value2));
		break;
	case COND_GOODSK:
		Condition::AddOperand(conditions, new SkillListCondition(operation, CLASS_SKILLS, value));
		break;
	case COND_GOODTBSK:
		Condition::AddOperand(conditions, new SkillListCondition(operation, CLASS_TAB_SKILLS, value));
		break;
	case COND_FOOLS:
		Condition::AddOperand(conditions, new FoolsCondition());
		break;
	case COND_LVLREQ:
		Condition::AddOperand(conditions, new RequiredLevelCondition(operation, value, value2));
		break;
	case COND_ARPER:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_TOHITPERCENT, 0, operation, value, value2));
		break;
	case COND_MFIND:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MAGICFIND, 0, operation, value, value2));
		break;
	case COND_GFIND:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_GOLDFIND, 0, operation, value, value2));
		break;
	case COND_STR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_STRENGTH, 0, operation, value, value2));
		break;
	case COND_DEX:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_DEXTERITY, 0, operation, value, value2));
		break;
	case COND_FRW:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_FASTERRUNWALK, 0, operation, value, value2));
		break;
	case COND_MINDMG:
		Condition::AddOperand(conditions, new MinMaxDamageCondition(MinMaxDamageCondition::DamageType::MIN, operation, value, value2));
		break;
	case COND_MAXDMG:
		Condition::AddOperand(conditions, new MinMaxDamageCondition(MinMaxDamageCondition::DamageType::MAX, operation, value, value2));
		break;
	case COND_AR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_ATTACKRATING, 0, operation, value, value2));
		break;
	case COND_DTM:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_DAMAGETOMANA, 0, operation, value, value2));
		break;
	case COND_MAEK:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MANAAFTEREACHKILL, 0, operation, value, value2));
		break;
	case COND_REPLIFE:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_REPLENISHLIFE, 0, operation, value, value2));
		break;
	case COND_REPQUANT:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_REPLENISHESQUANTITY, 0, operation, value, value2));
		break;
	case COND_REPAIR:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_REPAIRSDURABILITY, 0, operation, value, value2));
		break;
	case COND_ARMOR:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ALLARMOR, ITEMFLAG_ARMOR));
		break;
	case COND_BELT:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_BELT, ITEMFLAG_ARMOR));
		break;
	case COND_CHEST:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_BODY_ARMOR, ITEMFLAG_ARMOR));
		break;
	case COND_HELM:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_HELM, ITEMFLAG_ARMOR));
		break;
	case COND_SHIELD:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_SHIELD, ITEMFLAG_ARMOR));
		break;
	case COND_GLOVES:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_GLOVES, ITEMFLAG_ARMOR));
		break;
	case COND_BOOTS:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_BOOTS, ITEMFLAG_ARMOR));
		break;
	case COND_CIRC:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_CIRCLET, ITEMFLAG_ARMOR));
		break;
	case COND_DRU:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_DRUID_PELT, ITEMFLAG_ARMOR));
		break;
	case COND_BAR:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_BARBARIAN_HELM, ITEMFLAG_ARMOR));
		break;
	case COND_DIN:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_PALADIN_SHIELD, ITEMFLAG_ARMOR));
		break;
	case COND_NEC:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_NECROMANCER_SHIELD, ITEMFLAG_ARMOR));
		break;
	case COND_SIN:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ASSASSIN_KATAR, ITEMFLAG_WEAPON));
		break;
	case COND_SOR:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_SORCERESS_ORB, ITEMFLAG_WEAPON));
		break;
	case COND_ZON:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_AMAZON_WEAPON, ITEMFLAG_WEAPON));
		break;
	case COND_MISC:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ALLMISC, ITEMFLAG_MISC));
		break;
	case COND_JEWELRY:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_JEWELRY, ITEMFLAG_MISC));
		break;
	case COND_CHARM:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_CHARM, ITEMFLAG_MISC));
		break;
	case COND_QUIVER:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_QUIVER, ITEMFLAG_MISC));
		break;
	case COND_SHOP:
		Condition::AddOperand(conditions, new ShopCondition());
		break;
	case COND_EQUIPPED:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_EQUIPPED));
		break;
	case COND_MERC:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_MERCEQUIPPED));
		break;
	case COND_INVENTORY:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_INVENTORY));
		break;
	case COND_CUBE:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_CUBE));
		break;
	case COND_STASH:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_STASH));
		break;
	case COND_GROUND:
		Condition::AddOperand(conditions, new LocationCondition(LOCATIONFLAG_GROUND));
		break;
	case COND_1H:
		Condition::AddOperand(conditions, new OneHandedCondition());
		break;
	case COND_2H:
		Condition::AddOperand(conditions, new TwoHandedCondition());
		break;
	case COND_AXE:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_AXE, ITEMFLAG_WEAPON));
		break;
	case COND_MACE:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ALLMACE, ITEMFLAG_WEAPON));
		break;
	case COND_CLUB:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_CLUB, ITEMFLAG_WEAPON));
		break;
	case COND_TMACE:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_TIPPED_MACE, ITEMFLAG_WEAPON));
		break;
	case COND_HAMMER:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_HAMMER, ITEMFLAG_WEAPON));
		break;
	case COND_SWORD:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_SWORD, ITEMFLAG_WEAPON));
		break;
	case COND_DAGGER:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_DAGGER, ITEMFLAG_WEAPON));
		break;
	case COND_THROWING:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_THROWING, ITEMFLAG_WEAPON));
		break;
	case COND_JAV:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_JAVELIN, ITEMFLAG_WEAPON));
		break;
	case COND_SPEAR:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_SPEAR, ITEMFLAG_WEAPON));
		break;
	case COND_POLEARM:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_POLEARM, ITEMFLAG_WEAPON));
		break;
	case COND_BOW:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_BOW, ITEMFLAG_WEAPON));
		break;
	case COND_XBOW:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_CROSSBOW, ITEMFLAG_WEAPON));
		break;
	case COND_STAFF:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_STAFF, ITEMFLAG_WEAPON));
		break;
	case COND_WAND:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_WAND, ITEMFLAG_WEAPON));
		break;
	case COND_SCEPTER:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_SCEPTER, ITEMFLAG_WEAPON));
		break;
	case COND_WEAPON:
		Condition::AddOperand(conditions, new ItemGroupCondition(ITEM_GROUP_ALLWEAPON, ITEMFLAG_WEAPON));
		break;
	case COND_SK:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >(int)SKILL_MAX) { break; }
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_SINGLESKILL, cond_num, operation, value, value2));
		break;
	case COND_OS:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >(int)SKILL_MAX) { break; }
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_NONCLASSSKILL, cond_num, operation, value, value2));
		break;
	case COND_CHSK:
		// skills granted by charges
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >(int)SKILL_MAX) { break; }
		Condition::AddOperand(conditions, new ChargedCondition(operation, cond_num, value, value2));
		break;
	case COND_CLSK:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >= CLASS_NA) { break; }
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_CLASSSKILLS, cond_num, operation, value, value2));
		break;
	case COND_ALLSK:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_ALLSKILLS, 0, operation, value, value2));
		break;
	case COND_TABSK:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num > SKILLTAB_MAX) { break; }
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_SKILLTAB, cond_num, operation, value, value2));
		break;
	case COND_STAT:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >(int)STAT_MAX) { break; }
		Condition::AddOperand(conditions, new ItemStatCondition(cond_num, 0, operation, value, value2));
		break;
	case COND_CHARSTAT:
		if ((number_ss >> cond_num).fail() || cond_num < 0 || cond_num >(int)STAT_MAX) { break; }
		Condition::AddOperand(conditions, new CharStatCondition(cond_num, 0, operation, value, value2));
		break;
	case COND_MULTI:
		if (std::regex_search(key, multi_match, multi_reg))
		{
			int stat1, stat2;
			stat1 = stoi(multi_match[1].str(), nullptr, 10);
			stat2 = stoi(multi_match[2].str(), nullptr, 10);

			Condition::AddOperand(conditions, new ItemStatCondition(stat1, stat2, operation, value, value2));
		}
		break;
	case COND_BUYPRICE:
		Condition::AddOperand(conditions, new ItemPriceCondition(operation, value, value2, TRANSACTIONTYPE_BUY));
		break;
	case COND_PRICE:
		Condition::AddOperand(conditions, new ItemPriceCondition(operation, value, value2, TRANSACTIONTYPE_SELL));
		break;
	case COND_WIDTH:
		Condition::AddOperand(conditions, new ItemSizeCondition(operation, value, value2, ItemSizeCondition::Dimension::kWidth));
		break;
	case COND_HEIGHT:
		Condition::AddOperand(conditions, new ItemSizeCondition(operation, value, value2, ItemSizeCondition::Dimension::kHeight));
		break;
	case COND_AREA:
		Condition::AddOperand(conditions, new ItemSizeCondition(operation, value, value2, ItemSizeCondition::Dimension::kArea));
		break;
	case COND_BELT_1:
		Condition::AddOperand(conditions, new BeltCondition(operation, value, value2, 0));
		break;
	case COND_BELT_2:
		Condition::AddOperand(conditions, new BeltCondition(operation, value, value2, 1));
		break;
	case COND_BELT_3:
		Condition::AddOperand(conditions, new BeltCondition(operation, value, value2, 2));
		break;
	case COND_BELT_4:
		Condition::AddOperand(conditions, new BeltCondition(operation, value, value2, 3));
		break;
	case COND_ITEMCODE:
		Condition::AddOperand(conditions, new ItemCodeCondition(WideToAnsi(key.substr(0, 4)).c_str()));
		break;
	case COND_ADD:
		Condition::AddOperand(conditions, new AddCondition(key, operation, value));
		break;
	case COND_REQSTAT:
	{
		auto type = ReqStatCondition::ReqStatType::STRENGTH;
		if (key == L"REQDEX") {
			type = ReqStatCondition::ReqStatType::DEXTERITY;
		}
		else if (key == L"REQLVL") {
			type = ReqStatCondition::ReqStatType::LEVEL;
		}
		Condition::AddOperand(conditions, new ReqStatCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMIN1H:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MIN1H;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMIN2H:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MIN2H;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMINSMITE:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MINSMITE;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMINKICK:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MINKICK;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMINTHROW:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MINTHROW;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMAX1H:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MAX1H;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMAX2H:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MAX2H;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMAXSMITE:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MAXSMITE;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMAXKICK:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MAXKICK;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEDAMAGEMAXTHROW:
	{
		auto type = BaseWeaponDamageCondition::DamageType::MAXTHROW;
		Condition::AddOperand(conditions, new BaseWeaponDamageCondition(type, operation, value, value2));
		break;
	}
	case COND_BASEBLOCK:
		Condition::AddOperand(conditions, new BaseBlockCondition(operation, value, value2));
		break;
	case COND_ALLATTRIB:
		Condition::AddOperand(conditions, new AllAttributesCondition(operation, value, value2));
		break;
	case COND_MAXRES:
		Condition::AddOperand(conditions, new MaxResCondition(operation, value, value2));
		break;
	case COND_UPSTAT:
	{
		auto type = UpStatCondition::UpStatType::STRENGTH;
		if (key == L"UPDEX") {
			type = UpStatCondition::UpStatType::DEXTERITY;
		}
		else if (key == L"UPLVL") {
			type = UpStatCondition::UpStatType::LEVEL;
		}
		Condition::AddOperand(conditions, new UpStatCondition(type, operation, value, value2));
		break;
	}
	case COND_MAXSOCKETS:
		Condition::AddOperand(conditions, new MaxSocketsCondition(operation, value, value2));
		break;

	case COND_NULL:
		break;
	case COND_FORMULA:
		Condition::AddOperand(conditions, new FormulaCondition(key, operation, value, value2));
		break;
	default:
		break;
	}

	for (vector<Condition*>::iterator it = endConditions.begin(); it != endConditions.end(); it++) { Condition::AddNonOperand(conditions, (*it)); }
}

// Insert extra AND operators to stay backwards compatible with old config
// that implicitly ANDed all conditions
void Condition::AddOperand(vector<Condition*>& conditions,
	Condition* cond)
{
	if (LastConditionType == CT_Operand || LastConditionType == CT_RightParen) { conditions.push_back(new AndOperator()); }
	conditions.push_back(cond);
	LastConditionType = CT_Operand;
}

void Condition::AddNonOperand(vector<Condition*>& conditions,
	Condition* cond)
{
	if ((cond->conditionType == CT_NegationOperator || cond->conditionType == CT_LeftParen) &&
		(LastConditionType == CT_Operand || LastConditionType == CT_RightParen)) {
		conditions.push_back(new AndOperator());
	}
	conditions.push_back(cond);
	LastConditionType = cond->conditionType;
}

bool Condition::Evaluate(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return EvaluateInternal(uInfo, arg1, arg2);
}

bool FilterLevelCondition::EvaluateInternal(UnitItemInfo* uInfo, Condition* arg1, Condition* arg2) {
	return IntegerCompare(Item::GetFilterLevel(), operation, filterLevel, filterLevel2);
}

bool TrueCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return true;
}

bool FalseCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return false;
}

bool NegationOperator::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return !arg1->Evaluate(uInfo, arg1, arg2);
}

bool LeftParen::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return false;
}

bool RightParen::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return false;
}

bool AndOperator::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return arg1->Evaluate(uInfo, NULL, NULL) && arg2->Evaluate(uInfo, NULL, NULL);
}

bool OrOperator::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return arg1->Evaluate(uInfo, NULL, NULL) || arg2->Evaluate(uInfo, NULL, NULL);
}

bool ItemCodeCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return (targetCode[0] == uInfo->itemCode[0] && targetCode[1] == uInfo->itemCode[1] && targetCode[2] == uInfo->itemCode[2] && targetCode[3] == uInfo->itemCode[3]);
}

bool FlagsCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return ((uInfo->item->pItemData->dwFlags & flag) > 0);
}

bool QualityCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return (uInfo->item->pItemData->dwQuality == quality);
}

bool NonMagicalCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return (uInfo->item->pItemData->dwQuality == ITEM_QUALITY_INFERIOR ||
		uInfo->item->pItemData->dwQuality == ITEM_QUALITY_NORMAL ||
		uInfo->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR);
}

bool GemLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (IsGem(uInfo->attrs)) { return IntegerCompare(GetGemLevel(uInfo->attrs), operation, gemLevel, gemLevel2); }
	return false;
}

bool GemTypeCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (IsGem(uInfo->attrs)) { return IntegerCompare(GetGemType(uInfo->attrs), operation, gemType, gemType2); }
	return false;
}

bool RuneCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (IsRune(uInfo->attrs)) { return IntegerCompare(RuneNumberFromItemCode(uInfo->itemCode), operation, runeNumber, runeNumber2); }
	return false;
}

bool GoldCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (uInfo->itemCode[0] == 'g' && uInfo->itemCode[1] == 'l' && uInfo->itemCode[2] == 'd')
	{
		return IntegerCompare(D2COMMON_GetUnitStat(uInfo->item, STAT_GOLD, 0), operation, goldAmount, goldAmount2);
	}
	return false;
}

bool ItemLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return IntegerCompare(uInfo->item->pItemData->dwItemLevel, operation, itemLevel, itemLevel2);
}

bool QualityLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	BYTE qlvl = uInfo->attrs->qualityLevel;
	return IntegerCompare(qlvl, operation, qualityLevel, qualityLevel2);
}

bool AffixLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	BYTE qlvl = uInfo->attrs->qualityLevel;
	BYTE alvl = GetAffixLevel((BYTE)uInfo->item->pItemData->dwItemLevel, (BYTE)uInfo->attrs->qualityLevel, uInfo->attrs->magicLevel);
	return IntegerCompare(alvl, operation, affixLevel, affixLevel2);
}

bool MapIdCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	UnitAny* player = D2CLIENT_GetPlayerUnit();
	if (player)
	{
		int map_id = D2COMMON_GetLevelIdFromRoom(D2COMMON_GetRoomFromUnit(player));
		if (map_id > 0)
		{
			return IntegerCompare(map_id, operation, mapId, mapId2);
		}
	}
	return false;
}

bool MapTierCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int value = -1;
	if (maptiers.find(uInfo->attrs->category) != maptiers.end())
	{
		value = maptiers.at(uInfo->attrs->category);
	}
	return IntegerCompare(value, operation, mapTier, mapTier2);
}

bool CraftLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	auto ilvl_int = uInfo->item->pItemData->dwItemLevel;
	auto clvl_int = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);
	auto qlvl_int = uInfo->attrs->qualityLevel;
	auto mlvl_int = uInfo->attrs->magicLevel;

	auto craft_alvl = GetAffixLevel(
		(int)(0.5 * clvl_int) + (int)(0.5 * ilvl_int),
		qlvl_int,
		mlvl_int
	);

	return IntegerCompare(craft_alvl, operation, craftLevel, craftLevel2);
}

bool RerollLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int rerollAlvl = ComputeRerollAffixLevel(uInfo);
	return IntegerCompare(rerollAlvl, operation_, rerollLevel1_, rerollLevel2_);
}

BYTE ComputeRerollAffixLevel(UnitItemInfo* uInfo) {
	// Maps cannot be rerolled using the standard recipe.
	if (uInfo->attrs->miscFlags & ITEM_GROUP_MAP) {
		return 0;
	}

	// Corrupted items cannot be rerolled.
	if (D2COMMON_GetUnitStat(uInfo->item, STAT_CORRUPTED, 0) > 0) {
		return 0;
	}

	BYTE ilvl = uInfo->item->pItemData->dwItemLevel;
	BYTE reroll_ilvl;

	switch (uInfo->item->pItemData->dwQuality) {
		case ITEM_QUALITY_RARE:	{
			BYTE clvl = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);
			reroll_ilvl = (int)(0.4 * ilvl) + (int)(0.4 * clvl);
			break;
		}
		case ITEM_QUALITY_MAGIC:
			reroll_ilvl = ilvl;
			break;
		default:
			return 0;
	}

	BYTE qlvl = uInfo->attrs->qualityLevel;
	BYTE mlvl = uInfo->attrs->magicLevel;
	return GetAffixLevel(reroll_ilvl, qlvl, mlvl);
}

bool MagicPrefixCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	auto itemData = uInfo->item->pItemData;

	if (itemData->dwQuality == ITEM_QUALITY_RARE && !(itemData->dwFlags & ITEM_IDENTIFIED))
	{
		return false;
	}
	if (operation == GREATER_THAN || operation == LESS_THAN)
	{
		return false;
	}

	if ((itemData->wPrefix[0] - PREFIX_OFFSET > 0) ? IntegerCompare(itemData->wPrefix[0] - PREFIX_OFFSET, operation, prefixID1, prefixID2) : false)
	{
		return true;
	}
	if ((itemData->wPrefix[1] - PREFIX_OFFSET > 0) ? IntegerCompare(itemData->wPrefix[1] - PREFIX_OFFSET, operation, prefixID1, prefixID2) : false)
	{
		return true;
	}
	if ((itemData->wPrefix[2] - PREFIX_OFFSET > 0) ? IntegerCompare(itemData->wPrefix[2] - PREFIX_OFFSET, operation, prefixID1, prefixID2) : false)
	{
		return true;
	}

	return false;
}

bool MagicSuffixCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	auto itemData = uInfo->item->pItemData;

	if (itemData->dwQuality == ITEM_QUALITY_RARE && !(itemData->dwFlags & ITEM_IDENTIFIED))
	{
		return false;
	}
	if (operation == GREATER_THAN || operation == LESS_THAN)
	{
		return false;
	}

	if ((itemData->wSuffix[0] > 0) ? IntegerCompare(itemData->wSuffix[0], operation, suffixID1, suffixID2) : false)
	{
		return true;
	}
	if ((itemData->wSuffix[1] > 0) ? IntegerCompare(itemData->wSuffix[1], operation, suffixID1, suffixID2) : false)
	{
		return true;
	}
	if ((itemData->wSuffix[2] > 0) ? IntegerCompare(itemData->wSuffix[2], operation, suffixID1, suffixID2) : false)
	{
		return true;
	}

	return false;
}

bool AutomodCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	auto itemData = uInfo->item->pItemData;

	if ((itemData->dwQuality == ITEM_QUALITY_MAGIC || itemData->dwQuality == ITEM_QUALITY_RARE) && !(itemData->dwFlags & ITEM_IDENTIFIED))
	{
		return false;
	}

	return IntegerCompare(itemData->wAutoPrefix - AUTOMOD_OFFSET, operation, automodID, automodID2);
}

bool CharacterClassCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(D2CLIENT_GetPlayerUnit()->dwTxtFileNo, operation, characterClass);
}

bool RequiredLevelCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	unsigned int rlvl = GetRequiredLevel(uInfo->item);

	return IntegerCompare(rlvl, operation, requiredLevel, requiredLevel2);
}

bool ItemGroupCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	switch (flagType)
	{
	case ITEMFLAG_BASE:
		return ((uInfo->attrs->baseFlags & itemGroup) > 0);
	case ITEMFLAG_WEAPON:
		return ((uInfo->attrs->weaponFlags & itemGroup) > 0);
	case ITEMFLAG_ARMOR:
		return ((uInfo->attrs->armorFlags & itemGroup) > 0);
	case ITEMFLAG_MISC:
		return ((uInfo->attrs->miscFlags & itemGroup) > 0);
	default:
		return false;
	}
}

bool EDCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	// Either enhanced defense or enhanced damage depending on item type
	WORD stat;
	if (uInfo->attrs->armorFlags & ITEM_GROUP_ALLARMOR) { stat = STAT_ENHANCEDDEFENSE; }
	else
	{
		// Normal %ED will have the same value for STAT_ENHANCEDMAXIMUMDAMAGE and STAT_ENHANCEDMINIMUMDAMAGE
		stat = STAT_ENHANCEDMAXIMUMDAMAGE;
	}
	DWORD     value = 0;
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	if (pStatList)
	{
		value += D2COMMON_GetStatValueFromStatList(pStatList, stat, 0);
	}

	return IntegerCompare(value, operation, targetED, targetED2);
}

bool DurabilityCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	// Pulled from JSUnit.cpp in d2bs
	DWORD     value = 0;
	Stat      aStatList[256] = { NULL };
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	if (pStatList)
	{
		DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
		for (UINT i = 0; i < dwStats; i++) { if (aStatList[i].wStatIndex == STAT_ENHANCEDMAXDURABILITY && aStatList[i].wSubIndex == 0) { value += aStatList[i].dwStatValue; } }
	}
	return IntegerCompare(value, operation, targetDurability, targetDurability2);
}

bool ChargedCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	DWORD     value = 0;
	Stat      aStatList[256] = { NULL };
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	if (pStatList)
	{
		DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
		for (UINT i = 0; i < dwStats; i++)
		{
			//if (aStatList[i].wStatIndex == STAT_CHARGED)
			//	PrintText(1, "ChargedCondition::EvaluateInternal: Index=%hx, SubIndex=%hx, Value=%x", aStatList[i].wStatIndex, aStatList[i].wSubIndex, aStatList[i].dwStatValue);
			if (aStatList[i].wStatIndex == STAT_CHARGED && (aStatList[i].wSubIndex >> 6) == skill)
			{
				// 10 MSBs of subindex is the skill ID
				unsigned int level = aStatList[i].wSubIndex & 0x3F;     // 6 LSBs are the skill level
				value = (level > value) ? level : value; // use highest level
			}
		}
	}
	return IntegerCompare(value, operation, targetLevel, targetLevel2);
}

bool FoolsCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	// 1 = MAX DMG / level
	// 2 = AR / level
	// 3 = Fools

	// Pulled from JSUnit.cpp in d2bs
	unsigned int value = 0;
	Stat         aStatList[256] = { NULL };
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	if (pStatList)
	{
		DWORD dwStats = D2COMMON_CopyStatList(pStatList, (Stat*)aStatList, 256);
		for (UINT i = 0; i < dwStats; i++)
		{
			if (aStatList[i].wStatIndex == STAT_MAXDAMAGEPERLEVEL && aStatList[i].wSubIndex == 0) { value += 1; }
			if (aStatList[i].wStatIndex == STAT_ATTACKRATINGPERLEVEL && aStatList[i].wSubIndex == 0) { value += 2; }
		}
	}
	// We are returning a comparison on 3 here instead of any the actual number because the way it is setup is
	// to just write FOOLS in the mh file instead of FOOLS=3 this could be changed to accept 1-3 for the different
	// types it can produce
	return IntegerCompare(value, (BYTE)EQUAL, 3);
}

bool LocationCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	bool has_location = false;
	UnitAny* pMerc = NULL;
	if (uInfo->item && uInfo->item->pItemData)
	{
		ItemData* pItemData = uInfo->item->pItemData;
		switch (location)
		{
		case LOCATIONFLAG_EQUIPPED:
			if (pItemData->ItemLocation == STORAGE_NULL &&
				pItemData->BodyLocation > 0 &&
				pItemData->pOwnerInventory && pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit())
			{
				has_location = true;
			}
			break;
		case LOCATIONFLAG_MERCEQUIPPED:
			pMerc = GetClientMercUnit();
			if (pMerc &&
				pItemData->ItemLocation == STORAGE_NULL &&
				pItemData->BodyLocation > 0 &&
				pItemData->pOwnerInventory && pItemData->pOwnerInventory->pOwner == pMerc)
			{
				has_location = true;
			}
			break;
		case LOCATIONFLAG_INVENTORY:
			if (pItemData->ItemLocation == STORAGE_INVENTORY &&
				uInfo->item->dwMode == ITEM_MODE_INV_STASH_CUBE_STORE &&
				pItemData->pOwnerInventory && pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit())
			{
				has_location = true;
			}
			break;
		case LOCATIONFLAG_CUBE:
			if (pItemData->ItemLocation == STORAGE_CUBE &&
				pItemData->pOwnerInventory && pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit())
			{
				has_location = true;
			}
			break;
		case LOCATIONFLAG_STASH:
			if (pItemData->ItemLocation == STORAGE_STASH &&
				pItemData->pOwnerInventory && pItemData->pOwnerInventory->pOwner == D2CLIENT_GetPlayerUnit())
			{
				has_location = true;
			}
			break;
		case LOCATIONFLAG_GROUND:
			if (uInfo->item->dwMode == ITEM_MODE_ON_GROUND || uInfo->item->dwMode == ITEM_MODE_BEING_DROPPED)
			{
				has_location = true;
			}
			break;
		}
	}

	return IntegerCompare(has_location, (BYTE)EQUAL, 1);
}

bool ShopCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	bool is_shop = false;
	if (uInfo->item->pItemData &&
		uInfo->item->pItemData->pOwnerInventory &&
		uInfo->item->pItemData->pOwnerInventory->pOwner)
	{
		auto wNpcId = uInfo->item->pItemData->pOwnerInventory->pOwner->dwTxtFileNo;
		if (find(begin(ShopNPCs), end(ShopNPCs), wNpcId) != end(ShopNPCs))
		{
			is_shop = true;
		}
	}

	return IntegerCompare(is_shop, (BYTE)EQUAL, 1);
}

bool OneHandedCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (code_to_dwtxtfileno.find(uInfo->itemCode) != code_to_dwtxtfileno.end())
	{
		int weapon_number = code_to_dwtxtfileno[uInfo->itemCode];
		WeaponType weapon_type = Drawing::StatsDisplay::GetCurrentWeaponType(weapon_number);
		bool is_onehanded = false;

		if (weapon_type == WeaponType::kAxe ||
			weapon_type == WeaponType::kWand ||
			weapon_type == WeaponType::kClub ||
			weapon_type == WeaponType::kScepter ||
			weapon_type == WeaponType::kMace ||
			weapon_type == WeaponType::kHammer ||
			weapon_type == WeaponType::kSword ||
			weapon_type == WeaponType::kKnife ||
			weapon_type == WeaponType::kThrowing ||
			weapon_type == WeaponType::kJavelin ||
			weapon_type == WeaponType::kThrowingPot ||
			weapon_type == WeaponType::kClaw1 ||
			weapon_type == WeaponType::kClaw2 ||
			weapon_type == WeaponType::kOrb ||
			weapon_type == WeaponType::kAmaJav
			)
		{
			is_onehanded = true;
		}

		return IntegerCompare(is_onehanded, (BYTE)EQUAL, 1);
	}
	else
	{
		return false;
	}
}

bool TwoHandedCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	if (code_to_dwtxtfileno.find(uInfo->itemCode) != code_to_dwtxtfileno.end())
	{
		int weapon_number = code_to_dwtxtfileno[uInfo->itemCode];
		WeaponType weapon_type = Drawing::StatsDisplay::GetCurrentWeaponType(weapon_number);
		bool is_twohanded = false;

		if (weapon_type == WeaponType::kAxe2H ||
			weapon_type == WeaponType::kHammer2H ||
			weapon_type == WeaponType::kSword2H ||
			weapon_type == WeaponType::kSpear ||
			weapon_type == WeaponType::kPole ||
			weapon_type == WeaponType::kStaff ||
			weapon_type == WeaponType::kBow ||
			weapon_type == WeaponType::kCrossbow ||
			weapon_type == WeaponType::kAmaBow ||
			weapon_type == WeaponType::kAmaSpear
			)
		{
			is_twohanded = true;
		}

		return IntegerCompare(is_twohanded, (BYTE)EQUAL, true);
	}
	else
	{
		return false;
	}
}

bool GemmedCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	bool is_gemmed = false;
	if (uInfo->item->pInventory)
	{
		is_gemmed = uInfo->item->pInventory->dwItemCount > 0;
	}

	return IntegerCompare(is_gemmed, (BYTE)EQUAL, true);
}

void SkillListCondition::Init()
{
	// Clear lists
	goodClassSkills.clear();
	goodTabSkills.clear();

	// Build character skills list
	for (auto it = App.lootfilter.classSkillsList.values.cbegin(); it != App.lootfilter.classSkillsList.values.cend(); it++) { if (StringToBool((*it).second)) { goodClassSkills.push_back(stoi((*it).first)); } }

	// Build tab skills list
	for (auto it = App.lootfilter.tabSkillsList.values.cbegin(); it != App.lootfilter.tabSkillsList.values.cend(); it++) { if (StringToBool((*it).second)) { goodTabSkills.push_back(stoi((*it).first)); } }
}

bool SkillListCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int value = 0;
	if (type == CLASS_SKILLS)
	{
		for (unsigned int i = 0; i < goodClassSkills.size(); i++) { value += D2COMMON_GetUnitStat(uInfo->item, STAT_CLASSSKILLS, goodClassSkills.at(i)); }
	}
	else if (type == CLASS_TAB_SKILLS)
	{
		for (unsigned int i = 0; i < goodTabSkills.size(); i++) { value += D2COMMON_GetUnitStat(uInfo->item, STAT_SKILLTAB, goodTabSkills.at(i)); }
	}

	return IntegerCompare(value, operation, targetStat);
}

bool CharStatCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return IntegerCompare(D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), stat1, stat2), operation, targetStat, targetStat2);
}

bool DifficultyCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	return IntegerCompare(D2CLIENT_GetDifficulty(), operation, targetDiff, targetDiff2);
}

bool ItemStatCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2) {
	int newTarget = targetStat;
	int newTarget2 = targetStat2;
	if (itemStat == STAT_MAXHP || itemStat == STAT_MAXMANA)
	{
		newTarget *= 256;
		newTarget2 *= 256;
	}
	// These stat values need to be grabbed differently otherwise they just:
	else if (
		itemStat == STAT_ENHANCEDDEFENSE ||				// return 0
		itemStat == STAT_ENHANCEDMAXIMUMDAMAGE ||		// return 0
		itemStat == STAT_ENHANCEDMINIMUMDAMAGE ||		// return 0
		itemStat == STAT_MINIMUMDAMAGE ||				// return base min 1h weapon damage
		itemStat == STAT_MAXIMUMDAMAGE ||				// return base max 1h weapon damage
		itemStat == STAT_SECONDARYMINIMUMDAMAGE ||		// return base min 2h weapon damage
		itemStat == STAT_SECONDARYMAXIMUMDAMAGE	||		// return base max 2h weapon damage
		itemStat == STAT_MINIMUMTHROWINGDAMAGE ||		// return min throw weapon damage
		itemStat == STAT_MAXIMUMTHROWINGDAMAGE			// return max throw weapon damage
		)
	{
		return IntegerCompare(GetStatFromList(uInfo, itemStat), operation, targetStat, targetStat2);
	}
	return IntegerCompare(D2COMMON_GetUnitStat(uInfo->item, itemStat, itemStat2), operation, newTarget, newTarget2);

}

bool ItemPriceCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int nPrice = GetShopPrice(D2CLIENT_GetPlayerUnit(), uInfo->item, nTransactionType);
	return IntegerCompare(nPrice, operation, targetStat, targetStat2);
}

bool ItemSizeCondition::EvaluateInternal(UnitItemInfo* uInfo, Condition* arg1, Condition* arg2)
{
	int value;
	auto attrs = uInfo->attrs;

	switch (dimension_) {
		case ItemSizeCondition::Dimension::kHeight:
			value = attrs->height;
			break;
		case ItemSizeCondition::Dimension::kWidth:
			value = attrs->width;
			break;
		case ItemSizeCondition::Dimension::kArea:
			value = attrs->height * attrs->width;
			break;
		default:
			return false;
	}

	return IntegerCompare(value, op_, targetStat_, targetStat2_);
}

bool BeltCondition::EvaluateInternal(UnitItemInfo* uInfo, Condition* arg1, Condition* arg2)
{
	const int kBeltWidth = 4;
	const int kBeltHeight = 4;

	Inventory* inventory = D2CLIENT_GetPlayerUnit()->pInventory;
	if (inventory == nullptr) {
		return false;
	}
	if (column_ >= kBeltWidth) {
		return false;
	}
	if (inventory->dwStoresCount <= InventoryStoreType::INVENTORY_STORE_TYPE_BELT) {
		return false;
	}
	InventoryStore& store = inventory->pStores[InventoryStoreType::INVENTORY_STORE_TYPE_BELT];
	// Double check that this is indeed the correct `InventoryStore`.
	// Internally belt is represented as a flat, 16 elements, array.
	if (store.Width != kBeltWidth * kBeltHeight || store.Height != 1) {
		return false;
	}

	int itemsInColumnCount = 0;
	for (int row = 0; row < kBeltHeight; ++row) {
		auto slot = row * kBeltWidth + column_;
		if (store.pArray[slot] != nullptr) {
			itemsInColumnCount += 1;
		}
	}

	return IntegerCompare(itemsInColumnCount, op_, targetStat_, targetStat2_);
}

bool ResistAllCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int fRes = D2COMMON_GetUnitStat(uInfo->item, STAT_FIRERESIST, 0);
	int lRes = D2COMMON_GetUnitStat(uInfo->item, STAT_LIGHTNINGRESIST, 0);
	int cRes = D2COMMON_GetUnitStat(uInfo->item, STAT_COLDRESIST, 0);
	int pRes = D2COMMON_GetUnitStat(uInfo->item, STAT_POISONRESIST, 0);
	return (IntegerCompare(fRes, operation, targetStat, targetStat2) &&
		IntegerCompare(lRes, operation, targetStat, targetStat2) &&
		IntegerCompare(cRes, operation, targetStat, targetStat2) &&
		IntegerCompare(pRes, operation, targetStat, targetStat2));
}

void AddCondition::Init()
{
	static wregex statRegex(L"([A-Z_]+)(?:(\\d{1,9})(?:,(\\d{1,9}))?)?", wregex::ECMAScript);
	codes.clear();
	codes = wsplit(key, L'+');
	for (auto code : codes)
	{
		wsmatch match;
		if (regex_search(code, match, statRegex)) {
			if (skills.find(match[1]) == skills.end()) {
				if (formulaMap.find(match[1]) == formulaMap.end()) {
					continue;
				}
				fs.emplace_back(formulaMap.find(match[1])->second);
				continue;
			}
			auto found = skills.find(match[1]);
			DWORD id = found->second.id;
			DWORD params = found->second.params;
			int paramCount = (match[2].length() != 0) + (match[3].length() != 0);
			if (params != paramCount) {
				continue;
			}
			int param1 = match[2].length() > 0 ? stoi(match[2].str()) : id;
			int param2 = match[3].length() > 0 ? stoi(match[3].str()) : 0;
			if (match[1] == L"MAXDMG" || match[1] == L"MINDMG") {
				stats.emplace_back(param1, INT_MAX);
				continue;
			}
			stats.emplace_back(param1, param2);
		}
	}
}

bool AddCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	int value = 0;
	for (const auto& tuple : stats)
	{
		DWORD stat = get<0>(tuple);
		DWORD layer = get<1>(tuple);
		int tmpVal = D2COMMON_GetUnitStat(uInfo->item, stat, layer);
		if (stat == STAT_MAXHP || stat == STAT_MAXMANA)
		{
			tmpVal /= 256;
		}
		// layer used to flag MIN/MAXDMG named stat
		else if (layer == INT_MAX && (stat == STAT_MINIMUMDAMAGE || stat == STAT_MAXIMUMDAMAGE))
		{
			tmpVal = MinMaxDamageCondition::GetValue((MinMaxDamageCondition::DamageType)(stat - STAT_MINIMUMDAMAGE), uInfo);
		}
		else if (
			stat == STAT_ENHANCEDDEFENSE ||				// return 0
			stat == STAT_ENHANCEDMAXIMUMDAMAGE ||		// return 0
			stat == STAT_ENHANCEDMINIMUMDAMAGE ||		// return 0
			stat == STAT_MINIMUMDAMAGE ||				// return base min 1h weapon damage
			stat == STAT_MAXIMUMDAMAGE ||				// return base max 1h weapon damage
			stat == STAT_SECONDARYMINIMUMDAMAGE ||		// return base min 2h weapon damage
			stat == STAT_SECONDARYMAXIMUMDAMAGE	||		// return base max 2h weapon damage
			stat == STAT_MINIMUMTHROWINGDAMAGE ||		// return min throw weapon damage
			stat == STAT_MAXIMUMTHROWINGDAMAGE		    // return max throw weapon damage
			)
		{
			tmpVal = GetStatFromList(uInfo, stat);
		}
		value += tmpVal;
	}
	float fvalue = 0.0f;
	for (const auto& f : fs)
	{
		float out;
		if (f->execute(uInfo, out) == FormulaStatus::OK)
		{
			fvalue += out;
		}
	}

	if (fs.size() > 0) {
		return FloatCompare(value + fvalue, operation, targetStat);
	}
	return IntegerCompare(value, operation, targetStat);
}

int ReqStatCondition::GetValue(ReqStatType type, UnitItemInfo* info)
{
	auto txt = D2COMMON_GetItemText(info->item->dwTxtFileNo);
	if (!txt) {
		return 0;
	}
	if (type == ReqStatType::LEVEL) {
		return D2COMMON_GetItemLevelRequirement(info->item, D2CLIENT_GetPlayerUnit());
	}
	int req = type == ReqStatType::STRENGTH ? txt->wreqstr : txt->wreqdex;
	int ease = D2COMMON_GetUnitStat(info->item, STAT_REDUCEDREQUIREMENTS, 0);

	int delta = req * ease / 100;
	if (info->item->pItemData->dwFlags & ITEM_ETHEREAL) {
		delta -= 10;
	}

	return (std::max)(0, req + delta);
}

bool ReqStatCondition::EvaluateInternal(UnitItemInfo* info, Condition* arg1, Condition* arg2)
{
	return IntegerCompare(GetValue(type, info), operation, targetStat, targetStat2);
}

int BaseWeaponDamageCondition::GetValue(DamageType type, UnitItemInfo* uInfo)
{
	auto txt = D2COMMON_GetItemText(uInfo->item->dwTxtFileNo);
	const auto etherealDamage = [](int damage) {
		return (damage + damage / 2) * 5 / 6;
	};
	switch (type) {
		case DamageType::MINSMITE:
		{
			if (!(uInfo->attrs->armorFlags & ITEM_GROUP_SHIELD)) {
				return 0;
			}
			return txt->bmindam;
		}
		case DamageType::MINKICK:
		{
			if (!(uInfo->attrs->armorFlags & ITEM_GROUP_BOOTS)) {
				return 0;
			}
			return txt->bmindam;
		}
		case DamageType::MIN1H:
		{
			if (uInfo->attrs->armorFlags & (ITEM_GROUP_SHIELD | ITEM_GROUP_BOOTS)) {
				return 0;
			}
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->bmindam);
			}
			return txt->bmindam;
		}
		case DamageType::MAXSMITE:
		{
			if (!(uInfo->attrs->armorFlags & ITEM_GROUP_SHIELD)) {
				return 0;
			}
			return txt->bmaxdam;
		}
		case DamageType::MAXKICK:
		{
			if (!(uInfo->attrs->armorFlags & ITEM_GROUP_BOOTS)) {
				return 0;
			}
			return txt->bmaxdam;
		}
		case DamageType::MAX1H:
		{
			if (uInfo->attrs->armorFlags & (ITEM_GROUP_SHIELD | ITEM_GROUP_BOOTS)) {
				return 0;
			}
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->bmaxdam);
			}
			return txt->bmaxdam;
		}
		case DamageType::MIN2H:
		{
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->b2handmindam);
			}
			return txt->b2handmindam;
		}
		case DamageType::MAX2H:
		{
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->b2handmaxdam);
			}
			return txt->b2handmaxdam;
		}
		case DamageType::MINTHROW:
		{
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->bminmisdam);
			}
			return txt->bminmisdam;
		}
		case DamageType::MAXTHROW:
		{
			if (uInfo->item->pItemData->dwFlags & ITEM_ETHEREAL) {
				return etherealDamage(txt->bmaxmisdam);
			}
			return txt->bmaxmisdam;
		}
	}
	return 0;
}

bool BaseWeaponDamageCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(GetValue(type, uInfo), operation, targetStat, targetStat2);
}

int BaseBlockCondition::GetValue(UnitItemInfo* uInfo)
{
	auto txt = D2COMMON_GetItemText(uInfo->item->dwTxtFileNo);
	return txt->bblock;
}

bool BaseBlockCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(GetValue(uInfo), operation, targetStat, targetStat2);
}

int AllAttributesCondition::GetValue(UnitItemInfo* uInfo)
{
	int str = D2COMMON_GetUnitStat(uInfo->item, STAT_STRENGTH, 0);
	int dex = D2COMMON_GetUnitStat(uInfo->item, STAT_DEXTERITY, 0);
	int ene = D2COMMON_GetUnitStat(uInfo->item, STAT_ENERGY, 0);
	int vit = D2COMMON_GetUnitStat(uInfo->item, STAT_VITALITY, 0);
	if (str && dex && ene && vit) {
		return min(min(str, dex), min(vit, ene));
	}
	return 0;
}

bool AllAttributesCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(GetValue(uInfo), operation, targetStat, targetStat2);
}

int MaxResCondition::GetValue(UnitItemInfo* uInfo)
{
	int maxFire = D2COMMON_GetUnitStat(uInfo->item, STAT_MAXFIRERESIST, 0);
	int maxCold = D2COMMON_GetUnitStat(uInfo->item, STAT_MAXCOLDRESIST, 0);
	int maxLight = D2COMMON_GetUnitStat(uInfo->item, STAT_MAXLIGHTNINGRESIST, 0);
	int maxPois = D2COMMON_GetUnitStat(uInfo->item, STAT_MAXPOISONRESIST, 0);
	if (maxFire && maxCold && maxLight && maxPois) {
		return min(min(maxFire, maxCold), min(maxLight, maxPois));
	}
	return 0;
}

bool MaxResCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(GetValue(uInfo), operation, targetStat, targetStat2);
}

ItemsTxt* UpStatCondition::GetUpTxt(UnitItemInfo* info)
{
	auto txt = D2COMMON_GetItemText(info->item->dwTxtFileNo);
	if (!txt) {
		return nullptr;
	}
	int id = 0;
	if (txt->dwnormcode == txt->dwcode) {
		return D2COMMON_GetItemTextFromItemCode(txt->dwubercode, &id);
	}
	if (txt->dwubercode == txt->dwcode) {
		return D2COMMON_GetItemTextFromItemCode(txt->dwultracode, &id);
	}
	return nullptr;
}

int UpStatCondition::GetValue(UpStatType type, UnitItemInfo* info)
{
	auto upTxt = GetUpTxt(info);
	if (!upTxt) {
		return 0;
	}
	if (type == UpStatType::LEVEL) {
		auto currentReq = D2COMMON_GetItemLevelRequirement(info->item, D2CLIENT_GetPlayerUnit());
		return max(upTxt->blevelreq, currentReq);
	}
	int req = type == UpStatType::STRENGTH ? upTxt->wreqstr : upTxt->wreqdex;
	int ease = D2COMMON_GetUnitStat(info->item, STAT_REDUCEDREQUIREMENTS, 0);

	int delta = req * ease / 100;
	if (info->item->pItemData->dwFlags & ITEM_ETHEREAL) {
		delta -= 10;
	}

	return (std::max)(0, req + delta);
}

bool UpStatCondition::EvaluateInternal(UnitItemInfo* info, Condition* arg1, Condition* arg2)
{
	return IntegerCompare(GetValue(type, info), operation, targetStat, targetStat2);
}

int MaxSocketsCondition::GetValue(UnitItemInfo* uInfo)
{
	BYTE res = D2COMMON_GetMaxSockets(uInfo->item);
	auto txt = D2COMMON_GetItemText(uInfo->item->dwTxtFileNo);
	return min(res, txt->binvheight * txt->binvwidth);
}

bool MaxSocketsCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(GetValue(uInfo), operation, targetStat, targetStat2);
}

FormulaCondition::FormulaCondition(wstring& k,
	BYTE         op,
	unsigned int target,
	unsigned int target2) : key(k),
	operation(op),
	targetStat(target),
	targetStat2(target2)
{
	conditionType = CT_Operand;
	f = formulaMap.find(key)->second.get();
};

int MinMaxDamageCondition::GetValue(DamageType type, UnitItemInfo* uInfo)
{
	switch (type) {
		case DamageType::MIN:
		{
			int one = GetAdjustedUnitStat(uInfo, STAT_MINIMUMDAMAGE, 0);
			int two = GetAdjustedUnitStat(uInfo, STAT_SECONDARYMINIMUMDAMAGE, 0);
			int thr = GetAdjustedUnitStat(uInfo, STAT_MINIMUMTHROWINGDAMAGE, 0);
			return max(one, max(two, thr));
		}
		case DamageType::MAX:
		{
			int one = GetAdjustedUnitStat(uInfo, STAT_MAXIMUMDAMAGE, 0);
			int two = GetAdjustedUnitStat(uInfo, STAT_SECONDARYMAXIMUMDAMAGE, 0);
			int thr = GetAdjustedUnitStat(uInfo, STAT_MAXIMUMTHROWINGDAMAGE, 0);
			return max(one, max(two, thr));
			return 0;
		}
	}
	return 0;
}

bool MinMaxDamageCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	return IntegerCompare(MinMaxDamageCondition::GetValue(type, uInfo), operation, targetStat, targetStat2);
}

bool FormulaCondition::EvaluateInternal(UnitItemInfo* uInfo,
	Condition* arg1,
	Condition* arg2)
{
	float out;
	if (f->execute(uInfo, out) != FormulaStatus::OK)
	{
		return false;
	}
	if (operation == NONE)
	{
		return Formula<FormulaContext>::IsTrue(out);
	}
	return FloatCompare(out, operation, targetStat, targetStat2);
}

int GetStatFromList(UnitItemInfo* uInfo, int itemStat)
{
	int value = 0;
	StatList* pStatList = D2COMMON_GetStatList(uInfo->item, NULL, 0x40);
	int sockets = D2COMMON_GetUnitStat(uInfo->item, STAT_SOCKETS, 0);

	if (pStatList)
	{
		value += D2COMMON_GetStatValueFromStatList(pStatList, itemStat, 0);
	}

	if (uInfo->item->pItemData->dwFlags & ITEM_RUNEWORD)
	{
		StatList* pStateStatList = D2COMMON_GetStateStatList(uInfo->item, STATE_RUNEWORD);
		if (pStateStatList)
		{
			value += D2COMMON_GetStatValueFromStatList(pStateStatList, itemStat, 0);
		}
	}

	if (sockets)
	{
		for (UnitAny* sItem = uInfo->item->pInventory->pFirstItem; sItem; sItem = sItem->pItemData->pNextInvItem)
		{
			value += D2COMMON_GetUnitStat(sItem, itemStat, 0);
		}
	}
	return value;
}

void HandleUnknownItemCode(char* code,
	char* tag)
{
	// If the DATATBLS arent loaded yet then this is expected
	if (!IsInitialized()) { return; }

	// Avoid spamming endlessly
	if (UnknownItemCodes.size() > 10 || App.lootfilter.allowUnknownItems.toggle.isEnabled) { return; }
	if (UnknownItemCodes.find(code) == UnknownItemCodes.end())
	{
		//PrintText(1, "Unknown item code %s: %c%c%c\n", tag, code[0], code[1], code[2]);
		UnknownItemCodes[code] = 1;
	}
}
