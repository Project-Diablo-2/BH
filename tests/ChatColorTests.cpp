// Modules/ChatColor: in game, whispers from players listed under "whisperColor" in BH.json are shown
// in that player's colour instead of the game's whisper style. Battle.net delivers chat as
// SID_CHATEVENT (0x0F) packets: event id at offset 4, the sender's name at offset 28, then the text.
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

#include "doctest/doctest.h"

#include "BH.h"
#include "FakeEngine.h"
#include "Modules/ChatColor/ChatColor.h"

namespace {

const DWORD kEventWhisperReceived = 0x04;
const DWORD kEventTalk = 0x05;

std::vector<BYTE> ChatEvent(DWORD eventId, const std::string& from, const std::string& text, BYTE packetId = 0x0F) {
	std::vector<BYTE> packet(28, 0);
	packet[0] = 0xFF;
	packet[1] = packetId;
	std::memcpy(&packet[4], &eventId, sizeof(eventId));
	packet.insert(packet.end(), from.begin(), from.end());
	packet.push_back(0);
	packet.insert(packet.end(), text.begin(), text.end());
	packet.push_back(0);
	WORD length = static_cast<WORD>(packet.size());
	std::memcpy(&packet[2], &length, sizeof(length));
	return packet;
}

// A ChatColor module that has seen the player join a game.
struct InGame {
	ChatColor module;
	InGame() {
		module.OnGameJoin();
	}
	bool Receive(std::vector<BYTE> packet) {
		bool block = false;
		module.OnChatPacketRecv(packet.data(), &block);
		return block;
	}
};

}  // namespace

TEST_SUITE("ChatColor") {
	TEST_CASE("a whisper from a listed player is replaced by the same text in that player's colour") {
		App.bnet.whisperColor.values["Bob"] = "1";
		InGame chat;
		CHECK(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "trade?")));
		REQUIRE(fake::Printed().size() == 1);
		CHECK(fake::Printed()[0].first == L"Bob | trade?");
		CHECK(fake::Printed()[0].second == 1);
	}

	TEST_CASE("each listed player gets their own colour") {
		App.bnet.whisperColor.values["Bob"] = "1";
		App.bnet.whisperColor.values["*Ann"] = "9";
		InGame chat;
		CHECK(chat.Receive(ChatEvent(kEventWhisperReceived, "*Ann", "hi")));
		CHECK(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "yo")));
		REQUIRE(fake::Printed().size() == 2);
		CHECK(fake::Printed()[0].first == L"*Ann | hi");
		CHECK(fake::Printed()[0].second == 9);
		CHECK(fake::Printed()[1].first == L"Bob | yo");
		CHECK(fake::Printed()[1].second == 1);
	}

	TEST_CASE("text with printf directives is shown literally") {
		App.bnet.whisperColor.values["Bob"] = "2";
		InGame chat;
		CHECK(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "100% %s %d")));
		REQUIRE(fake::Printed().size() == 1);
		CHECK(fake::Printed()[0].first == L"Bob | 100% %s %d");
	}

	TEST_CASE("whispers from players who are not listed are left to the game") {
		App.bnet.whisperColor.values["Bob"] = "1";
		InGame chat;
		CHECK_FALSE(chat.Receive(ChatEvent(kEventWhisperReceived, "Bobby", "hi")));
		CHECK_FALSE(chat.Receive(ChatEvent(kEventWhisperReceived, "Bo", "hi")));
		CHECK(fake::Printed().empty());
	}

	TEST_CASE("only received whispers are recoloured, not channel talk or other packets") {
		App.bnet.whisperColor.values["Bob"] = "1";
		InGame chat;
		CHECK_FALSE(chat.Receive(ChatEvent(kEventTalk, "Bob", "hi")));
		CHECK_FALSE(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "hi", 0x0E)));
		CHECK(fake::Printed().empty());
	}

	TEST_CASE("whispers are left to the game outside a game") {
		App.bnet.whisperColor.values["Bob"] = "1";
		InGame chat;
		chat.module.OnGameExit();
		CHECK_FALSE(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "hi")));
		CHECK(fake::Printed().empty());

		chat.module.OnGameJoin();
		CHECK(chat.Receive(ChatEvent(kEventWhisperReceived, "Bob", "hi")));
	}

	// BUG: the colour is parsed with std::stoi and the exception is not caught, so a non-numeric colour
	// in BH.json ("whisperColor": {"Bob": "red"}) throws out of the chat packet handler (crashing the
	// game) the first time that player whispers. A bad colour should never cost the whisper.
	TEST_CASE("a non-numeric colour in the config does not lose the whisper" * doctest::should_fail()) {
		App.bnet.whisperColor.values["Bob"] = "red";
		InGame chat;
		bool block = false;
		std::vector<BYTE> packet = ChatEvent(kEventWhisperReceived, "Bob", "hi");
		CHECK_NOTHROW(chat.module.OnChatPacketRecv(packet.data(), &block));
		CHECK((!block || fake::Printed().size() == 1));
	}
}
