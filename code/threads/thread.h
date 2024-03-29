// thread.h 
//	Data structures for managing threads.  A thread represents
//	sequential execution of code within a program.
//	So the state of a thread includes the program counter,
//	the processor registers, and the execution stack.
//	
// 	Note that because we allocate a fixed size stack for each
//	thread, it is possible to overflow the stack -- for instance,
//	by recursing to too deep a level.  The most common reason
//	for this occuring is allocating large data structures
//	on the stack.  For instance, this will cause problems:
//
//		void foo() { int buf[1000]; ...}
//
//	Instead, you should allocate all data structures dynamically:
//
//		void foo() { int *buf = new int[1000]; ...}
//
//
// 	Bad things happen if you overflow the stack, and in the worst 
//	case, the problem may not be caught explicitly.  Instead,
//	the only symptom may be bizarre segmentation faults.  (Of course,
//	other problems can cause seg faults, so that isn't a sure sign
//	that your thread stacks are too small.)
//	
//	One thing to try if you find yourself with seg faults is to
//	increase the size of thread stack -- ThreadStackSize.
//
//  	In this interface, forking a thread takes two steps.
//	We must first allocate a data structure for it: "t = new Thread".
//	Only then can we do the fork: "t->fork(f, arg)".
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef THREAD_H
#define THREAD_H

#include "copyright.h"
#include "utility.h"

#ifdef USER_PROGRAM
#include "machine.h"
#include "addrspace.h"
#endif

// CPU register state to be saved on context switch.  
// The SPARC and MIPS only need 10 registers, but the Snake needs 18.
// For simplicity, this is just the max over all architectures.
#define MachineStateSize 18 


// Size of the thread's private execution stack.
// WATCH OUT IF THIS ISN'T BIG ENOUGH!!!!!
#define StackSize	(4 * 1024)	// in words

// max number of threads existing
#define ThreadListSize 5

// priority level
#define PriorityLevelSize 32
#define LowestPriority PriorityLevelSize - 1
// Thread state
enum ThreadStatus { JUST_CREATED, RUNNING, READY, BLOCKED };

// external function, dummy routine whose sole job is to call Thread::Print
extern void ThreadPrint(int arg);	 
extern void ThreadStatePrint(int arg);

#define OpenFileTableSize 32
#define ChildThreadSize 10

typedef struct{
  bool valid;
  void* openFile;
} OpenFileTableEntry;

// The following class defines a "thread control block" -- which
// represents a single thread of execution.
//
//  Every thread has:
//     an execution stack for activation records ("stackTop" and "stack")
//     space to save CPU registers while not running ("machineState")
//     a "status" (running/ready/blocked)
//    
//  Some threads also belong to a user address space; threads
//  that only run in the kernel have a NULL address space.
class Thread {
  private:
    // NOTE: DO NOT CHANGE the order of these first two members.
    // THEY MUST be in this position for SWITCH to work.
    int* stackTop;			 // the current stack pointer
    void *machineState[MachineStateSize];  // all registers except for stackTop

  public:
    // initialize a Thread
    Thread(char* debugName, int priorityLevel = LowestPriority);		 
    ~Thread(); 				// deallocate a Thread
					// NOTE -- thread being deleted
					// must not be running when delete 
					// is called

    // basic thread operations

    void Fork(VoidFunctionPtr func, void *arg); 	// Make thread run (*func)(arg)
    void Yield();  				// Relinquish the CPU if any 
						// other thread is runnable
    void Sleep();  				// Put the thread to sleep and 
						// relinquish the processor
    void Finish();  				// The thread is done executing
    
    void CheckOverflow();   			// Check if thread has 
						// overflowed its stack
    void setStatus(ThreadStatus st) { status = st; }
    char* getName() { return (name); }
    int getUid() { return uid; }
    int getTid() { return tid; }
    
    void Print() { printf("%s, ", name); }
    void StatePrint() { printf("%-6d%-6d%-20s\n", tid, uid, name); }

    void setPriority(int priorityLevel){ priority = priorityLevel; }
    int getPriority(){ return priority; }

    void setStartTime(int time){ startTime = time; }
    int getStartTime(){ return startTime; }
    
    int OpenFileTableAdd(void* _openFile);
    void* OpenFileTableFind(int _openFileId);
    bool OpenFileTableRemove(int _openFileId);

    void AddChild(Thread* childThread);
    void RemoveChild(Thread* childThread);
    void WaitTid(int childTid);
  private:
    // some of the private data for this class is listed above
    
    int* stack; 	 		// Bottom of the stack 
					// NULL if this is the main thread
					// (If NULL, don't deallocate stack)
    ThreadStatus status;		// ready, running or blocked
    char* name;
    int uid;  // user id
    int tid;  // thread id
    int priority; // thread priority
    static int threadSeq; // tid for next thread

    int startTime;  // time of being scheduled

    OpenFileTableEntry openFileTable[OpenFileTableSize];

    Thread* child[ChildThreadSize];
    Thread* parent;

    void StackAllocate(VoidFunctionPtr func, void *arg);
    					// Allocate a stack for thread.
					// Used internally by Fork()

#ifdef USER_PROGRAM
// A thread running a user program actually has *two* sets of CPU registers -- 
// one for its state while executing user code, one for its state 
// while executing kernel code.

    int userRegisters[NumTotalRegs];	// user-level CPU register state

  public:
    void SaveUserState();		// save user-level register state
    void RestoreUserState();		// restore user-level register state

    AddrSpace *space;			// User code this thread is running.
#endif
};

// Magical machine-dependent routines, defined in switch.s

extern "C" {
// First frame on thread execution stack; 
//   	enable interrupts
//	call "func"
//	(when func returns, if ever) call ThreadFinish()
void ThreadRoot();

// Stop running oldThread and start running newThread
void SWITCH(Thread *oldThread, Thread *newThread);
}

// create a Thread
Thread* createThread(char* threadName, int priorityLevel = LowestPriority);

// report thread states
void TS();

#endif // THREAD_H
