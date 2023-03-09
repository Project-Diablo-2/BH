#include <Windows.h>
#include "D2Ptrs.h"

DWORD __declspec(naked) __fastcall D2CLIENT_GetUnitName_STUB(DWORD UnitAny)
{
	__asm
	{
		mov eax, ecx
		jmp D2CLIENT_GetUnitName_I
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_InitAutomapLayer(DWORD nLayerNo)
{
	__asm
	{
		push eax;
		mov eax, ecx;
		call D2CLIENT_InitAutomapLayer_I;
		pop eax;
		ret;
	}
}

DWORD __declspec(naked) __fastcall TestPvpFlag_STUB(DWORD planum1, DWORD planum2, DWORD flagmask)
{
	__asm
	{
		push esi;
		push[esp + 8];
		mov esi, edx;
		mov edx, ecx;
		call D2CLIENT_TestPvpFlag_I;
		pop esi;
		ret 4;
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetLevelName_STUB(DWORD levelId)
{
	__asm
	{
		mov esi, ecx
		jmp D2CLIENT_GetLevelName_I
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_GetUIVar_STUB(DWORD varno)
{
	__asm
	{
		mov eax, ecx;
		jmp D2CLIENT_GetUiVar_I;
	}
}

__declspec(naked) CellFile* __fastcall D2CLIENT_LoadUiImage(CHAR* szPath)
{
	__asm
	{
		MOV EAX, ECX
		PUSH 0
		CALL D2CLIENT_LoadUiImage_I
		RETN
	}
}

DWORD __declspec(naked) __fastcall D2CLIENT_ClickParty_ASM(RosterUnit* RosterUnit, DWORD Mode)
{
	__asm
	{
		mov eax, ecx
		jmp D2CLIENT_ClickParty_I
	}
}

void __declspec(naked) __fastcall D2CLIENT_PlaySound(int SoundNo)
{
	__asm
	{
		push ebx
		mov ebx, ecx
		push 0
		push 0
		push 0
		push 0
		call D2CLIENT_PlaySound_I
		pop ebx;
		ret
	}
}

__declspec (naked) int __fastcall ITEMS_GetItemPropertiesString_STUB(int nStatFilter2, UnitAny* pItem, wchar_t* strBuffer, int nBufferLen, int a5, int nLayer, int nStatFilter, int a8, int a9, wchar_t* strBuffer2)
{
	__asm
	{
		push edi
		push esi
		mov edi, [esp + 40]
		mov esi, [esp + 36]
		push[esp + 32]
		push[esp + 32]
		push[esp + 32]
		push[esp + 32]
		push[esp + 32]
		push[esp + 32]
		push edx
		call D2CLIENT_GetItemPropertiesString
		pop esi
		pop edi
		retn 32
	}
}

__declspec(naked) MonSeqTxt* __stdcall D2COMMON_GetSequenceInfo_STUB(UnitAny* pUnit)
{
	__asm
	{
		mov EAX, [esp + 4]                                   // pUnit
		call D2COMMON_GetSequenceInfo                        // 0x2E840
		retn 4
	}
}

__declspec(naked) int __stdcall D2COMMON_GetSequenceIndex_STUB(UnitAny* pUnit)
{
	__asm
	{
		push ESI
		mov ESI, [esp + 8]                                   // param_1
		call D2COMMON_GetSequenceIndex                       // 0x2E790
		pop ESI
		retn 4
	}
}

__declspec(naked) int __stdcall D2COMMON_GetFrameMinAccr_STUB(int nIndex, UnitAny* pUnit)
{
	__asm
	{
		mov EAX, [esp + 4]                                   // nIndex
		push[esp + 8]                                       // pUnit
		call D2COMMON_GetFrameMinAccr                        // 0x323E0
		retn 8
	}
}
