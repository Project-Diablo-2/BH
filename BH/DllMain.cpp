#include "BH.h"
#include <Windows.h>

BOOL WINAPI DllMain(HMODULE instance, DWORD reason, VOID* reserved) {
	switch(reason) {
		case DLL_PROCESS_ATTACH:
			return BH::Startup(instance, reserved);
		case DLL_PROCESS_DETACH:
			return BH::Shutdown();
	}
	// Thread attach/detach: nothing to do, but we must return (falling off the end is undefined behavior).
	return TRUE;
}