// Modules/Party: auto-party and (hardcore) auto corpse-loot permission. Every 25 game loops BH looks at
// the party roster and does at most one party action: accept an invitation into the game's main
// party (the one with the lowest party id), invite unpartied players when the player is in that
// party (or nobody has a party yet), or leave a different party. In hardcore it also grants every
// other player permission to loot the player's corpse, once each.
#include <cstring>
#include <vector>

#include "doctest/doctest.h"

#include "BH.h"
#include "D2Helpers.h"
#include "D2Ptrs.h"
#include "FakeEngine.h"
#include "Modules/Party/Party.h"

namespace {

const WORD kNoParty = INVALID_PARTY_ID;
const DWORD kAccept = 2;  // D2CLIENT_ClickParty mode used for both invite and accept

RosterUnit Player(const char* name, DWORD id, WORD partyId, DWORD flags = PARTY_NOT_IN_PARTY) {
	RosterUnit roster;
	std::memset(&roster, 0, sizeof(roster));
	strcpy_s(roster.szName, name);
	roster.dwUnitId = id;
	roster.wPartyId = partyId;
	roster.dwPartyFlags = flags != PARTY_NOT_IN_PARTY ? flags : (partyId == kNoParty ? PARTY_NOT_IN_PARTY : PARTY_IN_PARTY);
	roster.wLevel = 90;
	return roster;
}

// The game's roster: the player (unit id 1, fake::Player()) first, then the others.
struct Game {
	std::vector<RosterUnit> roster;
	BnetData bnet;
	Party module;

	explicit Game(std::vector<RosterUnit> players, bool hardcore = false) : roster(players) {
		for (size_t i = 0; i + 1 < roster.size(); i++) {
			roster[i].pNext = &roster[i + 1];
		}
		fake::Var(Var_D2CLIENT_PlayerUnitList) = &roster[0];
		std::memset(&bnet, 0, sizeof(bnet));
		bnet.nCharFlags = hardcore ? PLAYER_TYPE_HARDCORE : 0;
		fake::Var(Var_D2LAUNCH_BnData) = &bnet;
		App.party.autoParty.toggle.isEnabled = true;
		App.party.autoCorpseLoot.toggle.isEnabled = true;
		module.OnLoad();
	}

	RosterUnit& operator[](size_t i) {
		return roster[i];
	}

	// Runs one game loop; returns the party clicks it made.
	std::vector<std::pair<RosterUnit*, DWORD>> Loop() {
		size_t before = fake::PartyClicks().size();
		module.OnLoop();
		return std::vector<std::pair<RosterUnit*, DWORD>>(fake::PartyClicks().begin() + before, fake::PartyClicks().end());
	}

	std::vector<std::pair<RosterUnit*, DWORD>> Click(size_t i) {
		return { std::make_pair(&roster[i], kAccept) };
	}
};

using Clicks = std::vector<std::pair<RosterUnit*, DWORD>>;

std::vector<BYTE> LootPermission(DWORD unitId) {
	std::vector<BYTE> packet = { 0x5d, 1, 1, 0, 0, 0, 0 };
	std::memcpy(&packet[3], &unitId, sizeof(unitId));
	return packet;
}

}  // namespace

TEST_SUITE("Party") {
	TEST_CASE("accepts an invitation when nobody has a party yet") {
		Game game({ Player("Me", 1, kNoParty), Player("Host", 2, kNoParty, PARTY_INVITED_YOU) });
		CHECK(game.Loop() == game.Click(1));
	}

	TEST_CASE("accepts an invitation from a member of the game's party") {
		Game game({ Player("Me", 1, kNoParty), Player("Lead", 2, 5, PARTY_IN_PARTY | PARTY_INVITED_YOU) });
		CHECK(game.Loop() == game.Click(1));
	}

	TEST_CASE("with two parties, only accepts the invitation into the one with the lowest id") {
		Game game({
			Player("Me", 1, kNoParty),
			Player("Other", 2, 7, PARTY_IN_PARTY | PARTY_INVITED_YOU),
			Player("Leader", 3, 5, PARTY_IN_PARTY | PARTY_INVITED_YOU),
		});
		CHECK(game.Loop() == game.Click(2));
	}

	TEST_CASE("does not accept an invitation into a party other than the lowest-id one") {
		Game game({
			Player("Me", 1, kNoParty),
			Player("Other", 2, 7, PARTY_IN_PARTY | PARTY_INVITED_YOU),
			Player("Leader", 3, 5),
		});
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 0);
	}

	TEST_CASE("invites an unpartied player when nobody has a party yet") {
		Game game({ Player("Me", 1, kNoParty), Player("Solo", 2, kNoParty) });
		CHECK(game.Loop() == game.Click(1));
	}

	TEST_CASE("does not invite a player again who already has an invitation from the player") {
		Game game({ Player("Me", 1, kNoParty), Player("Solo", 2, kNoParty, PARTY_INVITED_BY_YOU) });
		CHECK(game.Loop().empty());
	}

	TEST_CASE("invites unpartied players into the game's party when the player is in it") {
		Game game({ Player("Me", 1, 5), Player("Mate", 2, 5), Player("Solo", 3, kNoParty) });
		CHECK(game.Loop() == game.Click(2));
	}

	TEST_CASE("waits for an invitation when others have a party and the player has none") {
		Game game({ Player("Me", 1, kNoParty), Player("Lead", 2, 5), Player("Solo", 3, kNoParty) });
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 0);
	}

	TEST_CASE("leaves a party that is not the game's main party") {
		Game game({ Player("Me", 1, 7), Player("Lead", 2, 5), Player("Mate", 3, 7) });
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 1);
	}

	TEST_CASE("stays in the game's main party") {
		Game game({ Player("Me", 1, 5), Player("Lead", 2, 5), Player("Other", 3, 7) });
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 0);
	}

	TEST_CASE("does nothing with auto-party turned off") {
		Game game({ Player("Me", 1, 7), Player("Host", 2, kNoParty, PARTY_INVITED_YOU), Player("Lead", 3, 5) });
		App.party.autoParty.toggle.isEnabled = false;
		App.party.autoCorpseLoot.toggle.isEnabled = false;
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 0);
	}

	TEST_CASE("does nothing while a player's roster entry is not loaded yet") {
		Game game({ Player("Me", 1, kNoParty), Player("Host", 2, kNoParty, PARTY_INVITED_YOU), Player("New", 3, kNoParty) });
		game[2].wLevel = 0;
		CHECK(game.Loop().empty());
	}

	TEST_CASE("does nothing while a player has no party id but is flagged as in a party") {
		Game game({ Player("Me", 1, kNoParty), Player("Host", 2, kNoParty, PARTY_INVITED_YOU), Player("Odd", 3, kNoParty, PARTY_IN_PARTY) });
		CHECK(game.Loop().empty());
	}

	// BUG: the sanity check for "a party id but not in a party" tests `dwPartyFlags & PARTY_NOT_IN_PARTY`,
	// and PARTY_NOT_IN_PARTY is 0, so it never fires. With that half-updated roster entry (the window
	// the code comment describes) the player then leaves their party for a party that does not exist.
	TEST_CASE("does nothing while a player has a party id but is flagged as not in a party" * doctest::should_fail()) {
		Game game({ Player("Me", 1, 7), Player("Mate", 2, 7), Player("Odd", 3, 5) });
		game[2].dwPartyFlags = PARTY_NOT_IN_PARTY;
		CHECK(game.Loop().empty());
		CHECK(fake::PartyLeaves() == 0);
	}

	TEST_CASE("acts at most once every 25 game loops") {
		Game game({ Player("Me", 1, kNoParty), Player("Solo", 2, kNoParty) });
		CHECK(game.Loop().size() == 1);
		for (int loop = 2; loop <= 25; loop++) {
			CAPTURE(loop);
			CHECK(game.Loop().empty());
		}
		CHECK(game.Loop().size() == 1);  // loop 26
	}

	TEST_CASE("in hardcore, grants each other player corpse-loot permission once") {
		Game game({ Player("Me", 1, 5), Player("Mate", 2, 5), Player("Other", 3, 5) }, true);
		game.Loop();
		std::vector<std::vector<BYTE>> expected = { LootPermission(2), LootPermission(3) };
		CHECK(fake::SentPackets() == expected);

		for (int loop = 2; loop <= 26; loop++) {
			game.Loop();
		}
		CHECK(fake::SentPackets() == expected);
	}

	TEST_CASE("in hardcore, grants permission again to a player who left and came back") {
		Game game({ Player("Me", 1, 5), Player("Mate", 2, 5) }, true);
		game.Loop();
		REQUIRE(fake::SentPackets().size() == 1);

		game[0].pNext = nullptr;  // Mate leaves
		for (int loop = 2; loop <= 26; loop++) {
			game.Loop();
		}
		game[0].pNext = &game[1];  // and comes back
		for (int loop = 27; loop <= 51; loop++) {
			game.Loop();
		}
		std::vector<std::vector<BYTE>> expected = { LootPermission(2), LootPermission(2) };
		CHECK(fake::SentPackets() == expected);
	}

	TEST_CASE("no corpse-loot permission in softcore or with auto corpse-loot off") {
		SUBCASE("softcore") {
			Game game({ Player("Me", 1, 5), Player("Mate", 2, 5) }, false);
			game.Loop();
			CHECK(fake::SentPackets().empty());
		}
		SUBCASE("turned off") {
			Game game({ Player("Me", 1, 5), Player("Mate", 2, 5) }, true);
			App.party.autoCorpseLoot.toggle.isEnabled = false;
			game.Loop();
			CHECK(fake::SentPackets().empty());
		}
	}
}
