// BH's own globals for the test build (see BHGlobals.cpp).
#pragma once

class Item;
class ModuleManager;

namespace fake {

// BH::moduleManager points here. Get("item") returns ItemModule().
ModuleManager& Modules();

// The "item" module, whose ItemFilterNames ItemDisplay::InitializeItemRules fills in.
Item& ItemModule();

}  // namespace fake
