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
// ThreadTest4
//----------------------------------------------------------------------
void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");

    t->Fork(ComplexThread, (void*)1);
    
    ComplexThread(0);
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
	    ThreadTest4();
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
    default:
	    printf("No test specified.\n");
	    break;
    }
}

