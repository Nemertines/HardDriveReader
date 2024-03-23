#pragma once
#include <unordered_set>
#include <Windows.h>
#include <string>
#include <map>
#include "DefinedStructs.hpp"
#include "PhysicalDrive.h"
#include "FileSystemAbstract.h"

typedef DWORD MFT_NUMBER;
typedef std::vector<std::pair<CLUSTER, COUNT>> RUN_LIST;
#define SIZE_OF_FILE_RECORD 0x400
#define MAGIC_INDX 0x58444e49

typedef struct NTFS_ATTRIBUTE
{
	typedef struct NONRESIDENT_ATTRIBUTE_HEADER
	{
		UINT64 startingVCN;
		UINT64 lastVCN;
		WORD runListOffset;
		WORD compressionUnitSize;
		DWORD nothing;
		UINT64 allocatedSize;
		UINT64 realSize;
	}NTFS_NONRESIDENT_ATTRIBUTE, * PNTFS_NONRESIDENT_ATTRIBUTE;

	typedef struct RESIDENT_ATTRIBUTE_HEADER
	{
		DWORD bodySize;
		WORD bodyOffset;
		BYTE flag;
		BYTE nothing;
	}NTFS_RESIDENT_ATTRIBUTE, * PNTFS_RESIDENT_ATTRIBUTE;

	NTFS_ATTRIBUTE* NextAttribute()
	{
		return (NTFS_ATTRIBUTE*)(PBYTE(this) + Size);
	}

	BOOL IsResident()
	{
		return !NonResidentFlag;
	}

	void* AttributeBody()
	{
		if (IsResident())
			return PBYTE(this) + Header.ResidentHeader.bodyOffset;
		else
			return PBYTE(this) + Header.NonResidentHeader.runListOffset;
	}

	union ATTRIBUTE_HEADER
	{
		RESIDENT_ATTRIBUTE_HEADER ResidentHeader;
		NONRESIDENT_ATTRIBUTE_HEADER NonResidentHeader;
	};

	std::wstring GetName()
	{
		return std::wstring(
			PWCHAR(PBYTE(this) + AttrNameOffset),
			PWCHAR(PBYTE(this) + AttrNameOffset + SizeOfAttrName * sizeof(WCHAR))
		);
	}

	ATTRIBUTE_TYPE	Type;
	WORD	Size;
	WORD	hz;
	BYTE	NonResidentFlag; //	printf("Нерезидентный?:	0x%X\n", buffer[offset+8] ); //1 если нет, тело вне MFT
	BYTE	SizeOfAttrName;
	WORD	AttrNameOffset;
	WORD	Flags;
	WORD	AttrID;
	ATTRIBUTE_HEADER Header;
}NTFS_ATTRIBUTE, *PNTFS_ATTRIBUTE;

typedef struct NTFS_INDEX
{
	MFT_NUMBER fileReference;
	WORD n0;
	WORD fileFlags;
	WORD sizeOfIndexEntry;
	WORD streamLength;
	DWORD hz0;
	DWORD dirReference;
	WORD n1;
	WORD dirFlags;
	UINT64 nothing2[7];
	BYTE sizeOfFileName;
	BYTE namespaces;
}NTFS_INDEX, * PNTFS_INDEX;

typedef union FILE_RECORD
{
	FILE_RECORD(PhysicalDrive* physicalDrive, UINT64 rawAddress)
	{
		physicalDrive->Read(Raw, sizeof(*this), rawAddress);
		*PWORD(Raw + sizeof(Raw)/2 - sizeof(Header.TagInEndOfSector)) = Header.CorruptedWord1;
	}

	FILE_RECORD()
	{
	}

	NTFS_FILE_RECORD_HEADER Header;
	BYTE Raw[SIZE_OF_FILE_RECORD] = {0};

	PNTFS_ATTRIBUTE GetFirstAttribute()
	{
		return PNTFS_ATTRIBUTE(Raw + Header.firstAttributeOffset);
	}

	PNTFS_ATTRIBUTE GetOneAttribute(ATTRIBUTE_TYPE type)
	{
		const auto& attributes = GetAttributes(type);
		if (attributes.size() > 1)
		{
			printf("GetOneAttribute: must be one attribute?\n");
			MessageBoxA(0, 0, 0, 0);
		}
		if (attributes.size() == 0)
		{
			printf("GetOneAttribute: not found\n");
			MessageBoxA(0, 0, 0, 0);
			return nullptr;
		}
		return attributes[0];
	}

	std::vector<PNTFS_ATTRIBUTE> GetAttributes(ATTRIBUTE_TYPE desiredAttribute)
	{
		std::vector<PNTFS_ATTRIBUTE> result;
		PNTFS_ATTRIBUTE CurrentAttributeHeader = GetFirstAttribute();
		while (CurrentAttributeHeader->Type != ATTRIBUTE_TYPE::END)
		{
			if (CurrentAttributeHeader->Type == desiredAttribute)
				result.push_back(CurrentAttributeHeader);
			CurrentAttributeHeader = CurrentAttributeHeader->NextAttribute();
		}
		return result;
	}

	BOOL IsDirectory()
	{
		return Header.flags & 2;
	}

	RUN_LIST GetRunList(BYTE* pRunList)
	{
		RUN_LIST runList;
		UINT64 startCluster = 0;
		while (*pRunList)
		{
			BYTE cClusterLength = *pRunList & 0x0F;
			BYTE startClusterLength = *pRunList >> 4;
			pRunList++;

			if (!startClusterLength || !cClusterLength) //вот такие дела. его может не быть.
				break;

			UINT64 cCluster = 0;
			for (DWORD i = 0; i < cClusterLength; i++)
			{
				(PBYTE(&cCluster))[i] = *pRunList;
				pRunList++;
			}

			UINT64 temp = 0;
			for (DWORD i = 0; i < startClusterLength; i++)
			{
				(PBYTE(&temp))[i] = *pRunList;
				pRunList++;
			}

			if (temp >> (startClusterLength * 8 - 1)) //check sign
				startCluster += INT64(temp + (0xffffffffffffffff << (startClusterLength * 8))); //make int64 negative
			else
				startCluster += temp;
			runList.push_back(std::make_pair(startCluster, cCluster));
		}
		return runList;
	}

	RUN_LIST GetAttributeRunList(ATTRIBUTE_TYPE type)
	{
		PNTFS_ATTRIBUTE attribute = GetOneAttribute(type);
		return GetRunList(PBYTE(attribute->AttributeBody()));
	}

	MFT_NUMBER GetParentDirectory()
	{
		const auto FileNameAttribute = GetAttributes(ATTRIBUTE_TYPE::FILE_NAME);
		if (FileNameAttribute.size() == 0)
		{
			printf("GetParentDirectory error\n");
			MessageBoxA(0, 0, 0, 0);
		}
		PFILE_NAME_ATTR attrName = PFILE_NAME_ATTR(FileNameAttribute[0]->AttributeBody());
		return MFT_NUMBER(attrName->FileReferenceToTheParentDirectory);
	}

	std::wstring GetFileName()
	{
		std::vector<std::pair<DWORD, std::wstring>> names;
		const auto attributes = GetAttributes(ATTRIBUTE_TYPE::FILE_NAME);
		if (attributes.size() == 0)
		{
			printf("GetFileName error\n");
			MessageBoxA(0, 0, 0, 0);
		}

		for (const auto attribute : attributes)
		{
			PFILE_NAME_ATTR attrName = PFILE_NAME_ATTR(attribute->AttributeBody());
			std::wstring FileName = std::wstring(attrName->FileNameInUnicode, attrName->FileNameInUnicode + attrName->FilenameLengthInCharacters);
			names.push_back(std::make_pair(attrName->FilenameNamespace, FileName));
		}

		while (names.size() != 1)
		{
			for (size_t i = 0; i < names.size(); i++)
			{
				if (names[i].first == 3)
				{
					names.erase(names.begin() + i);
					break;
				}
				if (names[i].first == 2)
				{
					names.erase(names.begin() + i);
					break;
				}
				if (names[i].first == 1)
				{
					names.erase(names.begin() + i);
					break;
				}
				if (names[i].first == 0)
				{
					names.erase(names.begin() + i);
					break;
				}
			}
		}
		return names[0].second;
	}
}FILE_RECORD, * PFILE_RECORD;

class NTFS : public FileSystem
{
private:
	//LBA pointer on Partition Boot Sector(VBR)
	//https://en.wikipedia.org/wiki/NTFS
	LBA LBA_Entry;
	LBA MFT_Pointer;
	PhysicalDrive* physicalDrive;
	void PrintDirectoryContent(MFT_NUMBER number);
public:
	void DumpDirectory(MFT_NUMBER DirectoryStart)
	{
		PrintDirectoryContent(DirectoryStart);
	}
	UINT64 MFTNumberToRawAddress(MFT_NUMBER fileNumber);
	void ParseFile(MFT_NUMBER fileNumber);
	NTFS_BOOT_SECTOR_INFO bootSectorInfo;
	NTFS(PhysicalDrive* physicalDrive, LBA entry);
	FILE_RECORD MFT_FileRecord;
	void AttributeHandler2(PNTFS_ATTRIBUTE attributeHeader);
	BOOL ExtractFile(MFT_NUMBER n);
	void PrintFileInfo(std::unordered_set<MFT_NUMBER>& fileReferences, PNTFS_INDEX index);
	BOOL IsNTFS();
};
