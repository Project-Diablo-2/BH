#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <map>
#include <list>
#include "Common.h"

using namespace std;

struct Toggle {
	unsigned int toggle;
	bool state;
};

enum ConfigType {
	CTBoolean,
	CTString,
	CTInt,
	CTKey,
	CTToggle,
	CTArray,
	CTAssoc,
	CTAssocInt,
	CTAssocBool
};

class ConfigEntry {
public:
	ConfigType type;
	std::string key;
	std::string value;
	std::string comment;
	int line;
	void* pointer;
	Toggle* toggle;


};

inline bool operator< (const ConfigEntry& lhs, const ConfigEntry& rhs) {
	// std::tuple's lexicographic ordering does all the actual work for you
	// and using std::tie means no actual copies are made
	return std::tie(lhs.value, lhs.key) < std::tie(rhs.value, rhs.key);
}


class Config {
private:
	std::string configName;
	std::map<std::string, ConfigEntry> contents;
	vector<pair<string, string>> orderedKeyVals;

	static bool HasChanged(ConfigEntry entry, string& value);
	static bool StringToBool(std::string input);
public:
	Config(std::string name) : configName(name) {};

	//Parse the config file and store results
	bool Parse();
	bool Write();
	std::list<std::string> GetDefinedKeys();

	std::string GetConfigName();
	void SetConfigName(std::string name);

	//Functions to read values from the configuration
	bool				ReadBoolean(std::string key, bool& value);
	std::string			ReadString(std::string key, std::string& value);
	int					ReadInt(std::string key, int& value);
	unsigned int    	ReadInt(std::string key, unsigned int& value);
	unsigned int		ReadKey(std::string key, std::string toggle, unsigned int &value);
	Toggle				ReadToggle(std::string key, std::string toggle, bool defaultState, Toggle& value);
	std::vector<string> ReadArray(std::string key, std::vector<string>& value);
	map<string, string> ReadAssoc(std::string key, std::map<string, string>& value);
	map<string, unsigned int> ReadAssoc(std::string key, std::map<string, unsigned int>& value);
	map<string, bool> ReadAssoc(std::string key, std::map<string, bool>& value);
	vector<pair<string, string>> ReadMapList(std::string key, vector<pair<string,string>>& value);
};