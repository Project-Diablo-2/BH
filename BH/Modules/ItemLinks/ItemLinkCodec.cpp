#include "ItemLinkCodec.h"

#include <Windows.h>
#include <string.h>

#include "../../D2DataTables.h"

namespace ItemLinkCodec {

namespace {

// Item flags (ItemData.dwFlags) as stored in the record.
const uint32_t F_IDENTIFIED = 0x00000010;
const uint32_t F_SOCKETED = 0x00000800;
const uint32_t F_NEW = 0x00002000;
const uint32_t F_EAR = 0x00010000;
const uint32_t F_STARTER = 0x00020000;
const uint32_t F_COMPACT = 0x00200000;
const uint32_t F_ETHEREAL = 0x00400000;
const uint32_t F_LOD = 0x00800000;
const uint32_t F_PERSONALIZED = 0x01000000;
const uint32_t F_RUNEWORD = 0x04000000;
// Everything a view-only copy keeps; all other bits are runtime state and are cleared.
const uint32_t F_KEEP = F_IDENTIFIED | F_SOCKETED | F_COMPACT | F_ETHEREAL | F_LOD | F_PERSONALIZED | F_RUNEWORD;

const unsigned STAT_ID_BITS = 10;     // item version 103
const unsigned STAT_TERMINATOR = 0x3FF;
const unsigned STAT_ARMORCLASS = 31;
const unsigned STAT_DURABILITY = 72;
const unsigned STAT_MAXDURABILITY = 73;
const unsigned STAT_STATE = 98;
const unsigned STAT_NUMSOCKETS = 194;

const unsigned LIST_RUNEWORD = 6;
const int MAX_TYPE_DEPTH = 8;

// ---- bits ---------------------------------------------------------------------------------------

struct Reader {
	const uint8_t* p;
	size_t bits;
	size_t pos;
	bool overrun;

	Reader(const uint8_t* data, size_t len) : p(data), bits(len * 8), pos(0), overrun(false) {}

	// LSB-first, like the engine's bit buffer. Never reads past `bits`; an overrun sets the flag
	// and yields 0 for the rest of the parse.
	uint32_t Read(unsigned n) {
		if (n == 0)
			return 0;
		if (n > 32 || overrun || bits - pos < n) {
			overrun = true;
			return 0;
		}
		uint32_t v = 0;
		for (unsigned i = 0; i < n; ++i, ++pos)
			v |= (uint32_t)((p[pos >> 3] >> (pos & 7)) & 1) << i;
		return v;
	}
};

struct Writer {
	uint8_t* p;
	size_t cap;
	size_t pos;
	bool full;

	Writer(uint8_t* out, size_t capBytes) : p(out), cap(capBytes * 8), pos(0), full(false) {
		if (capBytes)
			memset(out, 0, capBytes);
	}
	void Put(unsigned n, uint32_t v) {
		if (n == 0)
			return;
		if (full || cap - pos < n) {
			full = true;
			return;
		}
		for (unsigned i = 0; i < n; ++i, ++pos)
			if ((v >> i) & 1)
				p[pos >> 3] |= (uint8_t)(1u << (pos & 7));
	}
	size_t Bytes() const { return (pos + 7) / 8; }
};

// ---- tables ---------------------------------------------------------------------------------------

const ItemStatCostTxt* Isc(const Tables& t, unsigned id) {
	if (!t.isc || id >= t.iscCount)
		return 0;
	return (const ItemStatCostTxt*)((const uint8_t*)t.isc + (size_t)id * t.iscStride);
}

const ItemsTxt* ItemAt(const Tables& t, unsigned i) {
	return (const ItemsTxt*)((const uint8_t*)t.items + (size_t)i * t.itemsStride);
}

const D2ItemTypesTxt* TypeAt(const Tables& t, unsigned i) {
	if (!t.itemTypes || i >= t.itemTypesCount)
		return 0;
	return (const D2ItemTypesTxt*)((const uint8_t*)t.itemTypes + (size_t)i * t.itemTypesStride);
}

bool TypeIs(const Tables& t, unsigned type, const char code[4], int depth) {
	if (depth > MAX_TYPE_DEPTH || type == 0)
		return false;
	const D2ItemTypesTxt* tt = TypeAt(t, type);
	if (!tt)
		return false;
	if (memcmp(tt->szCode, code, 4) == 0)
		return true;
	return TypeIs(t, tt->nEquiv1, code, depth + 1) || TypeIs(t, tt->nEquiv2, code, depth + 1);
}

bool ItemIs(const Tables& t, const ItemsTxt* it, const char code[4]) {
	return TypeIs(t, it->nType, code, 0) || TypeIs(t, it->wtype2, code, 0);
}

bool FindItem(const Tables& t, const char code[4], unsigned* index) {
	if (!t.items)
		return false;
	for (unsigned i = 0; i < t.itemsCount; ++i) // first row wins, like D2Common's code lookup
		if (memcmp(ItemAt(t, i)->szCode, code, 4) == 0) {
			*index = i;
			return true;
		}
	return false;
}

bool SkillOk(const Tables& t, unsigned skill) {
	// Only the bound: real items name skills without a SkillDesc row (PD2's splash-on-hit skill),
	// and the engine's description copes with them.
	return skill < t.skillsCount;
}

struct Width {
	unsigned bits, paramBits;
	const ItemStatCostTxt* row;
};

bool StatWidth(const Tables& t, unsigned id, Width* w) {
	const ItemStatCostTxt* r = Isc(t, id);
	if (!r)
		return false;
	w->bits = r->bSaveBits;
	w->paramBits = r->dwSaveParamBits;
	w->row = r;
	// Save Bits 0 is legal: the engine writes such ids (e.g. poison_count after poison damage) and
	// reads no value bits for them.
	return w->bits <= 32 && w->paramBits <= 32;
}

// A stat parameter that the item description resolves through another table must name a row that
// exists there (by ItemStatCost descfunc / encode: skills, classes, skill tabs, monsters, states).
bool ParamOk(const Tables& t, unsigned id, const ItemStatCostTxt* r, uint32_t param) {
	if (r->dwSaveParamBits == 0)
		return true;
	if (id == STAT_STATE)
		return param < t.statesCount;
	if (r->bEncode == 2 || r->bEncode == 3) // (skill << 6) | level: chance-to-cast, charges
		return SkillOk(t, param >> 6);
	switch (r->bDescFunc) {
	case 13: return param < t.classCount;                                // +N to <class> skills
	case 14: return (param >> 3) < t.classCount && (param & 7) < 3;     // +N to <tab> skills
	case 15: case 24: return SkillOk(t, param >> 6);
	case 16: case 27: case 28: return SkillOk(t, param);                // aura / single skill / oskill
	case 22: return param < t.monTypeCount;                              // vs monster type
	case 23: return param < t.monStatsCount;                             // reanimate as
	default: return true;
	}
}

// Grouped damage stats: the stream stores the head id and then the follower values.
int Followers(unsigned id, unsigned out[2]) {
	switch (id) {
	case 17: out[0] = 18; return 1;
	case 48: out[0] = 49; return 1;
	case 50: out[0] = 51; return 1;
	case 52: out[0] = 53; return 1;
	case 54: out[0] = 55; out[1] = 56; return 2;
	case 57: out[0] = 58; out[1] = 59; return 2;
	default: return 0;
	}
}

Result ReadStatList(const Tables& t, Reader& r, Item* it, uint8_t list) {
	int last = -1;
	for (;;) {
		unsigned id = r.Read(STAT_ID_BITS);
		if (r.overrun)
			return ERR_TRUNCATED;
		if (id == STAT_TERMINATOR)
			return OK;
		if (id == 0 && last == 0)
			return ERR_STAT; // the engine treats two strength entries in a row as corrupt
		last = (int)id;
		if (it->nstats >= kMaxStats)
			return ERR_STAT;
		Stat& s = it->stats[it->nstats];
		memset(&s, 0, sizeof s);
		s.id = (uint16_t)id;
		s.list = list;
		unsigned ids[3] = { id, 0, 0 };
		int n = 1 + Followers(id, ids + 1);
		for (int k = 0; k < n; ++k) {
			Width w;
			if (!StatWidth(t, ids[k], &w))
				return ERR_STAT;
			s.param[k] = r.Read(w.paramBits);
			s.raw[k] = r.Read(w.bits);
			if (r.overrun)
				return ERR_TRUNCATED;
			if (!ParamOk(t, ids[k], w.row, s.param[k]))
				return ERR_STAT_PARAM;
		}
		s.nvals = (uint8_t)n;
		++it->nstats;
	}
}

bool NameCharOk(unsigned c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '-' || c == '_';
}

} // namespace

const char* ResultName(Result r) {
	switch (r) {
	case OK: return "ok";
	case ERR_EMPTY: return "empty";
	case ERR_TOO_LONG: return "too long";
	case ERR_BASE64: return "bad base64";
	case ERR_TRUNCATED: return "truncated";
	case ERR_TRAILING: return "trailing bytes";
	case ERR_VERSION: return "item version";
	case ERR_EAR: return "ear";
	case ERR_CODE: return "item code";
	case ERR_QUALITY: return "quality";
	case ERR_AFFIX: return "affix";
	case ERR_FILEINDEX: return "unique/set id";
	case ERR_RUNEWORD: return "runeword";
	case ERR_NAME: return "personalized name";
	case ERR_REALM: return "realm data";
	case ERR_SOCKETS: return "sockets";
	case ERR_STAT: return "stat";
	case ERR_STAT_PARAM: return "stat parameter";
	case ERR_CHILD: return "socketed item";
	case ERR_TABLES: return "tables unavailable";
	}
	return "?";
}

// ---- base64url ------------------------------------------------------------------------------------

static const char kB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

bool IsBase64UrlChar(unsigned c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_';
}

static int B64Value(unsigned c) {
	if (c >= 'A' && c <= 'Z') return (int)(c - 'A');
	if (c >= 'a' && c <= 'z') return (int)(c - 'a') + 26;
	if (c >= '0' && c <= '9') return (int)(c - '0') + 52;
	if (c == '-') return 62;
	if (c == '_') return 63;
	return -1;
}

size_t Base64UrlEncode(const uint8_t* data, size_t len, char* out, size_t cap) {
	size_t need = (len * 4 + 2) / 3;
	if (need > cap)
		return 0;
	size_t o = 0;
	for (size_t i = 0; i < len; i += 3) {
		uint32_t v = (uint32_t)data[i] << 16;
		size_t n = len - i;
		if (n > 1) v |= (uint32_t)data[i + 1] << 8;
		if (n > 2) v |= data[i + 2];
		out[o++] = kB64[(v >> 18) & 63];
		out[o++] = kB64[(v >> 12) & 63];
		if (n > 1) out[o++] = kB64[(v >> 6) & 63];
		if (n > 2) out[o++] = kB64[v & 63];
	}
	if (o < cap)
		out[o] = 0;
	return o;
}

bool Base64UrlDecode(const char* text, size_t len, uint8_t* out, size_t cap, size_t* outLen) {
	if (len % 4 == 1)
		return false;
	size_t need = len * 3 / 4;
	if (need > cap)
		return false;
	size_t o = 0;
	uint32_t acc = 0;
	unsigned nacc = 0;
	for (size_t i = 0; i < len; ++i) {
		int v = B64Value((unsigned char)text[i]);
		if (v < 0)
			return false;
		acc = (acc << 6) | (uint32_t)v;
		nacc += 6;
		if (nacc >= 8) {
			nacc -= 8;
			out[o++] = (uint8_t)(acc >> nacc);
			acc &= (1u << nacc) - 1;
		}
	}
	if (acc != 0) // non-canonical: leftover bits must be zero
		return false;
	*outLen = o;
	return true;
}

// ---- parse ----------------------------------------------------------------------------------------

Result ParseRecord(const Tables& t, const uint8_t* data, size_t len, bool withMagic, Item* it, size_t* consumedBytes) {
	if (!t.isc || !t.items || !t.itemTypes)
		return ERR_TABLES;
	memset(it, 0, sizeof *it);
	Reader r(data, len);
	if (withMagic && r.Read(16) != 0x4D4A)
		return r.overrun ? ERR_TRUNCATED : ERR_VERSION;
	it->flags = r.Read(32);
	it->version = (uint16_t)r.Read(10);
	if (r.overrun)
		return ERR_TRUNCATED;
	if (it->version != kItemVersion)
		return ERR_VERSION;
	if (it->flags & F_EAR)
		return ERR_EAR;
	it->mode = (uint8_t)r.Read(3);
	if (it->mode == 3 || it->mode == 5) {
		it->x = (uint16_t)r.Read(16);
		it->y = (uint16_t)r.Read(16);
	} else {
		it->bodyloc = (uint8_t)r.Read(4);
		it->col = (uint8_t)r.Read(4);
		it->row = (uint8_t)r.Read(4);
		it->page = (uint8_t)r.Read(3);
	}
	for (int i = 0; i < 4; ++i)
		it->code[i] = (char)r.Read(8);
	if (r.overrun)
		return ERR_TRUNCATED;
	if (!FindItem(t, it->code, &it->itemIndex))
		return ERR_CODE;
	const ItemsTxt* base = ItemAt(t, it->itemIndex);
	if (ItemIs(t, base, "gold") || ItemIs(t, base, "book"))
		return ERR_CODE; // their records carry extra fields this codec doesn't model
	it->isWeapon = it->itemIndex < t.weaponsCount;
	it->isArmor = !it->isWeapon && it->itemIndex < t.weaponsCount + t.armorCount;
	it->stackable = base->bstackable != 0;

	if (it->flags & F_COMPACT) {
		it->compact = true;
		it->filled = (uint8_t)r.Read(1);
		if (r.overrun)
			return ERR_TRUNCATED;
		if (it->filled)
			return ERR_SOCKETS;
		*consumedBytes = (r.pos + 7) / 8;
		return OK;
	}

	it->filled = (uint8_t)r.Read(3);
	it->seed = r.Read(32);
	it->ilvl = (uint8_t)r.Read(7);
	it->quality = (uint8_t)r.Read(4);
	if (r.Read(1)) {
		it->hasGfx = true;
		it->gfx = (uint8_t)r.Read(3);
		if (it->gfx >= 6)
			return ERR_CODE;
	}
	if (r.Read(1)) {
		it->hasAuto = true;
		it->autoMagic = (uint16_t)r.Read(11);
		if (it->autoMagic > t.autoMagicCount)
			return ERR_AFFIX;
	}
	if (r.overrun)
		return ERR_TRUNCATED;
	switch (it->quality) {
	case 1: // low
		it->fileIndex = (uint16_t)r.Read(3);
		if (it->fileIndex >= t.lowQualityCount)
			return ERR_FILEINDEX;
		break;
	case 2: // normal
		break;
	case 3: // superior
		it->fileIndex = (uint16_t)r.Read(3);
		if (it->fileIndex >= t.qualityItemsCount)
			return ERR_FILEINDEX;
		break;
	case 4: // magic
		it->magicPrefix = (uint16_t)r.Read(11);
		it->magicSuffix = (uint16_t)r.Read(11);
		if (it->magicPrefix > t.magicPrefixCount || it->magicSuffix > t.magicSuffixCount)
			return ERR_AFFIX;
		break;
	case 5: // set
	case 7: // unique
		it->fileIndex = (uint16_t)r.Read(12);
		if (it->fileIndex >= (it->quality == 5 ? t.setItemCount : t.uniqueCount))
			return ERR_FILEINDEX;
		break;
	case 6: // rare
	case 8: // crafted
		it->rareName1 = (uint8_t)r.Read(8);
		it->rareName2 = (uint8_t)r.Read(8);
		// One table: rare suffixes, then rare prefixes, addressed from 1. An unidentified rare may
		// carry no name (0/0); the description never names it.
		if ((it->flags & F_IDENTIFIED) || it->rareName1 || it->rareName2) {
			if (it->rareName1 <= t.rareSuffixCount || it->rareName1 > t.rareSuffixCount + t.rarePrefixCount)
				return ERR_AFFIX;
			if (it->rareName2 == 0 || it->rareName2 > t.rareSuffixCount)
				return ERR_AFFIX;
		}
		for (int k = 0; k < 6; ++k) {
			it->rareHas[k] = r.Read(1) != 0;
			if (it->rareHas[k]) {
				it->rareAffix[k] = (uint16_t)r.Read(11);
				unsigned limit = (k & 1) ? t.magicSuffixCount : t.magicPrefixCount;
				if (it->rareAffix[k] > limit)
					return ERR_AFFIX;
			}
		}
		break;
	default:
		return ERR_QUALITY;
	}
	if (it->flags & F_RUNEWORD) {
		it->runeword = (uint16_t)r.Read(16);
		if ((unsigned)(it->runeword & 0xFFF) >= t.runesCount)
			return ERR_RUNEWORD;
	}
	if (it->flags & F_PERSONALIZED) {
		for (;;) {
			unsigned c = r.Read(7);
			if (r.overrun)
				return ERR_TRUNCATED;
			if (c == 0)
				break;
			if (it->nameLen >= kMaxNameChars || !NameCharOk(c))
				return ERR_NAME;
			it->name[it->nameLen++] = (char)c;
		}
		if (it->nameLen == 0)
			return ERR_NAME;
	}
	if (r.Read(1))
		return r.overrun ? ERR_TRUNCATED : ERR_REALM;
	if (it->isArmor) {
		Width w;
		if (!StatWidth(t, STAT_ARMORCLASS, &w))
			return ERR_STAT;
		it->defenseRaw = r.Read(w.bits);
	}
	if (it->isArmor || it->isWeapon) {
		Width w;
		if (!StatWidth(t, STAT_MAXDURABILITY, &w))
			return ERR_STAT;
		it->maxDurRaw = r.Read(w.bits);
		if (it->maxDurRaw != 0) {
			if (!StatWidth(t, STAT_DURABILITY, &w))
				return ERR_STAT;
			it->curDurRaw = r.Read(w.bits);
		}
	}
	if (it->stackable)
		it->quantity = (uint16_t)r.Read(9);
	if (it->flags & F_SOCKETED) {
		Width w;
		if (!StatWidth(t, STAT_NUMSOCKETS, &w))
			return ERR_STAT;
		it->socketsRaw = r.Read(w.bits);
		if (it->socketsRaw > (unsigned)kMaxChildren)
			return ERR_SOCKETS;
	}
	if (r.overrun)
		return ERR_TRUNCATED;
	if (it->filled > (it->flags & F_SOCKETED ? it->socketsRaw : 0))
		return ERR_SOCKETS;
	if (it->quality == 5)
		it->setMask = (uint8_t)r.Read(5);

	Result res = ReadStatList(t, r, it, 0);
	if (res != OK)
		return res;
	for (unsigned k = 0; k < 5; ++k)
		if (it->setMask & (1u << k)) {
			res = ReadStatList(t, r, it, (uint8_t)(1 + k));
			if (res != OK)
				return res;
		}
	if (it->flags & F_RUNEWORD) {
		res = ReadStatList(t, r, it, (uint8_t)LIST_RUNEWORD);
		if (res != OK)
			return res;
	}
	*consumedBytes = (r.pos + 7) / 8;
	return OK;
}

static Result ParseRecords(const Tables& t, const uint8_t* data, size_t len, bool withMagic, Link* out) {
	memset(out, 0, sizeof *out);
	if (len == 0)
		return ERR_EMPTY;
	if (len > kMaxPayload + (withMagic ? 2 * (1 + kMaxChildren) : 0))
		return ERR_TOO_LONG;
	size_t used = 0;
	Result res = ParseRecord(t, data, len, withMagic, &out->items[0], &used);
	if (res != OK)
		return res;
	out->count = 1;
	const Item& parent = out->items[0];
	if (parent.filled && !(parent.isArmor || parent.isWeapon))
		return ERR_CHILD;
	for (int k = 0; k < parent.filled; ++k) {
		if (used >= len)
			return ERR_CHILD;
		size_t n = 0;
		Item* child = &out->items[out->count];
		res = ParseRecord(t, data + used, len - used, withMagic, child, &n);
		if (res != OK)
			return res;
		used += n;
		++out->count;
		const ItemsTxt* cb = ItemAt(t, child->itemIndex);
		if (!ItemIs(t, cb, "sock") || child->filled || (child->flags & (F_SOCKETED | F_RUNEWORD)))
			return ERR_CHILD;
	}
	if (used != len)
		return ERR_TRAILING;
	return OK;
}

Result ParsePayload(const Tables& t, const uint8_t* data, size_t len, Link* out) {
	return ParseRecords(t, data, len, false, out);
}

Result ParseSaveRecords(const Tables& t, const uint8_t* data, size_t len, Link* out) {
	return ParseRecords(t, data, len, true, out);
}

Result ParseToken(const Tables& t, const char* b64, size_t len, Link* out) {
	if (len == 0)
		return ERR_EMPTY;
	if (len > kMaxTokenChars)
		return ERR_TOO_LONG;
	uint8_t buf[kMaxPayload];
	size_t n = 0;
	if (!Base64UrlDecode(b64, len, buf, sizeof buf, &n))
		return ERR_BASE64;
	return ParsePayload(t, buf, n, out);
}

void Canonicalize(Link* link) {
	for (int i = 0; i < link->count; ++i) {
		Item& it = link->items[i];
		it.flags &= F_KEEP;
		it.flags |= F_LOD;
		it.seed = 0;
		it.x = it.y = 0;
		if (i == 0) { // stored, inventory page
			it.mode = 0;
			it.bodyloc = 0;
			it.col = 0;
			it.row = 0;
			it.page = 1;
		} else { // in the parent's socket i-1
			it.mode = 6;
			it.bodyloc = 0;
			it.col = (uint8_t)(i - 1);
			it.row = 0;
			it.page = 0;
		}
	}
}

// ---- encode ---------------------------------------------------------------------------------------

static void PutStatList(const Tables& t, Writer& w, const Item& it, uint8_t list) {
	for (int i = 0; i < it.nstats; ++i) {
		const Stat& s = it.stats[i];
		if (s.list != list)
			continue;
		w.Put(STAT_ID_BITS, s.id);
		unsigned ids[3] = { s.id, 0, 0 };
		Followers(s.id, ids + 1);
		for (int k = 0; k < s.nvals; ++k) {
			Width wd;
			if (!StatWidth(t, ids[k], &wd)) {
				w.full = true;
				return;
			}
			w.Put(wd.paramBits, s.param[k]);
			w.Put(wd.bits, s.raw[k]);
		}
	}
	w.Put(STAT_ID_BITS, STAT_TERMINATOR);
}

size_t EncodeRecord(const Tables& t, const Item& it, bool withMagic, uint8_t* out, size_t cap) {
	Writer w(out, cap);
	if (withMagic)
		w.Put(16, 0x4D4A);
	w.Put(32, it.flags);
	w.Put(10, it.version);
	w.Put(3, it.mode);
	if (it.mode == 3 || it.mode == 5) {
		w.Put(16, it.x);
		w.Put(16, it.y);
	} else {
		w.Put(4, it.bodyloc);
		w.Put(4, it.col);
		w.Put(4, it.row);
		w.Put(3, it.page);
	}
	for (int i = 0; i < 4; ++i)
		w.Put(8, (uint8_t)it.code[i]);
	if (it.compact) {
		w.Put(1, it.filled);
		return w.full ? 0 : w.Bytes();
	}
	w.Put(3, it.filled);
	w.Put(32, it.seed);
	w.Put(7, it.ilvl);
	w.Put(4, it.quality);
	w.Put(1, it.hasGfx);
	if (it.hasGfx)
		w.Put(3, it.gfx);
	w.Put(1, it.hasAuto);
	if (it.hasAuto)
		w.Put(11, it.autoMagic);
	switch (it.quality) {
	case 1:
	case 3:
		w.Put(3, it.fileIndex);
		break;
	case 4:
		w.Put(11, it.magicPrefix);
		w.Put(11, it.magicSuffix);
		break;
	case 5:
	case 7:
		w.Put(12, it.fileIndex);
		break;
	case 6:
	case 8:
		w.Put(8, it.rareName1);
		w.Put(8, it.rareName2);
		for (int k = 0; k < 6; ++k) {
			w.Put(1, it.rareHas[k]);
			if (it.rareHas[k])
				w.Put(11, it.rareAffix[k]);
		}
		break;
	default:
		break;
	}
	if (it.flags & F_RUNEWORD)
		w.Put(16, it.runeword);
	if (it.flags & F_PERSONALIZED) {
		for (int i = 0; i < it.nameLen; ++i)
			w.Put(7, (uint8_t)it.name[i]);
		w.Put(7, 0);
	}
	w.Put(1, 0); // no realm data
	Width wd;
	if (it.isArmor) {
		if (!StatWidth(t, STAT_ARMORCLASS, &wd))
			return 0;
		w.Put(wd.bits, it.defenseRaw);
	}
	if (it.isArmor || it.isWeapon) {
		if (!StatWidth(t, STAT_MAXDURABILITY, &wd))
			return 0;
		w.Put(wd.bits, it.maxDurRaw);
		if (it.maxDurRaw != 0) {
			if (!StatWidth(t, STAT_DURABILITY, &wd))
				return 0;
			w.Put(wd.bits, it.curDurRaw);
		}
	}
	if (it.stackable)
		w.Put(9, it.quantity);
	if (it.flags & F_SOCKETED) {
		if (!StatWidth(t, STAT_NUMSOCKETS, &wd))
			return 0;
		w.Put(wd.bits, it.socketsRaw);
	}
	if (it.quality == 5)
		w.Put(5, it.setMask);
	PutStatList(t, w, it, 0);
	for (unsigned k = 0; k < 5; ++k)
		if (it.setMask & (1u << k))
			PutStatList(t, w, it, (uint8_t)(1 + k));
	if (it.flags & F_RUNEWORD)
		PutStatList(t, w, it, (uint8_t)LIST_RUNEWORD);
	return w.full ? 0 : w.Bytes();
}

size_t EncodePayload(const Tables& t, const Link& link, uint8_t* out, size_t cap) {
	size_t used = 0;
	for (int i = 0; i < link.count; ++i) {
		size_t n = EncodeRecord(t, link.items[i], false, out + used, cap - used);
		if (n == 0)
			return 0;
		used += n;
	}
	return used;
}

size_t EncodeToken(const Tables& t, const Link& link, char* out, size_t cap) {
	uint8_t buf[kMaxPayload];
	size_t n = EncodePayload(t, link, buf, sizeof buf);
	if (n == 0)
		return 0;
	return Base64UrlEncode(buf, n, out, cap);
}

// ---- token text -----------------------------------------------------------------------------------

bool FindToken(const wchar_t* s, size_t len, size_t from, size_t* start, size_t* end) {
	for (size_t i = from; i + 2 < len; ++i) {
		if (s[i] != L'#' || s[i + 1] != L'i')
			continue;
		if (i > 0 && s[i - 1] != L' ')
			continue;
		size_t j = i + 2;
		while (j < len && j - (i + 2) <= kMaxTokenChars && IsBase64UrlChar(s[j]))
			++j;
		size_t n = j - (i + 2);
		if (n < kMinTokenChars || n > kMaxTokenChars)
			continue;
		if (j < len && s[j] != L' ')
			continue;
		*start = i;
		*end = j;
		return true;
	}
	return false;
}

} // namespace ItemLinkCodec
