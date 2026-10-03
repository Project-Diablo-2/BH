#pragma once
// Chat item links: the API other chat UIs use to show and open item links.
//
// The ItemLinks module already renders links in the game's own chat (overlay and message log). A
// replacement chat UI that stores and draws messages itself calls these instead:
//   - FormatMessage once per received message body: plain text and link runs (validated tokens
//     become "[Item Name]" in the item's quality colour; invalid tokens stay plain text);
//   - OnClick when the player left-clicks a link run: opens the game's own item tooltip;
//   - Release when the message leaves its buffer.
#include <string>
#include <vector>

struct ChatRun {
	std::wstring text;
	int color; // D2 text colour index, or -1 = the line's own colour
	int link;  // 0 = plain text, else a link handle (> 0)
};

namespace ItemLinks {

// Split a message body into runs. Accepts raw "#i<token>" words and text the game-chat path has
// already rewritten (its zero-width link markers); a "[typed name] " right before a token is folded
// into the link run. Each link run holds one reference on its handle.
void FormatMessage(const wchar_t* msg, std::vector<ChatRun>& out);

// Drop one reference (0 / unknown handles are ignored). The item data is freed with the last one.
void Release(int link);

// Open the native item tooltip for `link`, anchored on the clicked run: x = run centre, yTop/yBottom
// = the run's box. The ItemLinks module draws it every frame until Esc, another click, or leaving
// the game. Returns true when the click was consumed.
bool OnClick(int link, int x, int yTop, int yBottom);

} // namespace ItemLinks
