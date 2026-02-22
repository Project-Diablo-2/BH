#include "ItemDisplay.h"
#include "Item.h"
#include "../../Drawing/Stats/StatsDisplay.h"
#include "../../D2Helpers.h"
#include <cctype>
#include <vector>
#include <string>

#define MAP_COLOR_WHITE     0x20

// All colors here must also be defined in ReplacementMap
#define MAP_COLOR_REPLACEMENTS	\
	{"WHITE", 0x20},		\
	{"RED", 0x0A},			\
	{"GREEN", 0x84},		\
	{"BLUE", 0x97},			\
	{"GOLD", 0x0D},			\
	{"GRAY", 0xD0},			\
	{"BLACK", 0x00},		\
	{"TAN", 0x5A},			\
	{"ORANGE", 0x60},		\
	{"YELLOW", 0x0C},		\
	{"PURPLE", 0x9B},		\
	{"DARK_GREEN", 0x76}, \
	{"CORAL", 0x66}, \
	{"SAGE", 0x82}, \
	{"TEAL", 0xCB}, \
	{"LIGHT_GRAY", 0xD6}, \
	{"FULL_TRANS", 0xCB}, \
	{"THREE_FOURTHS_TRANS", 0xCB}, \
	{"HALF_TRANS", 0xCB}, \
	{"QUARTER_TRANS", 0xCB}

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

	COND_NULL
};

std::map<std::string, FilterCondition> condition_map =
{
	{"AND", COND_AND},
	{"&&", COND_AND},
	{"OR", COND_OR},
	{"||", COND_OR},
	{"TRUE", COND_TRUE},
	{"FALSE", COND_FALSE},
	{"ETH", COND_ETH},
	{"SOCK", COND_SOCK},
	{"SOCKETS", COND_SOCK},
	{"SET", COND_SET},
	{"MAG", COND_MAG},
	{"RARE", COND_RARE},
	{"UNI", COND_UNI},
	{"AMAZON", COND_AMAZON},
	{"SORCERESS", COND_SORCERESS},
	{"NECROMANCER", COND_NECROMANCER},
	{"PALADIN", COND_PALADIN},
	{"BARBARIAN", COND_BARBARIAN},
	{"DRUID", COND_DRUID},
	{"ASSASSIN", COND_ASSASSIN},
	{"CRAFTALVL", COND_CRAFTALVL},
	{"REROLLALVL", COND_REROLLALVL},
	{"PREFIX", COND_PREFIX},
	{"SUFFIX", COND_SUFFIX},
	{"AUTOMOD", COND_AUTOMOD},
	{"MAPID", COND_MAPID},
	{"MAPTIER", COND_MAPTIER},
	{"CRAFT", COND_CRAFT},
	{"RW", COND_RW},
	{"NMAG", COND_NMAG},
	{"SUP", COND_SUP},
	{"INF", COND_INF},
	{"NORM", COND_NORM},
	{"EXC", COND_EXC},
	{"ELT", COND_ELT},
	{"CLASS", COND_CLASS},
	{"ID", COND_ID},
	{"ILVL", COND_ILVL},
	{"QLVL", COND_QLVL},
	{"ALVL", COND_ALVL},
	{"CLVL", COND_CLVL},
	{"FILTLVL", COND_FILTLVL},
	{"DIFF", COND_DIFF},
	{"RUNE", COND_RUNE},
	{"GOLD", COND_GOLD},
	{"GEMMED", COND_GEMMED},
	{"GEMTYPE", COND_GEMTYPE},
	{"GEM", COND_GEM},
	{"GEMLEVEL", COND_GEM},
	{"ED", COND_ED},
	{"EDEF", COND_EDEF},
	{"EDAM", COND_EDAM},
	{"DEF", COND_DEF},
	{"MAXDUR", COND_MAXDUR},
	{"RES", COND_RES},
	{"FRES", COND_FRES},
	{"CRES", COND_CRES},
	{"LRES", COND_LRES},
	{"PRES", COND_PRES},
	{"IAS", COND_IAS},
	{"FCR", COND_FCR},
	{"FHR", COND_FHR},
	{"FBR", COND_FBR},
	{"LIFE", COND_LIFE},
	{"MANA", COND_MANA},
	{"QTY", COND_QTY},
	{"GOODSK", COND_GOODSK},
	{"GOODTBSK", COND_GOODTBSK},
	{"FOOLS", COND_FOOLS},
	{"LVLREQ", COND_LVLREQ},
	{"ARPER", COND_ARPER},
	{"MFIND", COND_MFIND},
	{"GFIND", COND_GFIND},
	{"STR", COND_STR},
	{"DEX", COND_DEX},
	{"FRW", COND_FRW},
	{"MINDMG", COND_MINDMG},
	{"MAXDMG", COND_MAXDMG},
	{"AR", COND_AR},
	{"DTM", COND_DTM},
	{"MAEK", COND_MAEK},
	{"REPLIFE", COND_REPLIFE},
	{"REPQUANT", COND_REPQUANT},
	{"REPAIR", COND_REPAIR},
	{"ARMOR", COND_ARMOR},
	{"BELT", COND_BELT},
	{"CHEST", COND_CHEST},
	{"HELM", COND_HELM},
	{"SHIELD", COND_SHIELD},
	{"GLOVES", COND_GLOVES},
	{"BOOTS", COND_BOOTS},
	{"CIRC", COND_CIRC},
	{"DRU", COND_DRU},
	{"BAR", COND_BAR},
	{"DIN", COND_DIN},
	{"NEC", COND_NEC},
	{"SIN", COND_SIN},
	{"SOR", COND_SOR},
	{"ZON", COND_ZON},
	{"MISC", COND_MISC},
	{"JEWELRY", COND_JEWELRY},
	{"CHARM", COND_CHARM},
	{"QUIVER", COND_QUIVER},
	{"SHOP", COND_SHOP},
	{"EQUIPPED", COND_EQUIPPED},
	{"MERC", COND_MERC},
	{"CUBE", COND_CUBE},
	{"INVENTORY", COND_INVENTORY},
	{"STASH", COND_STASH},
	{"GROUND", COND_GROUND},
	{"1H", COND_1H},
	{"2H", COND_2H},
	{"AXE", COND_AXE},
	{"MACE", COND_MACE},
	{"CLUB", COND_CLUB},
	{"TMACE", COND_TMACE},
	{"HAMMER", COND_HAMMER},
	{"SWORD", COND_SWORD},
	{"DAGGER", COND_DAGGER},
	{"THROWING", COND_THROWING},
	{"JAV", COND_JAV},
	{"SPEAR", COND_SPEAR},
	{"POLEARM", COND_POLEARM},
	{"BOW", COND_BOW},
	{"XBOW", COND_XBOW},
	{"STAFF", COND_STAFF},
	{"WAND", COND_WAND},
	{"SCEPTER", COND_SCEPTER},
	{"EQ1", COND_HELM},
	{"EQ2", COND_CHEST},
	{"EQ3", COND_SHIELD},
	{"EQ4", COND_GLOVES},
	{"EQ5", COND_BOOTS},
	{"EQ6", COND_BELT},
	{"EQ7", COND_CIRC},
	{"CL1", COND_DRU},
	{"CL2", COND_BAR},
	{"CL3", COND_DIN},
	{"CL4", COND_NEC},
	{"CL5", COND_SIN},
	{"CL6", COND_SOR},
	{"CL7", COND_ZON},
	{"WEAPON", COND_WEAPON},
	{"WP1", COND_AXE},
	{"WP2", COND_MACE},
	{"WP3", COND_SWORD},
	{"WP4", COND_DAGGER},
	{"WP5", COND_THROWING},
	{"WP6", COND_JAV},
	{"WP7", COND_SPEAR},
	{"WP8", COND_POLEARM},
	{"WP9", COND_BOW},
	{"WP10", COND_XBOW},
	{"WP11", COND_STAFF},
	{"WP12", COND_WAND},
	{"WP13", COND_SCEPTER},
	{"ALLSK", COND_ALLSK},
	{"BUYPRICE", COND_BUYPRICE},
	{"SELLPRICE", COND_PRICE},
	{"PRICE", COND_PRICE},
	{"WIDTH", COND_WIDTH},
	{"HEIGHT", COND_HEIGHT},
	{"AREA", COND_AREA},
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
const unordered_map<string, const SkillReplace> skills = {
	{{"LIFE"}, { STAT_MAXHP, 0}},
	{{"MANA"}, { STAT_MAXMANA, 0}},
	{{"STR"}, { STAT_STRENGTH, 0}},
	{{"DEX"}, { STAT_DEXTERITY, 0}},
	{{"CRES"}, { STAT_COLDRESIST, 0}},
	{{"FRES"}, { STAT_FIRERESIST, 0}},
	{{"LRES"}, { STAT_LIGHTNINGRESIST, 0}},
	{{"PRES"}, { STAT_POISONRESIST, 0}},
	{{"MINDMG"}, { STAT_MINIMUMDAMAGE, 0}},
	{{"MAXDMG"}, { STAT_MAXIMUMDAMAGE, 0}},
	{{"EDEF"}, { STAT_ENHANCEDDEFENSE, 0}},
	{{"EDAM"}, { STAT_ENHANCEDMAXIMUMDAMAGE, 0}},
	{{"FCR"}, { STAT_FASTERCAST, 0}},
	{{"AR"}, { STAT_ATTACKRATING, 0}},
	{{"REPLIFE"}, { STAT_REPLENISHLIFE, 0}},
	{{"STAT"}, { ~0UL, 1}},
	{{"MULTI"}, { ~0UL, 2}},
};

std::map<std::string, int>   UnknownItemCodes;
vector<pair<string, string>> aliases;
vector<pair<string, string>> rules;
vector<Rule*>                RuleList;
vector<Rule*>                MapRuleList;
vector<Rule*>                IgnoreRuleList;
BYTE                         LastConditionType;

TrueCondition* trueCondition = new TrueCondition();
FalseCondition* falseCondition = new FalseCondition();

// Helper function to get a list of strings
vector<string> split(const string& s,
	char          delim)
{
	vector<string> result;
	stringstream   ss(s);
	string         item;
	while (getline(ss, item, delim)) { result.push_back(item); }
	return result;
}

// Helper function to join a list of string
template <typename C>
std::string join(C const& strings,
	std::string const& delim)
{
	std::ostringstream ostr;
	auto               last = std::prev(std::end(strings));
	std::copy(
		std::begin(strings),
		last,
		std::ostream_iterator<std::string>(ostr, delim.c_str())
	);
	ostr << *last;
	return ostr.str();
}

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

char* GemLevels[] = {
	"NONE",
	"Chipped",
	"Flawed",
	"Normal",
	"Flawless",
	"Perfect"
};

char* GemTypes[] = {
	"NONE",
	"Amethyst",
	"Diamond",
	"Emerald",
	"Ruby",
	"Sapphire",
	"Topaz",
	"Skull"
};

struct ReplaceContext {
	UnitItemInfo* info;
	string name;
	ItemsTxt* text;
	// no newlines allowed
	bool limit;
	bool inShop;
	bool nlAllowed;
	bool nmagStaffmod;
	bool blockedNL;

	ReplaceContext(UnitItemInfo* info, string name, bool limit);
};

struct ReplacementValue {
	const function<string(ReplaceContext&, const ReplacementValue&)> fn;
	string str = "";
	int param1 = 0;
	int param2 = 0;

	ReplacementValue(const string& str, const int param1, const int param2, function<string(ReplaceContext&, const ReplacementValue&)> fn) :
		str(str),
		param1(param1),
		param2(param2),
		fn(fn) {
	}

	string Replace(ReplaceContext& ctx) const;
};

struct ReplacementSpec {
	uint16_t params;
	function<string(ReplaceContext&, const ReplacementValue&)> fn;

	ReplacementSpec(uint16_t params, function<string(ReplaceContext&, const ReplacementValue&)> fn) :
		params(params),
		fn(fn) {
	}

	static ReplacementValue MakeReplacementValue(const string& str);
	static ReplacementValue MakeReplacementValue(const smatch& match, bool& fail);

	// STATIC keywords
	// no-op
	static string ReplaceNone(ReplaceContext& ctx, const ReplacementValue& val);
	// string between keywords
	static string ReplaceConst(ReplaceContext& ctx, const ReplacementValue& val);
	// %NAME%
	static string ReplaceName(ReplaceContext& ctx, const ReplacementValue& val);
	// %BASENAME%
	static string ReplaceBaseName(ReplaceContext& ctx, const ReplacementValue& val);
	// %SOCKETS%
	static string ReplaceSockets(ReplaceContext& ctx, const ReplacementValue& val);
	// %RUNENUM%
	static string ReplaceRuneNumber(ReplaceContext& ctx, const ReplacementValue& val);
	// %RUNNAME%
	static string ReplaceRuneName(ReplaceContext& ctx, const ReplacementValue& val);
	// %GEMLEVEL%
	static string ReplaceGemLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %GEMTYPE%
	static string ReplaceGemType(ReplaceContext& ctx, const ReplacementValue& val);
	// %ILVL%
	static string ReplaceItemLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %ALVL%
	static string ReplaceAffixLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %CRAFTALVL%
	static string ReplaceCraftLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %REROLLALVL%
	static string ReplaceRerollLevel(ReplaceContext& ctx, const ReplacementValue& val);
	// %LVLREQ%
	static string ReplaceLevelRequirement(ReplaceContext& ctx, const ReplacementValue& val);
	// %WPNSPD%
	static string ReplaceWeaponSpeed(ReplaceContext& ctx, const ReplacementValue& val);
	// %RANGE%
	static string ReplaceRange(ReplaceContext& ctx, const ReplacementValue& val);
	// %CODE%
	static string ReplaceCode(ReplaceContext& ctx, const ReplacementValue& val);
	// %BUYPRICE%
	static string ReplaceBuyPrice(ReplaceContext& ctx, const ReplacementValue& val);
	// %PRICE%
	static string ReplacePrice(ReplaceContext& ctx, const ReplacementValue& val);
	// %QTY%
	static string ReplaceQuantity(ReplaceContext& ctx, const ReplacementValue& val);
	// %RES%
	static string ReplaceAllResist(ReplaceContext& ctx, const ReplacementValue& val);
	// enhance damage or enhance defense %ED%
	static string ReplaceEnhancedD(ReplaceContext& ctx, const ReplacementValue& val);
	// %CL%
	static string ReplaceConditionalLine(ReplaceContext& ctx, const ReplacementValue& val);
	// %NL%
	static string ReplaceNewLine(ReplaceContext& ctx, const ReplacementValue& val);

	// DYNAMIC keywords
	// %STAT%
	static string ReplaceStat(ReplaceContext& ctx, const ReplacementValue& val);
	// %SK%
	static string ReplaceSingleSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %OS%
	static string ReplaceNonClassSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %CLSK%
	static string ReplaceClassSkill(ReplaceContext& ctx, const ReplacementValue& val);
	// %TABSK%
	static string ReplaceSkillTab(ReplaceContext& ctx, const ReplacementValue& val);
	// %CHARSTAT%
	static string ReplaceCharStat(ReplaceContext& ctx, const ReplacementValue& val);
	// %MULTI%
	static string ReplaceMulti(ReplaceContext& ctx, const ReplacementValue& val);

	// COLOR keywords
	static function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceGlideDependentColor(const string& primary, const string& secondary);
	static function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceHDTextDependentColor(const string& primary, const string& secondary);
	static function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceBindString(const string& str);
	static function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplaceNamedStat(int id);
};

unordered_map<string, ReplacementSpec> ReplacementMap = {
	// STATIC
	{ "NAME", { 0, ReplacementSpec::ReplaceName } },
	{ "BASENAME", { 0, ReplacementSpec::ReplaceBaseName } },
	{ "SOCKETS", { 0, ReplacementSpec::ReplaceSockets } },
	{ "RUNENUM", { 0, ReplacementSpec::ReplaceRuneNumber } },
	{ "RUNENAME", { 0, ReplacementSpec::ReplaceRuneName } },
	{ "GEMLEVEL", { 0, ReplacementSpec::ReplaceGemLevel } },
	{ "GEMTYPE", { 0, ReplacementSpec::ReplaceGemType } },
	{ "ILVL", { 0, ReplacementSpec::ReplaceItemLevel } },
	{ "ALVL", { 0, ReplacementSpec::ReplaceAffixLevel } },
	{ "CRAFTALVL", { 0, ReplacementSpec::ReplaceCraftLevel } },
	{ "REROLLALVL", {0, ReplacementSpec::ReplaceRerollLevel }},
	{ "LVLREQ", { 0, ReplacementSpec::ReplaceLevelRequirement } },
	{ "WPNSPD", { 0, ReplacementSpec::ReplaceWeaponSpeed } },
	{ "RANGE", { 0, ReplacementSpec::ReplaceRange } },
	{ "CODE", { 0, ReplacementSpec::ReplaceCode } },
	// %LBRACE%
	{ "LBRACE", { 0, ReplacementSpec::ReplaceBindString("{") } },
	// %RBRACE%
	{ "RBRACE", { 0, ReplacementSpec::ReplaceBindString("}") } },
	{ "BUYPRICE", { 0, ReplacementSpec::ReplaceBuyPrice } },
	{ "SELLPRICE", { 0, ReplacementSpec::ReplacePrice } },
	{ "PRICE", { 0, ReplacementSpec::ReplacePrice } },
	{ "QTY", { 0, ReplacementSpec::ReplaceQuantity } },
	{ "RES", { 0, ReplacementSpec::ReplaceAllResist } },
	{ "ED", { 0, ReplacementSpec::ReplaceEnhancedD } },
	{ "CL", { 0, ReplacementSpec::ReplaceConditionalLine } },
	{ "NL", { 0, ReplacementSpec::ReplaceNewLine } },
	// named stats
	{ "EDEF", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ENHANCEDDEFENSE) } },
	{ "EDAM", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ENHANCEDMAXIMUMDAMAGE) } },
	{ "DEF", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DEFENSE) } },
	{ "FRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FIRERESIST) } },
	{ "CRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_COLDRESIST) } },
	{ "LRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_LIGHTNINGRESIST) } },
	{ "PRES", { 0, ReplacementSpec::ReplaceNamedStat(STAT_POISONRESIST) } },
	{ "IAS", { 0, ReplacementSpec::ReplaceNamedStat(STAT_IAS) } },
	{ "FCR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERCAST) } },
	{ "FHR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERHITRECOVERY) } },
	{ "FBR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERBLOCK) } },
	{ "LIFE", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAXHP) } },
	{ "MANA", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAXMANA) } },
	{ "ARPER", { 0, ReplacementSpec::ReplaceNamedStat(STAT_TOHITPERCENT) } },
	{ "MFIND", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAGICFIND) } },
	{ "GFIND", { 0, ReplacementSpec::ReplaceNamedStat(STAT_GOLDFIND) } },
	{ "STR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_STRENGTH) } },
	{ "DEX", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DEXTERITY) } },
	{ "FRW", { 0, ReplacementSpec::ReplaceNamedStat(STAT_FASTERRUNWALK) } },
	{ "MINDMG", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MINIMUMDAMAGE) } },
	{ "MAXDMG", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MAXIMUMDAMAGE) } },
	{ "AR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_ATTACKRATING) } },
	{ "DTM", { 0, ReplacementSpec::ReplaceNamedStat(STAT_DAMAGETOMANA) } },
	{ "MAEK", { 0, ReplacementSpec::ReplaceNamedStat(STAT_MANAAFTEREACHKILL) } },
	{ "REPLIFE", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPLENISHLIFE) } },
	{ "REPQUANT", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPLENISHESQUANTITY) } },
	{ "REPAIR", { 0, ReplacementSpec::ReplaceNamedStat(STAT_REPAIRSDURABILITY) } },
	// DYNAMIC
	{ "STAT", { 1, ReplacementSpec::ReplaceStat } },
	{ "SK", { 1, ReplacementSpec::ReplaceSingleSkill } },
	{ "OS", { 1, ReplacementSpec::ReplaceNonClassSkill } },
	{ "CLSK", { 1, ReplacementSpec::ReplaceClassSkill } },
	{ "TABSK", { 1, ReplacementSpec::ReplaceSkillTab } },
	{ "CHARSTAT", { 1, ReplacementSpec::ReplaceCharStat } },
	{ "MULTI", { 2, ReplacementSpec::ReplaceMulti } },
	// COLORS
	{ "BLACK", { 0, ReplacementSpec::ReplaceGlideDependentColor("\xFF" "c\x02", "ÿc6") }},
	{ "CORAL", { 0, ReplacementSpec::ReplaceGlideDependentColor("\xFF" "c\x06", "ÿc1") }},
	{ "SAGE", { 0, ReplacementSpec::ReplaceGlideDependentColor("\xFF" "c\x07", "ÿc2") }},
	{ "TEAL", { 0, ReplacementSpec::ReplaceGlideDependentColor("\xFF" "c\x09", "ÿc3") }},
	{ "LIGHT_GRAY", { 0, ReplacementSpec::ReplaceGlideDependentColor("\xFF" "c\x0C", "ÿc5") }},
	{ "FULL_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor("\xFF" "c\x40", "") }},
	{ "THREE_FOURTHS_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor("\xFF" "c\x41", "") }},
	{ "HALF_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor("\xFF" "c\x42", "") }},
	{ "QUARTER_TRANS", { 0, ReplacementSpec::ReplaceHDTextDependentColor("\xFF" "c\x43", "") }},
	{ "WHITE", { 0, ReplacementSpec::ReplaceBindString("ÿc0") } },
	{ "RED", { 0, ReplacementSpec::ReplaceBindString("ÿc1") } },
	{ "GREEN", { 0, ReplacementSpec::ReplaceBindString("ÿc2") } },
	{ "BLUE", { 0, ReplacementSpec::ReplaceBindString("ÿc3") } },
	{ "GOLD", { 0, ReplacementSpec::ReplaceBindString("ÿc4") } },
	{ "GRAY", { 0, ReplacementSpec::ReplaceBindString("ÿc5") } },
	{ "TAN", { 0, ReplacementSpec::ReplaceBindString("ÿc7") } },
	{ "ORANGE", { 0, ReplacementSpec::ReplaceBindString("ÿc8") } },
	{ "YELLOW", { 0, ReplacementSpec::ReplaceBindString("ÿc9") } },
	{ "PURPLE", { 0, ReplacementSpec::ReplaceBindString("ÿc;") } },
	{ "DARK_GREEN", { 0, ReplacementSpec::ReplaceBindString("ÿc:") } },
};

regex ReplacementRegex("%([A-Z_]+)(?:(\\d{1,9})(?:,(\\d{1,9}))?)?%", regex::ECMAScript);
vector<ReplacementValue> BuildReplacementActions(const string& action)
{
	vector<ReplacementValue> res;
	smatch match;

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

ReplacementValue ReplacementSpec::MakeReplacementValue(const string& str)
{
	return ReplacementValue(str, 0, 0, ReplacementSpec::ReplaceConst);
}

ReplacementValue ReplacementSpec::MakeReplacementValue(const smatch& match, bool& fail)
{
	const auto& spec = ReplacementMap.find(match[1]);
	if (spec == ReplacementMap.end()) {
		fail = true;
		return ReplacementValue(match.str(), 0, 0, ReplacementSpec::ReplaceNone);
	}
	const auto& replacer = spec->second;
	const int count = (match[2].length() != 0) + (match[3].length() != 0);
	if (count != replacer.params) {
		return ReplacementValue(match.str(), 0, 0, ReplacementSpec::ReplaceConst);
	}
	int param1 = match[2].length() > 0 ? stoi(match[2].str()) : 0;
	int param2 = match[3].length() > 0 ? stoi(match[3].str()) : 0;
	return ReplacementValue("", param1, param2, replacer.fn);
}

function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceNamedStat(int id)
{
	ReplacementValue rep("", id, 0, ReplacementSpec::ReplaceNone);
	return [rep](ReplaceContext& ctx, const ReplacementValue& val) -> string {
		return ReplacementSpec::ReplaceStat(ctx, rep);
	};
}

function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceBindString(const std::string& str)
{
	return [str](ReplaceContext& ctx, const ReplacementValue& val) -> string {
		return str;
	};
}

string ReplacementSpec::ReplaceNone(ReplaceContext& ctx, const ReplacementValue& val)
{
	return "";
}

string ReplacementSpec::ReplaceConst(ReplaceContext& ctx, const ReplacementValue& val)
{
	return val.str;
}

string ReplacementSpec::ReplaceName(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.name.size() > 1023) {
		ctx.name.resize(1023);
	}
	return ctx.name;
}

string ReplacementSpec::ReplaceBaseName(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.info == nullptr || ctx.info->attrs == nullptr) {
		return "";
	}

	return MaybeStripColorPrefix(ctx.info->attrs->name);
}

string ReplacementSpec::ReplaceSockets(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarSockets(ctx.info);
}

string ReplacementSpec::ReplaceRuneNumber(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRuneNum(ctx.info);
}

string ReplacementSpec::ReplaceRuneName(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRuneName(ctx.info);
}

string ReplacementSpec::ReplaceGemLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarGemLevel(ctx.info);
}

string ReplacementSpec::ReplaceGemType(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarGemType(ctx.info);
}

string ReplacementSpec::ReplaceItemLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarIlvl(ctx.info);
}

string ReplacementSpec::ReplaceAffixLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarAlvl(ctx.info);
}

string ReplacementSpec::ReplaceCraftLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarCraftAlvl(ctx.info);
}

string ReplacementSpec::ReplaceRerollLevel(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRerollAlvl(ctx.info);
}

string ReplacementSpec::ReplaceLevelRequirement(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarLevelReq(ctx.info);
}

string ReplacementSpec::ReplaceWeaponSpeed(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarWeaponSpeed(ctx.text);
}

string ReplacementSpec::ReplaceRange(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarRangeAdder(ctx.text);
}

string ReplacementSpec::ReplaceCode(ReplaceContext& ctx, const ReplacementValue& val)
{
	return ctx.info->itemCode;
}

string ReplacementSpec::ReplaceBuyPrice(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarBuyValue(ctx.info, ctx.text);
}

string ReplacementSpec::ReplacePrice(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarSellValue(ctx.info, ctx.text);
}

string ReplacementSpec::ReplaceQuantity(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarQty(ctx.info);
}

string ReplacementSpec::ReplaceAllResist(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarAllRes(ctx.info);
}

string ReplacementSpec::ReplaceEnhancedD(ReplaceContext& ctx, const ReplacementValue& val)
{
	return NameVarEd(ctx.info);
}

string ReplacementSpec::ReplaceConditionalLine(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.blockedNL) {
		return "";
	}
	if (!ctx.limit || ctx.nlAllowed) {
		return "\r";
	}
	if (ctx.nmagStaffmod) {
		ctx.blockedNL = true;
		return "\n";
	}
	return "";
}

string ReplacementSpec::ReplaceNewLine(ReplaceContext& ctx, const ReplacementValue& val)
{
	if (ctx.blockedNL) {
		return "";
	}
	if (!ctx.limit || ctx.nlAllowed) {
		return "\n";
	}
	if (ctx.nmagStaffmod) {
		ctx.blockedNL = true;
		return "\n";
	}
	return "";
}

string ReplacementSpec::ReplaceStat(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat = val.param1;
	if (stat > STAT_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
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
		stat == STAT_SECONDARYMAXIMUMDAMAGE			// return base max 2h weapon damage
		)
	{
		value = GetStatFromList(ctx.info, stat);
	}
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceSingleSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_SINGLESKILL, skill);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceNonClassSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_NONCLASSSKILL, skill);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceClassSkill(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_CLASSSKILLS, skill);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceSkillTab(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto skill = val.param1;
	if (skill > SKILL_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, STAT_SKILLTAB, skill);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceCharStat(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat = val.param1;
	if (stat > STAT_MAX) {
		return "";
	}
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), stat, 0);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

string ReplacementSpec::ReplaceMulti(ReplaceContext& ctx, const ReplacementValue& val)
{
	const auto stat1 = val.param1;
	if (stat1 > STAT_MAX) {
		return "";
	}
	const auto stat2 = val.param2;
	char buffer[16] = { 0 };
	auto value = D2COMMON_GetUnitStat(ctx.info->item, stat1, stat2);
	sprintf_s(buffer, "%d", value);
	return buffer;
}

function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceGlideDependentColor(const string& primary, const string& secondary)
{
	return [primary, secondary](ReplaceContext& ctx, const ReplacementValue& val) {
		return *p_D2GFX_RenderMode != 4 ? secondary : primary;
		};
}

function<string(ReplaceContext& ctx, const ReplacementValue& val)> ReplacementSpec::ReplaceHDTextDependentColor(const string& primary, const string& secondary)
{
	return [primary, secondary](ReplaceContext& ctx, const ReplacementValue& val) {
		return (!App.d2gl.usingD2GL.value && !App.d2gl.usingHDText.value) ? secondary : primary;
		};
}

ReplaceContext::ReplaceContext(UnitItemInfo* info, string name, bool limit) :
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

	// Check if non-mag item capable of having staffmods
	nmagStaffmod = ((info->item->pItemData->dwQuality == ITEM_QUALITY_INFERIOR ||
		info->item->pItemData->dwQuality == ITEM_QUALITY_NORMAL ||
		info->item->pItemData->dwQuality == ITEM_QUALITY_SUPERIOR) &&
		info->attrs->staffmodClass < CLASS_NA);
}

string ReplacementValue::Replace(ReplaceContext& ctx) const
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

char* GetGemLevelString(BYTE level) { return GemLevels[level]; }

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

char* GetGemTypeString(BYTE type) { return GemTypes[type]; }

bool IsRune(ItemAttributes* attrs)
{
	return (attrs->miscFlags & ITEM_GROUP_RUNE) > 0;
	//char* code = attrs->code;
	//return (code[0] == 'r' && std::isdigit(code[1]) && std::isdigit(code[2]));
}

BYTE RuneNumberFromItemCode(char* code) { return (BYTE)(((code[1] - '0') * 10) + code[2] - '0'); }

// Find the item description. This code is called only when there's a cache miss
string ItemDescLookupCache::make_cached_T(UnitItemInfo* uInfo)
{
	ReplaceContext ctx(uInfo, "", FALSE);
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

string ItemDescLookupCache::to_str(const string& name)
{
	size_t      start_pos = 0;
	std::string itemName(name);
	while ((start_pos = itemName.find('\n', start_pos)) != std::string::npos)
	{
		itemName.replace(start_pos, 1, " - ");
		start_pos += 3;
	}
	return itemName;
}

// Find the item name. This code is called only when there's a cache miss
string ItemNameLookupCache::make_cached_T(UnitItemInfo* uInfo,
	const string& name)
{
	string new_name(name);
	if (!new_name.empty() && new_name.front() == ' ') { new_name.erase(0, 1); }					// removes one leading space (happens on magic items without a prefix)
	if (!new_name.empty() && new_name.back() == ' ') { new_name.erase(new_name.size() - 1, 1); }	// removes one trailing space (happens on magic items without a suffix)
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

string ItemNameLookupCache::to_str(const string& name)
{
	size_t      start_pos = 0;
	std::string itemName(name);
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
	for (auto& action : actions) { name += action.name + " "; }
	return name;
}

// least recently used cache for storing a limited number of item names
ItemDescLookupCache  item_desc_cache(RuleList);
ItemNameLookupCache  item_name_cache(RuleList);
MapActionLookupCache map_action_cache(MapRuleList);

void GetItemName(UnitItemInfo* uInfo,
	string& name)
{
	string new_name = item_name_cache.Get(uInfo, name);
	if (new_name == "" && Item::GetFilterLevel() == 0) {
		return;
	}
	name.assign(new_name);
}

string NameVarSockets(UnitItemInfo* uInfo)
{
	char sockets[4] = "";
	sprintf_s(sockets, "%d", D2COMMON_GetUnitStat(uInfo->item, STAT_SOCKETS, 0));
	return 	sockets;
}

string NameVarRuneNum(UnitItemInfo* uInfo)
{
	char runenum[4] = "0";
	if (IsRune(uInfo->attrs))
		sprintf_s(runenum, "%d", RuneNumberFromItemCode(uInfo->itemCode));
	return runenum;
}

string NameVarRuneName(UnitItemInfo* uInfo)
{
	// TODO: removes " Rune" from the rune name. Pretty likely to break on non-english strings
	// Utilize D2LANG_GetLocaleText(20473) area?
	char runename[16] = "";
	if (IsRune(uInfo->attrs))
	{
		sprintf_s(runename, uInfo->attrs->name.substr(0, uInfo->attrs->name.find(' ')).c_str());
	}
	return runename;
}

string NameVarGemLevel(UnitItemInfo* uInfo)
{
	char gemlevel[16] = "";
	if (IsGem(uInfo->attrs))
		sprintf_s(gemlevel, "%s", GetGemLevelString(GetGemLevel(uInfo->attrs)));
	return gemlevel;
}

string NameVarGemType(UnitItemInfo* uInfo)
{
	char gemtype[16] = "";
	if (IsGem(uInfo->attrs))
		sprintf_s(gemtype, "%s", GetGemTypeString(GetGemType(uInfo->attrs)));
	return gemtype;
}

string NameVarIlvl(UnitItemInfo* uInfo)
{
	char ilvl[4] = "0";
	sprintf_s(ilvl, "%d", uInfo->item->pItemData->dwItemLevel);
	return ilvl;
}

string NameVarAlvl(UnitItemInfo* uInfo)
{
	char alvl[4] = "0";
	int alvl_int = GetAffixLevel(
		uInfo->item->pItemData->dwItemLevel,
		uInfo->attrs->qualityLevel,
		uInfo->attrs->magicLevel
	);
	sprintf_s(alvl, "%d", alvl_int);
	return alvl;
}

string NameVarCraftAlvl(UnitItemInfo* uInfo)
{
	char craftalvl[4] = "0";
	int clvl = D2COMMON_GetUnitStat(D2CLIENT_GetPlayerUnit(), STAT_LEVEL, 0);

	int craft_alvl = GetAffixLevel(
		(int)(0.5 * clvl) + (int)(0.5 * uInfo->item->pItemData->dwItemLevel),
		uInfo->attrs->qualityLevel,
		uInfo->attrs->magicLevel
	);
	sprintf_s(craftalvl, "%d", craft_alvl);
	return craftalvl;
}

string NameVarRerollAlvl(UnitItemInfo* uInfo)
{
	char alvl[4] = "0";
	int reroll_alvl = ComputeRerollAffixLevel(uInfo);
	sprintf_s(alvl, "%d", reroll_alvl);
	return alvl;
}

string NameVarLevelReq(UnitItemInfo* uInfo)
{
	char lvlreq[4] = "0";
	sprintf_s(lvlreq, "%d", GetRequiredLevel(uInfo->item));
	return lvlreq;
}

string NameVarWeaponSpeed(ItemsTxt* itemTxt)
{
	char wpnspd[4] = "0";
	sprintf_s(wpnspd, "%d", itemTxt->dwspeed);
	return wpnspd;
}

string NameVarRangeAdder(ItemsTxt* itemTxt)
{
	char rangeadder[4] = "0";
	sprintf_s(rangeadder, "%d", itemTxt->brangeadder);
	return rangeadder;
}

string NameVarBuyValue(UnitItemInfo* uInfo,
	ItemsTxt* itemTxt)
{
	char sellvalue[16] = "";
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (pUnit && itemTxt->bquest == 0)
	{
		sprintf_s(sellvalue, "%d", GetShopPrice(pUnit, uInfo->item, TRANSACTIONTYPE_BUY));
	}
	return sellvalue;
}

string NameVarSellValue(UnitItemInfo* uInfo,
	ItemsTxt* itemTxt)
{
	char sellvalue[16] = "";
	UnitAny* pUnit = D2CLIENT_GetPlayerUnit();
	if (pUnit && itemTxt->bquest == 0)
	{
		sprintf_s(sellvalue, "%d", GetShopPrice(pUnit, uInfo->item, TRANSACTIONTYPE_SELL));
	}
	return sellvalue;
}

string NameVarQty(UnitItemInfo* uInfo)
{
	char qty[10] = "0";
	sprintf_s(qty, "%d", D2COMMON_GetUnitStat(uInfo->item, STAT_AMMOQUANTITY, 0));
	return qty;
}

string NameVarAllRes(UnitItemInfo* uInfo)
{
	char allres[4] = "0";
	int fRes = D2COMMON_GetUnitStat(uInfo->item, STAT_FIRERESIST, 0);
	int lRes = D2COMMON_GetUnitStat(uInfo->item, STAT_LIGHTNINGRESIST, 0);
	int cRes = D2COMMON_GetUnitStat(uInfo->item, STAT_COLDRESIST, 0);
	int pRes = D2COMMON_GetUnitStat(uInfo->item, STAT_POISONRESIST, 0);
	int minres = 0;
	if (fRes && lRes && cRes && pRes)
	{
		minres = min(min(fRes, lRes), min(cRes, pRes));
	}
	sprintf_s(allres, "%d", minres);
	return allres;
}

string NameVarEd(UnitItemInfo* uInfo)
{
	char ed[4] = "0";

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
	sprintf_s(ed, "%d", value);
	return ed;
}

void TrimItemText(UnitItemInfo* uInfo,
	string& name,
	BOOL bLimit)
{
	// Collapse paired CLs
	while (name.find("\r\r") != string::npos)
		name.replace(name.find("\r\r"), 2, "\r");
	// Delete leading/trailing CLs
	if (name.find("\r") == 0)
		name.erase(0, 1);
	if (!name.empty() && name.rfind("\r") == name.size() - 1)
		name.resize(name.size() - 1);
	// Convert to new line
	while (name.find("\r") != string::npos)
		name.replace(name.find("\r"), 1, "\n");

	int nColorCodesSize = 0;
	int lengthLimit = 0;
	if (bLimit)
	{
		// Calc the extra size from colors
		std::regex color_reg("ÿc[0-9;:\\x01-\\x1F]", std::regex_constants::ECMAScript);
		auto       color_matches = std::sregex_iterator(name.begin(), name.end(), color_reg);
		auto       color_end = std::sregex_iterator();
		auto       match_count = std::distance(color_matches, color_end);
		nColorCodesSize += 3 * match_count;

		bool inShop = (uInfo->item->pItemData->pOwnerInventory != 0 && // Skip on ground items
			find(begin(ShopNPCs), end(ShopNPCs), uInfo->item->pItemData->pOwnerInventory->pOwner->dwTxtFileNo) != end(ShopNPCs));

		// Increase limit for shop items
		lengthLimit = inShop ? MAX_ITEM_TEXT_SIZE : MAX_ITEM_NAME_SIZE;

		int nColorsToKeep = 0;
		for (std::sregex_iterator k = color_matches; k != color_end; ++k)
		{
			std::smatch match = *k;
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

BYTE GetOperation(string* op)
{
	if (op->length() < 1) { return NONE; }
	else if ((*op)[0] == '=') { return EQUAL; }
	else if ((*op)[0] == '<') { return LESS_THAN; }
	else if ((*op)[0] == '>') { return GREATER_THAN; }
	else if ((*op)[0] == '~') { return BETWEEN; }
	return NONE;
}

unsigned int GetItemCodeIndex(char codeChar)
{
	// Characters '0'-'9' map to 0-9, and a-z map to 10-35
	return codeChar - (codeChar < 90 ? 48 : 87);
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
		ResetCaches();
		BH::lootFilter->ReadMapList("Alias", aliases);
		BH::lootFilter->ReadMapList("ItemDisplay", rules);

		// Limit aliases to single keywords
		for (unsigned int i = 0; i < aliases.size(); i++)
		{
			aliases[i].first = Trim(aliases[i].first);

			if (aliases[i].first.find(" ") != string::npos)
				aliases[i].first.erase(aliases[i].first.find(" "));
		}

		for (unsigned int i = 0; i < rules.size(); i++)
		{
			for (auto alias : aliases)
			{
				if (alias.first.empty())
					continue;

				while (rules[i].first.find(alias.first) != string::npos)
					rules[i].first.replace(rules[i].first.find(alias.first), alias.first.length(), alias.second);

				transform(alias.first.begin(), alias.first.end(), alias.first.begin(), toupper);
				while (rules[i].second.find("%" + alias.first + "%") != string::npos)
					rules[i].second.replace(rules[i].second.find("%" + alias.first + "%"), alias.first.length() + 2, alias.second);
			}

			string         buf;
			stringstream   ss(rules[i].first);
			vector<string> tokens;
			while (ss >> buf) { tokens.push_back(buf); }

			LastConditionType = CT_None;
			vector<Condition*> RawConditions;
			for (vector<string>::iterator tok = tokens.begin(); tok < tokens.end(); tok++) { Condition::BuildConditions(RawConditions, (*tok)); }
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
	string* str)
{
	Condition::ProcessConditions(inputConditions, conditions);
	BuildAction(str, &action);
	if (!Convert()) {
		root = -1;
	}
	name = BuildReplacementActions(action.name);
	description = BuildReplacementActions(action.description);
}

string Rule::ApplyName(ReplaceContext& ctx) {
	string res;
	for (const auto& fn : name) {
		res += fn.Replace(ctx);
	}
	return res;
}

string Rule::ApplyDescription(ReplaceContext& ctx) {
	string res;
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

void BuildAction(string* str,
	Action* act)
{
	act->name = string(str->c_str());

	//// upcase all text in a %replacement_string%
	//// for some reason \w wasn't catching _, so I added it to the groups
	try
	{
		std::regex replace_reg(
			R"(^(?:(?:%[^%]*%)|[^%])*%((?:\w|_|-)*?[a-z]+?(?:\w|_|-)*?)%)",
			std::regex_constants::ECMAScript);
		std::smatch replace_match;
		while (std::regex_search(act->name, replace_match, replace_reg))
		{
			auto offset = replace_match[1].first - act->name.begin();
			std::transform(
				replace_match[1].first,
				replace_match[1].second,
				act->name.begin() + offset,
				toupper
			);
		}
	}
	catch (std::exception e)
	{
		act->name = "\377c1FILTER REGEX ERROR";
	}

	// new stuff:
	act->borderColor = ParseMapColor(act, "BORDER");
	act->colorOnMap = ParseMapColor(act, "MAP");
	act->dotColor = ParseMapColor(act, "DOT");
	act->pxColor = ParseMapColor(act, "PX");
	act->lineColor = ParseMapColor(act, "LINE");
	act->notifyColor = ParseMapColor(act, "NOTIFY");
	act->pingLevel = ParsePingLevel(act, "TIER");
	act->description = ParseDescription(act);
	act->soundID = ParseSoundID(act, "SOUNDID");

	// legacy support:
	size_t map = act->name.find("%MAP%");
	if (map != string::npos)
	{
		int          mapColor = MAP_COLOR_WHITE;
		size_t       lastColorPos = 0;
		ColorReplace colors[] = {
			MAP_COLOR_REPLACEMENTS
		};
		for (int n = 0; n < sizeof(colors) / sizeof(colors[0]); n++)
		{
			size_t pos = act->name.find("%" + colors[n].key + "%");
			if (pos != string::npos && pos < map && pos >= lastColorPos)
			{
				mapColor = colors[n].value;
				lastColorPos = pos;
			}
		}

		act->name.replace(map, 5, "");
		act->colorOnMap = mapColor;
		if (act->borderColor == UNDEFINED_COLOR)
			act->borderColor = act->colorOnMap;
	}

	size_t done = act->name.find("%CONTINUE%");
	if (done != string::npos)
	{
		act->name.replace(done, 10, "");
		act->stopProcessing = false;
	}
}

int ParsePingLevel(Action* act, const string& key_string) {
	std::regex pattern("%" + key_string + "-([0-9])%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	int ping_level = -1;
	std::smatch the_match;

	if (std::regex_search(act->name, the_match, pattern)) {
		ping_level = stoi(the_match[1].str());
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(), "");
	}
	return ping_level;
}

// ParseSoundID
// Returns an int ranging from 0 to the MAX_SOUND_ID.
// If the parsed soundID is not found in that range, this will return a 0.
int ParseSoundID(Action* act, const string& key_string) {
	std::regex pattern("%" + key_string + "-([0-9]{1,4})%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	// Default soundID should be 0 incase this is played.
	// 0 is none.wav
	int soundID = 0;
	std::smatch the_match;

	if (std::regex_search(act->name, the_match, pattern)) {
		int matchedSoundID = stoi(the_match[1].str());
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(), "");

		// Do our best to ensure the soundID is valid.
		// Ensure soundID is in the range of 0 and MAX_SOUND_ID.
		if (matchedSoundID < *p_D2CLIENT_SoundRecords) {
			soundID = matchedSoundID;
		}
	}

	return soundID;
}

string ParseDescription(Action* act)
{
	size_t l_idx = act->name.find("{");
	size_t r_idx = act->name.find("}");
	if (l_idx == string::npos || r_idx == string::npos || l_idx > r_idx) return "";
	size_t start_idx = l_idx + 1;
	size_t len = r_idx - start_idx;
	string desc_string = act->name.substr(start_idx, len);
	act->name.replace(l_idx, len + 2, "");
	return desc_string;
}

int ParseMapColor(Action* act,
	const string& key_string)
{
	std::regex pattern("%" + key_string + "-([a-f0-9]{1,4})%",
		std::regex_constants::ECMAScript | std::regex_constants::icase);
	int         color = UNDEFINED_COLOR;
	std::smatch the_match;

	if (std::regex_search(act->name, the_match, pattern))
	{
		color = stoi(the_match[1].str(), nullptr, 16);
		act->name.replace(
			the_match.prefix().length(),
			the_match[0].length(),
			"");
	}
	return color;
}

const string Condition::tokenDelims = "<=>~";

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
	string              token)
{
	vector<Condition*> endConditions;
	int                i;

	// Since we don't have a real parser, things will break if [!()] appear in
	// the middle of a token (e.g. "(X AND Y)(A AND B)")

	// Look for syntax characters at the beginning of the token
	for (i = 0; i < (int)token.length(); i++)
	{
		if (token[i] == '!') { Condition::AddNonOperand(conditions, new NegationOperator()); }
		else if (token[i] == '(') { Condition::AddNonOperand(conditions, new LeftParen()); }
		else if (token[i] == ')') { Condition::AddNonOperand(conditions, new RightParen()); }
		else { break; }
	}
	token.erase(0, i);

	// Look for syntax characters at the end of the token
	for (i = token.length() - 1; i >= 0; i--)
	{
		if (token[i] == '!') { endConditions.insert(endConditions.begin(), new NegationOperator()); }
		else if (token[i] == '(') { endConditions.insert(endConditions.begin(), new LeftParen()); }
		else if (token[i] == ')') { endConditions.insert(endConditions.begin(), new RightParen()); }
		else { break; }
	}
	if (i < (int)(token.length() - 1)) { token.erase(i + 1, string::npos); }

	size_t delPos = token.find_first_of(tokenDelims);
	string key;
	string delim = "";
	int    value = 0;
	int    value2 = 0;
	if (delPos != string::npos)
	{
		key = Trim(token.substr(0, delPos));
		delim = token.substr(delPos, 1);
		string valueStr = Trim(token.substr(delPos + 1));
		if (valueStr.length() > 0)
		{
			// Get min/max values if a range is given
			if (delim == "~" && valueStr.find("-") != string::npos)
			{
				auto rangeDelim = valueStr.find("-");
				stringstream ss1(valueStr.substr(0, rangeDelim));
				valueStr.erase(0, rangeDelim + 1);
				stringstream ss2(valueStr);
				if ((ss1 >> value).fail() || (ss2 >> value2).fail())
				{
					return; // TODO: returning errors
				}
			}
			else {
				stringstream ss(valueStr);
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
	stringstream number_ss("");
	int cond_num = -1;
	std::regex multi_reg("([0-9]{1,10}),([0-9]{1,10})", std::regex_constants::ECMAScript | std::regex_constants::icase);
	std::smatch multi_match;

	FilterCondition condition = COND_NULL;
	if (condition_map.find(key) != condition_map.end())
	{
		condition = condition_map[key];
	}
	else if (keylen >= 3 && !(isupper(key[0]) || isupper(key[1]) || isupper(key[2])))
	{
		condition = COND_ITEMCODE;
	}
	else if (key.find('+') != std::string::npos)
	{
		condition = COND_ADD;
	}
	else if (key.compare(0, 2, "SK") == 0)
	{
		condition = COND_SK;
		number_ss = stringstream(key.substr(2));
	}
	else if (key.compare(0, 2, "OS") == 0)
	{
		condition = COND_OS;
		number_ss = stringstream(key.substr(2));
	}
	else if (key.compare(0, 4, "CHSK") == 0)
	{
		condition = COND_CHSK;
		number_ss = stringstream(key.substr(4));
	}
	else if (key.compare(0, 4, "CLSK") == 0)
	{
		condition = COND_CLSK;
		number_ss = stringstream(key.substr(4));
	}
	else if (key.compare(0, 5, "TABSK") == 0)
	{
		condition = COND_TABSK;
		number_ss = stringstream(key.substr(5));
	}
	else if (key.compare(0, 4, "STAT") == 0)
	{
		condition = COND_STAT;
		number_ss = stringstream(key.substr(4));
	}
	else if (key.compare(0, 8, "CHARSTAT") == 0)
	{
		condition = COND_CHARSTAT;
		number_ss = stringstream(key.substr(8));
	}
	else if (key.compare(0, 5, "MULTI") == 0)
	{
		condition = COND_MULTI;
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
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MINIMUMDAMAGE, 0, operation, value, value2));
		break;
	case COND_MAXDMG:
		Condition::AddOperand(conditions, new ItemStatCondition(STAT_MAXIMUMDAMAGE, 0, operation, value, value2));
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
	case COND_ITEMCODE:
		Condition::AddOperand(conditions, new ItemCodeCondition(key.substr(0, 4).c_str()));
		break;
	case COND_ADD:
		Condition::AddOperand(conditions, new AddCondition(key, operation, value));
		break;

	case COND_NULL:
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
		itemStat == STAT_SECONDARYMAXIMUMDAMAGE			// return base max 2h weapon damage
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
	static regex statRegex("([A-Z_]+)(?:(\\d{1,9})(?:,(\\d{1,9}))?)?", regex::ECMAScript);
	codes.clear();
	codes = split(key, '+');
	for (auto code : codes)
	{
		smatch match;
		if (regex_search(code, match, statRegex)) {
			if (skills.find(match[1]) == skills.end()) {
				continue;
			}
			DWORD id = skills.find(match[1])->second.id;
			DWORD params = skills.find(match[1])->second.params;
			int paramCount = (match[2].length() != 0) + (match[3].length() != 0);
			if (params != paramCount) {
				continue;
			}
			int param1 = match[2].length() > 0 ? stoi(match[2].str()) : id;
			int param2 = match[3].length() > 0 ? stoi(match[3].str()) : 0;
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
		else if (
			stat == STAT_ENHANCEDDEFENSE ||				// return 0
			stat == STAT_ENHANCEDMAXIMUMDAMAGE ||		// return 0
			stat == STAT_ENHANCEDMINIMUMDAMAGE ||		// return 0
			stat == STAT_MINIMUMDAMAGE ||				// return base min 1h weapon damage
			stat == STAT_MAXIMUMDAMAGE ||				// return base max 1h weapon damage
			stat == STAT_SECONDARYMINIMUMDAMAGE ||		// return base min 2h weapon damage
			stat == STAT_SECONDARYMAXIMUMDAMAGE			// return base max 2h weapon damage
			)
		{
			tmpVal = GetStatFromList(uInfo, stat);
		}
		value += tmpVal;
	}

	return IntegerCompare(value, operation, targetStat);
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
