#pragma once
// Chat item links: everything that touches the game (Diablo II 1.13c client only).
//   - the live data tables the codec validates against,
//   - item -> save records (sender), validated canonical records -> a transient, view-only client
//     item unit and back to nothing (receiver; never hashed, never in a room or inventory),
//   - the item's own name, and the game's own hover tooltip for a transient unit.
#include <Windows.h>
#include <stdint.h>

#include "ItemLinkCodec.h"

struct UnitAny;

namespace ItemLinkGame {

// Resolve addresses and check the code bytes at every call/patch site. False = not 1.13c or the
// code isn't what we expect (another mod patched it): the feature stays off. *why names the site.
bool Init(const char** why);
bool Ready();

// Live tables (re-read every call; cheap). False if the game hasn't loaded them.
bool Tables(ItemLinkCodec::Tables* t);

// The item's save records ("JM" + record for the item and each socketed child) via the engine's
// own serializer. Returns the byte count, 0 on failure.
size_t SerializeItem(UnitAny* item, uint8_t* out, size_t cap);

// Build a transient item (+ socket fillers) from a validated, canonical link. The engine decodes
// the canonical records; any disagreement with the codec (failure, different length) frees it and
// returns null.
UnitAny* CreateItem(const ItemLinkCodec::Tables& t, const ItemLinkCodec::Link& link);
void FreeItem(UnitAny* item);

// The game's name for the item (loot-filter renames bypassed), last line, colour codes removed.
bool ItemName(UnitAny* item, wchar_t* out, size_t cap);
// D2 text colour of an item name by quality (unique gold, set green, rare yellow, ...).
int NameColor(const ItemLinkCodec::Item& it);

// The game's own item tooltip for `item`, anchored like an inventory hover on the box
// (x centre, yTop..yBottom). Call from the in-game draw (BH OnDraw). Skipped while the game
// staged a popup of its own this frame or an item is on the cursor.
void DrawTooltip(UnitAny* item, int x, int yTop, int yBottom);

// Width in pixels of the first n characters of s in the current font (colour codes are zero-width).
int TextWidth(const wchar_t* s, int n);

// Chat input box: append text (opens the box first). False if it wouldn't fit (255 characters).
bool AppendChatInput(const wchar_t* text);

} // namespace ItemLinkGame
