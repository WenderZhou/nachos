// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "addrspace.h"

void PageFaultExceptionHandler();
void CreateHandler();
void OpenHandler();
void CloseHandler();
void ReadHandler();
void WriteHandler();
void JoinHandler();
void ExecHandler();
void ForkHandler();
void YieldHandler();
void ExitHandler();
#ifdef FILESYS
void LsHandler();
void MvHandler();
void CpHandler();
void RmHandler();
void MkdirHandler();
void PsHandler();
void PwdHandler();
void CdHandler();
#endif


void ExecFunc(int which);
void ForkFunc(int which);
//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    switch(which){
        case SyscallException:
        {
            switch(type)
            {
                case SC_Halt:
                    DEBUG('a', "Shutdown, initiated by user program.\n");
                    printf("Shutdown, initiated by user program.\n");
   	                machine->MemRecycle();
                    interrupt->Halt();
                    break;
                case SC_Exit:ExitHandler();break;
                case SC_Exec:ExecHandler();break;
                case SC_Join:JoinHandler();break;
                case SC_Create:CreateHandler();break;
                case SC_Open:OpenHandler();break;
                case SC_Read:ReadHandler();break;
                case SC_Write:WriteHandler();break;
                case SC_Close:CloseHandler();break;
                case SC_Fork:ForkHandler();break;
                case SC_Yield:YieldHandler();break;
#ifdef FILESYS
                case SC_LS:LsHandler();break;
                case SC_MV:MvHandler();break;
                case SC_CP:CpHandler();break;
                case SC_RM:RmHandler();break;
                case SC_MKDIR:MkdirHandler();break;
                case SC_PS:PsHandler();break;
                case SC_PWD:PwdHandler();break;
                case SC_CD:CdHandler();break;
#endif
                default:
                    printf("Undefined System Call with #%d!\n",type);
                    interrupt->Halt();
                    break;
            }
        }
            break;
        case PageFaultException:
            PageFaultExceptionHandler();
            break;
        default:
            printf("Unexpected user mode exception %d %d\n", which, type);
	        ASSERT(FALSE);
    }
}

void CreateHandler()
{
    int nameIdx = machine->ReadRegister(4);

    int i = 0;
    char buffer[100];
    do{
        machine->ReadMem(nameIdx + i, 1, (int*)&buffer[i]);
    }while(buffer[i++] != '\0');

#ifdef SHOWTRACE
    printf("Create %s\n", buffer);
#endif

#ifdef FILESYS
    fileSystem->touch(buffer);
#else
    fileSystem->Create(buffer, 0);
#endif

    machine->PcPlus4();
}

void OpenHandler()
{
    int nameIdx = machine->ReadRegister(4);
    
    int i = 0;
    char buffer[100];
    do{
        machine->ReadMem(nameIdx + i, 1, (int*)&buffer[i]);
    }while(buffer[i++] != '\0');

#ifdef SHOWTRACE
    printf("Open %s\n", buffer);
#endif

    OpenFile* openFile = fileSystem->Open(buffer);
    if(openFile != NULL)
    {
#ifdef SHOWTRACE
        printf("Open succeed!\n");
#endif
        OpenFileId openFileId = currentThread->OpenFileTableAdd((void*)openFile);
        machine->WriteRegister(2,openFileId);
    }
    else
        printf("Open fail!\n");
        
    machine->PcPlus4();
}

void CloseHandler()
{
    int openFileId = machine->ReadRegister(4);

#ifdef SHOWTRACE
    printf("Close file with id %d\n", openFileId);
#endif

    OpenFile* openFile = currentThread->OpenFileTableFind(openFileId);
    currentThread->OpenFileTableRemove(openFileId);
    if(openFile != NULL)
    {
#ifdef SHOWTRACE
        printf("Close succeed!\n");
#endif
        delete openFile;
    }
    else
        printf("Close fail!\n");
    machine->PcPlus4();
}

void ReadHandler()
{
    int bufferIdx = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId openFileId =  machine->ReadRegister(6);

    int i = 0;
    char buffer[100];
    do{
        machine->ReadMem(bufferIdx + i, 1, (int*)&buffer[i]);
    }while(buffer[i++] != '\0');

    if(openFileId == ConsoleInput)
    {
        for(int i = 0; i < size; i++)
            scanf("%c",buffer + i);
        machine->WriteRegister(2,size);
        machine->PcPlus4();
        return;
    }

    OpenFile* openFile = currentThread->OpenFileTableFind(openFileId);
    if(openFile != NULL)
    {
        int readSize = openFile->Read(buffer,size);
#ifdef SHOWTRACE
        printf("Read succeed!\n");
        for(int i = 0; i < size; i++)
            printf("%c",buffer[i]);
        printf("\n");
#endif
        machine->WriteRegister(2,readSize);
    }
    else
        printf("Read fail!\n");
    machine->PcPlus4();
}

void WriteHandler()
{
    int bufferIdx = machine->ReadRegister(4);
    int size = machine->ReadRegister(5);
    OpenFileId openFileId =  machine->ReadRegister(6);

    char* buffer = new char[size + 4];
    for(int i = 0; i < size; i++)
        machine->ReadMem(bufferIdx + i, 1, (int*)&buffer[i]);
        
    if(openFileId == ConsoleOutput)
    {
        for(int i = 0; i < size; i++)
            printf("%c", buffer[i]);
        machine->PcPlus4();
        return;
    }    

    OpenFile* openFile = currentThread->OpenFileTableFind(openFileId);
    if(openFile != NULL)
    {
        openFile->Write(buffer,size);
#ifdef SHOWTRACE 
        printf("Write succeed!\n");
        printf("%s\n",buffer);
#endif
    }
    else
        printf("Write fail!\n");
    machine->PcPlus4();

    delete buffer;
}

void JoinHandler()
{
    int childTid = machine->ReadRegister(4);
    currentThread->WaitTid(childTid);
    machine->PcPlus4();
}

void ExecHandler()
{
    int nameIdx = machine->ReadRegister(4);
    char name[100];
    
    int i = 0;
    do{
        machine->ReadMem(nameIdx + i, 1, (int*)&name[i]);
    }while(name[i++] != '\0');

    char execName[MAX_PATH_LENGTH];

#ifdef FILESYS
    fileSystem->GetPath(name, execName);
#else
    strcpy(execName, name);
#endif

#ifdef SHOWTRACE
    printf("Execute %s\n", execName);
#endif

    OpenFile* executable = fileSystem->Open(execName);
    if(executable != NULL)
    {
        Thread *thread = createThread("ExecThread");
        machine->WriteRegister(2,thread->getTid());
        AddrSpace *space = new AddrSpace(executable,execName);
        thread->space = space;
        thread->Fork(ExecFunc,(void*)1);
        delete executable;
    }
    else
    {
        printf("Can not find file %s\n",name);
        machine->WriteRegister(2,-1);
    }
    machine->PcPlus4();
}

void ForkHandler()
{
    char* name = currentThread->space->getFileName();
    OpenFile* executable = fileSystem->Open(name);
    Thread *thread = NULL;
    if(executable != NULL)
    {
        thread = createThread("ForkThread");
        AddrSpace *space = new AddrSpace(executable,name);
        thread->space = space;
        delete executable;
    }
    else
    {
        printf("Failed to open executable file!\n");
        machine->WriteRegister(2,-1);
    }

    int PC = machine->ReadRegister(4);

    thread->Fork(ForkFunc, (void*)PC);

    machine->PcPlus4();
}

void YieldHandler()
{
    printf("%s Yield!\n", currentThread->getName());
    machine->PcPlus4();
    currentThread->Yield();
}

void ExitHandler()
{
    // printf("Exit!\n");
    machine->MemRecycle();
    currentThread->Finish();
}

void ExecFunc(int which)
{
    currentThread->space->InitRegisters();
    currentThread->space->RestoreState();
    machine->Run();
}

void ForkFunc(int which)
{
    currentThread->space->RestoreState();

    machine->WriteRegister(PCReg, which);
    machine->WriteRegister(NextPCReg, which+4);

    machine->Run();
}

#ifdef FILESYS

void LsHandler()
{
    fileSystem->ListCurrent();
    machine->PcPlus4();
}

void MvHandler()
{
    int srcIdx = machine->ReadRegister(4);
    int dstIdx = machine->ReadRegister(5);
    int i;
    
    char src[MAX_PATH_LENGTH];
    char dst[MAX_PATH_LENGTH];

    i = 0;
    do{
        machine->ReadMem(srcIdx + i, 1, (int*)&src[i]);
    }while(src[i++] != '\0');

    i = 0;
    do{
        machine->ReadMem(dstIdx + i, 1, (int*)&dst[i]);
    }while(dst[i++] != '\0');

    fileSystem->mv(src, dst);
    machine->PcPlus4();
}

void CpHandler()
{
    int srcIdx = machine->ReadRegister(4);
    int dstIdx = machine->ReadRegister(5);
    int i;
    
    char src[MAX_PATH_LENGTH];
    char dst[MAX_PATH_LENGTH];

    i = 0;
    do{
        machine->ReadMem(srcIdx + i, 1, (int*)&src[i]);
    }while(src[i++] != '\0');

    i = 0;
    do{
        machine->ReadMem(dstIdx + i, 1, (int*)&dst[i]);
    }while(dst[i++] != '\0');

    fileSystem->cp(src, dst);
    machine->PcPlus4();
}

void RmHandler()
{
    int srcIdx = machine->ReadRegister(4);
    int i;
    
    char src[MAX_PATH_LENGTH];

    i = 0;
    do{
        machine->ReadMem(srcIdx + i, 1, (int*)&src[i]);
    }while(src[i++] != '\0');

    fileSystem->rm(src);
    machine->PcPlus4();
}

void MkdirHandler()
{
    int nameIdx = machine->ReadRegister(4);
    char name[100];
    
    int i = 0;
    do{
        machine->ReadMem(nameIdx + i, 1, (int*)&name[i]);
    }while(name[i++] != '\0');

    if(strlen(name) == 0)
        printf("mkdir: missing operand\n");
    if(fileSystem->mkdir(name) == false)
        printf("mkdir: cannot create directory ‘%s’: File exists\n", name);
    machine->PcPlus4();
}

void PsHandler()
{
    TS();
    machine->PcPlus4();
}

void PwdHandler()
{
    fileSystem->PrintCurrentPath();
    machine->PcPlus4();
}

void CdHandler()
{
    int nameIdx = machine->ReadRegister(4);
    char name[100];
    
    int i = 0;
    do{
        machine->ReadMem(nameIdx + i, 1, (int*)&name[i]);
    }while(name[i++] != '\0');

    fileSystem->ChangeCurrentPath(name);
    machine->PcPlus4();
}

#endif

void PageFaultExceptionHandler()
{
    int virtAddr = machine->ReadRegister(BadVAddrReg);
    int vpn = (unsigned) virtAddr / PageSize;
    
#ifdef INVERTED_PAGETABLE
    machine->pageFaultHandler(vpn);
#else
    if(machine->pageTable[vpn].valid == false)
    {
#ifdef SHOW_INFO
        printf("visit virtual page %d cause page fault\n", vpn);
#endif
        machine->pageFaultHandler(vpn);
#ifdef SHOW_INFO
        printf("----------------------------------------\n");
#endif
    }
#endif
        
    // by now the page is in memory

#ifdef USE_TLB
    machine->tlb->tlbMissHandler(vpn);
#endif

    stats->numPageFaults++;
}