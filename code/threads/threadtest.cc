// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "elevatortest.h"

// testnum is set in main.cc
int testnum = 1;

//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    
    for (num = 0; num < 5; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        currentThread->Yield();
    }
}

//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(SimpleThread, (void*)1);
    
    SimpleThread(0);
}

//----------------------------------------------------------------------
// ThreadTest2 
//----------------------------------------------------------------------

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");

    

    for(int i = 1; i <= ThreadListSize; ++i)
    {
        Thread *t = createThread("forked thread");
        if(t)
            t->Fork(SimpleThread, (void*)i);
    }

    TS();

    SimpleThread(0);
}


//----------------------------------------------------------------------
// ThreadTest3
//----------------------------------------------------------------------
void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    Thread *t1 = createThread("forked thread 1",0);
    t1->Fork(SimpleThread, (void*)1);
    Thread *t2 = createThread("forked thread 2",1);
    t2->Fork(SimpleThread, (void*)2);
    SimpleThread(0);
}

//
void
ComplexThread(int which)
{
    int num;
    
    for (num = 1; num <= 30; num++) {
	printf("*** thread %d looped %d times\n", which, num);
        interrupt->OneTick();
    }
}


//----------------------------------------------------------------------
// ThreadTest4: test RR
//----------------------------------------------------------------------
void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");

    Thread *t = new Thread("forked thread");

    t->Fork(ComplexThread, (void*)1);
    
    ComplexThread(0);
}


//----------------------------------------------------------------------
// ThreadTest5: consumer and producer problem semaphore version
//----------------------------------------------------------------------
Semaphore *load;
Semaphore *vacancy;
Lock    *bufferLock;
int in,out;
#define bufferSize 10

void Producer_sem(int which)
{
    for(int i = 0; i < 20; ++i)
    {
        vacancy->P();
        bufferLock->Acquire();
        printf("Producer %d produce in buffer %d\n", which, in);
        in = (in + 1) % bufferSize;
        bufferLock->Release();
        load->V();
    }
}

void Consumer_sem(int which)
{
    for(int i = 0; i < 20; ++i)
    {
        load->P();
        bufferLock->Acquire();
        printf("Consumer %d consume in buffer %d\n", which, out);
        out = (out + 1) % bufferSize;
        bufferLock->Release();
        vacancy->V();
    }
}


void
ThreadTest5()
{
    DEBUG('t', "Entering ThreadTest5");

    load = new Semaphore("load", 0);
    vacancy = new Semaphore("vacancy", bufferSize);
    bufferLock = new Lock("bufferLock");
    in = 0;
    out = 0;

    Thread *t = new Thread("forked thread");

    t->Fork(Producer_sem, (void*)1);
    
    Consumer_sem(0);
}

//----------------------------------------------------------------------
// ThreadTest6: consumer and producer problem condition variable version
//----------------------------------------------------------------------
Condition *notEmpty;
Condition *notFull;
int things;

void Producer_cond(int which)
{
    for(int i = 0; i < 20; ++i)
    {
        bufferLock->Acquire();
        if(things == bufferSize)
            notFull->Wait(bufferLock);
        printf("Producer %d produce in buffer %d\n", which, in);
        in = (in + 1) % bufferSize;
        things++;
        notEmpty->Signal(bufferLock);
        bufferLock->Release();
    }
}

void Consumer_cond(int which)
{
    for(int i = 0; i < 20; ++i)
    {
        bufferLock->Acquire();
        if(things == 0)
            notEmpty->Wait(bufferLock);
        printf("Consumer %d consume in buffer %d\n", which, out);
        out = (out + 1) % bufferSize;
        things--;
        notFull->Signal(bufferLock);
        bufferLock->Release();
    }
}


void
ThreadTest6()
{
    DEBUG('t', "Entering ThreadTest6");

    notEmpty = new Condition("notEmpty");
    notFull = new Condition("notFull");
    bufferLock = new Lock("bufferLock");
    in = 0;
    out = 0;
    things = 0;

    Thread *t = new Thread("forked thread");

    t->Fork(Consumer_cond, (void*)1);
    
    Producer_cond(0);
}


//----------------------------------------------------------------------
// ThreadTest7: read/write lock
//----------------------------------------------------------------------
ReadWriteLock *rwlock;

void Reader(int which)
{
    
    for(int i = 0; i < 100; ++i)
    {
        printf("reader %d wants to read\n", which);
        rwlock->Acquire(MYREAD);

        printf("reader %d start reading\n", which);

        for(int j = 0; j < 30; ++j)
            interrupt->OneTick();

        printf("reader %d finish reading\n", which);

        rwlock->Release(MYREAD);
    }
}

void Writer(int which)
{
    
    for(int i = 0; i < 100; ++i)
    {
        printf("writer %d wants to write\n", which);
        rwlock->Acquire(MYWRITE);

        printf("writer %d start writing\n", which);

        for(int j = 0; j < 5; ++j)
            interrupt->OneTick();
            
        printf("writer %d finish writing\n", which);

        rwlock->Release(MYWRITE);
    }
}

void
ThreadTest7()
{
    DEBUG('t', "Entering ThreadTest7");

    rwlock = new ReadWriteLock("ReadWriteLock");

    Thread *r1 = new Thread("reader1");
    Thread *r2 = new Thread("reader2");

    r1->Fork(Reader, (void*)1);
    r2->Fork(Reader, (void*)2);
    Writer(0);
}

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	    ThreadTest7();
	    break;
    case 2:
	    ThreadTest2();
	    break;
    case 3:
	    ThreadTest3();
	    break;
    case 4:
	    ThreadTest4();
	    break;
    case 5:
	    ThreadTest5();
	    break;
    case 6:
	    ThreadTest6();
	    break;
    case 7:
	    ThreadTest7();
	    break;
    default:
	    printf("No test specified.\n");
	    break;
    }
}

