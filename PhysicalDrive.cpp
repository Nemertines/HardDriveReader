#include "PhysicalDrive.h"

std::vector<std::string> PhysicalDrive::GetValidPhysicalDrives()
{
	std::vector<std::string> physicalDrivesList;
	char deviceName[] = "\\\\.\\PhysicalDrive0";
	for (int i = 0; i < 10; i++)
	{
		HANDLE hDevice = CreateFileA(deviceName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (hDevice != INVALID_HANDLE_VALUE)
		{
			physicalDrivesList.push_back(deviceName);
			CloseHandle(hDevice);
		}
		int error = GetLastError();
		deviceName[17]++;
	}
	return physicalDrivesList;
}

std::string PhysicalDrive::ChoosingHardDriveNumber()
{
	const auto validPhysicalDrives = GetValidPhysicalDrives();

	printf("Enter physical drive number:\n");
	for (int i = 0; i < validPhysicalDrives.size(); i++)
		printf("	%02d %s\n", i, validPhysicalDrives[i].c_str());
	int number = 0;
	scanf_s("%d", &number);
	if (number >= validPhysicalDrives.size())
	{
		printf("error\n");
		exit(0);
	}
	return validPhysicalDrives[number];
}

PhysicalDrive::PhysicalDrive(const char* DeviceName)
{
	this->DeviceName = DeviceName;
	InitDiskGeometry();
}

BOOL PhysicalDrive::Read(void* buffer, DWORD size, UINT64 pointer)
{
	HANDLE hDevice = OpenDevice();
	if (hDevice == INVALID_HANDLE_VALUE)
		return false;
	if (!SetFilePointerEx(hDevice, *PLARGE_INTEGER(&pointer), 0, FILE_BEGIN))
	{
		MessageBoxA(0, "error", 0, 0);
		CloseHandle(hDevice);
		return false;
	}
	DWORD numberOfBytesRead = 0;
	if (!ReadFile(hDevice, buffer, size, &numberOfBytesRead, 0))
	{
		MessageBoxA(0, "error", 0, 0);
		CloseHandle(hDevice);
		return false;
	}
	CloseHandle(hDevice);
	return true;
}

BOOL PhysicalDrive::ReadSectorByLBA(void* buffer, LBA number)
{
	return Read(buffer, DiskGeometry.BytesPerSector, number * DiskGeometry.BytesPerSector);
}

BOOL PhysicalDrive::ReadByLBA(void* buffer, DWORD size, LBA number)
{
	return Read(buffer, size, number * DiskGeometry.BytesPerSector);
}

DWORD PhysicalDrive::BytesPerSector()
{
	return DiskGeometry.BytesPerSector;
}

BOOL PhysicalDrive::InitDiskGeometry()
{
	DWORD bytesWrited = 0;
	HANDLE hDevice = OpenDevice();
	BOOL bResult = DeviceIoControl(
		hDevice,
		IOCTL_DISK_GET_DRIVE_GEOMETRY,
		0,
		0,
		&DiskGeometry,
		sizeof(DiskGeometry),
		&bytesWrited,
		0
	);
	CloseHandle(hDevice);
	return true;
}

HANDLE PhysicalDrive::OpenDevice()
{
	//winhex
	//	\\.\PhysicalDrive0
	//	0x80000000
	// 3
	// 0
	// 3 
	// 0x80
	// 0 
	return CreateFileA(
		DeviceName,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0
	);
}



DataStore::DataStore(size_t size)
{
	Raw = VirtualAlloc(0, size, MEM_COMMIT, PAGE_READWRITE);
	if (Raw == nullptr)
		MessageBoxA(0, "VirtualAlloc error", 0, 0);
}

DataStore::~DataStore()
{
	if (Raw != nullptr)
	{
		VirtualFree(Raw, 0, MEM_RELEASE);
		Raw = nullptr;
	}
}
