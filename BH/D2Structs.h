/**
*
* D2Structs.h
* BH: Copyright 2011 (C) McGod
* SlashDiablo Maphack: Copyright (C) SlashDiablo Community
*
*  This file is part of SlashDiablo Maphack.
*
*  SlashDiablo Maphack is free software: you can redistribute it and/or modify
*  it under the terms of the GNU Affero General Public License as published
*  by the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU Affero General Public License for more details.
*
*  You should have received a copy of the GNU Affero General Public License
*  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This file incorporates work covered by the following copyright and
* permission notice:
*
*   ==========================================================
*   D2Ex2
*   https://github.com/lolet/D2Ex2
*   ==========================================================
*   Copyright (c) 2011-2014 Bartosz Jankowski
*
*   Licensed under the Apache License, Version 2.0 (the "License");
*   you may not use this file except in compliance with the License.
*   You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
*   Unless required by applicable law or agreed to in writing, software
*   distributed under the License is distributed on an "AS IS" BASIS,
*   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*   See the License for the specific language governing permissions and
*   limitations under the License.
*   ==========================================================
*
*/

#pragma once
#ifndef _D2STRUCTS_H
#define _D2STRUCTS_H

#include <windows.h>
#include "CommonStructs.h"
#include "D2DataTables.h"

#pragma warning ( push )
#pragma warning ( disable: 4201 )
#pragma optimize ( "", off )

struct UnitAny;
struct Room1;
struct Room2;
struct Level;
struct Act;
struct ActMisc;
struct RosterUnit;
struct OverheadMsg;

struct InventoryInfo {
	int nLocation;
	int nMaxXCells;
	int nMaxYCells;
};

struct GameStructInfo {
	BYTE _1[0x1B];					//0x00
	char szGameName[0x18];			//0x1B
	char szGameServerIp[0x56];		//0x33
	char szAccountName[0x30];		//0x89
	char szCharName[0x18];			//0xB9
	char szRealmName[0x18];			//0xD1
	BYTE _2[0x158];					//0xE9
	char szGamePassword[0x18];		//0x241
};

struct AutomapCell {
	DWORD fSaved;					//0x00
	WORD nCellNo;					//0x04
	WORD xPixel;					//0x06
	WORD yPixel;					//0x08
	WORD wWeight;					//0x0A
	AutomapCell* pLess;				//0x0C
	AutomapCell* pMore;				//0x10
};

struct GfxCell {
	DWORD flags;					//0x00
	DWORD width;					//0x04
	DWORD height;					//0x08
	DWORD xoffs;					//0x0C
	DWORD yoffs;					//0x10
	DWORD _2;						//0x14
	DWORD lpParent;					//0x18
	DWORD length;					//0x1C
	BYTE cols;						//0x20
};

struct InteractStruct {
	DWORD dwMoveType;			//0x00
	UnitAny* lpPlayerUnit;		//0x04
	UnitAny* lpTargetUnit;		//0x08
	DWORD dwTargetX;			//0x0C
	DWORD dwTargetY;			//0x10
	DWORD _1;					//0x14
	DWORD _2;					//0x18
};

struct CellContext {
	DWORD nCellNo;					//0x00
	DWORD _1[12];					//0x04
	CellFile* pCellFile;			//0x34
};

struct AutomapLayer {
	DWORD nLayerNo;					//0x00
	DWORD fSaved;					//0x04
	AutomapCell* pFloors;			//0x08
	AutomapCell* pWalls;			//0x0C
	AutomapCell* pObjects;			//0x10
	AutomapCell* pExtras;			//0x14
	AutomapLayer* pNextLayer;		//0x18
};

struct AutomapLayer2 {
	DWORD _1[2];					//0x00
	DWORD nLayerNo;					//0x08
};

struct LevelText {
	DWORD dwLevelNo;				//0x00
	DWORD _1[60];					//0x04
	BYTE _2;						//0xF4
	char szName[40];				//0xF5
	char szEntranceText[40];		//0x11D
	char szLevelDesc[41];			//0x145
	wchar_t wName[40];				//0x16E
	wchar_t wEntranceText[40];		//0x1BE
	BYTE nObjGroup[8];				//0x196
	BYTE nObjPrb[8];				//0x19E
};

struct ControlPreferences
{
	DWORD dwType;//0x00
	// ..
};

#pragma pack(push)
#pragma pack(1)

struct RoomTile {
	Room2* pRoom2;				//0x00
	RoomTile* pNext; 			//0x04
	DWORD _2[2];				//0x08
	DWORD* nNum; 				//0x10
};

struct RosterUnit {
	char szName[16];				//0x00
	DWORD dwUnitId;					//0x10
	DWORD dwPartyLife;				//0x14
	DWORD _1;						//0x18
	DWORD dwClassId;                //0x1C
	WORD wLevel;					//0x20
	WORD wPartyId;  				//0x22
	DWORD dwLevelId;                //0x24
	DWORD Xpos;						//0x28
	DWORD Ypos;						//0x2C
	DWORD dwPartyFlags;				//0x30
	BYTE* _5;						//0x34
	DWORD _6[11];					//0x38
	WORD _7;						//0x64
	char szName2[16];				//0x66
	WORD _8;						//0x76
	DWORD _9[2];					//0x78
	RosterUnit* pNext;				//0x80
};

struct QuestInfo {
	void* pBuffer;					//0x00
	DWORD _1;						//0x04
};

struct Waypoint {
	BYTE flags;						//0x00
};

struct ClientPetData
{
	DWORD nPetType;			// 0x00
	int nPetTypeId;			// 0x04
	int nPetUnitId;			// 0x08
	int nOwnerId;			// 0x0C
	DWORD nPetLevelId;		// 0x10		Levels.txt
	DWORD _0x14;			// 0x14
	DWORD _0x18;			// 0x18
	DWORD nPetLifePercent;	// 0x1C
	int nOwnerType;			// 0x20
	DWORD nSeed;			// 0x24
	DWORD nNameId;			// 0x28
	DWORD _0x2C;			// 0x2C
	ClientPetData* pNext;	// 0x30
};

struct PlayerData {
	char szName[0x10];				//0x00
	QuestInfo* pNormalQuest;		//0x10
	QuestInfo* pNightmareQuest;		//0x14
	QuestInfo* pHellQuest;			//0x18
	Waypoint* pNormalWaypoint;		//0x1c
	Waypoint* pNightmareWaypoint;	//0x20
	Waypoint* pHellWaypoint;		//0x24
	DWORD* _0x28;					//0x28
	DWORD _0x2C;					//0x2C
	DWORD _0x30;					//0x30
	DWORD* ptArenaUnit;				//0x34
	DWORD* pPartyData;				//0x38
	WORD MPSourcePortalUniqueID;	//0x3C
	WORD _0x3E;						//0x3E
	WORD MPDestPortalUniqueID;		//0x40
	WORD _0x42;						//0x42
	DWORD* pPetList;				//0x44
	DWORD ptObjectUnID;				//0x48
	DWORD _0x4C;					//0x4C
	DWORD _0x50;					//0x50
	DWORD _0x54;					//0x54
	DWORD _0x58;					//0x58
	DWORD* pTradeBuffer;			//0x5C
	BYTE _0x60[16];					//0x60
	DWORD nRightSkillId;			//0x70
	DWORD nLeftSkillId;				//0x74
	DWORD nRightSkillFlags;			//0x78
	DWORD nLeftSkillFlags;			//0x7C
	DWORD nSwitchRightSkillId;		//0x80
	DWORD nSwitchLeftSkillId;		//0x84
	DWORD nSwitchRightSkillFlags;	//0x88
	DWORD nSwitchLeftSkillFlags;	//0x8C
	DWORD nWeaponGuid;				//0x90
	BYTE _0x94[8];					//0x94
	DWORD* pClient;					//0x9C
	DWORD _0xA0[51];				//0xA0
	DWORD _0x16C;					//0x16C
	BOOL _0x170;					//0x170
	DWORD _0x174;					//0x174
	DWORD _0x178;					//0x178
	DWORD _0x17C;					//0x17C
	DWORD _0x180;					//0x180
	DWORD _0x184;					//0x184
	DWORD _0x188;					//0x188
	DWORD _0x18C;					//0x18C
	BOOL _0x190;					//0x190
	DWORD _0x194;					//0x194
	DWORD _0x198;					//0x198
	DWORD _0x19C;		//0x19C
	BOOL _0x1A0;					//0x1A0
	DWORD _0x1A4;
	DWORD _0x1A8;
	DWORD _0x1AC;
	DWORD _0x1B0;
	DWORD _0x1B4;
	DWORD _0x1B8;
	DWORD _0x1BC;
	BOOL _0x1C0;
	BOOL _0x1C4;
	BOOL _0x1C8;
	BOOL _0x1CC[13];
	DWORD _0x200;
	DWORD _0x204;
	DWORD _0x208;
	DWORD _0x20C;
	DWORD _0x210;
	DWORD _0x214;
	DWORD _0x218;
	DWORD _0x21C;
	DWORD _0x220;
	DWORD _0x224;
	DWORD _0x228;
	DWORD _0x22C;
	DWORD _0x230;
	DWORD _0x234;
	DWORD _0x238;
	DWORD _0x23C;
	DWORD _0x240;
	DWORD _0x244;
	DWORD _0x248;
	DWORD _0x24C;
	BOOL _0x250;				// 
	DWORD _0x254;			//
	DWORD _0x258;	//
	DWORD _0x25C;	//
	BOOL _0x260;				// 
	BYTE _0x264;			//
	BOOL _0x265;
	DWORD _0x269;
	DWORD nCorruptZone;
	DWORD nItemAllocation;
};

struct PresetUnit {
	DWORD _1;						//0x00
	DWORD dwTxtFileNo;				//0x04
	DWORD dwPosX;					//0x08
	PresetUnit* pPresetNext;		//0x0C
	DWORD _3;						//0x10
	DWORD dwType;					//0x14
	DWORD dwPosY;					//0x18
};

struct Level {
	DWORD _1[4];			//0x00
	Room2* pRoom2First;		//0x10
	DWORD _2[2];			//0x14
	DWORD dwPosX;			//0x1C
	DWORD dwPosY;			//0x20
	DWORD dwSizeX;			//0x24
	DWORD dwSizeY;			//0x28
	DWORD _3[96];			//0x2C
	Level* pNextLevel;		//0x1AC
	DWORD _4;				//0x1B0
	ActMisc* pMisc;			//0x1B4
	DWORD _5[6];			//0x1BC
	DWORD dwLevelNo;		//0x1D0
};

struct Room2 {
	DWORD _1[2];			//0x00
	Room2** pRoom2Near;		//0x08
	DWORD _2[5];			//0x0C
	struct {
		DWORD dwRoomNumber; //0x00
		DWORD _1;			//0x04
		DWORD* pdwSubNumber;//0x08
	} *pType2Info;			//0x20
	Room2* pRoom2Next;		//0x24
	DWORD dwRoomFlags;		//0x28
	DWORD dwRoomsNear;		//0x2C
	Room1* pRoom1;			//0x30
	DWORD dwPosX;			//0x34
	DWORD dwPosY;			//0x38
	DWORD dwSizeX;			//0x3C
	DWORD dwSizeY;			//0x40
	DWORD _3;				//0x44
	DWORD dwPresetType;		//0x48
	RoomTile* pRoomTiles;	//0x4C
	DWORD _4[2];			//0x50
	Level* pLevel;			//0x58
	PresetUnit* pPreset;	//0x5C
};

#pragma pack(pop)

struct Room1 {
	Room1** pRoomsNear; 	//0x00
	DWORD _1[3];			//0x04
	Room2* pRoom2;			//0x10
	DWORD _2[3];			//0x14
	CollMap* Coll;			//0x20
	DWORD dwRoomsNear;		//0x24
	DWORD _3[9];			//0x28
	DWORD dwXStart;			//0x4C
	DWORD dwYStart;			//0x50
	DWORD dwXSize;			//0x54
	DWORD dwYSize;			//0x58
	DWORD _4[6];			//0x5C
	UnitAny* pUnitFirst;	//0x74
	DWORD _5;				//0x78
	Room1* pRoomNext;		//0x7C
};

struct ActMisc {
	DWORD _1[37];			//0x00
	DWORD dwStaffTombLevel; //0x94
	DWORD _2[245];			//0x98
	Act* pAct;				//0x46C
	DWORD _3[3];			//0x470
	Level* pLevelFirst;		//0x47C
};

struct Act {
	DWORD _1[3];			//0x00
	DWORD dwMapSeed;		//0x0C
	Room1* pRoom1;			//0x10
	DWORD dwAct;			//0x14
	DWORD _2[12];			//0x18
	ActMisc* pMisc;			//0x48
};

struct Path {
	WORD xOffset;					//0x00
	WORD xPos;						//0x02
	WORD yOffset;					//0x04
	WORD yPos;						//0x06
	DWORD _1[2];					//0x08
	WORD xTarget;					//0x10
	WORD yTarget;					//0x12
	DWORD _2[2];					//0x14
	Room1* pRoom1;					//0x1C
	Room1* pRoomUnk;				//0x20
	DWORD _3[3];					//0x24
	UnitAny* pUnit;					//0x30
	DWORD dwFlags;					//0x34
	DWORD _4;						//0x38
	DWORD dwPathType;				//0x3C
	DWORD dwPrevPathType;			//0x40
	DWORD dwUnitSize;				//0x44
	DWORD _5[4];					//0x48
	UnitAny* pTargetUnit;			//0x58
	DWORD dwTargetType;				//0x5C
	DWORD dwTargetId;				//0x60
	BYTE bDirection;				//0x64
};

struct ItemPath {
	DWORD _1[3];					//0x00
	DWORD dwPosX;					//0x0C
	DWORD dwPosY;					//0x10
	//Use Path for the rest
};

struct Stat {
	WORD wSubIndex;					//0x00
	WORD wStatIndex;				//0x02
	DWORD dwStatValue;				//0x04
};

// Credits to SVR, http://phrozenkeep.hugelaser.com/forum/viewtopic.php?f=8&t=31458&p=224066
struct StatList {
	DWORD _1;						//0x00
	UnitAny* pUnit;					//0x04
	DWORD dwUnitType;				//0x08
	DWORD dwUnitId;					//0x0C
	DWORD dwFlags;					//0x10
	DWORD _2[4];					//0x14
	Stat* pStat;					//0x24
	WORD wStatCount1;				//0x28
	WORD wnSize;					//0x2A
	StatList* pPrevLink;			//0x2C
	DWORD _3;						//0x30
	StatList* pPrev;				//0x34
	DWORD _4;						//0x38
	StatList* pNext;				//0x3C
	StatList* pSetList;				//0x40
	DWORD _5;						//0x44
	Stat* pSetStat;					//0x48
	WORD wSetStatCount;				//0x4C
};

struct InventoryStore
{
	DWORD pFirstItem;				//0x00
	DWORD pLastItem;				//0x04
	BYTE Width;						//0x08
	BYTE Height;					//0x09
	BYTE unk[2];					//0x0A
	DWORD pArray;					//0x0C UnitAny* [height][width]
};

struct Inventory {
	DWORD dwSignature;				//0x00
	BYTE* bGame1C;					//0x04
	UnitAny* pOwner;				//0x08
	UnitAny* pFirstItem;			//0x0C
	UnitAny* pLastItem;				//0x10
	InventoryStore* pStores;		//0x14 InventoryStore* [count]
	DWORD dwStoresCount;			//0x18
	DWORD dwLeftItemUid;			//0x1C
	UnitAny* pCursorItem;			//0x20
	DWORD dwOwnerId;				//0x24
	DWORD dwItemCount;				//0x28
};

struct Light {
	DWORD _1[3];					//0x00
	DWORD dwType;					//0x0C
	DWORD _2[7];					//0x10
	DWORD dwStaticValid;			//0x2C
	int* pnStaticMap;				//0x30
};

struct Skill {
	SkillsTxt* pSkillInfo;			//0x00
	Skill* pNextSkill;				//0x04
	int mode;						//0x08
	DWORD flags;					//0x0C
	DWORD _1[2];					//0x10
	int params[4];					//0x18
	DWORD skillLevel;				//0x28
	DWORD skillLevelBonus;			//0x2C
	DWORD quantity;					//0x30
	int itemGUID;					//0x34
	int charges;					//0x38
	DWORD _2;						//0x3C
};

struct Info {
	BYTE* pGame1C;					//0x00
	Skill* pFirstSkill;				//0x04
	Skill* pLeftSkill;				//0x08
	Skill* pRightSkill;				//0x0C
};

struct ItemData {
	DWORD dwQuality;				//0x00
	DWORD _1[2];					//0x04
	DWORD dwItemFlags;				//0x0C 1 = Owned by player, 0xFFFFFFFF = Not owned
	DWORD _2[2];					//0x10
	DWORD dwFlags;					//0x18
	DWORD _3[3];					//0x1C
	DWORD dwFileIndex;				//0x28
	DWORD dwItemLevel;				//0x2C
	WORD wVersion;					//0x30
	WORD wRarePrefix;				//0x32
	WORD wRareSuffix;				//0x34
	WORD wAutoPrefix;				//0x36
	WORD wPrefix[3];				//0x38
	WORD wSuffix[3];				//0x3E
	BYTE BodyLocation;				//0x44
	BYTE ItemLocation;				//0x45 Non-body/belt location (Body/Belt == 0xFF)
	BYTE EarLevel;					//0x46
	BYTE VarGfx;					//0x47
	CHAR personalizedName[16];		//0x4A
	WORD _10;						//0x5A
	Inventory* pOwnerInventory;		//0x5C
	UnitAny* pPrevInvItem;			//0x60
	UnitAny* pNextInvItem;			//0x64
	BYTE _11;						//0x68
	BYTE NodePage;					//0x69 Actual location, this is the most reliable by far
	WORD _12;						//0x6A
	DWORD _13[6];					//0x6C
	UnitAny* pOwner;				//0x84
};

struct D2PropertyStrc
{
	DWORD nProperty;					//0x00
	DWORD nLayer;						//0x04
	DWORD nMin;						//0x08
	DWORD nMax;						//0x0C
};

struct D2GemsTxt
{
	char szName[32];						//0x00
	char szLetter[8];						//0x20
	union
	{
		DWORD dwItemCode;				//0x28
		char szItemCode[4];					//0x28
	};
	WORD wStringId;						//0x2C
	BYTE nNumMods;						//0x2E
	BYTE nTransForm;						//0x2F
	D2PropertyStrc pProperties[3][3];		//0x30
};

struct ItemText {
	wchar_t szName2[0x40];			//0x00
	union {
		DWORD dwCode;
		char szCode[4];
	};								//0x40
	BYTE _uncharted1[0x54];			//0x44
	DWORD speed;					//0x98
	BYTE _uncharted2[0x18];			//0x9C
	WORD nLocaleTxtNo;				//0xB4
	BYTE _uncharted3[0x0E];			//0xB6
	WORD rangeadder;				//0xC4
	BYTE _uncharted4[0x09];			//0xC6
	BYTE xSize;						//0xCF
	BYTE ySize;						//0xD0
	BYTE _uncharted5[0x0d];			//0xD1
	BYTE nType;						//0xDE
	BYTE _uncharted6[0x0d];			//0xDF
	BYTE fQuest;					//0xEC
	BYTE _uncharted7[0x12];			//0xED
	BYTE reqlvl;					//0xFF 
	BYTE magiclvl;					//0x100
};

struct MonsterText {
	BYTE _1[0x6];					//0x00
	WORD nLocaleTxtNo;				//0x06
	WORD flag;						//0x08
	WORD _1a;						//0x0A
	union {
		DWORD flag1;				//0x0C
		struct {
			BYTE flag1a;			//0x0C
			BYTE flag1b;			//0x0D
			BYTE flag1c[2];			//0x0E
		};
	};
	BYTE _2[0x22];					//0x10
	WORD velocity;					//0x32
	BYTE _2a[0x52];					//0x34
	WORD tcs[3][4];					//0x86
	BYTE _2b[0x52];					//0x9E
	wchar_t szDescriptor[0x3c];		//0xF0
	BYTE _3[0x1a0];					//0x12C
};

struct MonsterData {
	MonStatsTxt* pMonStatsTxt;				//0x00
	BYTE Components[16];					//0x04
	WORD NameSeed;							//0x14
	struct
	{
		BYTE fUnk : 1;
		BYTE fNormal : 1;
		BYTE fChamp : 1;
		BYTE fBoss : 1;
		BYTE fMinion : 1;
	};				//0x16
	BYTE dwLastMode;				//0x17
	DWORD dwDuriel;					//0x18
	BYTE anEnchants[9];				//0x1C
	WORD wUniqueNo;					//0x26
	DWORD _5;						//0x28
	struct {
		wchar_t wName[28];
	};								//0x2C
};

struct ObjectData {
	ObjectTxt* pTxt;				//0x00
	union {
		BYTE Type;					//0x04 (0x0F would be a Exp Shrine)
		struct {
			BYTE _1 : 7;
			BYTE ChestLocked : 1;
		};
	};
	DWORD _2[8];					//0x08
	char szOwner[0x10];				//0x28
};

struct ObjectPath {
	Room1* pRoom1;					//0x00
	DWORD _1[2];					//0x04
	DWORD dwPosX;					//0x0C
	DWORD dwPosY;					//0x10
	//Leaving rest undefined, use Path
};

struct MonSeqTxt
{
	WORD wSeq;
	BYTE bMode;
	BYTE bFrame;
	BYTE bDir;
	BYTE bEvent;
};

struct SequenceInfo
{
	MonSeqTxt* pSeqData;
	int nFrameCount;
	int nStartFrame;
};

struct AnimDataRecord
{
	char szAnimDataName[8];    //0x00
	DWORD dwFrames;				//0x08
	DWORD dwAnimSpeed;             //0x0C
	BYTE dwAnimData[144];          //0x10
};

struct GfxData
{
	int nFrame;                  //0x00
	DWORD __04[10];               //0x04
	char* szName;               //0x2C
	int nMaxFrames;               //0x30
	CellFile* pCellFile;      //0x34
	DWORD __38;                  //0x38
	CellFile* pCurrentCell;   //0x3C
	int nDirection;               //0x40
	DWORD __44;                  //0x44
};

struct UnitAny {
	DWORD dwType;					//0x00
	DWORD dwTxtFileNo;				//0x04
	DWORD _1;						//0x08
	DWORD dwUnitId;					//0x0C
	DWORD dwMode;					//0x10
	union
	{
		PlayerData* pPlayerData;
		ItemData* pItemData;
		MonsterData* pMonsterData;
		ObjectData* pObjectData;
		//TileData *pTileData doesn't appear to exist anymore
	};								//0x14
	DWORD dwAct;					//0x18
	Act* pAct;						//0x1C
	DWORD dwSeed[2];				//0x20
	DWORD _2;						//0x28
	union
	{
		Path* pPath;
		ItemPath* pItemPath;
		ObjectPath* pObjectPath;
	};								//0x2C
	MonSeqTxt* pSeqMode;			//0x30
	int nSeqFrameCount;				//0x34 frame * 256
	int nSeqFrame;					//0x38 frame * 256 remaining for seq
	DWORD animSpeed;				//0x3C
	DWORD eSeqMode;					//0x40
	DWORD dwGfxFrame;				//0x44
	DWORD dwFrameRemain;			//0x48
	WORD wFrameRate;				//0x4C
	WORD bActionFrame;				//0x4E
	AnimDataRecord* pGfxAnimData;	//0x50
	GfxData* pGfxInfo;				//0x54
	void* pGfxInfo2;				//0x58
	StatList* pStats;				//0x5C
	Inventory* pInventory;			//0x60
	Light* ptLight;					//0x64
	DWORD _6[9];					//0x68
	WORD wX;						//0x8C
	WORD wY;						//0x8E
	DWORD _7;						//0x90
	DWORD dwOwnerType;				//0x94
	DWORD dwOwnerId;				//0x98
	DWORD _8[2];					//0x9C
	OverheadMsg* pOMsg;				//0xA4
	Info* pInfo;					//0xA8
	DWORD _9[6];					//0xAC
	DWORD dwFlags;					//0xC4
	DWORD dwFlags2;					//0xC8
	DWORD _10[5];					//0xCC
	UnitAny* pChangedNext;			//0xE0
	UnitAny* pRoomNext;				//0xE4
	UnitAny* pListNext;				//0xE8 -> 0xD8
};

struct BnetData {
	DWORD dwId;					//0x00
	DWORD dwId2;				//0x04	
	BYTE _12[13];				//0xC0
	//DWORD dwId3;				//0x14
	//WORD Unk3;					//0x18	
	BYTE _13[6];				//0xC0
	char szGameName[22];		//0x1A
	char szGameIP[16];			//0x30
	DWORD _2[15];				//0x40
	DWORD dwId4;				//0x80
	BYTE _3[5];					//0x84
	char szAccountName[48];		//0x88
	char szPlayerName[24];		//0xB8
	char szRealmName[8];		//0xD0
	BYTE _4[273];				//0xD8
	BYTE nCharClass;			//0x1E9
	BYTE nCharFlags;			//0x1EA
	BYTE nMaxLvlDifference;		//0x1EB
	BYTE _5[31];				//0x1EC
	BYTE nDifficulty;			//0x20B
	void* _6;					//0x20C
	DWORD _7[3];				//0x210
	WORD _8;					//0x224
	BYTE _9[7];					//0x226
	char szRealmName2[24];		//0x227
	char szGamePass[24];		//0x23F
	char szGameDesc[256];		//0x257
	WORD _10;					//0x348
	BYTE _11;					//0x34B
};


struct WardenClientRegion_t {
	DWORD cbAllocSize; //+00
	DWORD offsetFunc1; //+04
	DWORD offsetRelocAddressTable; //+08
	DWORD nRelocCount; //+0c
	DWORD offsetWardenSetup; //+10
	DWORD _2[2];
	DWORD offsetImportAddressTable; //+1c
	DWORD nImportDllCount; //+20
	DWORD nSectionCount; //+24
};

struct SMemBlock_t {
	DWORD _1[6];
	DWORD cbSize; //+18
	DWORD _2[31];
	BYTE data[1]; //+98
};

struct WardenClient_t {
	WardenClientRegion_t* pWardenRegion; //+00
	DWORD cbSize; //+04
	DWORD nModuleCount; //+08
	DWORD param; //+0c
	DWORD fnSetupWarden; //+10
};

struct WardenIATInfo_t {
	DWORD offsetModuleName;
	DWORD offsetImportTable;
};

struct AttackStruct {
	DWORD dwAttackType;			//0x00
	UnitAny* lpPlayerUnit;		//0x04
	UnitAny* lpTargetUnit;		//0x08
	DWORD dwTargetX;			//0x0C
	DWORD dwTargetY;			//0x10
	DWORD _1;					//0x14
	DWORD _2;					//0x18
};

#pragma pack(push)
#pragma pack(1)

struct NPCMenu {
	DWORD dwNPCClassId;
	DWORD dwEntryAmount;
	WORD wEntryId1;
	WORD wEntryId2;
	WORD wEntryId3;
	WORD wEntryId4;
	WORD _1;
	DWORD dwEntryFunc1;
	DWORD dwEntryFunc2;
	DWORD dwEntryFunc3;
	DWORD dwEntryFunc4;
	BYTE _2[5];
};

struct NPCDialogItem {
	wchar_t szItemText[120];
	DWORD nTextHeight;
	DWORD nAdjustX;
	DWORD nTextWidth;
	DWORD field4_0xfc;
	DWORD field5_0x100;
	DWORD field6_0x104;
	DWORD* pfCallback;
	DWORD field8_0x10c;
};

struct NPCDialog {
	DWORD nCreationTime;
	DWORD field1_0x4;
	DWORD pCellfile;
	DWORD field3_0xc;
	DWORD bDialogIsPlaying;
	DWORD field5_0x14;
	DWORD field6_0x18;
	DWORD field7_0x1c;
	DWORD bBlockChatBox;
	DWORD nXMidpoint; /* nXStart + (nXWidth / 2) */
	DWORD nYTopThird; /* nYStart + (nXHeight / 3) */
	DWORD nXStart;
	DWORD nYStart;
	DWORD nXWidth;
	DWORD nYHeight;
	BYTE field15_0x3c[8];
	DWORD nKeyboardHover; /* -1 if not on any menu items */
	DWORD nMouseHover;
	DWORD nLines;
	DWORD nNextFreeLine;
	DWORD nSomeTickCount;
	DWORD _0x58;
	DWORD isActive;
	DWORD* pfCallback1;
	DWORD* pfCallback2;
	wchar_t szDialogTitle[120];
	DWORD nLineHeight;
	DWORD nLineXAdjust; /* adjustment to center the text in the box */
	DWORD nTitleTextWidth;
	DWORD _0x164;
	DWORD nLineTextTransform;
	DWORD nLineTextColor;
	DWORD* pClickCallback;
	DWORD field33_0x174;
	struct NPCDialogItem menuRow[9];
	DWORD field35_0xb08;
	DWORD field36_0xb0c;
};

struct OverheadMsg {
	DWORD _1;
	DWORD dwTrigger;
	DWORD _2[2];
	char Msg[232];
};

struct DSBCAPS
{
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwBufferBytes;
	DWORD dwUnlockTransferRate;
	DWORD dwPlayCpuOverhead;
};

struct DirectSoundBufferInterface
{ /* https://ccrma.stanford.edu/software/stk/Misc/dsound.h */
	HRESULT(*QueryInterface)(int*, int*, int*);
	int* AddRef;
	int* Release;
	HRESULT(*GetCaps)(int, struct DSBCAPS);
	int* GetCurrentPosition;
	int* GetFormat;
	int* GetVolume;
	int* GetPan;
	int* GetFrequency;
	int* GetStatus;
	int* Initialize;
	int* Lock;
	int* Play;
	int* SetCurrentPosition;
	int* SetFormat;
	int* SetVolume;
	int* SetPan;
	int* SetFrequency;
	int* Stop;
	int* Unlock;
	int* Restore;
};

struct DirectSound3DBufferInterface
{
	HRESULT(*QueryInterface)(int*, int*, int*);
	int* AddRef;
	int* Release;
	int* GetAllParameters;
	int* GetConeAngles;
	int* GetConeOrientation;
	int* GetConeOutsideVolume;
	int* GetMaxDistance;
	int* GetMinDistance;
	int* GetMode;
	int* GetPosition;
	int* GetVelocity;
	int* SetAllParameters;
	int* SetConeAngles;
	int* SetConeOrientation;
	int* SetConeOutsideVolume;
	int* SetMaxDistance;
	int* SetMinDistance;
	int* SetMode;
	int* SetPosition;
	int* SetVelocity;
};

struct IKsPropertySetInterface
{
	HRESULT(*QueryInterface)(int*, int*, int*);
	int* AddRef;
	int* Release;
	HRESULT(*Get)(int*, int, int, int, int, int, int, int);
	HRESULT(*Set)(int*, int, int, int, int, int, int);
	HRESULT(*QuerySupport)(int*, int, int, int);
};

struct SoundSample
{
	struct DirectSoundBufferInterface** ppDirectSoundBufferInterface;
	DWORD dwFlags;
	DWORD field2_0x8;
	struct DirectSound3DBufferInterface** ppDirectSound3DBufferInterface;
	struct IKsPropertySetInterface** ppIKsPropertySetInterface;
	void* pRawData;
	DWORD nWaveSize;
	DWORD isPlaying;
	DWORD field8_0x20;
	DWORD field9_0x24;
	DWORD field10_0x28;
	DWORD field11_0x2c;
	DWORD ukn_FFFFFFFF;
	DWORD* field13_0x34;
	DWORD nVolume;
	float fCollisionAdjust;
	DWORD loop;
	DWORD nLoopBlockStart; // block1
	DWORD* pStreamFile;
	DWORD isMuted;
	DWORD nPrevVolume;
	DWORD nSomeOtherVolume;
	DWORD tickCount;
	DWORD tickCountEx;
};

struct SoundInstance
{
	DWORD isActive;
	DWORD nSound;
	DWORD nSoundInstanceId;
	float fSoundPosX;
	float fSoundPosY;
	float fSoundPosZ;
	float field6_0x18;
	DWORD nVolumeAdjust;
	float fCollisionAdjust;
	DWORD nPrevSampleOffset;
	struct SoundInstanceLink* pInstanceLink;
	struct SoundChannel* pSoundChannel;
	DWORD field12_0x30;
	DWORD nStartFrame;
	BOOL bSoundIsMusic;
	BYTE priority;
	DWORD field16_0x3d;
	DWORD nSampleOffset; // related to Block 1/2/3 in sounds.txt
	DWORD field18_0x45;
	DWORD field19_0x49;
	DWORD field20_0x4d;
	DWORD nSoundFrame;
	DWORD nSoundFramePlusFadeout;
};

struct SoundInstanceLink
{
	SoundInstance* pSoundInstance;
	SoundInstanceLink* pNextInstance;
};

struct SoundChannel
{
	SoundSample* pSoundSample;
	DWORD flags;
	SoundInstance* pSoundInstance;
	float fSoundPosX;
	float fSoundPosY;
	float fSoundPosZ;
	DWORD nMasterVolume;
	float fCollisionAdjust;
};

#pragma pack(pop)

struct D2MSG {
	HWND myHWND;
	char lpBuf[256];
};

#pragma warning ( pop )
#pragma optimize ( "", on )

#endif
