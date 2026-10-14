#include "doctest/doctest.h"

#include <cstdio>
#include <string>
#include <vector>

#include "BH.h"
#include "FakeEngine.h"
#include "Modules/Gamefilter/Gamefilter.h"
#include "Modules/Gamefilter/ParsedFilterString.h"

namespace {

// Status bits of an MCP_GAMELIST entry (see Gamefilter.cpp): neither bit = normal difficulty.
const DWORD STATUS_NIGHTMARE = 0x1000;
const DWORD STATUS_HELL = 0x2000;

// `gs` is the zero-based game server index the realm sends; players see it as server gs + 1.
GameListEntry Game(const char* name, BYTE gs = 0, const char* desc = "") {
	GameListEntry e = {};
	e.bPlayers = 1;
	e.gs = gs;
	e.sGameName = name;
	e.sGameDesc = desc;
	return e;
}

bool Shows(const char* filter, const GameListEntry& entry) {
	ParsedFilterString parsed(filter);
	return parsed.IsIncluded(&entry);
}

// S>C 0x05 MCP_GAMELIST as PD2's realm sends it:
// id(2) index(4) players(1) status(4) gs(1) lootAlloc(1) name\0 desc\0
std::vector<BYTE> GameListPacket(const char* name, BYTE players, DWORD status, BYTE gs, BYTE lootAlloc = 0,
	const char* desc = "") {
	std::vector<BYTE> p(14, 0);
	p[0] = 0x05;
	// Request id and list index (bytes 1-6) are never used by BH; left zero.
	p[7] = players;
	memcpy(&p[8], &status, sizeof(status));
	p[12] = gs;
	p[13] = lootAlloc;
	p.insert(p.end(), name, name + strlen(name) + 1);
	p.insert(p.end(), desc, desc + strlen(desc) + 1);
	return p;
}

// The join-game screen: the game list text box (with its scroll bar) and the filter edit box
// BH creates next to it. Everything Gamefilter allocated is released when the test ends.
struct JoinScreen {
	TextBox list;
	ScrollBar scroll;
	EditBox filter;

	JoinScreen() {
		memset(&list, 0, sizeof(list));
		memset(&scroll, 0, sizeof(scroll));
		memset(&filter, 0, sizeof(filter));
		list.dwType = 4;
		list.ptScrollBar = &scroll;
		scroll.dwType = 5;
		fake::Var(Var_D2MULTI_GameListControl) = &list;
		Gamefilter::filterBox = &filter;
		// Every difficulty shown unless a test hides one (independent of the settings' defaults).
		App.bnet.showNormalDiff.value = true;
		App.bnet.showNightmareDiff.value = true;
		App.bnet.showHellDiff.value = true;
	}

	~JoinScreen() {
		for (auto it = Gamefilter::gameList.begin(); it != Gamefilter::gameList.end(); ++it)
			delete *it;
		Gamefilter::gameList.clear();
		Gamefilter::filterVector.clear();
		Gamefilter::gServerVector.clear();
		FreeLines();
		Gamefilter::filterBox = NULL;
		fake::Var(Var_D2MULTI_GameListControl) = NULL;
	}

	void FreeLines() {
		ControlText* t = list.pFirstText;
		while (t) {
			ControlText* next = t->pNext;
			delete[] t->wText[0];
			delete[] t->wText[1];
			delete t;
			t = next;
		}
		list.pFirstText = list.pLastText = list.pSelectedText = NULL;
	}

	void SetFilterText(const wchar_t* text) { wcscpy_s(filter.wText, 256, text); }

	// Returns whether BH blocked the packet.
	bool Receive(std::vector<BYTE> packet) {
		bool blocked = false;
		module.OnRealmPacketRecv(packet.data(), &blocked);
		return blocked;
	}

	// The user typing `key` into the filter box (whose text is what was typed so far).
	void Type(char key) {
		Gamefilter::Filterbox_InputHandler(&filter, (DWORD)wcslen(filter.wText), &key);
	}

	// Game names of the visible lines, top to bottom.
	std::vector<std::wstring> Names() const {
		std::vector<std::wstring> names;
		for (ControlText* t = list.pFirstText; t; t = t->pNext)
			names.push_back(t->wText[0]);
		return names;
	}

	Gamefilter module;
};

std::vector<std::wstring> Lines(std::initializer_list<const wchar_t*> names) {
	return std::vector<std::wstring>(names.begin(), names.end());
}

}  // namespace

TEST_SUITE("GameFilter") {

// ---- ParsedFilterString: game name ----------------------------------------------------------

TEST_CASE("An empty filter shows every game on every server") {
	GameListEntry g = Game("Baal Run 12", 3);
	CHECK(Shows("", g));
	CHECK(Shows("   ", g));
	CHECK(Shows(" \t\r\n", g));
}

TEST_CASE("A word matches the game name as a case-insensitive substring anywhere in it") {
	CHECK(Shows("baal", Game("baal")));
	CHECK(Shows("baal", Game("BAAL-RUN")));
	CHECK(Shows("baal", Game("fastBaal3")));
	CHECK(Shows("baal", Game("run-baal")));
	CHECK(Shows("BaAl", Game("bAaL")));
	CHECK(Shows("bk5", Game("Shop BK5 ring")));
	CHECK_FALSE(Shows("baal", Game("bal run")));
	CHECK_FALSE(Shows("baal", Game("ba al")));
	CHECK_FALSE(Shows("baal", Game("")));
	// Digits and punctuation are compared as typed.
	CHECK(Shows("cs-1", Game("CS-12")));
	CHECK_FALSE(Shows("cs-1", Game("cs_12")));
}

TEST_CASE("A filter longer than the game name does not match it") {
	CHECK_FALSE(Shows("baals", Game("baal")));
	CHECK_FALSE(Shows("xbaal", Game("baal")));
}

TEST_CASE("Only the game name is searched, not the game description") {
	CHECK_FALSE(Shows("baal", Game("chaos12", 0, "baal runs")));
	CHECK(Shows("chaos", Game("chaos12", 0, "baal runs")));
}

TEST_CASE("Several words must appear together, in order, as one phrase") {
	CHECK(Shows("baal run", Game("Baal Run 5")));
	CHECK(Shows("baal run", Game("fast baal runs")));
	CHECK_FALSE(Shows("baal run", Game("Run Baal 5")));
	CHECK_FALSE(Shows("baal run", Game("BaalRun5")));
	CHECK_FALSE(Shows("baal run", Game("baal-run")));
	CHECK(Shows("a b c", Game("xa b cx")));
	CHECK_FALSE(Shows("a b c", Game("a b")));
}

TEST_CASE("Whitespace around the filter is ignored but whitespace inside a phrase is kept") {
	CHECK(Shows("  baal  ", Game("baal")));
	CHECK(Shows("\tbaal\n", Game("BAAL")));
	CHECK(Shows("baal  run", Game("baal  run")));
	CHECK_FALSE(Shows("baal  run", Game("baal run")));
}

// ---- ParsedFilterString: game servers --------------------------------------------------------

TEST_CASE("gs:N shows only games on server N, where server N is the realm's zero-based index N-1") {
	CHECK(Shows("gs:1", Game("a", 0)));
	CHECK_FALSE(Shows("gs:1", Game("a", 1)));
	CHECK(Shows("gs:2", Game("a", 1)));
	CHECK_FALSE(Shows("gs:2", Game("a", 0)));
	CHECK_FALSE(Shows("gs:2", Game("a", 2)));
}

TEST_CASE("gs: accepts a comma separated list of servers") {
	CHECK(Shows("gs:1,2", Game("a", 0)));
	CHECK(Shows("gs:1,2", Game("a", 1)));
	CHECK_FALSE(Shows("gs:1,2", Game("a", 2)));
	CHECK(Shows("gs:3,1,7", Game("a", 6)));
	CHECK_FALSE(Shows("gs:3,1,7", Game("a", 4)));
}

TEST_CASE("Server numbers are compared whole, never as a prefix") {
	CHECK(Shows("gs:10", Game("a", 9)));
	CHECK_FALSE(Shows("gs:10", Game("a", 0)));
	CHECK_FALSE(Shows("gs:1", Game("a", 9)));
	CHECK_FALSE(Shows("gs:1", Game("a", 10)));
}

TEST_CASE("The highest server index the realm can send is server 256, not server 0") {
	CHECK(Shows("gs:256", Game("a", 255)));
	CHECK_FALSE(Shows("gs:0", Game("a", 255)));
	CHECK_FALSE(Shows("gs:255", Game("a", 255)));
}

TEST_CASE("The gs: prefix is case-insensitive") {
	CHECK(Shows("GS:2", Game("a", 1)));
	CHECK(Shows("Gs:2", Game("a", 1)));
	CHECK(Shows("gS:2", Game("a", 1)));
	CHECK_FALSE(Shows("GS:2", Game("a", 0)));
}

TEST_CASE("Empty entries in a gs: list are skipped and a bare gs: allows every server") {
	CHECK(Shows("gs:", Game("a", 0)));
	CHECK(Shows("gs:", Game("a", 5)));
	CHECK(Shows("gs:,", Game("a", 5)));
	CHECK(Shows("gs:,1,,", Game("a", 0)));
	CHECK_FALSE(Shows("gs:,1,,", Game("a", 1)));
	CHECK(Shows("gs:1,,3", Game("a", 2)));
	CHECK_FALSE(Shows("gs:1,,3", Game("a", 1)));
}

TEST_CASE("A gs: list naming no real server shows nothing") {
	CHECK_FALSE(Shows("gs:x", Game("a", 0)));
	CHECK_FALSE(Shows("gs:one", Game("one", 0)));
}

TEST_CASE("Repeated gs: words add up to one set of servers") {
	CHECK(Shows("gs:1 gs:3", Game("a", 0)));
	CHECK(Shows("gs:1 gs:3", Game("a", 2)));
	CHECK_FALSE(Shows("gs:1 gs:3", Game("a", 1)));
}

TEST_CASE("Words that only look like a server parameter are part of the game name") {
	// No colon, or the prefix not at the start of the word: searched for in the name, no server limit.
	CHECK(Shows("gs1", Game("GS1 baal", 4)));
	CHECK_FALSE(Shows("gs1", Game("baal", 0)));
	CHECK(Shows("xgs:1", Game("xgs:1", 4)));
	CHECK_FALSE(Shows("xgs:1", Game("x", 0)));
	CHECK(Shows("gs", Game("gs", 4)));
	// "gs:" inside a word is not a parameter either.
	CHECK(Shows("a,gs:1", Game("A,GS:1", 3)));
}

// ---- ParsedFilterString: name and server together --------------------------------------------

TEST_CASE("A name and a server list must both match, in any order") {
	// The examples documented in ParsedFilterString.h.
	CHECK(Shows("gs:1 baal", Game("Baal-5", 0)));
	CHECK_FALSE(Shows("gs:1 baal", Game("Baal-5", 1)));
	CHECK_FALSE(Shows("gs:1 baal", Game("Chaos-5", 0)));
	CHECK(Shows("trist gs:1,2", Game("trist12", 0)));
	CHECK(Shows("trist gs:1,2", Game("trist12", 1)));
	CHECK_FALSE(Shows("trist gs:1,2", Game("trist12", 2)));
	CHECK_FALSE(Shows("trist gs:1,2", Game("cows", 1)));
	CHECK(Shows("gs:1,2", Game("anything", 1)));
}

TEST_CASE("A server parameter between words is removed from the phrase") {
	CHECK(Shows("baal gs:1 run", Game("baal run", 0)));
	CHECK_FALSE(Shows("baal gs:1 run", Game("baal run", 1)));
	// The parameter's own text is not part of the phrase searched for.
	CHECK_FALSE(Shows("baal gs:1 run", Game("baal gs:1 run", 0)));
}

// ---- Gamefilter: the join-game list -----------------------------------------------------------

TEST_CASE("A game list entry becomes a line with its name, player count and server label") {
	JoinScreen screen;
	CHECK_FALSE(screen.Receive(GameListPacket("Baal-12", 3, 0, 1, 1, "fast baal")));

	REQUIRE(screen.list.pFirstText != NULL);
	ControlText* line = screen.list.pFirstText;
	CHECK(std::wstring(line->wText[0]) == L"Baal-12");
	CHECK(std::wstring(line->wText[1]) == L"3");
	CHECK(screen.list.pLastText == line);
	CHECK(screen.list.pSelectedText == line);
	CHECK(screen.list.dwMaxLines == 1);

	REQUIRE(Gamefilter::gameList.size() == 1);
	const GameListEntry* entry = Gamefilter::gameList.front();
	// The fields the list, server label and loot "A" marker are drawn from.
	CHECK(entry->bPlayers == 3);
	CHECK(entry->gs == 1);
	CHECK(entry->lootAlloc == 1);
	CHECK(entry->sGameName == "Baal-12");

	REQUIRE(Gamefilter::filterVector.size() == 1);
	CHECK(Gamefilter::filterVector[0] == entry);
	REQUIRE(Gamefilter::gServerVector.size() == 1);
	CHECK(std::wstring(Gamefilter::gServerVector[0]) == L"gs2");
}

TEST_CASE("Games are listed in the order the realm sends them") {
	JoinScreen screen;
	screen.Receive(GameListPacket("first", 1, 0, 0));
	screen.Receive(GameListPacket("second", 2, 0, 8));
	screen.Receive(GameListPacket("third", 8, 0, 4));

	CHECK(screen.Names() == Lines({ L"first", L"second", L"third" }));
	CHECK(screen.list.pSelectedText == screen.list.pFirstText);
	CHECK(screen.list.dwMaxLines == 3);
	REQUIRE(Gamefilter::gServerVector.size() == 3);
	CHECK(std::wstring(Gamefilter::gServerVector[0]) == L"gs1");
	CHECK(std::wstring(Gamefilter::gServerVector[1]) == L"gs9");
	CHECK(std::wstring(Gamefilter::gServerVector[2]) == L"gs5");
	REQUIRE(screen.list.pLastText != NULL);
	CHECK(std::wstring(screen.list.pLastText->wText[1]) == L"8");
}

TEST_CASE("Empty games are dropped and the packet is blocked") {
	JoinScreen screen;
	CHECK(screen.Receive(GameListPacket("ghost", 0, 0, 0)));
	CHECK(Gamefilter::gameList.empty());
	CHECK(screen.list.pFirstText == NULL);
	CHECK(screen.list.dwMaxLines == 0);
}

TEST_CASE("A game the list already has is not listed twice") {
	JoinScreen screen;
	CHECK_FALSE(screen.Receive(GameListPacket("baal-1", 2, 0, 0)));
	CHECK(screen.Receive(GameListPacket("baal-1", 5, 0, 3)));
	CHECK(Gamefilter::gameList.size() == 1);
	CHECK(screen.Names() == Lines({ L"baal-1" }));
	CHECK(std::wstring(screen.list.pFirstText->wText[1]) == L"2");
	// A different name is a different game.
	CHECK_FALSE(screen.Receive(GameListPacket("baal-10", 1, 0, 0)));
	CHECK(screen.Names() == Lines({ L"baal-1", L"baal-10" }));
}

TEST_CASE("Game list packets are ignored until the filter box exists") {
	JoinScreen screen;
	Gamefilter::filterBox = NULL;
	CHECK_FALSE(screen.Receive(GameListPacket("baal", 1, 0, 0)));
	CHECK(Gamefilter::gameList.empty());
	CHECK(screen.list.pFirstText == NULL);
}

TEST_CASE("Hidden difficulties are kept and come back when shown again") {
	JoinScreen screen;
	App.bnet.showNightmareDiff.value = false;
	screen.Receive(GameListPacket("norm", 1, 0, 0));
	screen.Receive(GameListPacket("night", 1, STATUS_NIGHTMARE, 0));
	screen.Receive(GameListPacket("hell", 1, STATUS_HELL, 0));
	CHECK(screen.Names() == Lines({ L"norm", L"hell" }));
	// "Games: listed/total" counts the hidden game in the total.
	CHECK(Gamefilter::gameList.size() == 3);
	CHECK(Gamefilter::filterVector.size() == 2);
	CHECK(Gamefilter::gServerVector.size() == 2);

	// Relisting (as the nightmare button does) with nightmare shown again.
	App.bnet.showNightmareDiff.value = true;
	Gamefilter::BuildGameList("");
	CHECK(screen.Names() == Lines({ L"norm", L"night", L"hell" }));
	CHECK(Gamefilter::filterVector.size() == 3);
}

TEST_CASE("Each difficulty button hides only its own difficulty") {
	SUBCASE("normal hidden") {
		JoinScreen screen;
		App.bnet.showNormalDiff.value = false;
		screen.Receive(GameListPacket("norm", 1, 0, 0));
		screen.Receive(GameListPacket("night", 1, STATUS_NIGHTMARE, 0));
		screen.Receive(GameListPacket("hell", 1, STATUS_HELL, 0));
		CHECK(screen.Names() == Lines({ L"night", L"hell" }));
	}
	SUBCASE("hell hidden") {
		JoinScreen screen;
		App.bnet.showHellDiff.value = false;
		screen.Receive(GameListPacket("norm", 1, 0, 0));
		screen.Receive(GameListPacket("night", 1, STATUS_NIGHTMARE, 0));
		screen.Receive(GameListPacket("hell", 1, STATUS_HELL, 0));
		CHECK(screen.Names() == Lines({ L"norm", L"night" }));
	}
	SUBCASE("other status bits do not change the difficulty") {
		JoinScreen screen;
		App.bnet.showNormalDiff.value = false;
		screen.Receive(GameListPacket("norm-ladder", 1, 0x0004, 0));
		screen.Receive(GameListPacket("hell-ladder", 1, STATUS_HELL | 0x0004, 0));
		CHECK(screen.Names() == Lines({ L"hell-ladder" }));
	}
}

TEST_CASE("Incoming games are matched against the text in the filter box, whatever its case") {
	JoinScreen screen;
	screen.SetFilterText(L"baal gs:2");
	screen.Receive(GameListPacket("BAAL-1", 1, 0, 1));
	screen.Receive(GameListPacket("baal-2", 1, 0, 0));
	screen.Receive(GameListPacket("cows-3", 1, 0, 1));
	screen.Receive(GameListPacket("Baal-4", 1, 0, 1));
	CHECK(screen.Names() == Lines({ L"BAAL-1", L"Baal-4" }));
	CHECK(Gamefilter::gameList.size() == 4);
}

TEST_CASE("The scroll bar scrolls once more than nine lines are listed") {
	JoinScreen screen;
	char name[8];
	for (int i = 0; i < 9; i++) {
		sprintf_s(name, "g%d", i);
		screen.Receive(GameListPacket(name, 1, 0, 0));
	}
	CHECK(screen.list.dwMaxLines == 9);
	CHECK(screen.scroll.dwScrollEntries == 0);
	screen.Receive(GameListPacket("g9", 1, 0, 0));
	CHECK(screen.list.dwMaxLines == 10);
	CHECK(screen.scroll.dwScrollEntries == 1);
	screen.Receive(GameListPacket("g10", 1, 0, 0));
	CHECK(screen.scroll.dwScrollEntries == 2);
}

// ---- Gamefilter: typing in the filter box -----------------------------------------------------

TEST_CASE("Typing in the filter box relists the known games that match the new text") {
	JoinScreen screen;
	screen.Receive(GameListPacket("baal-1", 1, 0, 0));
	screen.Receive(GameListPacket("cows-2", 1, 0, 0));
	screen.Receive(GameListPacket("bas-3", 1, 0, 2));

	// The box holds what was typed before this key.
	screen.SetFilterText(L"b");
	screen.Type('a');
	CHECK(screen.Names() == Lines({ L"baal-1", L"bas-3" }));
	CHECK(screen.list.dwMaxLines == 2);
	REQUIRE(Gamefilter::gServerVector.size() == 2);
	CHECK(std::wstring(Gamefilter::gServerVector[1]) == L"gs3");
	CHECK(Gamefilter::filterVector.size() == 2);

	screen.SetFilterText(L"ba");
	screen.Type('A');
	CHECK(screen.Names() == Lines({ L"baal-1" }));
	CHECK(screen.list.pSelectedText == screen.list.pFirstText);
	CHECK(screen.list.pLastText == screen.list.pFirstText);
	CHECK(Gamefilter::gameList.size() == 3);
}

TEST_CASE("Typing a server filter relists only that server's games") {
	JoinScreen screen;
	screen.Receive(GameListPacket("a", 1, 0, 0));
	screen.Receive(GameListPacket("b", 1, 0, 1));
	screen.Receive(GameListPacket("c", 1, 0, 2));
	screen.SetFilterText(L"GS:1,");
	screen.Type('3');
	CHECK(screen.Names() == Lines({ L"a", L"c" }));
}

TEST_CASE("Relisting resets the scroll position and keeps difficulties hidden") {
	JoinScreen screen;
	App.bnet.showHellDiff.value = false;
	char name[8];
	for (int i = 0; i < 12; i++) {
		sprintf_s(name, "run%d", i);
		screen.Receive(GameListPacket(name, 1, i % 2 ? STATUS_HELL : 0, 0));
	}
	CHECK(screen.list.dwMaxLines == 6);
	screen.list.dwCurrentLine = 4;
	screen.list.dwTopOffset = 3;
	screen.scroll.dwScrollPosition = 4;
	screen.scroll.bMovedDown = 1;

	screen.SetFilterText(L"run");
	screen.Type('1');  // "run1", "run10", "run11"; run11 is hell
	CHECK(screen.Names() == Lines({ L"run10" }));
	CHECK(screen.list.dwMaxLines == 1);
	CHECK(screen.list.dwCurrentLine == 0);
	CHECK(screen.list.dwTopOffset == 0);
	CHECK(screen.scroll.dwScrollPosition == 0);
	CHECK(screen.scroll.bMovedDown == 0);
	CHECK(screen.scroll.dwScrollEntries == 0);
}

TEST_CASE("Relisting with a filter that matches everything shows every game again") {
	JoinScreen screen;
	screen.SetFilterText(L"zz");
	char name[8];
	for (int i = 0; i < 11; i++) {
		sprintf_s(name, "g%d", i);
		screen.Receive(GameListPacket(name, 1, 0, 0));
	}
	CHECK(screen.list.pFirstText == NULL);
	screen.SetFilterText(L"");
	screen.Type('g');
	CHECK(screen.list.dwMaxLines == 11);
	CHECK(screen.scroll.dwScrollEntries == 2);
	CHECK(screen.Names().front() == L"g0");
	CHECK(screen.Names().back() == L"g10");
}

// ---- Gamefilter: character list ---------------------------------------------------------------

TEST_CASE("The character list scrolls by rows of two once more than eight characters exist") {
	struct Case {
		BYTE packetId;
		unsigned int chars;
		DWORD scrollRows;
	};
	const Case cases[] = {
		{ 0x19, 0, 0 }, { 0x19, 8, 0 }, { 0x19, 9, 1 }, { 0x19, 10, 1 }, { 0x19, 11, 2 }, { 0x17, 18, 5 },
	};
	for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
		const Case& c = cases[i];
		CAPTURE(c.chars);
		TextBox charList;
		ScrollBar scroll;
		TextBox otherText;  // a text box without a scroll bar is not the character list
		memset(&charList, 0, sizeof(charList));
		memset(&scroll, 0, sizeof(scroll));
		memset(&otherText, 0, sizeof(otherText));
		charList.dwType = 4;
		charList.ptScrollBar = &scroll;
		charList.dwCurrentLine = 6;
		scroll.dwType = 5;
		scroll.dwScrollPosition = 3;
		otherText.dwType = 4;
		otherText.dwMaxLines = 99;
		otherText.pNext = &charList;
		fake::Var(Var_D2WIN_FirstControl) = &otherText;

		BYTE packet[7] = { c.packetId, 0, 0 };
		memcpy(packet + 3, &c.chars, sizeof(c.chars));
		bool blocked = false;
		Gamefilter module;
		module.OnRealmPacketRecv(packet, &blocked);

		CHECK_FALSE(blocked);
		CHECK(charList.dwMaxLines == c.chars);
		CHECK(charList.dwCurrentLine == 0);
		CHECK(scroll.dwScrollPosition == 0);
		CHECK(scroll.dwScrollEntries == c.scrollRows);
		CHECK(otherText.dwMaxLines == 99);
		fake::Var(Var_D2WIN_FirstControl) = NULL;
	}
}

}  // TEST_SUITE
