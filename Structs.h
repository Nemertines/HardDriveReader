#pragma once

typedef unsigned long long LBA;
typedef unsigned long long CLUSTER;
typedef unsigned long long COUNT;
typedef unsigned long long SECTOR;
const DWORD BYTES_PER_SECTOR = 0x200;
#define INSERT_QWORD(x) DWORD(x>>32),DWORD(x)

#pragma pack(push, 1)
typedef struct GPT_HEADER
{
	CHAR Signature[8];
	DWORD Revision;
	DWORD HeaderSize;
	DWORD CRC32OfHeader;
	DWORD Reserved;
	LBA CurrentLBA;
	LBA BackupLBA;
	LBA FirstUsableLBA;
	LBA LastUsableLBA;
	BYTE DiskGUID[16];
	LBA LBAPartitionEntries;
	DWORD NumberOfPartitionEntries;
	DWORD SizeOfASinglePartitionEntry;
	DWORD CRC32OfPartitionEntries;
}GPT, * PGPT;

struct MBR_PARTITION_ENTRY
{
	BYTE partitionActivityStatus;
	BYTE headStart;
	BYTE sectorStart;
	BYTE cylinderStart;
	BYTE type;
	BYTE headEnd;
	BYTE sectorEnd;
	BYTE cylinderEnd;
	DWORD firstSectorOffset;
	DWORD cSectorsOfPartition;
};

typedef struct NTFS_BOOT_SECTOR_INFO
{
	struct HEADER
	{
		BYTE asmCode[3];
		DWORD magic;
		DWORD unk1;
		WORD bytesPerSector;
		BYTE sectorsPerCluster;
		BYTE nothing1[0x1A];
		UINT64 totalSectors;
		UINT64 MFT_clusterNumber;
		UINT64 MFT_mirrClusterNumber;
	};
	union
	{
		HEADER Header;
		BYTE Raw[BYTES_PER_SECTOR];
	};

}NTFS_BOOT_SECTOR_INFO, *PNTFS_BOOT_SECTOR_INFO;

typedef struct NTFS_FILE_RECORD_HEADER
{
	//CHAR Magic[4];
	//BYTE unk1;
	DWORD unk1[5];
	WORD firstAttributeOffset;
	WORD flags;
	DWORD realSizeOfFileRecord;
	DWORD allocatedSizeOfFileRecord;
	UINT64 referenceTobaseFileRecord;
	WORD nextAttributeID;
	WORD unk2;
	DWORD CurrentMFTNumber;
	WORD TagInEndOfSector;
	WORD CorruptedWord1;
	WORD unk3; //предполагаю, тут второй ворд
	WORD unk4;
	WORD unk5;
}NTFS_FILE_RECORD_HEADER, *PNTFS_FILE_RECORD_HEADER;

#define SMACRO(MACRO)\
MACRO(0x010,		STANDARD_INFORMATION)\
MACRO(0x020,		ATTRIBUTE_LIST)\
MACRO(0x030,		FILE_NAME)\
MACRO(0x040,		VOLUME_VERSION_OR_OBJECT_ID)\
MACRO(0x050,		SECURITY_DESCRIPTOR)\
MACRO(0x060,		VOLUME_NAME)\
MACRO(0x070,		VOLUME_INFORMATION)\
MACRO(0x080,		DATA)\
MACRO(0x090,		INDEX_ROOT)\
MACRO(0x0A0,		INDEX_ALLOCATION)\
MACRO(0x0B0,		BITMAP)\
MACRO(0x0C0,		SYMBOLIC_LINK_OR_REPARSE_POINT)\
MACRO(0x0D0,		EA_INFORMATION)\
MACRO(0x0E0,		EA)\
MACRO(0x0F0,		PROPERTY_SET)\
MACRO(0x100,		LOGGED_UTILITY_STREAM)\
MACRO(0xFFFFFFFF,	END)

#define INIT_ENUM(value, key) key = value,
enum class ATTRIBUTE_TYPE : DWORD
{
	SMACRO(INIT_ENUM)
};

typedef struct INDEX_NODE_HEADER
{
	DWORD	OffsetToFirstIndexEntry;
	DWORD	TotalSizeOfTheIndexEntries;
	DWORD	AllocatedSizeOfTheNode;
	BYTE	NonLeafNodeFlag;
	BYTE	Padding[3];
}INDEX_NODE_HEADER, *PINDEX_NODE_HEADER;

typedef struct INDEX_ROOT_ATTR
{
	DWORD	AttributeType;
	DWORD	CollationRule;
	DWORD	BytesPerIndexRecord;
	BYTE	ClustersPerIndexRecord;
	BYTE	Padding[3];
	//X		Index Entry
	//+		X	Y	Next Index Entry
}INDEX_ROOT_ATTR, *PINDEX_ROOT_ATTR;

typedef struct FILE_NAME_ATTR
{
	UINT64		FileReferenceToTheParentDirectory;
	FILETIME	CTimeFileCreation;
	FILETIME	ATimeFilAltered;
	FILETIME	MTimeMFTChanged;
	FILETIME	RTimeFileRead;
	UINT64		AllocatedSizeOfTheFile;
	UINT64		RealSizeOfTheFile;
	DWORD		Flags;
	DWORD		UsedByEAsAndReparse;
	BYTE		FilenameLengthInCharacters;
	BYTE		FilenameNamespace;
	WCHAR		FileNameInUnicode[1];
}FILE_NAME_ATTR, *PFILE_NAME_ATTR;

typedef struct INDEX_ENTRY
{
	DWORD FileReference;
	WORD n0;
	WORD fileFlags;
	WORD sizeOfIndexEntry;
	WORD streamLength;
}INDEX_ENTRY, * PINDEX_ENTRY;

typedef struct NTFS_INDEX_HEADER
{
	DWORD indx;
	WORD offsetToUpdateSequence;
	WORD sizeInWordsOfUpdateSequenceNumber;
	UINT64 logFileSequenceNumber;
	UINT64 VCN_ofThisIndx;
	DWORD offsetToIndexEntries;
	DWORD realSizeOfIndexEntries;
	DWORD allocSizeOfIndexEntries;
}NTFS_INDEX_HEADER, *PNTFS_INDEX_HEADER;

typedef struct PARTITION_ENTRY
{
	BYTE partitionTypeGUID[16];
	BYTE uniquePartitionTypeGUID[16];
	UINT64 firstLBA;
	UINT64 lastLBA;
	UINT64 attributeFlags;
	WCHAR partitionName[36];
}PARTITION_ENTRY, *PPARTITION_ENTRY;

typedef struct PARTITION_TABLE_HEADER
{
	UINT	partitionEntrySize;
	UINT64	partitionEntriesStarting;
}PARTITION_TABLE_HEADER, *PPARTITION_TABLE_HEADER;
#pragma pack(pop)

