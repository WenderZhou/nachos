// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------
Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	    table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------
Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------
void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------
void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        {
            if(table[i].name[FileNameMaxLen] == '\0'){
                if(!strncmp(table[i].name, name, FileNameMaxLen))
	                return i;
            }
            else
            {
                OpenFile *ffln = new OpenFile(Find("FFLN"));
                char buf[128];
                ffln->ReadAt(buf, 128, *(int*)table[i].name);
                delete ffln;
                if(!strncmp(buf, name, strlen(name)))
	                return i;
            }
        }
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------
int
Directory::Find(char *name)
{
    int i = FindIndex(name);

    if (i != -1)
	    return table[i].sector;
    return -1;
}

int
Directory::FindDir(char* name)
{
    int i = FindIndex(name);
    if(i != -1)
    {
        ASSERT(table[i].type == DIRECTORY);
        return table[i].sector;
    }
    return -1;
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------
bool
Directory::Add(char *name, int newSector, FileType filetype)
{ 
    if (FindIndex(name) != -1)
	    return FALSE;

    for (int i = 0; i < tableSize; i++)
        if (!table[i].inUse) {
            table[i].inUse = TRUE;
            if(strlen(name) <= FileNameMaxLen)
            {
                strncpy(table[i].name, name, FileNameMaxLen);
                table[i].name[FileNameMaxLen] = '\0';
            }
            else
            {
                OpenFile *ffln = new OpenFile(Find("FFLN"));
                *(int*)table[i].name = ffln->Length();
                table[i].name[FileNameMaxLen] = 1;
                ffln->WriteAt(name, strlen(name) + 1, ffln->Length());
                delete ffln;
            }
            table[i].sector = newSector;
            table[i].type = filetype;
            return TRUE;
	    }
    return FALSE;	// no space.  Fix when we have extensible files.
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------
bool
Directory::Remove(char *name)
{ 
    int i = FindIndex(name);

    if (i == -1)
	    return FALSE; 		// name not in directory
    
    if(table[i].type == DIRECTORY)
    {
        OpenFile* dirFile = new OpenFile(table[i].sector);
        Directory* dir = new Directory(10);
        dir->FetchFrom(dirFile);

        for(int j = 0; j < 10; j++)
            if(dir->table[j].inUse)
                dir->Remove(dir->table[j].name);
                
        dir->WriteBack(dirFile);
        delete dir;
        delete dirFile;
    }

    memset(table[i].name, 0, sizeof(table[i].name));
    table[i].sector = 0;
    table[i].inUse = FALSE;
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------
void
Directory::List(int depth)
{
    for (int i = 0; i < tableSize; i++)
	    if (table[i].inUse)
        {
            int j;
            for(j = 0; j < depth - 1; j++)
                printf("  ");
            for(; j < depth; j++)
                printf("|-");
            printf("%s\n", table[i].name);
            if(table[i].type == DIRECTORY)
            {
                Directory* dir = new Directory(10);
                OpenFile* openfile = new OpenFile(table[i].sector);
                dir->FetchFrom(openfile);
                dir->List(depth + 1);
                delete openfile;
            }
        }
	        
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------
void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse) {
        if(table[i].name[FileNameMaxLen] == '\0')
	        printf("Name: %s, Sector: %d\n", table[i].name, table[i].sector);
        else
        {
            OpenFile *ffln = new OpenFile(Find("FFLN"));
            char buf[128];
            ffln->ReadAt(buf, 128, *(int*)table[i].name);
            printf("Name: %s, Sector: %d\n", buf, table[i].sector);
            delete ffln;
        }
        if(table[i].type == REGULAR)
            printf("type: regular file\n");
        else
            printf("type: directory\n");
	    hdr->FetchFrom(table[i].sector);
	    hdr->Print();
	}
    printf("\n");
    delete hdr;
}