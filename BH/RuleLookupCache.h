// A cache wrapper class useful for inserting caches around rule list lookups

#ifndef RULE_LOOKUP_CACHE_H_
#define RULE_LOOKUP_CACHE_H_

struct UnitItemInfo;
struct Rule;

#include <memory>
#include <vector>
#include <utility>
#include "lrucache.hpp"

template <typename T, typename... Args>
class RuleLookupCache {
	std::unique_ptr<cache::lru_cache<DWORD, std::pair<DWORD, T>>> cache;
	
	protected:
	const std::vector<Rule*> &RuleList;
	virtual T make_cached_T(UnitItemInfo *uInfo, Args&&... pack) = 0;
	virtual std::string to_str(const T &cached_T) {
		// This function only needs to be implemented for debug printing
		return std::string("???");
	}

	public:
	RuleLookupCache(const std::vector<Rule*> &rule_list) 
		: RuleList(rule_list), cache(new cache::lru_cache<DWORD, std::pair<DWORD, T>>(100)) {}

	void ResetCache() {
		//PrintText(1, "Reseting LRU cache.");
		cache.reset(new cache::lru_cache<DWORD, std::pair<DWORD, T>>(100));
	}
	
	// TODO: UnitItemInfo should probably be const, but call to Evaluate needs non-const
	T Get(UnitItemInfo *uInfo, Args&&... pack) {
		// leave this false. doesn't work 
		static DWORD last_printed_guid = 0; // to prevent excessive printing
		DWORD guid = uInfo->item->dwUnitId; // global unique identifier
		// TODO: should we also use fingerprint or seed? Currently we trigger cache updates based
		// on item flag changes. This should cover everything that I can think of, including IDing
		// items, crafting items, making runewords, etc. Still would be nice to get some reassurance
		// that GUIDs aren't reused in some unexpected way. Having a cache that is wrong is no bueno.
		DWORD flags = uInfo->item->pItemData->dwFlags;
		DWORD orig_cached_flags; // the cached flags
		T cached_T; // the cached T after rules applied
		bool cache_hit = false;
		// First check if the name exists in the cache
		if (cache && cache->exists(guid)) {
			orig_cached_flags = cache->get(guid).first;
			if (orig_cached_flags == flags) {
				cached_T = cache->get(guid).second;
				cache_hit = true; // needed because empty string is also a valid item name
			} else {
				// This print can give a hint if the GUID of an item ever changes. Problem is that it will also
				// print whenever you ID an item, make a runeword, personalize an item, etc.
				// Even seems to change when items get 'old'.
				//PrintText(1, "Detected change in item flags. Cached: %x Actual: %x", orig_cached_flags, flags);
				//PrintText(1, "    Cached name str: %s", to_str(cache->get(guid).second).c_str());
			}
			// cache_hit is false if the unmodified item name has changed from cached version
		}
		if (!cache_hit) {
			cached_T = make_cached_T(uInfo, pack...);
			if (cache && !cache_hit) {
				std::pair<DWORD, T> pair_to_cache(flags, cached_T);
				cache->put(guid, pair_to_cache);
				//PrintText(1, "Adding key value pair %u, (%s, %x) to cache.", guid, to_str(cached_T).c_str(), flags);
			}
			// Debug stuff below doesn't work anymore because there's no general comparison function implemented
			//else if (cached_name != name) {
				// only runs if item_name_debug is on
			//	if (guid != last_printed_guid) {
			//		PrintText(1, "Mismatch in modified item name! Cached: %s Actual: %s", cached_name.c_str(), name.c_str());
			//		last_printed_guid = guid;
			//	}
			//}
		}
			return cached_T;
		}
};

#endif // RULE_LOOKUP_CACHE_H_
