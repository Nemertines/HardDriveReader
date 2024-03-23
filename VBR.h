#pragma once

struct InfoFromVBR
{
	WORD BytesPerSector;
	UCHAR SectorsPerCluster;
	WORD SectorsPerTrack;
	UINT64 MFT_ClusterNumber;
	UINT64 MFTMirr_ClusterNumber;
};

void ReadVBR(InfoFromVBR &a,UCHAR* buffer)
{
	a.BytesPerSector = *(WORD*)(buffer+0x0B); //wiki ntfs en
	a.SectorsPerCluster = buffer[0x0D];
	a.MFT_ClusterNumber = *(UINT64*)(buffer + 0x30);
	a.MFTMirr_ClusterNumber = *(UINT64*)(buffer + 0x38);
}


