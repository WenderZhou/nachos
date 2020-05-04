/* halt.c
 *	Simple program to test whether running a user program works.
 *	
 *	Just do a "syscall" that shuts down the OS.
 *
 * 	NOTE: for some reason, user programs with global data structures 
 *	sometimes haven't worked in the Nachos environment.  So be careful
 *	out there!  One option is to allocate data structures as 
 * 	automatics within a procedure, but if you do this, you have to
 *	be careful to allocate a big enough stack to hold the automatics!
 */

#include "syscall.h"

void Test()
{
    int i;

    // OpenFileId openFileId;
    // char result[10];
    // openFileId = Open("helloworld.txt");
    // Read(result,10,openFileId);
    // Close(openFileId);

    for(i = 0; i < 3; i++)
        Yield();
    Exit(0);
}

int
main()
{
    OpenFileId openFileId;
    int tid, i;
    char result[10];
    // Halt();

    Create("helloworld.txt");
    
    openFileId = Open("helloworld.txt");
    Write("helloworld",10,openFileId);
    Close(openFileId);

    // Fork(Test);

    // tid = Exec("../test/matmult");
    // Join(tid);

    openFileId = Open("helloworld.txt");
    Read(result,10,openFileId);
    Close(openFileId);

    // for(i = 0; i < 3; i++)
    //     Yield();

    /* not reached */
}
