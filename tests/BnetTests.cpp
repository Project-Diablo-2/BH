// Modules/Bnet: remembering the last game's name, password and description on joining, and, with
// "autofill next game", proposing the next game name on leaving by counting up the trailing number
// (Battle.net game names are at most 15 characters).
#include <cstring>
#include <string>

#include "doctest/doctest.h"

#include "BH.h"
#include "D2Ptrs.h"
#include "FakeEngine.h"
#include "Modules/Bnet/Bnet.h"

namespace {

std::string NextGameAfter(const std::string& lastGame) {
	App.bnet.autofillNextGame.value = true;
	App.bnet.saveLastGame.value = lastGame;
	Bnet bnet;
	bnet.OnGameExit();
	return App.bnet.saveLastGame.value;
}

struct BnetGame {
	BnetData data;
	BnetGame(const char* name, const char* pass, const char* desc) {
		std::memset(&data, 0, sizeof(data));
		strcpy_s(data.szGameName, name);
		strcpy_s(data.szGamePass, pass);
		strcpy_s(data.szGameDesc, desc);
		fake::Var(Var_D2LAUNCH_BnData) = &data;
	}
};

}  // namespace

TEST_SUITE("Bnet") {
	TEST_CASE("next game name counts up the trailing number") {
		CHECK(NextGameAfter("Baal-1") == "Baal-2");
		CHECK(NextGameAfter("cows 41") == "cows 42");
		CHECK(NextGameAfter("run9") == "run10");
		CHECK(NextGameAfter("run99") == "run100");
	}

	TEST_CASE("next game name only counts the digits at the end") {
		CHECK(NextGameAfter("a1b2") == "a1b3");
		CHECK(NextGameAfter("2baal19") == "2baal20");
	}

	TEST_CASE("a game name without a trailing number is kept") {
		CHECK(NextGameAfter("baalruns") == "baalruns");
		CHECK(NextGameAfter("run1 x") == "run1 x");
		CHECK(NextGameAfter("") == "");
	}

	TEST_CASE("next game name stays within the 15-character limit") {
		// 14 characters: room for one more digit.
		CHECK(NextGameAfter("abcdefghijkl99") == "abcdefghijkl100");
		// 15 characters: the next number still fits.
		CHECK(NextGameAfter("abcdefghijklm98") == "abcdefghijklm99");
		CHECK(NextGameAfter("abcdefghijk1234") == "abcdefghijk1235");
		// 15 characters and the next number would make it 16: start again at 1.
		CHECK(NextGameAfter("abcdefghijklm99") == "abcdefghijklm1");
		CHECK(NextGameAfter("abcdefghijklmn9") == "abcdefghijklmn1");
	}

	TEST_CASE("next game name is left alone when autofill next game is off") {
		App.bnet.autofillNextGame.value = false;
		App.bnet.saveLastGame.value = "Baal-1";
		Bnet bnet;
		bnet.OnGameExit();
		CHECK(App.bnet.saveLastGame.value == "Baal-1");
	}

	TEST_CASE("joining a game remembers its name, password and description") {
		BnetGame game("Baal-7", "pw", "lvl 90+");
		Bnet bnet;
		bnet.OnGameJoin();
		CHECK(App.bnet.saveLastGame.value == "Baal-7");
		CHECK(App.bnet.saveLastPass.value == "pw");
		CHECK(App.bnet.saveLastDesc.value == "lvl 90+");
	}

	TEST_CASE("joining a game without password or description forgets the previous ones") {
		App.bnet.saveLastPass.value = "old";
		App.bnet.saveLastDesc.value = "old desc";
		BnetGame game("Baal-7", "", "");
		Bnet bnet;
		bnet.OnGameJoin();
		CHECK(App.bnet.saveLastPass.value == "");
		CHECK(App.bnet.saveLastDesc.value == "");
	}

	TEST_CASE("joining a game with no name keeps the previous game name") {
		App.bnet.saveLastGame.value = "Baal-7";
		BnetGame game("", "", "");
		Bnet bnet;
		bnet.OnGameJoin();
		CHECK(App.bnet.saveLastGame.value == "Baal-7");
	}

	TEST_CASE("leaving a joined game proposes the next one in the series") {
		App.bnet.autofillNextGame.value = true;
		BnetGame game("Chaos-12", "", "");
		Bnet bnet;
		bnet.OnGameJoin();
		bnet.OnGameExit();
		CHECK(App.bnet.saveLastGame.value == "Chaos-13");
	}
}
