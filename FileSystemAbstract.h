#pragma once
#include "PhysicalDrive.h"

class FileSystem abstract
{
private:
public:
	PhysicalDrive* pshysicalDrive = nullptr;
	FileSystem(PhysicalDrive* pshysicalDrive, LBA BootSector)
	{
		this->pshysicalDrive = pshysicalDrive;
	}
	virtual void DumpDirectory(UINT64 DirectoryStart) {};
	virtual void ExtractFile(UINT64 FileStart) {};
	virtual void FileInfo(UINT64 FileStart) {};
};
