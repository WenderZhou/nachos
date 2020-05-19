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

void Receiver()
{
    int fd;
    char buffer[12];
    fd = msgget(1);
    msgrcv(fd, buffer, 12, 1);
    Write(buffer, 12, ConsoleOutput);
    Exit(0);
}

int main()
{
    int fd;
    fd = msgget(1);
    msgsnd(fd, "HelloWorld!\n", 12, 1);
    Fork(Receiver);

    // OpenFileId fd;
    // int tid, i;
    // char result[10];

    // Create("data");

    // fd = Open("data");
    // Write("HelloWorld!", 11, fd);
    // Close(fd);

    // fd = Open("data");
    // Read(result,10,fd);
    // Close(fd);

    // int tid;
    // tid = Exec("matmult");
    // Join(tid);
}
