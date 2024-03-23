#include "NTFS.h"
static std::string FileTimeToString(PFILETIME time);

void NTFS::PrintFileInfo(std::unordered_set<MFT_NUMBER>& fileReferences, PNTFS_INDEX index)
{
	if (fileReferences.find(index->fileReference) != fileReferences.end())
		return;
	UINT64 RawPointer = MFTNumberToRawAddress(index->fileReference);
	FILE_RECORD FileInDirectory = FILE_RECORD(physicalDrive, RawPointer);
	std::wstring FileName = FileInDirectory.GetFileName();
	fileReferences.insert(index->fileReference);
	wprintf(
		L"%08X %02X %08X %08X%08X %s\n",
		DWORD(fileReferences.size()),
		FileInDirectory.IsDirectory(),
		index->fileReference,
		INSERT_QWORD(RawPointer),
		FileName.c_str()
	);
}

void NTFS::DumpDirectory(UINT64 MFTNumber)
{
	UINT64 RawPointer = MFTNumberToRawAddress(MFT_NUMBER(MFTNumber));
	FILE_RECORD FileRecord = FILE_RECORD(physicalDrive, RawPointer);
	std::unordered_set<MFT_NUMBER> fileReferences;
	if (!FileRecord.IsDirectory())
		return;
	PNTFS_ATTRIBUTE attributeIndexRoot = FileRecord.GetOneAttribute(ATTRIBUTE_TYPE::INDEX_ROOT);
	PINDEX_ROOT_ATTR IndexRootAttribute = PINDEX_ROOT_ATTR(attributeIndexRoot->AttributeBody());
	PINDEX_NODE_HEADER IndexNodeHeader = PINDEX_NODE_HEADER(IndexRootAttribute + 1);
	PNTFS_INDEX index = PNTFS_INDEX(PBYTE(IndexNodeHeader) + IndexNodeHeader->OffsetToFirstIndexEntry);
	PBYTE endOfEntries = PBYTE(IndexNodeHeader) + IndexNodeHeader->TotalSizeOfTheIndexEntries; //or allocated?
	while (PBYTE(index) + index->sizeOfIndexEntry != endOfEntries)
	{
		PrintFileInfo(fileReferences, index);
		index = PNTFS_INDEX(PBYTE(index) + index->sizeOfIndexEntry);
	}

	const auto idxAllocationRunList = FileRecord.GetAttributeRunList(ATTRIBUTE_TYPE::INDEX_ALLOCATION);
	for (DWORD i = 0; i < idxAllocationRunList.size(); i++)
	{
		COUNT clusterCount = idxAllocationRunList[i].second;
		for (DWORD j = 0; j < clusterCount; j++)
		{
			CLUSTER startingCluster = idxAllocationRunList[i].first;
			if (IndexRootAttribute->ClustersPerIndexRecord != 1)
				MessageBoxA(0, "todo: fix ClustersPerIndexRecord > 1", 0, 0);
			DWORD size = IndexRootAttribute->ClustersPerIndexRecord * bootSectorInfo.Header.sectorsPerCluster * bootSectorInfo.Header.bytesPerSector;
			DataStore dataStore = DataStore(size);

			LBA LBAPointerForRead = LBA_Entry + (startingCluster + j) * bootSectorInfo.Header.sectorsPerCluster;
			physicalDrive->Read(dataStore.Raw, size, LBAPointerForRead * bootSectorInfo.Header.bytesPerSector);
			NTFS_INDEX_HEADER* indexHeader = (NTFS_INDEX_HEADER*)dataStore.Raw;
			if (indexHeader->indx != MAGIC_INDX)
			{
				printf("error indexHeader->indx is not INDX\n"); //it's ok?
				continue;
			}

			PNTFS_INDEX currentIndex = PNTFS_INDEX(PBYTE(dataStore.Raw) + 0x18 + indexHeader->offsetToIndexEntries);
			PBYTE endOfEntries = PBYTE(dataStore.Raw) + 0x18 + indexHeader->realSizeOfIndexEntries;
			while ((PBYTE(currentIndex) + currentIndex->sizeOfIndexEntry) != endOfEntries)
			{
				PrintFileInfo(fileReferences, currentIndex);
				currentIndex = PNTFS_INDEX(PBYTE(currentIndex) + currentIndex->sizeOfIndexEntry);
			}
		}
	}

}

BOOL NTFS::IsNTFS()
{
	const DWORD MAGIC_NTFS = 0x5346544E;
	return bootSectorInfo.Header.magic == MAGIC_NTFS;
}

NTFS::NTFS(PhysicalDrive* physicalDrive, LBA entry) : FileSystem(physicalDrive)
{
	LBA_Entry = entry;
	physicalDrive->ReadSectorByLBA(bootSectorInfo.Raw, LBA_Entry);
	if (!IsNTFS())
		return;
	MFT_Pointer = LBA_Entry + bootSectorInfo.Header.MFT_clusterNumber * bootSectorInfo.Header.sectorsPerCluster;
	physicalDrive->ReadByLBA(&MFT_FileRecord, sizeof(MFT_FileRecord), MFT_Pointer);
}

void NTFS::AttributeHandler2(PNTFS_ATTRIBUTE attributeHeader)
{
#define INIT_MAP(value, key) {ATTRIBUTE_TYPE::key, #key},
	const std::map<ATTRIBUTE_TYPE, std::string> AttributeNames =
	{
		SMACRO(INIT_MAP)
	};
	const auto& pair = AttributeNames.find(attributeHeader->Type);
	printf("Type: %08X %s\n", attributeHeader->Type, pair->second.c_str());
	printf("NonResidentFlag: %02X %s\n",
		attributeHeader->NonResidentFlag,
		(attributeHeader->IsResident()) ? "IsResident" : "NonResident"
	);

	std::wstring AttributeName = attributeHeader->GetName();
	wprintf(L"AttributeName: %s\n", AttributeName.c_str());

	if (attributeHeader->IsResident())
	{
		void* attributeBody = attributeHeader->AttributeBody();
		switch (attributeHeader->Type)
		{
		case ATTRIBUTE_TYPE::FILE_NAME:
		{
			PFILE_NAME_ATTR fileNameAttribute = PFILE_NAME_ATTR(attributeBody);
			std::wstring FileName = std::wstring(fileNameAttribute->FileNameInUnicode, fileNameAttribute->FileNameInUnicode + fileNameAttribute->FilenameLengthInCharacters);
			wprintf(L"FilenameNamespace: %02X FileName: %s\n", fileNameAttribute->FilenameNamespace, FileName.c_str());
			printf("ATimeFilAltered		%s\n", FileTimeToString(&fileNameAttribute->ATimeFilAltered).c_str());
			printf("CTimeFileCreation	%s\n", FileTimeToString(&fileNameAttribute->CTimeFileCreation).c_str());
			printf("RTimeFileRead		%s\n", FileTimeToString(&fileNameAttribute->RTimeFileRead).c_str());
			printf("MTimeMFTChanged		%s\n", FileTimeToString(&fileNameAttribute->MTimeMFTChanged).c_str());
			break;
		}
		case ATTRIBUTE_TYPE::INDEX_ROOT:
		{
			//PINDEX_ROOT_ATTR IndexRootAttribute = PINDEX_ROOT_ATTR(attributeBody);
			//PINDEX_NODE_HEADER IndexNodeHeader = PINDEX_NODE_HEADER(IndexRootAttribute + 1);
			//PNTFS_INDEX index = PNTFS_INDEX(PBYTE(IndexNodeHeader) + IndexNodeHeader->OffsetToFirstIndexEntry);
			//PBYTE endOfEntries = PBYTE(IndexNodeHeader) + IndexNodeHeader->TotalSizeOfTheIndexEntries; //or allocated?
			//while (PBYTE(index) + index->sizeOfIndexEntry != endOfEntries)
			//{
			//	//там есть с разными именами одинаковые ссылки
			//	//if (directory.directoryContent.find(index->fileReference) == directory.directoryContent.end())
			//	//{
			//	//	BYTES_OFFSET rawAddress2 = MFTNumberToRawAddress(index->fileReference);
			//	//	NTFS_FILE file(GetFileRecord(rawAddress2), rawAddress2);
			//	//	printf("%08X %09d ", index->fileReference, (DWORD)file.rawAddress);
			//	//	std::wcout << file.fileName << L" " << file.isDirectory << std::endl;
			//	//	directory.directoryContent[index->fileReference] = file.fileName;
			//	//}
			//	//wprintf(L"%s\n", index.n);
			//	index = PNTFS_INDEX(PBYTE(index) + index->sizeOfIndexEntry);
			//}
			break;
		}
		case ATTRIBUTE_TYPE::STANDARD_INFORMATION:
		{
			PFILETIME p = PFILETIME(attributeBody);
			printf("ATimeFilAltered		%s\n", FileTimeToString(p).c_str());
			p++;
			printf("ATimeFilAltered		%s\n", FileTimeToString(p).c_str());
			p++;
			printf("ATimeFilAltered		%s\n", FileTimeToString(p).c_str());
			p++;
			printf("ATimeFilAltered		%s\n", FileTimeToString(p).c_str());
			p++;
			break;
		}
		case ATTRIBUTE_TYPE::ATTRIBUTE_LIST:
		case ATTRIBUTE_TYPE::VOLUME_VERSION_OR_OBJECT_ID:
		case ATTRIBUTE_TYPE::SECURITY_DESCRIPTOR:
		case ATTRIBUTE_TYPE::VOLUME_NAME:
		case ATTRIBUTE_TYPE::VOLUME_INFORMATION:
		case ATTRIBUTE_TYPE::DATA:
		case ATTRIBUTE_TYPE::INDEX_ALLOCATION:
		case ATTRIBUTE_TYPE::BITMAP:
		case ATTRIBUTE_TYPE::SYMBOLIC_LINK_OR_REPARSE_POINT:
		case ATTRIBUTE_TYPE::EA_INFORMATION:
		case ATTRIBUTE_TYPE::EA:
		case ATTRIBUTE_TYPE::PROPERTY_SET:
		case ATTRIBUTE_TYPE::LOGGED_UTILITY_STREAM:
		case ATTRIBUTE_TYPE::END:
		default:
			break;
		}
	}
	else
	{
		switch (attributeHeader->Type)
		{
		case ATTRIBUTE_TYPE::DATA:
		case ATTRIBUTE_TYPE::INDEX_ALLOCATION:
		case ATTRIBUTE_TYPE::FILE_NAME:
		case ATTRIBUTE_TYPE::STANDARD_INFORMATION:
		case ATTRIBUTE_TYPE::ATTRIBUTE_LIST:
		case ATTRIBUTE_TYPE::VOLUME_VERSION_OR_OBJECT_ID:
		case ATTRIBUTE_TYPE::SECURITY_DESCRIPTOR:
		case ATTRIBUTE_TYPE::VOLUME_NAME:
		case ATTRIBUTE_TYPE::VOLUME_INFORMATION:
		case ATTRIBUTE_TYPE::INDEX_ROOT:
		case ATTRIBUTE_TYPE::BITMAP:
		case ATTRIBUTE_TYPE::SYMBOLIC_LINK_OR_REPARSE_POINT:
		case ATTRIBUTE_TYPE::EA_INFORMATION:
		case ATTRIBUTE_TYPE::EA:
		case ATTRIBUTE_TYPE::PROPERTY_SET:
		case ATTRIBUTE_TYPE::LOGGED_UTILITY_STREAM:
		case ATTRIBUTE_TYPE::END:
		default:
			break;
		}
	}
	printf("\n");
}

UINT64 NTFS::MFTNumberToRawAddress(MFT_NUMBER MFTNumber)
{
	const auto MFTDataRunList = MFT_FileRecord.GetAttributeRunList(ATTRIBUTE_TYPE::DATA); //need update mft file record everytime
	for (DWORD i = 0; i < MFTDataRunList.size(); i++)
	{
		//first это смещение в кластерах от LBA_NTFSEntry
		//second это размер данных в кластерах
		UINT64 SectorsInCurrentRunData = MFTDataRunList[i].second * bootSectorInfo.Header.sectorsPerCluster;
		SectorsInCurrentRunData /= 2;
		MFT_NUMBER limit = MFT_NUMBER(SectorsInCurrentRunData);
		//RangeOfFileRecords 0 - limit, limit - nextLimit, 
		if (MFTNumber < limit)
		{
			UINT64 startingSector = LBA_Entry + MFTDataRunList[i].first * bootSectorInfo.Header.sectorsPerCluster;
			return startingSector * bootSectorInfo.Header.bytesPerSector + MFTNumber * SIZE_OF_FILE_RECORD;
		}
		MFTNumber -= limit;
	}
	return 0;
}

void NTFS::FileInfo(UINT64 MFTNumber)
{
	UINT64 RawPointer = MFTNumberToRawAddress(MFT_NUMBER(MFTNumber));
	FILE_RECORD FileRecord = FILE_RECORD(physicalDrive, RawPointer);

	printf("MFTNumber: %08X, RawPointer: %08X%08X\n", MFTNumber, INSERT_QWORD(RawPointer));
	printf("FileFlag: %04X %s\n\n", FileRecord.Header.flags, (FileRecord.Header.flags & 2) ? "dir" : "file");

	PNTFS_ATTRIBUTE CurrentAttributeHeader = FileRecord.GetFirstAttribute();
	while (CurrentAttributeHeader->Type != ATTRIBUTE_TYPE::END)
	{
		AttributeHandler2(CurrentAttributeHeader);
		CurrentAttributeHeader = CurrentAttributeHeader->NextAttribute();
	}
}

static std::string FileTimeToString(PFILETIME time)
{
	std::string result;
	SYSTEMTIME st;
	if (!FileTimeToSystemTime(time, &st))
		return result;
	char buffer[0x100];
	sprintf_s(buffer, sizeof(buffer), "[%02d:%02d:%04d %02d:%02d]", st.wDay, st.wMonth, st.wYear, st.wHour, st.wMinute);
	return std::string(buffer);
}

void NTFS::ExtractFile(UINT64 MFTNumber)
{
	UINT64 RawPointer = MFTNumberToRawAddress(MFT_NUMBER(MFTNumber));
	FILE_RECORD FileRecord = FILE_RECORD(physicalDrive, RawPointer);
	if (FileRecord.IsDirectory())
		return;
	std::wstring FileName = FileRecord.GetFileName();
	HANDLE hFile = CreateFileW(FileName.c_str(),
		GENERIC_WRITE,
		FILE_SHARE_READ|FILE_SHARE_WRITE,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	DWORD NumberOfBytesToWrite;
	PNTFS_ATTRIBUTE AttributeData = FileRecord.GetOneAttribute(ATTRIBUTE_TYPE::DATA);
	if (AttributeData->IsResident())
	{
		void* DataBody = AttributeData->AttributeBody();
		WriteFile(hFile, DataBody, AttributeData->Header.ResidentHeader.bodySize, &NumberOfBytesToWrite, NULL);
		CloseHandle(hFile);
		return;
	}

	UINT64 RemainingFileSize = AttributeData->Header.NonResidentHeader.realSize;
	const auto DataRunList = FileRecord.GetAttributeRunList(ATTRIBUTE_TYPE::DATA);
	for (DWORD i = 0; i < DataRunList.size(); i++)
	{
		COUNT clusterCount = DataRunList[i].second;
		for (DWORD j = 0; j < clusterCount; j++)
		{
			CLUSTER startingCluster = DataRunList[i].first;
			DWORD BytesPerCluster = bootSectorInfo.Header.bytesPerSector * bootSectorInfo.Header.sectorsPerCluster;
			DataStore dataStore = DataStore(BytesPerCluster); //alloc cluster size
			LBA LBAPointer = LBA_Entry + (startingCluster + j) * bootSectorInfo.Header.sectorsPerCluster;
			physicalDrive->Read(dataStore.Raw, BytesPerCluster, LBAPointer * bootSectorInfo.Header.bytesPerSector); //read full cluster

			UINT64 SizeForWrite;
			if (RemainingFileSize > BytesPerCluster)
				SizeForWrite = BytesPerCluster;
			else
				SizeForWrite = RemainingFileSize;
			WriteFile(hFile, dataStore.Raw, SizeForWrite, &NumberOfBytesToWrite, NULL);
			RemainingFileSize -= SizeForWrite;
			if (RemainingFileSize == 0)
				break;
		}
		if (RemainingFileSize == 0)
			break;
	}
	CloseHandle(hFile);
	return;
}
