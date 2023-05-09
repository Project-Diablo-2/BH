#pragma once
#include <Windows.h>
#include <algorithm>
#include <locale>
#include <cstdlib>
#include <fstream>
#include <map>
#include <unordered_map>
#include "Constants.h"
#include "Common.h"
#include "D2Structs.h"

/*
 * MPQInit handles the data we can initialize from MPQ files, provided we
 * are able to load StormLib. It also provides defaults in case we cannot
 * read the MPQ archive.
 */


extern unsigned int SKILL_MAX;
extern unsigned int PREFIX_MAX;
extern unsigned int SUFFIX_MAX;





struct ItemAffixProperties {
	unsigned int ID;
	unsigned int costAdd;
	unsigned int costMultiply;
};







extern std::vector<ItemAffixProperties*> AllPrefixList;
extern std::vector<ItemAffixProperties*> AllSuffixList;



bool IsInitialized();
void InitializeMPQData();
