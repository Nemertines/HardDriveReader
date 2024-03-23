#include "ConsoleApplication1.hpp"

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
}

void NTFSCommandHandler(NTFS& ntfs)
{
	ntfs.DumpDirectory(ntfs.MFT_FileRecord.GetParentDirectory());
	MFT_NUMBER MFT_FileNumber = 0;
	std::string command;
	while (true)
	{
		printf("\nEnter command and MFT_FileNumber\n");
		printf("o - open/c - copy/i - info\n");
		std::cin >> std::hex >> command >> MFT_FileNumber;
		system("cls");
		
		switch (command[0])
		{
		case 'o':
		{
			ntfs.DumpDirectory(MFT_FileNumber);
			break;
		}
		case 'c':
		{
			ntfs.ExtractFile(MFT_FileNumber);
			break;
		}
		case 'i':
		{
			ntfs.ParseFile(MFT_FileNumber);
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
	DumpEntries(&physicalDrive);

	LBA selectedLBA = 0;
	std::cin >> std::hex >> selectedLBA;
	system("cls");
	NTFS ntfs(&physicalDrive, selectedLBA);
	if (ntfs.IsNTFS())
		NTFSCommandHandler(ntfs);
	
	FAT fat = FAT(&physicalDrive, selectedLBA);
	if (!fat.IsFAT())
		fat.CommandHandler();
	return 0;
}
