#include "ParsedFilterString.h"
#include "GameFilter.h"
#include <vector>
#include <map>

static std::string UpperString(const char* begin, const char* end)
{
	if (begin >= end)
		return "";
	std::string s;
	s.reserve(end - begin);
	for (const char* p = begin; p < end; p++)
		s.push_back(::toupper(*p));
	return s;
}

static std::string UpperString(const std::string s) {
	return UpperString(s.c_str(), s.c_str()+s.size());
}

bool ParsedFilterString::IsIncluded(const GameListEntry* entry) const
{
	if (!servers.empty()) {
		std::string serverName = std::to_string(entry->gs + 1);
		if (servers.find(serverName) == servers.end())
			return false;
	}

	if (!gameName.empty() && !strstr(UpperString(entry->sGameName).c_str(), gameName.c_str()))
		return false;

	return true;
}

ParsedFilterString::ParsedFilterString(const char* filter)
{
	ParseNoClear(filter);
}

void ParsedFilterString::Parse(const char* filter)
{
	servers.clear();
	gameName.clear();
	ParseNoClear(filter);
}

void ParsedFilterString::ParseNoClear(const char* filter)
{
	if (!filter)
		return;
	const char* pEnd = filter;

	// an iteration of this loop processes one of the space separated words of the filter string
	for (const char* pSpace = pEnd; *pSpace; pSpace = pEnd)
	{
		const char* pWord = pSpace;
		// skipping spaces in front of the word
		while (*pWord && ::isspace(*pWord))
			pWord++;

		pEnd = pWord;
		// skipping to the end of the word
		while (*pEnd && !::isspace(*pEnd))
			pEnd++;

		// zero token length - we reached the end of the input string
		if (pWord == pEnd)
			break;

		if (TryParseAnyParams(pWord, pEnd))
			continue;

		if (!gameName.empty())
			gameName.append(pSpace, pWord);
		gameName.append(UpperString(pWord, pEnd));
	}
}

bool ParsedFilterString::TryParseAnyParams(const char* begin, const char* end)
{
	// TODO: if you need additional params like "myparam:p1,p2" in the future then
	// add a method like ParseGameServerParams and call it from here in a similar way:
	if (ParseGameServerParams(begin, end))
		return true;
	return false;
}

// ParseParamList tries to parse a string that looks like "ParamPrefix:Param1,Param2,Param3,...".
// Returns true on success along with the uppercase parameter values in the params vector.
// The params vector may be empty on return. Empty strings are never pushed into the vector.
static bool ParseParamList(const char* paramPrefix, const char* begin, const char* end, std::vector<std::string>& params)
{
	size_t prefixLen = strlen(paramPrefix);
	if (prefixLen > size_t(end - begin) || _strnicmp(begin, paramPrefix, prefixLen) != 0)
		return false;

	for (const char* p = begin + prefixLen; p < end;)
	{
		const char* paramBegin = p;
		while (p < end && *p != L',')
			p++;
		if (p > paramBegin)
			params.push_back(UpperString(paramBegin, p));
		// skipping the comma, not a problem if we reached the end of the input string
		p++;
	}
	return true;
}

bool ParsedFilterString::ParseGameServerParams(const char* begin, const char* end)
{
	std::vector<std::string> params;
	if (!ParseParamList("gs:", begin, end, params))
		return false;
	for (auto it = params.begin(); it != params.end(); ++it)
		servers.insert(*it);
	return true;
}
