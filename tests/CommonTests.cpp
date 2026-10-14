#include "doctest/doctest.h"

#include <Windows.h>

#include <cstring>
#include <string>
#include <vector>

#include "Common.h"
#include "FakeEngine.h"

namespace {

std::vector<std::string> Tokens(const std::string& str, const std::string& delimiters) {
	std::vector<std::string> out;
	Tokenize(str, out, delimiters);
	return out;
}

std::vector<std::string> Strs(const char* a = nullptr, const char* b = nullptr, const char* c = nullptr) {
	std::vector<std::string> out;
	if (a) out.push_back(a);
	if (b) out.push_back(b);
	if (c) out.push_back(c);
	return out;
}

// A temp file with the given bytes, deleted when the helper goes out of scope.
struct TempFile {
	std::string path;
	explicit TempFile(const std::string& contents) {
		char dir[MAX_PATH];
		char name[MAX_PATH];
		GetTempPathA(MAX_PATH, dir);
		GetTempFileNameA(dir, "bht", 0, name);
		path = name;
		HANDLE h = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		DWORD written = 0;
		if (!contents.empty()) ::WriteFile(h, contents.data(), (DWORD)contents.size(), &written, NULL);
		CloseHandle(h);
	}
	~TempFile() { DeleteFileA(path.c_str()); }
	char* c_path() { return &path[0]; }
};

const std::wstring kColorPrefix = L"\xFF" L"c";

}  // namespace

TEST_SUITE("Common") {

// ---- Tokenize ------------------------------------------------------------------------------

TEST_CASE("Tokenize splits on any delimiter character and drops empty tokens") {
	CHECK(Tokens("a b c", " ") == Strs("a", "b", "c"));
	CHECK(Tokens("  a   b  ", " ") == Strs("a", "b"));
	CHECK(Tokens("key=value;x", "=;") == Strs("key", "value", "x"));
	CHECK(Tokens("a,,b", ",") == Strs("a", "b"));
}

TEST_CASE("Tokenize yields nothing for empty or delimiter-only input and appends to the vector") {
	CHECK(Tokens("", " ").empty());
	CHECK(Tokens("   ", " ").empty());
	CHECK(Tokens("single", " ") == Strs("single"));

	std::vector<std::string> out;
	out.push_back("existing");
	Tokenize("x y", out, " ");
	CHECK(out == Strs("existing", "x", "y"));
}

// ---- Encoding conversions (CODE_PAGE is UTF-8) ---------------------------------------------

TEST_CASE("AnsiToWide and WideToAnsi convert UTF-8 and round-trip") {
	CHECK(AnsiToWide("") == L"");
	CHECK(WideToAnsi(L"") == "");
	CHECK(AnsiToWide("Stone of Jordan") == L"Stone of Jordan");
	// "ÿc" in UTF-8 is C3 BF 63; the wide form is U+00FF 'c' (the game's colour prefix).
	CHECK(AnsiToWide("\xC3\xBF" "c1") == L"\xFF" L"c1");
	CHECK(WideToAnsi(L"\xFF" L"c1") == "\xC3\xBF" "c1");
	CHECK(WideToAnsi(L"\xFF" L"c1").size() == 4u);

	// A non-BMP character (U+1F5E1 dagger) is a surrogate pair in UTF-16 and 4 bytes in UTF-8.
	const std::string dagger = "\xF0\x9F\x97\xA1";
	std::wstring wide = AnsiToWide(dagger);
	REQUIRE(wide.size() == 2u);
	CHECK(wide[0] == (wchar_t)0xD83D);
	CHECK(wide[1] == (wchar_t)0xDDE1);
	CHECK(WideToAnsi(wide) == dagger);

	const std::string mixed = "Gr\xC3\xBC\xC3\x9F" "e \xE6\x97\xA5\xE6\x9C\xAC";  // "Grüße 日本"
	CHECK(AnsiToWide(mixed).size() == 8u);
	CHECK(WideToAnsi(AnsiToWide(mixed)) == mixed);
}

TEST_CASE("AnsiToUnicode and UnicodeToAnsi return null-terminated UTF-8 conversions") {
	wchar_t* w = AnsiToUnicode("\xC3\xBF" "c4Hello");
	CHECK(std::wstring(w) == L"\xFF" L"c4Hello");
	delete[] w;

	char* a = UnicodeToAnsi(L"\xFF" L"c4Hello");
	CHECK(std::string(a) == "\xC3\xBF" "c4Hello");
	delete[] a;

	wchar_t* emptyW = AnsiToUnicode("");
	CHECK(emptyW[0] == L'\0');
	delete[] emptyW;

	char* emptyA = UnicodeToAnsi(L"");
	CHECK(emptyA[0] == '\0');
	delete[] emptyA;
}

// ---- Colour codes --------------------------------------------------------------------------

TEST_CASE("GetColorCode appends the colour character to the game's colour prefix") {
	fake::SetLocaleText(3994, kColorPrefix);
	CHECK(GetColorCode(0) == L"\xFF" L"c0");  // white
	CHECK(GetColorCode(1) == L"\xFF" L"c1");  // red
	CHECK(GetColorCode(9) == L"\xFF" L"c9");  // yellow
	// The game's colours past 9 continue in ASCII order: 10 ':' (dark green), 11 ';' (purple).
	CHECK(GetColorCode(10) == L"\xFF" L"c:");
	CHECK(GetColorCode(11) == L"\xFF" L"c;");
}

TEST_CASE("MaybeStripColorPrefixW removes one leading colour code only") {
	CHECK(MaybeStripColorPrefixW(L"\xFF" L"c4Shako") == L"Shako");
	CHECK(MaybeStripColorPrefixW(L"\xFF" L"c;X") == L"X");
	CHECK(MaybeStripColorPrefixW(L"\xFF" L"c1\xFF" L"c2Two") == L"\xFF" L"c2Two");
	CHECK(MaybeStripColorPrefixW(L"Shako") == L"Shako");
	CHECK(MaybeStripColorPrefixW(L"Sh\xFF" L"c4ako") == L"Sh\xFF" L"c4ako");
	CHECK(MaybeStripColorPrefixW(L"c4Shako") == L"c4Shako");
	CHECK(MaybeStripColorPrefixW(L"\xFF" L"x4Shako") == L"\xFF" L"x4Shako");
	CHECK(MaybeStripColorPrefixW(L"") == L"");
}

TEST_CASE("MaybeStripColorPrefix removes one leading UTF-8 colour code only") {
	CHECK(MaybeStripColorPrefix("\xC3\xBF" "c4Shako") == "Shako");
	CHECK(MaybeStripColorPrefix("\xC3\xBF" "c1\xC3\xBF" "c2Two") == "\xC3\xBF" "c2Two");
	CHECK(MaybeStripColorPrefix("Shako") == "Shako");
	CHECK(MaybeStripColorPrefix("Sh\xC3\xBF" "c4ako") == "Sh\xC3\xBF" "c4ako");
	CHECK(MaybeStripColorPrefix("\xC3\xBF" "x4Shako") == "\xC3\xBF" "x4Shako");
	CHECK(MaybeStripColorPrefix("") == "");
}

// ---- Trim ----------------------------------------------------------------------------------

TEST_CASE("Trim and TrimW remove surrounding spaces and tabs but keep inner whitespace") {
	CHECK(Trim("  value  ") == "value");
	CHECK(Trim("\tvalue\t\t") == "value");
	CHECK(Trim(" \tvalue\t ") == "value");
	CHECK(Trim("a b\tc") == "a b\tc");
	CHECK(Trim("") == "");
	CHECK(Trim("    ") == "");
	CHECK(Trim("\t\t") == "");

	CHECK(TrimW(L"  value  ") == L"value");
	CHECK(TrimW(L"\tvalue\t") == L"value");
	CHECK(TrimW(L" \tvalue\t ") == L"value");
	CHECK(TrimW(L"a b\tc") == L"a b\tc");
	CHECK(TrimW(L"") == L"");
	CHECK(TrimW(L"   ") == L"");
}

// The PD2 Item Filtering wiki documents this order: "Whitespace surrounding the Output of each rule
// gets removed prior to evaluation (spaces first, followed by tabs)". Filters rely on it to pad
// names with spaces, e.g. "ItemDisplay[RUNE>9]:<TAB> %NAME% <TAB>".
TEST_CASE("Trim and TrimW strip spaces before tabs, so tab-wrapped space padding survives") {
	CHECK(Trim("\t value \t") == " value ");
	CHECK(Trim(" \t value \t ") == " value ");
	CHECK(Trim("\t\t  value  \t") == "  value  ");
	CHECK(TrimW(L"\t value \t") == L" value ");
	CHECK(TrimW(L" \t value \t ") == L" value ");
}

// ---- Booleans and numbers ------------------------------------------------------------------

TEST_CASE("IsTrue and StringToBool accept 1/y/yes/true case-insensitively and nothing else") {
	CHECK(IsTrue("1"));
	CHECK(IsTrue("y"));
	CHECK(IsTrue("Y"));
	CHECK(IsTrue("yes"));
	CHECK(IsTrue("YeS"));
	CHECK(IsTrue("true"));
	CHECK(IsTrue("TRUE"));

	CHECK_FALSE(IsTrue("0"));
	CHECK_FALSE(IsTrue("n"));
	CHECK_FALSE(IsTrue("no"));
	CHECK_FALSE(IsTrue("false"));
	CHECK_FALSE(IsTrue(""));
	CHECK_FALSE(IsTrue("2"));
	CHECK_FALSE(IsTrue("yess"));
	CHECK_FALSE(IsTrue(" true"));
	CHECK_FALSE(IsTrue("on"));

	CHECK(StringToBool("True"));
	CHECK(StringToBool("1"));
	CHECK_FALSE(StringToBool("False"));
	CHECK_FALSE(StringToBool(""));
}

// ---- PrintText -----------------------------------------------------------------------------

TEST_CASE("PrintText formats its arguments and prints one line in the given colour") {
	PrintText(4, "Found %s x%d", "Ber", 2);
	REQUIRE(fake::Printed().size() == 1u);
	CHECK(fake::Printed()[0].first == L"Found Ber x2");
	CHECK(fake::Printed()[0].second == 4);

	PrintText(0, "\xC3\xBF" "c1red");
	REQUIRE(fake::Printed().size() == 2u);
	CHECK(fake::Printed()[1].first == L"\xFF" L"c1red");
	CHECK(fake::Printed()[1].second == 0);
}

// Common.cpp documents a 151-character limit with a TODO to lift it, so only require that long
// text keeps its start intact; the exact cut-off is not pinned.
TEST_CASE("PrintText prints 151 characters intact and keeps the start of longer text") {
	std::string exact(151, 'a');
	PrintText(1, "%s", exact.c_str());
	std::string longer(151, 'b');
	longer += "OVERFLOW";
	PrintText(1, "%s", longer.c_str());

	REQUIRE(fake::Printed().size() == 2u);
	CHECK(fake::Printed()[0].first == std::wstring(151, L'a'));
	const std::wstring& printed = fake::Printed()[1].first;
	const std::wstring wideLonger(longer.begin(), longer.end());
	CHECK(printed.size() >= 151u);
	CHECK(printed.size() <= wideLonger.size());
	CHECK(wideLonger.compare(0, printed.size(), printed) == 0);
}

// ---- Key codes -----------------------------------------------------------------------------

TEST_CASE("GetKeyCode(name) maps names to Windows virtual-key codes case-insensitively") {
	CHECK(GetKeyCode("VK_A").value == (unsigned)'A');
	CHECK(GetKeyCode("vk_a").value == (unsigned)'A');
	CHECK(GetKeyCode("VK_Z").value == (unsigned)'Z');
	CHECK(GetKeyCode("VK_0").value == (unsigned)'0');
	CHECK(GetKeyCode("VK_9").value == (unsigned)'9');
	CHECK(GetKeyCode("VK_BACK").value == (unsigned)VK_BACK);
	CHECK(GetKeyCode("VK_RETURN").value == (unsigned)VK_RETURN);
	CHECK(GetKeyCode("VK_ALT").value == (unsigned)VK_MENU);
	CHECK(GetKeyCode("VK_CAPSLOCK").value == (unsigned)VK_CAPITAL);
	CHECK(GetKeyCode("VK_PAGEUP").value == (unsigned)VK_PRIOR);
	CHECK(GetKeyCode("VK_PAGEDN").value == (unsigned)VK_NEXT);
	CHECK(GetKeyCode("VK_SNAPSHOT").value == (unsigned)VK_SNAPSHOT);
	CHECK(GetKeyCode("VK_NUMPAD0").value == (unsigned)VK_NUMPAD0);
	CHECK(GetKeyCode("VK_NUMPAD9").value == (unsigned)VK_NUMPAD9);
	CHECK(GetKeyCode("VK_NUMPADMULTIPLY").value == (unsigned)VK_MULTIPLY);
	CHECK(GetKeyCode("VK_NUMPADADD").value == (unsigned)VK_ADD);
	CHECK(GetKeyCode("VK_NUMPADSUBTRACT").value == (unsigned)VK_SUBTRACT);
	CHECK(GetKeyCode("VK_NUMPADDECIMAL").value == (unsigned)VK_DECIMAL);
	CHECK(GetKeyCode("VK_NUMPADDIVIDE").value == (unsigned)VK_DIVIDE);
	CHECK(GetKeyCode("VK_F1").value == (unsigned)VK_F1);
	CHECK(GetKeyCode("VK_F12").value == (unsigned)VK_F12);
	CHECK(GetKeyCode("VK_F16").value == (unsigned)VK_F16);
	CHECK(GetKeyCode("VK_NUMLOCK").value == (unsigned)VK_NUMLOCK);
	CHECK(GetKeyCode("VK_SCROLL").value == (unsigned)VK_SCROLL);
	CHECK(GetKeyCode("VK_LCTRL").value == (unsigned)VK_LCONTROL);
	CHECK(GetKeyCode("VK_RMENU").value == (unsigned)VK_RMENU);
	CHECK(GetKeyCode("VK_SEMICOLON").value == (unsigned)VK_OEM_1);
	CHECK(GetKeyCode("VK_PLUS").value == (unsigned)VK_OEM_PLUS);
	CHECK(GetKeyCode("VK_COMMA").value == (unsigned)VK_OEM_COMMA);
	CHECK(GetKeyCode("VK_MINUS").value == (unsigned)VK_OEM_MINUS);
	CHECK(GetKeyCode("VK_PERIOD").value == (unsigned)VK_OEM_PERIOD);
	CHECK(GetKeyCode("VK_LEFTBRACKET").value == (unsigned)VK_OEM_4);
	CHECK(GetKeyCode("VK_BACKSLASH").value == (unsigned)VK_OEM_5);
	CHECK(GetKeyCode("VK_RIGHTBRACKET").value == (unsigned)VK_OEM_6);
	CHECK(GetKeyCode("VK_QUOTE").value == (unsigned)VK_OEM_7);

	KeyCode f5 = GetKeyCode("Vk_F5");
	CHECK(f5.name == "VK_F5");
	CHECK(f5.literalName == "F5");
}

TEST_CASE("GetKeyCode(name) falls back to 'None' for unknown names") {
	KeyCode unknown = GetKeyCode("VK_NOPE");
	CHECK(unknown.name == "None");
	CHECK(unknown.value == 0u);
	CHECK(unknown.literalName == "Not Set");
	CHECK(GetKeyCode("").name == "None");
	CHECK(GetKeyCode("VK_A ").name == "None");
}

TEST_CASE("GetKeyCode(value) reports the key's name and display name") {
	KeyCode esc = GetKeyCode((unsigned)VK_ESCAPE);
	CHECK(esc.name == "VK_ESCAPE");
	CHECK(esc.literalName == "Esc");
	CHECK(GetKeyCode((unsigned)'Q').literalName == "Q");
	CHECK(GetKeyCode((unsigned)VK_NUMPAD5).literalName == "Numpad 5");
	CHECK(GetKeyCode((unsigned)VK_OEM_1).literalName == ";");
	CHECK(GetKeyCode((unsigned)VK_OEM_PLUS).literalName == "+");
	CHECK(GetKeyCode((unsigned)VK_OEM_MINUS).literalName == "-");
	CHECK(GetKeyCode((unsigned)VK_OEM_5).literalName == "\\");
	CHECK(GetKeyCode((unsigned)VK_OEM_7).literalName == "'");

	CHECK(GetKeyCode(0u).name == "None");
	CHECK(GetKeyCode(0u).literalName == "Not Set");
	CHECK(GetKeyCode((unsigned)VK_LBUTTON).name == "None");
	CHECK(GetKeyCode(0x3Au).name == "None");  // gap between '9' and 'A'
	CHECK(GetKeyCode(0xFFFFu).name == "None");
}

TEST_CASE("GetKeyCode name and value lookups round-trip") {
	const char* names[] = {"VK_TAB", "VK_SPACE", "VK_LEFT", "VK_DELETE", "VK_M", "VK_F9", "VK_NUMPAD7",
		"VK_LSHIFT", "VK_COMMA", "VK_QUOTE"};
	for (const char* name : names) {
		CAPTURE(name);
		CHECK(GetKeyCode(GetKeyCode(name).value).name == name);
	}
}

// BUG: the table maps VK_FORWARDSLASH to 0xBD (VK_OEM_MINUS, already used by VK_MINUS) and
// VK_TILDE to 0xBF, which is VK_OEM_2, the '/?' key. On a US keyboard '`~' is VK_OEM_3 (0xC0).
// So binding "VK_FORWARDSLASH" fires on '-', binding "VK_TILDE" fires on '/', the '/' key is
// displayed as "~", and the '~' key cannot be bound or displayed at all.
TEST_CASE("GetKeyCode maps '/' and '~' to VK_OEM_2 and VK_OEM_3" * doctest::should_fail()) {
	CHECK(GetKeyCode("VK_FORWARDSLASH").value == (unsigned)VK_OEM_2);
	CHECK(GetKeyCode("VK_TILDE").value == (unsigned)VK_OEM_3);
	CHECK(GetKeyCode((unsigned)VK_OEM_2).literalName == "/");
	CHECK(GetKeyCode((unsigned)VK_OEM_3).literalName == "~");
	CHECK(GetKeyCode(GetKeyCode("VK_FORWARDSLASH").value).name == "VK_FORWARDSLASH");
}

// ---- string_format -------------------------------------------------------------------------

TEST_CASE("string_format behaves like sprintf without a length limit") {
	CHECK(string_format("%d/%d", 3, 7) == "3/7");
	CHECK(string_format("%s-%s", "a", "b") == "a-b");
	CHECK(string_format("%05.1f", 2.26) == "002.3");
	CHECK(string_format("100%%") == "100%");
	CHECK(string_format("plain") == "plain");
	CHECK(string_format("%x", 255) == "ff");

	// Output far longer than the initial 2x-format-length buffer.
	std::string big(5000, 'q');
	std::string out = string_format("<%s>", big.c_str());
	CHECK(out.size() == 5002u);
	CHECK(out == "<" + big + ">");

	// Output exactly one char longer than the initial buffer (2 * 2 = 4 chars + null).
	CHECK(string_format("%s", "abcd") == "abcd");
	CHECK(string_format("%s", "abc") == "abc");
}

// ---- Memory and files ----------------------------------------------------------------------

TEST_CASE("memcpy2 copies the bytes and returns the end of the destination") {
	char src[] = "abcdef";
	char dst[8];
	memset(dst, 'x', sizeof(dst));
	void* end = memcpy2(dst, src, 3);
	CHECK(end == (void*)(dst + 3));
	CHECK(memcmp(dst, "abcxxxxx", 8) == 0);

	// Chained writes append.
	end = memcpy2(end, src + 3, 3);
	CHECK(end == (void*)(dst + 6));
	CHECK(memcmp(dst, "abcdefxx", 8) == 0);

	CHECK(memcpy2(dst, src, 0) == (void*)dst);
}

TEST_CASE("AllocReadFile returns the whole file contents") {
	const std::string contents("D2\0data\xFF", 8);
	TempFile file(contents);
	BYTE* buf = AllocReadFile(file.c_path());
	REQUIRE(buf != nullptr);
	CHECK(memcmp(buf, contents.data(), contents.size()) == 0);
	delete[] buf;
}

TEST_CASE("AllocReadFile returns null for a missing file") {
	TempFile file("x");
	std::string missing = file.path + ".missing";
	CHECK(AllocReadFile(&missing[0]) == nullptr);
}

}  // TEST_SUITE
