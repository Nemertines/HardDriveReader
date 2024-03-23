#pragma once
#include "PhysicalDrive.h"

class FileSystem abstract
{
protected:
	PhysicalDrive* physicalDrive;
private:

public:
	FileSystem(PhysicalDrive* pshysicalDrive)
	{
		this->physicalDrive = pshysicalDrive;
	}
	virtual void DumpRootDirectory() {};
	virtual void DumpDirectory(UINT64 DirectoryStart) {};
	virtual void ExtractFile(UINT64 FileStart) {};
	virtual void FileInfo(UINT64 FileStart) {};
};
