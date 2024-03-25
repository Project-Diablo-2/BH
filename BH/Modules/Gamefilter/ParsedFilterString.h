#pragma once
#include <windows.h>
#include <set>
#include <string>

struct GameListEntry;

// ParsedFilterString can tell if a GameList entry matches the game name
// and any additional (optional) parameters recognized by this parser.
//
// "gs:1 baal"         nightmare baal runs on game server 1
// "trist gs:1,2"      normal trist runs on game servers 1 & 2
// "gs:1,2"            all games on game servers 1 & 2
// "bk5"               search all servers on all difficulties for a bk5 ring
class ParsedFilterString
{
public:
	ParsedFilterString(const char* filter = NULL);
	void Parse(const char* filter);
	bool IsIncluded(const GameListEntry* entry) const;

private:
	void ParseNoClear(const char* filter);
	bool TryParseAnyParams(const char* begin, const char* end);
	bool ParseGameServerParams(const char* begin, const char* end);

private:
	std::set<std::string> servers;		// empty set allows any server (the server names are uppercase)
	std::string gameName;				// empty string allows any game name (gameName is uppercase)
};
