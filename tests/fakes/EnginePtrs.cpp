// Defines the engine pointers declared in D2Ptrs.h for the test executable, the way BH.cpp does
// for the real DLL (_DEFINE_PTRS). There is no game in the test process, so Patch::GetDllOffset
// and D2Version are replaced here: an engine function the tests need resolves to its fake in
// FakeEngine.cpp, and anything else (game variables, asm addresses) resolves to a zeroed block
// of memory (fake::Slot). Calling an engine function that has no fake therefore crashes the test
// that does it, and doctest reports which test that was.
#define _DEFINE_PTRS
#include "D2Ptrs.h"

#include "FakeEngine.h"

namespace {

struct Route {
	Dll dll;
	int offset;
	void* target;
};

// `real` is only there for its type: a fake must have exactly the engine function's signature
// and calling convention, or this does not compile.
template <class F>
Route MakeRoute(Dll dll, const Offsets& offsets, F* /*real*/, F* fake) {
	return Route{ dll, offsets._113c, reinterpret_cast<void*>(fake) };
}

#define ROUTE(dll, name, fake) MakeRoute(dll, f##dll##_##name##_offsets, &dll##_##name, fake)

const Route* Routes(size_t& count) {
	static const Route routes[] = {
		ROUTE(D2LANG, GetLocaleText, &fake::GetLocaleText),
		ROUTE(D2CLIENT, PrintGameString, &fake::PrintGameString),
		ROUTE(D2CLIENT, GetPlayerUnit, &fake::GetPlayerUnit),
		ROUTE(D2CLIENT, GetDifficulty, &fake::GetDifficulty),
		ROUTE(D2CLIENT, GetCurrentInteractingNPC, &fake::GetCurrentInteractingNPC),
		ROUTE(D2CLIENT, GetQuestInfo, &fake::GetQuestInfo),
		ROUTE(D2COMMON, GetUnitStat, &fake::GetUnitStat),
		ROUTE(D2COMMON, GetStatList, &fake::GetStatList),
		ROUTE(D2COMMON, CopyStatList, &fake::CopyStatList),
		ROUTE(D2COMMON, GetStatValueFromStatList, &fake::GetStatValueFromStatList),
		ROUTE(D2COMMON, GetStateStatList, &fake::GetStateStatList),
		ROUTE(D2COMMON, GetItemText, &fake::GetItemText),
		ROUTE(D2COMMON, GetItemTextFromItemCode, &fake::GetItemTextFromItemCode),
		ROUTE(D2COMMON, GetItemLevelRequirement, &fake::GetItemLevelRequirement),
		ROUTE(D2COMMON, GetMaxSockets, &fake::GetMaxSockets),
		ROUTE(D2COMMON, GetItemPrice, &fake::GetItemPrice),
		ROUTE(D2COMMON, GetRoomFromUnit, &fake::GetRoomFromUnit),
		ROUTE(D2COMMON, GetLevelIdFromRoom, &fake::GetLevelIdFromRoom),
		ROUTE(D2WIN, DrawText, &fake::WinDrawText),
		ROUTE(D2WIN, SetTextSize, &fake::SetTextSize),
		ROUTE(D2WIN, GetTextWidthFileNo, &fake::GetTextWidthFileNo),
		ROUTE(D2CLIENT, FindServerSideUnit, &fake::FindServerSideUnit),
		ROUTE(D2CLIENT, GetItemName, &fake::ClientGetItemName),
		ROUTE(D2CLIENT, LeaveParty, &fake::LeaveParty),
		ROUTE(D2NET, SendPacket, &fake::SendPacket),
	};
	count = sizeof(routes) / sizeof(routes[0]);
	return routes;
}

}  // namespace

int Patch::GetDllOffset(Dll dll, int offset) {
	size_t count = 0;
	const Route* routes = Routes(count);
	for (size_t i = 0; i < count; i++) {
		if (routes[i].dll == dll && routes[i].offset == offset) {
			return reinterpret_cast<int>(routes[i].target);
		}
	}
	return reinterpret_cast<int>(fake::Slot(dll, offset));
}

VersionID D2Version::versionID = VERSION_113c;

VersionID D2Version::GetGameVersionID() {
	return VERSION_113c;
}

// Modules under test (e.g. Gamefilter.cpp) create their code patches as globals. There is no game
// code to patch, so a patch only records whether it is installed and never writes memory.
Patch::Patch(PatchType type, Dll dll, Offsets offsets, int function, int length)
	: dll(dll), type(type), offsets(offsets), length(length), function(function), oldCode(NULL), injected(false) {}

bool Patch::Install() {
	injected = true;
	return true;
}

bool Patch::Remove() {
	injected = false;
	return true;
}
