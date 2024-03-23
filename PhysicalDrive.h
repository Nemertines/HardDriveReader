#pragma once
#include <Windows.h>
#include <vector>
#include <string>
#include "DefinedStructs.hpp"

class PhysicalDrive
{
public:
	PhysicalDrive(const char* DeviceName);
	BOOL Read(void* buffer, DWORD size, UINT64 pointer);
	BOOL ReadSectorByLBA(void* buffer, LBA number);
	BOOL ReadByLBA(void* buffer, DWORD size, LBA number);
	static std::string ChoosingHardDriveNumber();
	DWORD BytesPerSector();
private:
	const char* DeviceName = nullptr;
	DISK_GEOMETRY DiskGeometry{ 0 };
	BOOL InitDiskGeometry();
	HANDLE OpenDevice();
	static std::vector<std::string> GetValidPhysicalDrives();
};

struct DataStore
{
	void* Raw = nullptr;
	DataStore(size_t size);
	~DataStore();
};
