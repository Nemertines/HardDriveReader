#include "HardDriveReader.h"

static void DumpEntries(PhysicalDrive* physicalDrive)
{
	system("cls");
	GUIDPartitionTable gpt = GUIDPartitionTable(physicalDrive);
	if (gpt.IsGPT())
		gpt.DumpArrayOfPartitionEntries();
	else
		printf("Is not GPT\n");

	MBR mbr = MBR(physicalDrive, 0);
	printf("MBR LBA entry points:\n");
	for (BYTE i = 0; i < 4; i++)
		printf("%08X%08X\n",
			PLARGE_INTEGER(&mbr.entries_LBA[i])->HighPart,
			PLARGE_INTEGER(&mbr.entries_LBA[i])->LowPart
		);
	printf("Is not GPT\n");
	printf("LBA enter:\n");
}

void CommandHandler(FileSystem* fs)
{
	fs->DumpRootDirectory();
	UINT64 v = 0;
	std::string command;
	while (true)
	{
		printf("\nEnter command and MFT_FileNumber or FAT_TableIndex\n");
		printf("o - open/c - copy/i - info\n");
		std::cin >> std::hex >> command >> v;
		system("cls");

		switch (command[0])
		{
		case 'o':
		{
			fs->DumpDirectory(v);
			break;
		}
		case 'e':
		{
			fs->ExtractFile(v);
			break;
		}
		case 'i':
		{
			fs->FileInfo(v);
			break;
		}
		case 'q':
		{
			exit(0);
			break;
		}
		default:
			break;
		}
	}
}

int main()
{
	setlocale(LC_ALL, "RU");
	std::string driveName = PhysicalDrive::ChoosingHardDriveNumber();
	PhysicalDrive physicalDrive(driveName.c_str());
	//maybe this is a bad solution, but I left it in order to minimize dynamic memory allocation
	if (physicalDrive.BytesPerSector() != BYTES_PER_SECTOR)
	{
		MessageBoxA(0,"need rebuild with new BYTES_PER_SECTOR value", 0, 0);
		return -1;
	}

	DumpEntries(&physicalDrive);
	LBA selectedLBA = 0;
	std::cin >> std::hex >> selectedLBA;
	system("cls");
	NTFS ntfs(&physicalDrive, selectedLBA);
	if (ntfs.IsNTFS())
		CommandHandler(&ntfs);

	FAT fat = FAT(&physicalDrive, selectedLBA);
	if (fat.IsFAT())
		CommandHandler(&fat);
	return 0;
}
