#pragma once
// Chat item links.
//   Send:    Ctrl+Shift+left-click an item in the inventory/stash/cube/equipment -> "[Item Name] #i<token>"
//            is appended to the chat input box.
//   Receive: chat lines carrying a valid token show "[Item Name]" in the item's quality colour with
//            the token hidden; left-click the name -> the game's own item tooltip (click elsewhere,
//            right-click or Esc closes it).
// Token format and validation: ItemLinkCodec.h. Other chat UIs: ChatLinkApi.h.
#include "../Module.h"

class ChatItemLinks : public Module {
public:
	ChatItemLinks() : Module("Item Links") {}

	void OnLoad();
	void OnUnload();
	void LoadConfig();
	void OnGameJoin();
	void OnGameExit();
	void OnDraw();
	void OnLeftClick(bool up, int x, int y, bool* block);
	void OnRightClick(bool up, int x, int y, bool* block);
	void OnKey(bool up, BYTE key, LPARAM lParam, bool* block);
};
