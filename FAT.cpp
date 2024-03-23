#include "FAT.h"

FAT::FAT(PhysicalDrive* physicalDrive, UINT64 LBA) : FileSystem(physicalDrive)
{
	physicalDrive->ReadByLBA(&BootSector, BYTES_PER_SECTOR, LBA);
	if (!IsFAT())
		return;
	rootLBA = LBA;
	rootLBA += BootSector.Header.ReservedLogicalSectors;
	rootLBA += (BootSector.Header.SizeOfAllocationTable * BootSector.Header.NumberOfAllocTable);
	startTableLBA = LBA + BootSector.Header.ReservedLogicalSectors;
}

static std::wstring GetStringFromStruct(PENTRY_INFO_FAT eif)
{
	const BYTE abc[3][2] =
	{
		{0x01, 0x0b},
		{0x0e, 0x1a},
		{0x1c, 0x20},
	};
	std::wstring result;
	for (BYTE i = 0; i < sizeof(abc) / sizeof(*abc); i++)
		result += std::wstring(PWCHAR(PBYTE(eif) + abc[i][0]), PWCHAR(PBYTE(eif) + abc[i][1]));
	return result;
}

UINT64 FAT::GetRawAddressByClusterIndex(DWORD ClusterIndex)
{
	//-2 because:
	//The first entry (cluster 0 in the FAT) holds the FAT ID since MS-DOS 1.20 and PC DOS 1.1
	//The second entry (cluster 1 in the FAT) nominally stores the end-of-cluster-chain marker as used by the formater
	return (rootLBA + (ClusterIndex - 2) * BootSector.Header.LogicalSectorsPerCluster) * BootSector.Header.BytesPerLogicalSector;
}

DWORD FAT::GetNextTableIndex(DWORD TableIndex)
{
	UINT64 ByteOffset = TableIndex * sizeof(TableIndex); //смещение в байтах от начала таблицы
	LBA LBAIndex = ByteOffset / BootSector.Header.BytesPerLogicalSector;
	ByteOffset -= LBAIndex * BootSector.Header.BytesPerLogicalSector; //смещение в байтах от начала LBA
	if ((TableSectorIsInit == false) || LBAIndex != CurrentTableLBA)
	{
		physicalDrive->ReadSectorByLBA(&CurrentSectorOfFileAllocationTable, LBAIndex + startTableLBA);
		TableSectorIsInit = true;
		CurrentTableLBA = LBAIndex;
	}
	return CurrentSectorOfFileAllocationTable[ByteOffset / sizeof(TableIndex)];
}

std::vector<UINT64> FAT::GetPartsOfFileByStartingCluster(DWORD TableIndex)
{
	DWORD CurrentIndex = TableIndex;
	std::vector<UINT64> RawAddressList;
	const DWORD END_MARKER = 0x0FFFFFFF;
	do {
		RawAddressList.push_back(GetRawAddressByClusterIndex(CurrentIndex));
		CurrentIndex = GetNextTableIndex(CurrentIndex);
	} while (CurrentIndex != END_MARKER);
	return RawAddressList;
}

void FAT::DumpDirectory(UINT64 dirStart)
{
	if (dirStart == 0)
		dirStart = 2;
	DWORD BytesPerCluster = BootSector.Header.LogicalSectorsPerCluster * BootSector.Header.BytesPerLogicalSector;
	const auto& Pointers = GetPartsOfFileByStartingCluster(DWORD(dirStart));
	UINT64 SizeInBytes = Pointers.size() * BytesPerCluster;
	DataStore Data = DataStore(SizeInBytes);
	for (DWORD i = 0; i < Pointers.size(); i++)
		physicalDrive->Read(PBYTE(Data.Raw) + (i * BytesPerCluster), BytesPerCluster, Pointers[i]);

	PENTRY_INFO_FAT entry = PENTRY_INFO_FAT(Data.Raw);
	for(; entry->entryType; entry++)
	{
		//Additionally, directory entries of deleted files will be marked 0xE5 since DOS 3.0.
		const BYTE DELETED_FILE = 0xE5; 

		if (entry->entryType == DELETED_FILE)
			continue;

		std::wstring name;

		if (entry->attribute == 0x0F)
		{
			BYTE countEntriesForFileName = 0b00011111 & entry->entryType;
			while (countEntriesForFileName != 0)
			{
				name = GetStringFromStruct(entry) + name;
				entry++;
				countEntriesForFileName--;
			}
		}
		else
		{
			PCHAR p = entry->shortName + 8;
			while (*p == ' ')
				p--;
			name = std::wstring(entry->shortName, p);
		}
		DWORD StartCluster = entry->highBytes;
		StartCluster <<= (sizeof(entry->lowBytes) * 8);
		StartCluster += entry->lowBytes;
		wprintf(L"%08X %08X %s\n", StartCluster, entry->sizeInBytes, name.c_str());
	};
}

//void FAT::CommandHandler()
//{
//	printf("Root %08X\n", 2);
//	std::string command1;
//	LBA command2;
//	while (true)
//	{
//		std::cin >> command1 >> std::hex >> command2;
//		if (!strcmp(command1.c_str(), "cd"))
//		{
//			system("cls");
//			DumpDirectory(DWORD(command2));
//		}
//	}
//}

BOOL FAT::IsFAT()
{
	const char MagicMSDOS[] = "MSDOS5.0";
	for (BYTE i = 0; i < sizeof(BootSector.Header.Name); i++)
		if (BootSector.Header.Name[i] != MagicMSDOS[i])
			return false;
	return true;
}
