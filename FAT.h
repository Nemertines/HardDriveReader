#pragma once
#include "PhysicalDrive.h"
#include "DefinedStructs.hpp"
#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>
#include "FileSystemAbstract.h"
typedef DWORD FAT32_TABLE_INDEX;

#pragma pack(push, 1)
typedef union FAT_BOOT_SECTOR
{
	struct HEADER
	{
		BYTE AsmCode[3];
		CHAR Name[8];
		//bios parameter block 0xB (varies size) (max 79 bytes)
		WORD BytesPerLogicalSector;
		BYTE LogicalSectorsPerCluster;
		WORD ReservedLogicalSectors;
		BYTE NumberOfAllocTable;
		BYTE Unk2;
		WORD Unk3;
		DWORD Unk4[4];
		DWORD SizeOfAllocationTable;
	};
	HEADER Header;
	BYTE Raw[BYTES_PER_SECTOR];
}FAT_BOOT_SECTOR, *PFAT_BOOT_SECTOR;
const DWORD s = sizeof(FAT_BOOT_SECTOR::HEADER);
typedef struct ENTRY_INFO_FAT
{
	union
	{
		CHAR shortName[8];
		BYTE entryType;
	};
	CHAR shortExtension[3];
	UCHAR attribute;
	UCHAR nothing[8];
	WORD highBytes;
	UCHAR nothing2[4];
	WORD lowBytes;
	DWORD sizeInBytes;
}ENTRY_INFO_FAT, * PENTRY_INFO_FAT;
#pragma pack(pop)

class FAT
{
public:
	FAT(PhysicalDrive* physicalDrive, UINT64 LBA);
	void CommandHandler();
	BOOL IsFAT();
private:
	LBA startTableLBA = 0;
	LBA rootLBA = 0;
	BOOL TableSectorIsInit = false;
	LBA CurrentTableLBA = 0;
	FAT_BOOT_SECTOR BootSector;
	PhysicalDrive* physicalDrive;
	std::vector<UINT64> GetPartsOfFileByStartingCluster(DWORD TableIndex);
	UINT64 GetRawAddressByClusterIndex(DWORD ClusterIndex);
	DWORD GetNextTableIndex(DWORD TableIndex);
	void DumpDirectory(DWORD dirStart);
	DWORD CurrentSectorOfFileAllocationTable[BYTES_PER_SECTOR / sizeof(DWORD)] = { 0 }; //to do: size must be BootSector.Header.BytesPerLogicalSector
};
