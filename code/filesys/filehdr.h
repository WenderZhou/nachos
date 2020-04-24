// filehdr.h 
//	Data structures for managing a disk file header.  
//
//	A file header describes where on disk to find the data in a file,
//	along with other information about the file (for instance, its
//	length, owner, etc.)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#ifndef FILEHDR_H
#define FILEHDR_H

#include "disk.h"
#include "bitmap.h"
#include "time.h"

#define FilePathLen 24
#define NumDirect 	((SectorSize - 19 * sizeof(int) - FilePathLen * sizeof(char)) / sizeof(int))
#define NumIndirect 	((SectorSize - 1 * sizeof(int)) / sizeof(int))
#define MaxFileSize 	(NumDirect * SectorSize)

// The following class defines the Nachos "file header" (in UNIX terms,  
// the "i-node"), describing where on disk to find all of the data in the file.
// The file header is organized as a simple table of pointers to
// data blocks. 
//
// The file header data structure can be stored in memory or on disk.
// When it is on disk, it is stored in a single sector -- this means
// that we assume the size of this data structure to be the same
// as one disk sector.  Without indirect addressing, this
// limits the maximum file length to just under 4K bytes.
//
// There is no constructor; rather the file header can be initialized
// by allocating blocks for the file (if it is a new file), or by
// reading it from disk.

class FileHeader {
  public:
    bool Allocate(BitMap *bitMap, int fileSize);// Initialize a file header, 
						//  including allocating space 
						//  on disk for the file data
    void Deallocate(BitMap *bitMap);  		// De-allocate this file's 
						//  data blocks

    void FetchFrom(int sectorNumber); 	// Initialize file header from disk
    void WriteBack(int sectorNumber); 	// Write modifications to file header
					//  back to disk

    int ByteToSector(int offset);	// Convert a byte offset into the file
					// to the disk sector containing
					// the byte

    int FileLength();			// Return the length of the file 
					// in bytes

    bool Extend(BitMap *freeMap, int extraSize);

    void Print();			// Print the contents of the file.

    int GetHdrSector();

    void UpdateVisitTime();
    void UpdateModifyTime();

    void SetPath(char* filePath);

  private:
    int numBytes;			// Number of bytes in the file
    int numSectors;			// Number of data sectors in the file
    int hdrSector;    // sector number of the header
    char path[FilePathLen];      // sector of directory on the path
    int createTime[5];
    int visitTime[5];
    int modifyTime[5];
    int extraSector;  // sector for indirect index, -1 if there isn't one
    int dataSectors[NumDirect];		// Disk sector numbers for each data 
					// block in the file
};

class ExtraFileHeader{
public:
  void FetchFrom(int sectorNumber);
  void WriteBack(int sectorNumber);

  int extraSector;  // sector for indirect index, -1 if there isn't one
  int dataSectors[NumIndirect];
};

#endif // FILEHDR_H
