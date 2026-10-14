#pragma once
// Chat overhaul: the pure parts (no Windows, no game calls), unit-tested natively.
//
//   Classify      which kind a received chat line is, from the packet that printed it
//   TabsFor       which tabs show a kind
//   Ring          the bounded line buffer (oldest evicted first)
//   IsMention     your character name / configured keywords as whole words, case-insensitive
//   Timestamp     "[hh:mm]"
//   Wrap          word wrap of styled segments (links stay whole, colour codes carried over)
//   SentHistory   Up/Down recall of sent messages with the unsent draft kept
//   ScrollState   per-tab scroll position, unread badges, "jump to latest"
//
// Text is D2's: wide strings with inline colour codes "\xFF" 'c' <code>.

#include <stddef.h>
#include <stdint.h>

#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace ChatModel {

enum Tab { TAB_ALL = 0, TAB_GAME, TAB_WHISPERS, TAB_SYSTEM, TAB_GLOBAL, TAB_COUNT };
const wchar_t* TabName(int tab);
inline unsigned TabBit(int tab) { return 1u << tab; }

enum Kind {
	KIND_CHAT,        // a player in this game (party members and you included) talking: Game tab
	KIND_WHISPER_IN,  // "<name> whispers: ..."
	KIND_WHISPER_OUT, // "You whispered to <name>: ..."
	KIND_SYSTEM,      // everything else: server text, game/party events, item notifications, BH/PD2 messages
};

// What printed a line. Filled by the game glue from the packet being handled when the engine
// printed it; ORIGIN_NONE when no chat packet was in flight (game events, BH notifications, engine text).
enum Origin { ORIGIN_NONE, ORIGIN_CHAT_PACKET, ORIGIN_BNET_WHISPER_IN, ORIGIN_BNET_WHISPER_OUT };

struct Source {
	Origin origin = ORIGIN_NONE;
	uint8_t chatType = 0;   // S->C 0x26 byte 1: 1 chat, 2 whisper from, 4 server text, 6 whisper sent
	uint8_t unitType = 0;   // 0x26 byte 3: 2 = named player chat
};

Kind Classify(const Source& s);
unsigned TabsFor(Kind k); // bit mask over Tab; always includes TAB_ALL, never TAB_GLOBAL

// Bounded FIFO: Push evicts the oldest element once `capacity` elements are held.
template <class T>
class Ring {
public:
	explicit Ring(size_t capacity) : buf(capacity ? capacity : 1) {}
	size_t Size() const { return count; }
	size_t Capacity() const { return buf.size(); }
	uint64_t Pushed() const { return pushed; }
	// i = 0 is the oldest held element.
	T& At(size_t i) { return buf[(head + i) % buf.size()]; }
	const T& At(size_t i) const { return buf[(head + i) % buf.size()]; }
	// i = 0 is the newest.
	T& FromNewest(size_t i) { return At(count - 1 - i); }
	const T& FromNewest(size_t i) const { return At(count - 1 - i); }
	// Returns true and moves the evicted element into *evicted (when non-null) if the ring was full.
	bool Push(T value, T* evicted = nullptr) {
		pushed++;
		if (count < buf.size()) {
			buf[(head + count) % buf.size()] = std::move(value);
			count++;
			return false;
		}
		if (evicted) *evicted = std::move(buf[head]);
		buf[head] = std::move(value);
		head = (head + 1) % buf.size();
		return true;
	}
	template <class F>
	void Clear(F onEvict) {
		for (size_t i = 0; i < count; i++) {
			onEvict(At(i));
			At(i) = T();
		}
		head = count = 0;
	}

private:
	std::vector<T> buf;
	size_t head = 0, count = 0;
	uint64_t pushed = 0;
};

const wchar_t kColorChar = 0xFF; // D2 colour code: 0xFF 'c' <code>
// Removes every colour code (also a trailing incomplete one).
std::wstring StripColorCodes(const std::wstring& s);
// Whole-word, case-insensitive (ASCII + Latin-1 letters), colour codes ignored. Empty needle: false.
bool ContainsWord(const std::wstring& haystack, const std::wstring& needle);
bool IsMention(const std::wstring& body, const std::wstring& myName, const std::vector<std::wstring>& keywords);

std::wstring Timestamp(int hour, int minute); // "[hh:mm]", 24 h, values clamped

// A run of text drawn in one style. color -1 = the line's colour (inline codes still apply);
// link 0 = plain text, otherwise an opaque handle that is never split by Wrap.
struct Seg {
	std::wstring text;
	int color = -1;
	int link = 0;
};
typedef std::vector<Seg> Row;
typedef std::function<int(const std::wstring&)> WidthFn; // pixel width, colour codes = 0 px

// Greedy word wrap at maxWidth px. Breaks after spaces; a word wider than a whole row is cut by
// characters; a link segment moves to the next row whole (alone on a row it may overflow).
// The colour code in effect at the end of a row is repeated at the start of the next one, and
// spaces at the start of a continuation row are dropped. Always returns at least one row.
std::vector<Row> Wrap(const std::vector<Seg>& segs, int maxWidth, const WidthFn& width);

// Sent-message recall: Up walks to older entries (the current draft is kept), Down walks back and
// finally restores the draft. Consecutive duplicates and blank lines are not stored.
class SentHistory {
public:
	explicit SentHistory(size_t capacity) : cap(capacity ? capacity : 1) {}
	void Add(const std::wstring& line);
	bool Up(const std::wstring& current, std::wstring* out);
	bool Down(std::wstring* out);
	void ResetBrowse() { cursor = -1; }
	bool Browsing() const { return cursor >= 0; }
	size_t Size() const { return items.size(); }
	const std::wstring& Newest() const { return items.back(); }

private:
	size_t cap;
	std::deque<std::wstring> items; // oldest .. newest
	long cursor = -1;               // index into items while browsing
	std::wstring draft;
};

// Scroll position + unread badges for every tab. Offsets count wrapped rows up from the newest.
class ScrollState {
public:
	int Active() const { return active; }
	// Switch tab: clears its unread count.
	void Select(int tab);
	// A line with `rows` wrapped rows arrived for the tabs in `mask`. Tabs other than the active one
	// count it unread; a tab that is scrolled up keeps its view still (offset grows by `rows`).
	void OnLine(unsigned mask, int rows);
	// Scroll the active tab by delta rows (positive = older), clamped to [0, totalRows - visibleRows].
	void Scroll(int delta, int totalRows, int visibleRows);
	void JumpToLatest();
	void Clamp(int tab, int totalRows, int visibleRows);
	int Offset(int tab) const { return offset[tab]; }
	int Unread(int tab) const { return unread[tab]; }
	int NewBelow(int tab) const { return newBelow[tab]; } // lines that arrived while scrolled up
	void Reset();

private:
	int active = TAB_ALL;
	int offset[TAB_COUNT] = {};
	int unread[TAB_COUNT] = {};
	int newBelow[TAB_COUNT] = {};
};

} // namespace ChatModel
