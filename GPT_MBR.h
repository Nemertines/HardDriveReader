#include "PhysicalDrive.h"

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
	BYTE buffer[BYTES_PER_SECTOR];
	physicalDrive->ReadSectorByLBA(buffer, 1);
	memcpy(&gpt, buffer, sizeof(gpt));
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
	DWORD sizeOfEntriesArray = gpt.NumberOfPartitionEntries * gpt.SizeOfASinglePartitionEntry;
	DataStore ds = DataStore(sizeOfEntriesArray);
	if (sizeof(PARTITION_ENTRY) != gpt.SizeOfASinglePartitionEntry)
	{
		MessageBoxA(0, "SizeOfASinglePartitionEntry error", 0, 0);
		return false;
	}
	if (!physicalDrive->ReadByLBA(ds.Raw, sizeOfEntriesArray, gpt.LBAPartitionEntries))
		return false;

	for (DWORD i = 0; i < gpt.NumberOfPartitionEntries; i++)
		PartitionEntries.push_back(PPARTITION_ENTRY(ds.Raw)[i]);

	for (const auto& PartitionEntry : PartitionEntries)
	{
		if (!PartitionEntry.firstLBA) //how to detect end of entries?
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
	return true;
}


class MBR
{
public:
	UINT64 entries_LBA[4];
	PhysicalDrive* physicalDrive;
	MBR(PhysicalDrive* physicalDrive, LBA LBANumber);
private:
	MBR_PARTITION_ENTRY partition_entries[4];
};

static UINT64 CHStoLBA(const PDISK_GEOMETRY dg, UINT64 c, UINT64 h, UINT64 s)
{
	//In CHS addressing the sector numbers always start at 1, there is no sector 0.
	//https://en.wikipedia.org/wiki/Cylinder-head-sector
	if (!s)
		return 0;
	return (c * (UINT64)dg->TracksPerCylinder * (UINT64)dg->SectorsPerTrack) + (h * (UINT64)dg->SectorsPerTrack) + (s - 1);
}

MBR::MBR(PhysicalDrive* physicalDrive, LBA LBANumber)
{
	this->physicalDrive = physicalDrive;
	BYTE workBuffer[BYTES_PER_SECTOR];
	physicalDrive->ReadSectorByLBA(workBuffer, LBANumber);
	for (BYTE i = 0; i < 4; i++)
	{
		DWORD offset = 0x10 * i + 0x1BE;
		partition_entries[i] = *(MBR_PARTITION_ENTRY*)(workBuffer + offset);
		UINT64 head = partition_entries[i].headStart;
		UINT64 sector = partition_entries[i].sectorStart & 0b00111111;
		UINT64 cylinder = partition_entries[i].cylinderStart + ((partition_entries[i].sectorStart >> 6) << 8);
		entries_LBA[i] = CHStoLBA(physicalDrive->GetDiskGeometry(), cylinder, head, sector);
		if (partition_entries[i].firstSectorOffset != entries_LBA[i])
			MessageBoxA(0,"MBR parsing error", 0, 0);
	}
}
