// filesys.cc 
//	Routines to manage the overall operation of the file system.
//	Implements routines to map from textual file names to files.
//
//	Each file in the file system has:
//	   A file header, stored in a sector on disk 
//		(the size of the file header data structure is arranged
//		to be precisely the size of 1 disk sector)
//	   A number of data blocks
//	   An entry in the file system directory
//
// 	The file system consists of several data structures:
//	   A bitmap of free disk sectors (cf. bitmap.h)
//	   A directory of file names and file headers
//
//      Both the bitmap and the directory are represented as normal
//	files.  Their file headers are located in specific sectors
//	(sector 0 and sector 1), so that the file system can find them 
//	on bootup.
//
//	The file system assumes that the bitmap and directory files are
//	kept "open" continuously while Nachos is running.
//
//	For those operations (such as Create, Remove) that modify the
//	directory and/or bitmap, if the operation succeeds, the changes
//	are written immediately back to disk (the two files are kept
//	open during all this time).  If the operation fails, and we have
//	modified part of the directory and/or bitmap, we simply discard
//	the changed version, without writing it back to disk.
//
// 	Our implementation at this point has the following restrictions:
//
//	   there is no synchronization for concurrent accesses
//	   files have a fixed size, set when the file is created
//	   files cannot be bigger than about 3KB in size
//	   there is no hierarchical directory structure, and only a limited
//	     number of files can be added to the system
//	   there is no attempt to make the system robust to failures
//	    (if Nachos exits in the middle of an operation that modifies
//	    the file system, it may corrupt the disk)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "disk.h"
#include "bitmap.h"
#include "directory.h"
#include "filehdr.h"
#include "filesys.h"
#include "system.h"

// Sectors containing the file headers for the bitmap of free sectors,
// and the directory of files.  These file headers are placed in well-known 
// sectors, so that they can be located on boot-up.
#define FreeMapSector 		0
#define DirectorySector 	1

// Initial file sizes for the bitmap and directory; until the file system
// supports extensible files, the directory size sets the maximum number 
// of files that can be loaded onto the disk.
#define FreeMapFileSize 	(NumSectors / BitsInByte)
#define NumDirEntries 		10
#define DirectoryFileSize 	(sizeof(DirectoryEntry) * NumDirEntries)

//----------------------------------------------------------------------
// FileSystem::FileSystem
// 	Initialize the file system.  If format = TRUE, the disk has
//	nothing on it, and we need to initialize the disk to contain
//	an empty directory, and a bitmap of free sectors (with almost but
//	not all of the sectors marked as free).  
//
//	If format = FALSE, we just have to open the files
//	representing the bitmap and the directory.
//
//	"format" -- should we initialize the disk?
//----------------------------------------------------------------------
FileSystem::FileSystem(bool format)
{ 
    DEBUG('f', "Initializing the file system.\n");
    if (format) {
        char emptySector[SectorSize];
        memset(emptySector, 0, sizeof(emptySector));
        for(int i = 0; i < NumSectors; i++)
            synchDisk->WriteSector(i, emptySector);

        BitMap *freeMap = new BitMap(NumSectors);
        Directory *directory = new Directory(NumDirEntries);
        FileHeader *mapHdr = new FileHeader;
        FileHeader *dirHdr = new FileHeader;

        DEBUG('f', "Formatting the file system.\n");

        // First, allocate space for FileHeaders for the directory and bitmap
        // (make sure no one else grabs these!)
        freeMap->Mark(FreeMapSector);	    
        freeMap->Mark(DirectorySector);

        // Second, allocate space for the data blocks containing the contents
        // of the directory and bitmap files.  There better be enough space!

        ASSERT(mapHdr->Allocate(freeMap, FreeMapFileSize));
        ASSERT(dirHdr->Allocate(freeMap, DirectoryFileSize));

        // Flush the bitmap and directory FileHeaders back to disk
        // We need to do this before we can "Open" the file, since open
        // reads the file header off of disk (and currently the disk has garbage
        // on it!).

        DEBUG('f', "Writing headers back to disk.\n");
        mapHdr->WriteBack(FreeMapSector);    
        dirHdr->WriteBack(DirectorySector);

        // OK to open the bitmap and directory files now
        // The file system operations assume these two files are left open
        // while Nachos is running.

        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
     
        // Once we have the files "open", we can write the initial version
        // of each file back to disk.  The directory at this point is completely
        // empty; but the bitmap has been changed to reflect the fact that
        // sectors on the disk have been allocated for the file headers and
        // to hold the file data for the directory and bitmap.

        DEBUG('f', "Writing bitmap and directory back to disk.\n");
        freeMap->WriteBack(freeMapFile);	 // flush changes to disk
        directory->WriteBack(directoryFile);

        if (DebugIsEnabled('f')) {
            freeMap->Print();
            directory->Print();

            delete freeMap; 
            delete directory; 
            delete mapHdr; 
            delete dirHdr;
        }
        
        Create("FFLN",0);   // file for long name
        Create("PIPE",0);
    }
    else {
        // if we are not formatting the disk, just open the files representing
        // the bitmap and directory; these are left open while Nachos is running
        freeMapFile = new OpenFile(FreeMapSector);
        directoryFile = new OpenFile(DirectorySector);
    }
    currentPath[0] = '~';
    currentPath[1] = '\0';
}

//----------------------------------------------------------------------
// FileSystem::Create
// 	Create a file in the Nachos file system (similar to UNIX create).
//	Since we can't increase the size of files dynamically, we have
//	to give Create the initial size of the file.
//
//	The steps to create a file are:
//	  Make sure the file doesn't already exist
//    Allocate a sector for the file header
// 	  Allocate space on disk for the data blocks for the file
//	  Add the name to the directory
//	  Store the new file header on disk 
//	  Flush the changes to the bitmap and the directory back to disk
//
//	Return TRUE if everything goes ok, otherwise, return FALSE.
//
// 	Create fails if:
//   	file is already in directory
//	 	no free space for file header
//	 	no free entry for file in directory
//	 	no free space for data blocks for the file 
//
// 	Note that this implementation assumes there is no concurrent access
//	to the file system!
//
//	"name" -- name of file to be created
//	"initialSize" -- size of file to be created,
//                   -1 if want to create a directory
//----------------------------------------------------------------------
bool
FileSystem::Create(char *name, int initialSize)
{
    Directory *directory;
    BitMap *freeMap;
    FileHeader *hdr;
    int sector;
    bool success;

    DEBUG('f', "Creating file %s, size %d\n", name, initialSize);

    Path* path = new Path(name);
    directory = path->GetDirectory(directoryFile);
    if(directory == -1 || directory == -2)
        return;

    FileType fileType = REGULAR;
    if(initialSize == -1)
    {
        initialSize = DirectoryFileSize;
        fileType = DIRECTORY;
    }

    if (directory->Find(path->GetName()) != -1)
        success = FALSE;			// file is already in directory
    else {	
        freeMap = new BitMap(NumSectors);
        freeMap->FetchFrom(freeMapFile);
        sector = freeMap->Find();	// find a sector to hold the file header
    	if (sector == -1) 		
            success = FALSE;		// no free block for file header 
        else
        {
             // NOTE: Writeback first because directory add may need it
            freeMap->WriteBack(freeMapFile);
            if (!directory->Add(path->GetName(), sector, fileType))
                success = FALSE;	// no space in directory
            else {
                hdr = new FileHeader;
                // NOTE: freeMap read back again
                freeMap->FetchFrom(freeMapFile);
                if (!hdr->Allocate(freeMap, initialSize))
                    success = FALSE;	// no space on disk for data
                else {	
                    success = TRUE;
                    // everthing worked, flush all changes back to disk
                    // hdr->SetPath(path->GetPath());
                    hdr->WriteBack(sector);
                    OpenFile *temp = path->GetDirOpenFile(directoryFile);
                    directory->WriteBack(temp);
                    if(temp != directoryFile)
                        delete temp;
    	    	    freeMap->WriteBack(freeMapFile);    
                }
            delete hdr;
	        }
        }
        delete freeMap;
    }
    delete directory;
    delete path;
    return success;
}

//----------------------------------------------------------------------
// FileSystem::Open
// 	Open a file for reading and writing.  
//	To open a file:
//	  Find the location of the file's header, using the directory 
//	  Bring the header into memory
//
//	"name" -- the text name of the file to be opened
//----------------------------------------------------------------------
OpenFile *
FileSystem::Open(char *name)
{ 
    Directory *directory = new Directory(NumDirEntries);
    OpenFile *openFile = NULL;
    int sector;

    DEBUG('f', "Opening file %s\n", name);

    Path* path = new Path(name);
    directory = path->GetDirectory(directoryFile);
    if(directory == -1 || directory == -2)
        return;

    sector = directory->Find(path->GetName()); 
    if (sector >= 0) 		
	    openFile = new OpenFile(sector);	// name was found in directory 
    delete directory;
    delete path;
    return openFile;				// return NULL if not found
}

//----------------------------------------------------------------------
// FileSystem::Remove
// 	Delete a file from the file system.  This requires:
//	    Remove it from the directory
//	    Delete the space for its header
//	    Delete the space for its data blocks
//	    Write changes to directory, bitmap back to disk
//
//	Return TRUE if the file was deleted, FALSE if the file wasn't
//	in the file system.
//
//	"name" -- the text name of the file to be removed
//----------------------------------------------------------------------
bool
FileSystem::Remove(char *name)
{ 
    Directory *directory;
    BitMap *freeMap;
    FileHeader *fileHdr;
    int sector;
    
    directory = new Directory(NumDirEntries);

    Path* path = new Path(name);
    directory = path->GetDirectory(directoryFile);
    if(directory == -1 || directory == -2)
        return;
    
    sector = directory->Find(path->GetName());
    if (sector == -1) {
       delete directory;
       return FALSE;			 // file not found 
    }
    if (synchDisk->visiter[sector] != 0)
    {
        delete directory;
        return FALSE;
    }
    fileHdr = new FileHeader;
    fileHdr->FetchFrom(sector);

    freeMap = new BitMap(NumSectors);
    freeMap->FetchFrom(freeMapFile);

    fileHdr->Deallocate(freeMap);  		// remove data blocks
    freeMap->Clear(sector);			// remove header block
    directory->Remove(path->GetName());

    freeMap->WriteBack(freeMapFile);		// flush to disk

    OpenFile *temp = path->GetDirOpenFile(directoryFile);
    directory->WriteBack(temp);
    if(temp != directoryFile)
        delete temp;       // flush to disk

    delete fileHdr;
    delete directory;
    delete freeMap;
    return TRUE;
} 

//----------------------------------------------------------------------
// FileSystem::List
// 	List all the files in the file system directory.
//----------------------------------------------------------------------
void
FileSystem::List()
{
    Directory *directory = new Directory(NumDirEntries);

    directory->FetchFrom(directoryFile);
    directory->List();
    delete directory;
}

//----------------------------------------------------------------------
// FileSystem::Print
// 	Print everything about the file system:
//	  the contents of the bitmap
//	  the contents of the directory
//	  for each file in the directory,
//	      the contents of the file header
//	      the data in the file
//----------------------------------------------------------------------
void
FileSystem::Print()
{
    FileHeader *bitHdr = new FileHeader;
    FileHeader *dirHdr = new FileHeader;
    BitMap *freeMap = new BitMap(NumSectors);
    Directory *directory = new Directory(NumDirEntries);

    printf("Bit map file header:\n");
    bitHdr->FetchFrom(FreeMapSector);
    bitHdr->Print();

    printf("Directory file header:\n");
    dirHdr->FetchFrom(DirectorySector);
    dirHdr->Print();

    freeMap->FetchFrom(freeMapFile);
    freeMap->Print();

    directory->FetchFrom(directoryFile);
    directory->Print();

    delete bitHdr;
    delete dirHdr;
    delete freeMap;
    delete directory;
} 

void FileSystem::ChangePath(char* path, char* change)
{
    Path* cpath = new Path(change);
    
    int i = 0;

    // absolute path
    if(strcmp(cpath->path[0], "~") == 0)
    {
        strcpy(path, "~");
        i++;
    }

    // relative path
    for(; i < cpath->path.size(); i++)
    {
        if(strlen(cpath->path[i]) == 0)
            continue;
        if(strncmp(cpath->path[i], "..", 2) == 0)
            PathRetreat(path);
        else
        {
            strcpy(&path[strlen(path)], "/");
            strcpy(&path[strlen(path)], cpath->path[i]);
        }
    }

    delete cpath;
}

void FileSystem::PathRetreat(char* path)
{
    int length = strlen(path);
    if(length == 1)
        return;
    int i;
    for(i = length - 1; path[i] != '/'; i--);
    path[i] = '\0';
}

bool FileSystem::ChangeCurrentPath(char* change)
{
    // cd without parameters will lead to ~
    if(strlen(change) == 0) 
        strcpy(change, "~");
    char newPath[MAX_PATH_LENGTH];
    strcpy(newPath, currentPath);
    ChangePath(newPath, change);
    // check the existence
    if(strlen(newPath) > 1)
    {
        strcpy(newPath + strlen(newPath), PATH_PAD);
        Path* path = new Path(newPath + 2);
        newPath[strlen(newPath) - strlen(PATH_PAD)] = '\0';
        Directory* dir = path->GetDirectory(directoryFile);
        if(dir == -1)
        {
            printf("cd: %s: No such file or directory\n", newPath + 2);
            return false;
        }
        if(dir == -2)
        {
            printf("cd: %s: Not a directory\n", newPath + 2);
            return false;
        }
        delete dir;
    }
    
    strcpy(currentPath, newPath);
    return true;
}

void FileSystem::PrintCurrentPath()
{
    printf("%s", currentPath);
}

void FileSystem::GetPath(char* relativePath, char* path)
{
    char newPath[MAX_PATH_LENGTH];
    strcpy(newPath, currentPath);
    ChangePath(newPath, relativePath);
    strcpy(path, newPath + 2);
}

void FileSystem::ListCurrent()
{
    char newPath[MAX_PATH_LENGTH];
    strcpy(newPath, currentPath);
    strcpy(newPath + strlen(newPath), PATH_PAD);
    Path* path = new Path(newPath + 2);
    Directory* dir = path->GetDirectory(directoryFile);
    dir->ListCurrent();
    delete dir;
}

bool FileSystem::touch(char* name)
{
    char newPath[MAX_PATH_LENGTH];
    strcpy(newPath, currentPath);
    ChangePath(newPath, name);
    if(Create(newPath + 2, 0) == false)
        return false;
    return true;
}

bool FileSystem::mkdir(char* name)
{
    char newPath[MAX_PATH_LENGTH];
    strcpy(newPath, currentPath);
    ChangePath(newPath, name);
    if(Create(newPath + 2, -1) == false)
        return false;
    return true;
}

bool FileSystem::mv(char* src, char* dst)
{
    cp(src, dst);
    rm(src);
}

bool FileSystem::cp(char* src, char* dst)
{
    char srcPath[MAX_PATH_LENGTH];
    char dstPath[MAX_PATH_LENGTH];
    strcpy(srcPath, currentPath);
    strcpy(dstPath, currentPath);
    ChangePath(srcPath, src);
    ChangePath(dstPath, dst);
    char* srcName = srcPath + 2;
    char* dstName = dstPath + 2;
    OpenFile* srcFile = Open(srcName);
    if(srcFile == NULL)
    {
        printf("cp: cannot stat '%s': No such file or directory\n", srcName);
        return;
    }
    OpenFile* dstFile = Open(dstName);
    if(dstFile == NULL)
    {
        Create(dstName, 0);
        dstFile = Open(dstName);
    }

    char c;
    while(srcFile->Read(&c,1) != 0)
        dstFile->Write(&c,1);

    delete srcFile;
    delete dstFile;
}

bool FileSystem::rm(char* name)
{
    char srcPath[MAX_PATH_LENGTH];
    strcpy(srcPath, currentPath);
    ChangePath(srcPath, name);
    char* srcName = srcPath + 2;

    Remove(srcName);
}

Path::Path(char* name)
{
    int length = strlen(name);
    char* temp;
    int i = 0;
    while(i <= length)
    {
        temp = new char[length + 1];
        int j;
        for(j = 0; i <= length; i++,j++)
        {
            if(name[i] == '/')
            {
                temp[j] = '\0';
                break;
            }
            else
                temp[j] = name[i];
        }
        i++;
        path.push_back(temp);
    }
}

Path::~Path()
{
    for(int i = 0; i < path.size(); i++)
        delete path[i];
}

char* Path::GetPath()
{
    char* filePath = new char[100];
    strcat(filePath,"/");
    for(int i = 0; i < path.size() - 1; i++)
    {
        strcat(filePath,path[i]);
        strcat(filePath,"/");
    }
    return filePath;
}

Directory* Path::GetDirectory(OpenFile* directoryFile)
{       
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    for(int i = 0; i < path.size() - 1; i++)
    {
        int sector = directory->FindDir(path[i]);
        if(sector == -1)
            return -1;
        if(sector == -2)
            return -2;
        OpenFile* openfile = new OpenFile(sector);
        directory->FetchFrom(openfile);
        delete openfile;
    }
    return directory;
}

OpenFile* Path::GetDirOpenFile(OpenFile* directoryFile)
{
    Directory *directory = new Directory(NumDirEntries);
    directory->FetchFrom(directoryFile);
    for(int i = 0; i < path.size() - 1; i++)
    {
        int sector = directory->FindDir(path[i]);
        if(sector == -1)
            return NULL;
        OpenFile* openfile = new OpenFile(sector);
        if(i == path.size() - 2)
            return openfile;
        directory->FetchFrom(openfile);
        delete openfile;
    }
    delete directory;
    return directoryFile;
}

char* Path::GetName()
{
    return path[path.size() - 1];
}