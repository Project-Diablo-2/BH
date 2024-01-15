#pragma once
#include <string>
#include <iostream>
#include <fstream>
#include <Windows.h>
#include <map>
#include <list>
#include "Common.h"
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::ordered_json;

struct Toggle {
	unsigned int hotkey;
	bool isEnabled;
};

struct SettingsInt {
	union {
		int value;
		unsigned int uValue;
	};
	int defValue;
	int minValue;
	int maxValue;
};

struct SettingsFloat {
	float value;
	float defValue;
	float minValue;
	float maxValue;
};

struct SettingsBool {
	bool value;
	bool defValue;
};

struct SettingsString {
	std::string value;
	std::string defValue;
};

struct SettingsToggle {
	Toggle toggle;
	Toggle defToggle;
};

struct SettingsKey {
	unsigned int hotkey;
	unsigned int defHotkey;
};

struct SettingsArray {
	std::vector<std::string> values;
	std::vector<std::string> defValues;
};

struct SettingsAssoc {
	std::map<std::string, std::string> values;
	std::map<std::string, std::string> defValues;
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
	void SaveConfig();
	void LoadConfig();
	Config(std::string name) : configName(name) {};

	//Parse the config file and store results
	bool Parse();
	bool Write();
	std::list<std::string> GetDefinedKeys();

	std::string GetConfigName();
	void SetConfigName(std::string name);

	//Functions to read values from the configuration
	int GetInt(json::json_pointer sectionKey, std::string key, SettingsInt def);
	float GetFloat(json::json_pointer sectionKey, std::string key, SettingsFloat def);
	bool GetBool(json::json_pointer sectionKey, std::string key, SettingsBool def);
	std::string GetString(json::json_pointer sectionKey, std::string key, SettingsString def);
	Toggle GetToggle(json::json_pointer sectionKey, std::string key, SettingsToggle def);
	unsigned int GetKey(json::json_pointer sectionKey, std::string key, SettingsKey def);
	std::vector<std::string> GetArray(json::json_pointer sectionKey, std::string key, SettingsArray def);
	std::map<std::string, std::string> GetAssoc(json::json_pointer sectionKey, std::string key, SettingsAssoc def);

	//Functions to read values from the configuration (OLD)
	bool				ReadBoolean(std::string key, bool& value);
	std::string			ReadString(std::string key, std::string& value);
	int					ReadInt(std::string key, int& value);
	unsigned int		ReadInt(std::string key, unsigned int& value, unsigned int defaultValue = 0);
	unsigned int		ReadKey(std::string key, std::string toggle, unsigned int &value);
	Toggle				ReadToggle(std::string key, std::string toggle, bool defaultState, Toggle& value);
	std::vector<string> ReadArray(std::string key, std::vector<string>& value);
	map<string, string> ReadAssoc(std::string key, std::map<string, string>& value);
	map<string, unsigned int> ReadAssoc(std::string key, std::map<string, unsigned int>& value);
	map<string, bool> ReadAssoc(std::string key, std::map<string, bool>& value);
	vector<pair<string, string>> ReadMapList(std::string key, vector<pair<string,string>>& value);
};