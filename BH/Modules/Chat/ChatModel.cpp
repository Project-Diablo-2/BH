#include "ChatModel.h"

namespace ChatModel {

const wchar_t* TabName(int tab) {
	switch (tab) {
	case TAB_ALL: return L"All";
	case TAB_GAME: return L"Game";
	case TAB_WHISPERS: return L"Whispers";
	case TAB_SYSTEM: return L"System";
	case TAB_GLOBAL: return L"Global";
	}
	return L"";
}

Kind Classify(const Source& s) {
	switch (s.origin) {
	case ORIGIN_BNET_WHISPER_IN: return KIND_WHISPER_IN;
	case ORIGIN_BNET_WHISPER_OUT: return KIND_WHISPER_OUT;
	case ORIGIN_CHAT_PACKET:
		switch (s.chatType) {
		case 2: return KIND_WHISPER_IN;
		case 6: return KIND_WHISPER_OUT;
		case 1:
			// Only unitType >= 2 is printed as "<name>: <msg>"; 0/1 print the bare message.
			return s.unitType < 2 ? KIND_SYSTEM : KIND_CHAT;
		}
		return KIND_SYSTEM;
	case ORIGIN_NONE:
		break;
	}
	return KIND_SYSTEM;
}

unsigned TabsFor(Kind k) {
	unsigned m = TabBit(TAB_ALL);
	switch (k) {
	case KIND_CHAT: m |= TabBit(TAB_GAME); break;
	case KIND_WHISPER_IN:
	case KIND_WHISPER_OUT: m |= TabBit(TAB_WHISPERS); break;
	case KIND_SYSTEM: m |= TabBit(TAB_SYSTEM); break;
	}
	return m;
}

std::wstring StripColorCodes(const std::wstring& s) {
	std::wstring out;
	out.reserve(s.size());
	for (size_t i = 0; i < s.size(); i++) {
		if (s[i] == kColorChar && i + 1 < s.size() && s[i + 1] == L'c') {
			i += 2; // skip the code character too (if present)
			continue;
		}
		out += s[i];
	}
	return out;
}

namespace {

wchar_t Fold(wchar_t c) {
	if (c >= L'A' && c <= L'Z') return (wchar_t)(c + 32);
	if (c >= 0xC0 && c <= 0xDE && c != 0xD7) return (wchar_t)(c + 32);
	return c;
}

bool IsWordChar(wchar_t c) {
	if ((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') || (c >= L'0' && c <= L'9') || c == L'_') return true;
	return c >= 0xC0 && c <= 0xFF && c != 0xD7 && c != 0xF7;
}

} // namespace

bool ContainsWord(const std::wstring& haystack, const std::wstring& needle) {
	if (needle.empty()) return false;
	std::wstring h = StripColorCodes(haystack);
	if (needle.size() > h.size()) return false;
	for (size_t i = 0; i + needle.size() <= h.size(); i++) {
		size_t k = 0;
		while (k < needle.size() && Fold(h[i + k]) == Fold(needle[k])) k++;
		if (k != needle.size()) continue;
		bool before = i > 0 && IsWordChar(h[i - 1]) && IsWordChar(needle[0]);
		size_t end = i + needle.size();
		bool after = end < h.size() && IsWordChar(h[end]) && IsWordChar(needle[needle.size() - 1]);
		if (!before && !after) return true;
	}
	return false;
}

bool IsMention(const std::wstring& body, const std::wstring& myName, const std::vector<std::wstring>& keywords) {
	if (ContainsWord(body, myName)) return true;
	for (const std::wstring& k : keywords)
		if (ContainsWord(body, k)) return true;
	return false;
}

std::wstring Timestamp(int hour, int minute) {
	if (hour < 0) hour = 0;
	if (hour > 23) hour = 23;
	if (minute < 0) minute = 0;
	if (minute > 59) minute = 59;
	wchar_t b[8] = { L'[', (wchar_t)(L'0' + hour / 10), (wchar_t)(L'0' + hour % 10), L':',
		(wchar_t)(L'0' + minute / 10), (wchar_t)(L'0' + minute % 10), L']', 0 };
	return b;
}

namespace {

enum AtomKind { A_WORD, A_SPACE, A_CODE, A_LINK };
struct Atom {
	AtomKind kind;
	std::wstring text;
	int color;
	int link;
};

void Tokenize(const std::vector<Seg>& segs, std::vector<Atom>& out) {
	for (const Seg& s : segs) {
		if (s.link) {
			out.push_back({ A_LINK, s.text, s.color, s.link });
			continue;
		}
		const std::wstring& t = s.text;
		size_t i = 0;
		while (i < t.size()) {
			if (t[i] == kColorChar && i + 1 < t.size() && t[i + 1] == L'c') {
				size_t n = i + 2 < t.size() ? 3 : 2;
				out.push_back({ A_CODE, t.substr(i, n), s.color, 0 });
				i += n;
				continue;
			}
			bool space = t[i] == L' ';
			size_t j = i;
			while (j < t.size() && (t[j] == L' ') == space && !(t[j] == kColorChar && j + 1 < t.size() && t[j + 1] == L'c')) j++;
			out.push_back({ space ? A_SPACE : A_WORD, t.substr(i, j - i), s.color, 0 });
			i = j;
		}
	}
}

struct RowBuilder {
	std::vector<Row>& rows;
	Row row;
	int width = 0;
	bool content = false;

	void Append(const std::wstring& text, int color, int link) {
		if (!link && !row.empty() && !row.back().link && row.back().color == color) {
			row.back().text += text;
			return;
		}
		Seg s;
		s.text = text;
		s.color = color;
		s.link = link;
		row.push_back(s);
	}
	void Break(const std::wstring& carry, int carryColor) {
		rows.push_back(row);
		row.clear();
		width = 0;
		content = false;
		if (!carry.empty()) Append(carry, carryColor, 0);
	}
};

} // namespace

std::vector<Row> Wrap(const std::vector<Seg>& segs, int maxWidth, const WidthFn& width) {
	std::vector<Atom> atoms;
	Tokenize(segs, atoms);
	std::vector<Row> rows;
	RowBuilder b{ rows };
	std::wstring carry; // colour code in effect (inline codes of line-colour text)
	int carryColor = -1;
	for (size_t i = 0; i < atoms.size(); i++) {
		Atom& a = atoms[i];
		switch (a.kind) {
		case A_CODE:
			b.Append(a.text, a.color, 0);
			carry = a.text;
			carryColor = a.color;
			break;
		case A_SPACE: {
			if (!b.content && !rows.empty()) break; // no leading spaces on continuation rows
			int w = width(a.text);
			if (b.content && b.width + w > maxWidth) {
				b.Break(carry, carryColor);
				break;
			}
			b.Append(a.text, a.color, 0);
			b.width += w;
			break;
		}
		case A_WORD:
		case A_LINK: {
			int w = width(a.text);
			if (b.width + w <= maxWidth) {
				b.Append(a.text, a.color, a.link);
				b.width += w;
				b.content = true;
				break;
			}
			if (b.content) {
				b.Break(carry, carryColor);
				i--; // retry this atom on the fresh row
				break;
			}
			if (a.kind == A_LINK || a.text.size() <= 1) { // alone and too wide: place it anyway
				b.Append(a.text, a.color, a.link);
				b.width += w;
				b.content = true;
				break;
			}
			// A word wider than the row: cut at the longest prefix that fits (at least one char).
			size_t n = 1;
			while (n < a.text.size() && b.width + width(a.text.substr(0, n + 1)) <= maxWidth) n++;
			b.Append(a.text.substr(0, n), a.color, 0);
			b.content = true;
			a.text.erase(0, n);
			b.Break(carry, carryColor);
			i--;
			break;
		}
		}
	}
	rows.push_back(b.row);
	return rows;
}

void SentHistory::Add(const std::wstring& line) {
	cursor = -1;
	draft.clear();
	if (line.find_first_not_of(L" \t") == std::wstring::npos) return;
	if (!items.empty() && items.back() == line) return;
	items.push_back(line);
	while (items.size() > cap) items.pop_front();
}

bool SentHistory::Up(const std::wstring& current, std::wstring* out) {
	if (items.empty()) return false;
	if (cursor < 0) {
		draft = current;
		cursor = (long)items.size() - 1;
	} else if (cursor > 0) {
		cursor--;
	} else {
		return false;
	}
	*out = items[(size_t)cursor];
	return true;
}

bool SentHistory::Down(std::wstring* out) {
	if (cursor < 0) return false;
	if ((size_t)cursor + 1 < items.size()) {
		cursor++;
		*out = items[(size_t)cursor];
	} else {
		cursor = -1;
		*out = draft;
	}
	return true;
}

void ScrollState::Select(int tab) {
	if (tab < 0 || tab >= TAB_COUNT) return;
	active = tab;
	unread[tab] = 0;
}

void ScrollState::OnLine(unsigned mask, int rows) {
	for (int t = 0; t < TAB_COUNT; t++) {
		if (!(mask & TabBit(t))) continue;
		if (t != active) unread[t]++;
		if (offset[t] > 0) {
			offset[t] += rows;
			newBelow[t]++;
		}
	}
}

void ScrollState::Clamp(int tab, int totalRows, int visibleRows) {
	int maxOff = totalRows - visibleRows;
	if (maxOff < 0) maxOff = 0;
	if (offset[tab] > maxOff) offset[tab] = maxOff;
	if (offset[tab] < 0) offset[tab] = 0;
	if (offset[tab] == 0) newBelow[tab] = 0;
}

void ScrollState::Scroll(int delta, int totalRows, int visibleRows) {
	offset[active] += delta;
	Clamp(active, totalRows, visibleRows);
}

void ScrollState::JumpToLatest() {
	offset[active] = 0;
	newBelow[active] = 0;
}

void ScrollState::Reset() {
	for (int t = 0; t < TAB_COUNT; t++) offset[t] = unread[t] = newBelow[t] = 0;
}

} // namespace ChatModel
