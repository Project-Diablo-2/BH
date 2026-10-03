#pragma once
// Chat overhaul: a tabbed in-game chat panel (All / Game / Whispers / System / Global placeholder)
// with scrollback, timestamps, mention highlight, sent-message history and a reply hotkey.
// Client-only. With "chat.enabled" off nothing is hooked and the vanilla chat is untouched.
//
// Hooks (1.13c D2Client, all verified byte-for-byte before patching, all removed when disabled):
//   ItemLinks line observer (PrintGameString 0x7D850) and the     -> every line also goes to our buffer
//   PrintPartyString 0x7D610 entry
//                                                                  (the engine's own lists, message
//                                                                  log and side effects still run)
//   S->C packet table entry 0x26 (chat)                          -> which packet printed a line
//   call DrawChatLines 0x7ECDA / call DrawPartyLines 0x7ECDF    -> the vanilla overlay is not drawn
//   game window subclass                                         -> wheel, PgUp/PgDn/End, Up/Down,
//                                                                  Ctrl+Tab, Ctrl+<reply key>
#include <windows.h>
#include "../Module.h"

class ChatOverhaul : public Module {
public:
	ChatOverhaul() : Module("chat") {};

	void OnLoad();
	void OnUnload();
	void LoadConfig();
	void OnLoop();
	void OnGameJoin();
	void OnGameExit();
	void OnDraw();
	void OnLeftClick(bool up, int x, int y, bool* block);
	void OnChatPacketRecv(BYTE* packet, bool* block);
	void OnUserInput(const wchar_t* msg, bool fromGame, bool* block);
};
