#include "PhysicalDrive.h"


class FileSystemParser
{
public:
private:
};


class GUIDPartitionTable
{
public:
	PhysicalDrive* physicalDrive;
	GUIDPartitionTable(PhysicalDrive* physicalDrive);
	BOOL DumpHeader();
	BOOL DumpArrayOfPartitionEntries();
	BOOL IsGPT();
private:
	GPT_HEADER gpt{};
	std::vector<PARTITION_ENTRY> PartitionEntries;
};

BOOL GUIDPartitionTable::IsGPT()
{
	const UINT64 EFI_PART_MAGIC = 0x5452415020494645;
	return *PUINT64(&gpt.Signature) == EFI_PART_MAGIC;
}

GUIDPartitionTable::GUIDPartitionTable(PhysicalDrive* physicalDrive)
{
	this->physicalDrive = physicalDrive;
	BYTE* buffer = new BYTE[physicalDrive->BytesPerSector()];
	physicalDrive->ReadSectorByLBA(buffer, 1);
	memcpy(&gpt, buffer, sizeof(gpt));
	delete[] buffer;
}

BOOL GUIDPartitionTable::DumpHeader()
{
	printf("Signature:			");
	for (int i = 0; i < sizeof(gpt.Signature); i++)
		printf("%c", gpt.Signature[i]);
	printf("\n");
	printf("Revision:			%08X\n", gpt.Revision);
	printf("HeaderSize:			%08X\n", gpt.HeaderSize);
	printf("CRC32OfHeader:			%08X\n", gpt.CRC32OfHeader);
	printf("Reserved(zero):			%08X\n", gpt.Reserved);
	printf("CurrentLBA:			%08X%08X\n", INSERT_QWORD(gpt.CurrentLBA));
	printf("BackupLBA:			%08X%08X\n", INSERT_QWORD(gpt.BackupLBA));
	printf("FirstUsableLBA:			%08X%08X\n", INSERT_QWORD(gpt.FirstUsableLBA));
	printf("LastUsableLBA:			%08X%08X\n", INSERT_QWORD(gpt.LastUsableLBA));
	printf("DiskGUID			");
	for (int i = 0; i < sizeof(gpt.DiskGUID); i++)
		printf("%02X ", gpt.DiskGUID[i]);
	printf("\n");
	printf("LBAPartitionEntries:		%08X%08X\n", INSERT_QWORD(gpt.LBAPartitionEntries));
	printf("NumberOfPartitionEntries:	%08X\n", gpt.NumberOfPartitionEntries);
	printf("SizeOfASinglePartitionEntry:	%08X\n", gpt.SizeOfASinglePartitionEntry);
	printf("CRC32OfPartitionEntries:	%08X\n", gpt.CRC32OfPartitionEntries);
	printf("____________________________________\n");
	return 1;
}

BOOL GUIDPartitionTable::DumpArrayOfPartitionEntries()
{
	//как узнать, сколько PARTITION_ENTRY? количества нет.
	//был тупой цикл, который смотрит не равен ли нулю первый байт
	SIZE_T sizeOfEntriesArray = gpt.NumberOfPartitionEntries * gpt.SizeOfASinglePartitionEntry;
	DataStore ds = DataStore(sizeOfEntriesArray);
	if (sizeof(PARTITION_ENTRY) != gpt.SizeOfASinglePartitionEntry)
		MessageBoxA(0,"SizeOfASinglePartitionEntry error", 0, 0);
	physicalDrive->ReadByLBA(ds.Raw, sizeOfEntriesArray, gpt.LBAPartitionEntries);
	for (DWORD i = 0; i < gpt.NumberOfPartitionEntries; i++)
		PartitionEntries.push_back(PPARTITION_ENTRY(ds.Raw)[i]);

	for (const auto& PartitionEntry : PartitionEntries)
	{
		if (!PartitionEntry.firstLBA) //???
			break;
		wprintf(L"PartitionName		");
		for (int i = 0; i < sizeof(PartitionEntry.partitionName); i++)
		{
			if (!PartitionEntry.partitionName[i])
				break;
			wprintf(L"%c", PartitionEntry.partitionName[i]);
		}
		printf("\n");
		printf("FirstLBA:		%08X%08X\n", INSERT_QWORD(PartitionEntry.firstLBA));
		printf("LastLBA:		%08X%08X\n", INSERT_QWORD(PartitionEntry.lastLBA));
	}
	return 1;
}









class MBR
{
public:
	UINT64 entries_LBA[4];
	PhysicalDrive* physicalDrive;
	MBR(PhysicalDrive* physicalDrive, LBA LBANumber);
	//static BOOL IsMBR(PhysicalDrive* physicalDrive)
	//{
	//	BOOL result = false;
	//	BYTE* buffer = new BYTE[physicalDrive->BytesPerSector()];
	//	physicalDrive->ReadSectorByLBA(buffer, 0);
	//	delete[] buffer;
	//	return 
	//}
private:
	MBR_PARTITION_ENTRY partition_entries[4];
	void DumpEntries();
};

static UINT64 CHStoLBA(DISK_GEOMETRY dg, UINT64 c, UINT64 h, UINT64 s)
{
	//UINT64 HPT = *(UINT64*)&dg.Cylinders;
	return (c * (UINT64)dg.TracksPerCylinder * (UINT64)dg.SectorsPerTrack) + (h * (UINT64)dg.SectorsPerTrack) + (s - 1);
}

MBR::MBR(PhysicalDrive* physicalDrive, LBA LBANumber)
{
	this->physicalDrive = physicalDrive;
	BYTE workBuffer[0x200];
	physicalDrive->ReadSectorByLBA(workBuffer, LBANumber);
	for (BYTE i = 0; i < 4; i++)
	{
		DWORD offset = 0x10 * i + 0x1BE;
		partition_entries[i] = *(MBR_PARTITION_ENTRY*)(workBuffer + offset);

		/*partition_entries[i].cylinderStart = 0x04;
		partition_entries[i].headStart = 0xfe;
		partition_entries[i].sectorStart = 0x7f;*/
		UINT64 head = partition_entries[i].headStart;
		UINT64 sector = partition_entries[i].sectorStart & 0b00111111;
		UINT64 asd = partition_entries[i].sectorStart >> 6;
		asd <<= 8;
		UINT64 cylinder = partition_entries[i].cylinderStart;
		cylinder += asd;
		entries_LBA[i] = partition_entries[i].firstSectorOffset;
		/*
		 20 21 00 800
		df 14 0c 32800
		fe ff ff c382800

		1111 1110	fe
		1111 1 		1f	сектор (биты 0—5)
		111 1111 1111	7FF	(старшие биты 8, 9 хранятся в байте номера сектора)

		1111 1110	fe	головка
		1111 11 	3f	сектор (биты 0—5)
		11 1111 1111	3FF	цилиндр (старшие биты 8, 9 хранятся в байте номера сектора)

		ff  	255	tpc
		3f 	 63	spt
		200 	512	bps
		ed81 	60801 	cylindres
		*/
		//entries_LBA[i] = CHStoLBA(diskGeometry, cylinder, head, sector);
		//std::cout << std::hex << partition_entries[i].firstSectorOffset << " " << entries_LBA[i] << std::endl;
	}
}
