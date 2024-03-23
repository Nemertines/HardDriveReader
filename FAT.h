#pragma once
#include "PhysicalDrive.h"
#include "Structs.h"
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

class FAT : public FileSystem
{
public:
	FAT(PhysicalDrive* physicalDrive, UINT64 LBA);
	BOOL IsFAT();
	void DumpRootDirectory()
	{
		DumpDirectory(0);
	};
	void DumpDirectory(UINT64 DirectoryStart);
	void ExtractFile(UINT64 FileStart) {};
	void FileInfo(UINT64 FileStart) {};
private:
	LBA startTableLBA;
	LBA rootLBA;
	BOOL TableSectorIsInit = false;
	LBA CurrentTableLBA;
	FAT_BOOT_SECTOR BootSector;
	std::vector<UINT64> GetPartsOfFileByStartingCluster(DWORD TableIndex);
	UINT64 GetRawAddressByClusterIndex(DWORD ClusterIndex);
	DWORD GetNextTableIndex(DWORD TableIndex);
	DWORD CurrentSectorOfFileAllocationTable[BYTES_PER_SECTOR / sizeof(DWORD)] = { 0 }; //todo: size must be BootSector.Header.BytesPerLogicalSector
};
