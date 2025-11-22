#pragma once
#include <Windows.h>

DWORD __fastcall D2CLIENT_GetUnitName_STUB(DWORD UnitAny);
DWORD __fastcall D2CLIENT_InitAutomapLayer(DWORD nLayerNo);
DWORD __fastcall TestPvpFlag_STUB(DWORD planum1, DWORD planum2, DWORD flagmask);
DWORD __fastcall D2CLIENT_GetLevelName_STUB(DWORD levelId);
DWORD __fastcall D2CLIENT_GetUIVar_STUB(DWORD varno);
CellFile* __fastcall D2CLIENT_LoadUiImage(CHAR* szPath);
DWORD __fastcall D2CLIENT_ClickParty_ASM(RosterUnit* RosterUnit, DWORD Mode);
int __stdcall D2CLIENT_PlaySound_STUB(int nSound, UnitAny* pUnit, int nTriggerFrame, int param_4, int param_5);
void __stdcall D2CLIENT_PlayItemDropSounds_STUB(UnitAny* pUnit);
int __fastcall ITEMS_GetItemPropertiesString_STUB(int nStatFilter2, UnitAny* pItem, wchar_t* strBuffer, int nBufferLen, int a5, int nLayer, int nStatFilter, int a8, int a9, wchar_t* strBuffer2);
MonSeqTxt* __stdcall D2COMMON_GetSequenceInfo_STUB(UnitAny* pUnit);
int __stdcall D2COMMON_GetSequenceIndex_STUB(UnitAny* pUnit);
int __stdcall D2COMMON_GetFrameMinAccr_STUB(int nIndex, UnitAny* pUnit);
void __stdcall D2CLIENT_GetItemFromPacket_NewGround_STUB(px9c* packet);
void __stdcall D2CLIENT_GetItemFromPacketIntercept_NewGround_STUB();
void __stdcall D2CLIENT_ItemPacketBuildAction2_DropToGround_STUB(px9c* packet);
void __stdcall D2CLIENT_ItemPacketBuildAction2_DropToGround_INTERCEPT_STUB();
void __stdcall D2CLIENT_ItemPacketBuildAction4_PutInContainer_STUB(px9c* packet);
void __stdcall D2CLIENT_ItemPacketBuildAction4_PutInContainer_INTERCEPT_STUB();
void __stdcall D2CLIENT_GetItemDescription_STUB(ItemsTxt* pItemTxtRec, wchar_t* pBuffer, UnitAny* pItem);
SoundInstance* __stdcall D2CLIENT_GetExistingSoundInstanceInQueue_STUB(int nSound, SoundInstance* pSoundInstanceToSkip);
void __stdcall D2CLIENT_AdjustSoundBasedOnCoords_STUB(float* fPos, UnitAny* pUnit, int nSound);
float __stdcall D2CLIENT_SoundTestCollision_STUB(int nSound, UnitAny* pTarget);
void __stdcall D2CLIENT_LinkSoundInstanceToClientUnit_STUB(UnitAny* pUnit, int nSoundInstanceId);