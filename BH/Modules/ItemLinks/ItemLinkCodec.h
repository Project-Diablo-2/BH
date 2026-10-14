#pragma once
// Chat item links: the token codec and validator (pure logic, no game calls).
//
// Token text:  "#i" + base64url (alphabet A-Z a-z 0-9 - _, no padding) of a payload.
// Payload:     the item's save-format record WITHOUT the 2-byte "JM" magic, followed by the save
//              records of its socketed children (gems/runes/jewels) in socket order, exactly how a
//              character file stores them. Every record is byte-aligned; the parent's 3-bit
//              "filled sockets" field says how many child records follow.
//
// A token comes from anyone in chat, so the receiver never hands it to the engine as-is:
//   1. ParsePayload walks the item grammar with a bounds-checked bit reader, caps every count and
//      checks every table id (item code, qualities, affixes, unique/set/runeword ids, stat ids and
//      their parameters) against the game's own tables.
//   2. Canonicalize clears what a view-only copy must not carry (location, seed, runtime flags).
//   3. EncodeRecord writes the canonical record back. The engine only ever decodes bytes that this
//      encoder produced from validated fields.
// Item version 103 (PD2's live format: 10-bit stat ids) only.

#include <stddef.h>
#include <stdint.h>

struct ItemStatCostTxt;
struct ItemsTxt;
struct D2ItemTypesTxt;

namespace ItemLinkCodec {

const uint16_t kItemVersion = 103;
// A chat message is at most 255 characters (the engine's input box, C->S 0x15 and S->C 0x26 caps),
// so a token can't be longer than 255 - "#i" anyway; 186 bytes = 248 base64 characters.
const size_t kMaxPayload = 186;                          // bytes after base64 decoding
const size_t kMaxTokenChars = (kMaxPayload * 4 + 2) / 3; // base64 characters after "#i"
const size_t kMinTokenChars = 16;
const int kMaxChildren = 6;
const int kMaxStats = 80;     // stat entries (head + followers) per item, all lists together
const int kMaxNameChars = 15; // personalized name

enum Result {
	OK = 0,
	ERR_EMPTY,
	ERR_TOO_LONG,
	ERR_BASE64,
	ERR_TRUNCATED,   // the grammar ran past the end of the payload
	ERR_TRAILING,    // bytes left after the last record
	ERR_VERSION,
	ERR_EAR,
	ERR_CODE,        // unknown item code or a kind we refuse (gold, books)
	ERR_QUALITY,
	ERR_AFFIX,
	ERR_FILEINDEX,
	ERR_RUNEWORD,
	ERR_NAME,
	ERR_REALM,
	ERR_SOCKETS,
	ERR_STAT,        // unknown stat id, or too many stats
	ERR_STAT_PARAM,  // a stat parameter that names a skill/class/monster/state that doesn't exist
	ERR_CHILD,       // a child that is not a socket filler, or a parent/child count mismatch
	ERR_TABLES,      // tables not available
};
const char* ResultName(Result r);

// The game's tables the validator checks ids against (filled from D2Common's data tables in game,
// from the PD2 fixture tables in the native tests). Counts are record counts; a null pointer with
// a zero count means "not available" and fails validation (ERR_TABLES) where it is needed.
struct Tables {
	const ItemStatCostTxt* isc;
	unsigned iscCount;
	size_t iscStride; // sizeof one ItemStatCost record in this game build
	const ItemsTxt* items;
	unsigned itemsCount;
	size_t itemsStride;
	unsigned weaponsCount, armorCount; // items = weapons, then armor, then misc
	const D2ItemTypesTxt* itemTypes;
	unsigned itemTypesCount;
	size_t itemTypesStride;
	unsigned skillsCount;
	unsigned magicPrefixCount, magicSuffixCount, autoMagicCount;
	unsigned rarePrefixCount, rareSuffixCount;
	unsigned uniqueCount, setItemCount, runesCount;
	unsigned lowQualityCount, qualityItemsCount;
	unsigned classCount, monStatsCount, monTypeCount, statesCount;
};

// One stat entry as stored: a head id with its value (and the follower values of grouped damage
// stats, which the stream writes without their own ids). `raw` is the stored bit pattern.
struct Stat {
	uint16_t id;
	uint8_t list;   // 0 = own list, 1..5 = set bonus lists, 6 = runeword list
	uint8_t nvals;  // 1 + followers
	uint32_t param[3];
	uint32_t raw[3];
};

struct Item {
	uint32_t flags;
	uint16_t version;
	uint8_t mode, bodyloc, col, row, page;
	uint16_t x, y;
	char code[4];
	unsigned itemIndex; // index into Tables::items
	bool compact;
	uint8_t filled;     // filled sockets (children that follow)
	uint32_t seed;
	uint8_t ilvl, quality;
	bool hasGfx;
	uint8_t gfx;
	bool hasAuto;
	uint16_t autoMagic;
	uint16_t fileIndex;
	uint16_t magicPrefix, magicSuffix;
	uint8_t rareName1, rareName2;
	bool rareHas[6];
	uint16_t rareAffix[6]; // prefix1, suffix1, prefix2, suffix2, prefix3, suffix3
	uint16_t runeword;
	char name[kMaxNameChars + 1];
	uint8_t nameLen;
	bool isArmor, isWeapon, stackable;
	uint32_t defenseRaw, maxDurRaw, curDurRaw;
	uint16_t quantity;
	uint32_t socketsRaw;
	uint8_t setMask;
	Stat stats[kMaxStats];
	int nstats;
};

struct Link {
	Item items[1 + kMaxChildren];
	int count; // parent + children
};

// Base64url without padding. Encode returns the number of chars written (0 if `cap` is too small;
// a terminating NUL is written when there is room). Decode rejects any other character, padding,
// and an impossible length (len % 4 == 1).
size_t Base64UrlEncode(const uint8_t* data, size_t len, char* out, size_t cap);
bool Base64UrlDecode(const char* text, size_t len, uint8_t* out, size_t cap, size_t* outLen);
bool IsBase64UrlChar(unsigned c);

// One record. `withMagic`: data starts with "JM". On success *consumedBytes is the record's
// byte length (rounded up to a whole byte, as records are stored).
Result ParseRecord(const Tables& t, const uint8_t* data, size_t len, bool withMagic, Item* out, size_t* consumedBytes);
// A whole token payload: parent + exactly `filled` socket fillers, no trailing bytes.
Result ParsePayload(const Tables& t, const uint8_t* data, size_t len, Link* out);
// The engine's save-form serialization of an item with its socketed children (every record starts
// with "JM", as in a character file): the sender's input. No trailing bytes either.
Result ParseSaveRecords(const Tables& t, const uint8_t* data, size_t len, Link* out);
// Token text (the characters after "#i") -> validated link.
Result ParseToken(const Tables& t, const char* b64, size_t len, Link* out);

// Clear location, seed and every flag outside the allowlist; children get the socket location.
void Canonicalize(Link* link);

// Write one record (0 = didn't fit `cap`). Never fails otherwise for an Item ParseRecord accepted.
size_t EncodeRecord(const Tables& t, const Item& it, bool withMagic, uint8_t* out, size_t cap);
// Parent + children back to a payload (no magic) / to token text (without the "#i" marker).
size_t EncodePayload(const Tables& t, const Link& link, uint8_t* out, size_t cap);
size_t EncodeToken(const Tables& t, const Link& link, char* out, size_t cap);

// Find the next "#i<base64url>" token in a wide string, starting at `from`. The token must start
// the string or follow a space and end at a space/end of string; its length must be within
// [kMinTokenChars, kMaxTokenChars]. Returns false if none. [*start, *end) covers "#i..." and
// [*start + 2, *end) is the base64 text.
bool FindToken(const wchar_t* s, size_t len, size_t from, size_t* start, size_t* end);

} // namespace ItemLinkCodec
