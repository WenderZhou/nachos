// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"

void SetTime(int* Time)
{
    time_t timep;
    time(&timep);
    struct tm *p = gmtime(&timep);
    Time[0] = 1900 + p->tm_year;
    Time[1] = 1 + p->tm_mon;
    Time[2] = p->tm_mday;
    Time[3] = 8 + p->tm_hour;
    Time[4] = p->tm_min;
}

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------
bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 
    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);

    SetTime(createTime);
    memcpy(visitTime,createTime,sizeof(createTime));
    memcpy(modifyTime,createTime,sizeof(createTime));

    if (freeMap->NumClear() < numSectors)
	    return FALSE;		// not enough space
    
    for (int i = 0; i < numSectors && i < NumDirect; i++)
        dataSectors[i] = freeMap->Find();
    extraSector = -1;
    if(numSectors > NumDirect)
        extraSector = freeMap->Find();

    int sector = extraSector;
    int rest = numSectors - NumDirect;
    while(rest > 0)
    {
        ExtraFileHeader *extraHdr = new ExtraFileHeader();
        for (int i = 0; i < rest && i < NumIndirect; i++)
            extraHdr->dataSectors[i] = freeMap->Find();
        extraHdr->extraSector = -1;
        if(rest > NumIndirect)
            extraHdr->extraSector = freeMap->Find();

        extraHdr->WriteBack(sector);

        rest -= NumIndirect;
        sector = extraHdr->extraSector;
        delete extraHdr;
    }

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------
void 
FileHeader::Deallocate(BitMap *freeMap)
{
    for (int i = 0; i < NumDirect && i < numSectors; i++) {
        ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
        freeMap->Clear((int) dataSectors[i]);
    }
    int sector = extraSector;
    int rest = numSectors - NumDirect;
    while(sector != -1)
    {
        ExtraFileHeader *extraHdr = new ExtraFileHeader();
        extraHdr->FetchFrom(sector);

        for (int i = 0; i < NumIndirect && i < rest; i++)
        {
            ASSERT(freeMap->Test((int) extraHdr->dataSectors[i]));
            freeMap->Clear((int) extraHdr->dataSectors[i]);
        }
        ASSERT(freeMap->Test(sector));
        freeMap->Clear(sector);
        
        rest -= NumIndirect;
        sector = extraHdr->extraSector;
        delete extraHdr;
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------
void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------
void
FileHeader::WriteBack(int sector)
{
    hdrSector = sector;
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------
int
FileHeader::ByteToSector(int offset)
{
    int idx = offset / SectorSize;
    if(idx < NumDirect)
        return dataSectors[idx];

    idx -= NumDirect;
    int sector = extraSector;
    ExtraFileHeader *extraHdr = new ExtraFileHeader();
    extraHdr->FetchFrom(sector);
    while(idx >= NumIndirect)
    {
        sector = extraHdr->extraSector;
        extraHdr->FetchFrom(sector);
        idx -= NumIndirect;
    }
    int result = extraHdr->dataSectors[idx];
    delete extraHdr;
    
    ASSERT(result != 0);
    return result;
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------
int
FileHeader::FileLength()
{
    return numBytes;
}

bool FileHeader::Extend(BitMap *freeMap, int extraSize)
{
    numBytes += extraSize;
    int numSectorsNew = divRoundUp(numBytes, SectorSize);
    if (numBytes == numSectorsNew)  // need no more space
        return TRUE;
    if (freeMap->NumClear() < numSectorsNew - numSectors)
	    return FALSE;		// not enough space
    if (numSectorsNew < NumDirect)
    {
        for (int i = numSectors; i < numSectorsNew; i++)
            dataSectors[i] = freeMap->Find();
    }
    else if(numSectors < NumDirect && numSectorsNew >= NumDirect)
    {
        for (int i = numSectors; i < NumDirect; i++)
            dataSectors[i] = freeMap->Find();
        extraSector = freeMap->Find();
        int sector = extraSector;
        int rest = numSectorsNew - NumDirect;
        while(rest > 0)
        {
            ExtraFileHeader *extraHdr = new ExtraFileHeader();
            for (int i = 0; i < rest && i < NumIndirect; i++)
                extraHdr->dataSectors[i] = freeMap->Find();
            extraHdr->extraSector = -1;
            if(rest > NumIndirect)
                freeMap->Find();
            extraHdr->WriteBack(sector);
            rest -= NumIndirect;
            sector = extraHdr->extraSector;
            delete extraHdr;
        }
    }
    else
    {
        int rest1 = numSectors - NumDirect;
        int rest2 = numSectorsNew - NumDirect;
        int sector = extraSector;
        ExtraFileHeader *extraHdr = new ExtraFileHeader();
        extraHdr->FetchFrom(sector);
        while(rest1 > NumIndirect)
        {
            sector = extraHdr->extraSector;
            extraHdr->FetchFrom(sector);
            rest1 -= NumIndirect;
            rest2 -= NumIndirect;
        }

        for (int i = rest1; i < rest2 && i < NumIndirect; i++)
            extraHdr->dataSectors[i] = freeMap->Find();
        extraHdr->extraSector = -1;
        if(rest2 > NumIndirect)
            extraHdr->extraSector = freeMap->Find();
        
        extraHdr->WriteBack(sector);

        sector = extraHdr->extraSector;
        rest2 -= NumIndirect;
        while(rest2 > 0)
        {
            extraHdr->FetchFrom(sector);

            for (int i = 0; i < rest2 && i < NumIndirect; i++)
                extraHdr->dataSectors[i] = freeMap->Find();
            extraHdr->extraSector = -1;
            if(rest2 > NumIndirect)
                extraHdr->extraSector = freeMap->Find();

            extraHdr->WriteBack(sector);

            rest2 -= NumIndirect;
            sector = extraHdr->extraSector;
        }
        delete extraHdr;
    }

    numSectors = numSectorsNew;
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------
void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];

    printf("FileHeader contents.  File size: %d.  File blocks:\n", numBytes);
    for (i = 0; i < NumDirect && i < numSectors; i++)
        printf("%d ", dataSectors[i]);
    int sector = extraSector;
    int rest = numSectors - NumDirect;
    while(sector != -1)
    {
        ExtraFileHeader *extraHdr = new ExtraFileHeader();
        extraHdr->FetchFrom(sector);
        for(int i = 0; i < rest && i < NumIndirect; i++)
            printf("%d ", extraHdr->dataSectors[i]);
        sector = extraHdr->extraSector;
        rest -= NumIndirect;
    }
    
    // printf("\nCreate time:  %d.%d.%d %d:%02d",createTime[0],createTime[1],
    //                 createTime[2],createTime[3],createTime[4]);
    // printf("\nLast visit:   %d.%d.%d %d:%02d",visitTime[0],visitTime[1],
    //                 visitTime[2],visitTime[3],visitTime[4]);
    // printf("\nLast modify:  %d.%d.%d %d:%02d",modifyTime[0],modifyTime[1],
    //                 modifyTime[2],modifyTime[3],modifyTime[4]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < NumDirect && i < numSectors; i++) {
	    synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
	        if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
		        printf("%c", data[j]);
            else
		        printf("\\%x", (unsigned char)data[j]);
	    }
        printf("\n"); 
    }
    sector = extraSector;
    rest = numSectors - NumDirect;
    while(sector != -1)
    {
        ExtraFileHeader *extraHdr = new ExtraFileHeader();
        extraHdr->FetchFrom(sector);
        for(int i = 0; i < rest && i < NumIndirect; i++)
        {
            synchDisk->ReadSector(extraHdr->dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
	        }
            printf("\n"); 
        }
        rest -= NumIndirect;
        sector = extraHdr->extraSector;
    }

    delete [] data;
}

int FileHeader::GetHdrSector()
{
    return hdrSector;
}

void FileHeader::UpdateVisitTime(){ SetTime(visitTime); }
void FileHeader::UpdateModifyTime(){ SetTime(modifyTime); }

void
ExtraFileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
}

void
ExtraFileHeader::WriteBack(int sector)
{
    synchDisk->WriteSector(sector, (char *)this); 
}