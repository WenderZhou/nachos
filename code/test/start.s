/* Start.s 
 *	Assembly language assist for user programs running on top of Nachos.
 *
 *	Since we don't want to pull in the entire C library, we define
 *	what we need for a user program here, namely Start and the system
 *	calls.
 */

#define IN_ASM
#include "syscall.h"

        .text   
        .align  2

/* -------------------------------------------------------------
 * __start
 *	Initialize running a C program, by calling "main". 
 *
 * 	NOTE: This has to be first, so that it gets loaded at location 0.
 *	The Nachos kernel always starts a program by jumping to location 0.
 * -------------------------------------------------------------
 */

	.globl __start
	.ent	__start
__start:
	jal	main
	move	$4,$0		
	jal	Exit	 /* if we return from main, exit(0) */
	.end __start

/* -------------------------------------------------------------
 * System call stubs:
 *	Assembly language assist to make system calls to the Nachos kernel.
 *	There is one stub per system call, that places the code for the
 *	system call into register r2, and leaves the arguments to the
 *	system call alone (in other words, arg1 is in r4, arg2 is 
 *	in r5, arg3 is in r6, arg4 is in r7)
 *
 * 	The return value is in r2. This follows the standard C calling
 * 	convention on the MIPS.
 * -------------------------------------------------------------
 */

	.globl Halt
	.ent	Halt
Halt:
	addiu $2,$0,SC_Halt
	syscall
	j	$31
	.end Halt

	.globl Exit
	.ent	Exit
Exit:
	addiu $2,$0,SC_Exit
	syscall
	j	$31
	.end Exit

	.globl Exec
	.ent	Exec
Exec:
	addiu $2,$0,SC_Exec
	syscall
	j	$31
	.end Exec

	.globl Join
	.ent	Join
Join:
	addiu $2,$0,SC_Join
	syscall
	j	$31
	.end Join

	.globl Create
	.ent	Create
Create:
	addiu $2,$0,SC_Create
	syscall
	j	$31
	.end Create

	.globl Open
	.ent	Open
Open:
	addiu $2,$0,SC_Open
	syscall
	j	$31
	.end Open

	.globl Read
	.ent	Read
Read:
	addiu $2,$0,SC_Read
	syscall
	j	$31
	.end Read

	.globl Write
	.ent	Write
Write:
	addiu $2,$0,SC_Write
	syscall
	j	$31
	.end Write

	.globl Close
	.ent	Close
Close:
	addiu $2,$0,SC_Close
	syscall
	j	$31
	.end Close

	.globl Fork
	.ent	Fork
Fork:
	addiu $2,$0,SC_Fork
	syscall
	j	$31
	.end Fork

	.globl Yield
	.ent	Yield
Yield:
	addiu $2,$0,SC_Yield
	syscall
	j	$31
	.end Yield

	.globl Ls
	.ent	Ls
Ls:
	addiu $2,$0,SC_LS
	syscall
	j	$31
	.end LS

	.globl Mv
	.ent	Mv
Mv:
	addiu $2,$0,SC_MV
	syscall
	j	$31
	.end Mv

	.globl Cp
	.ent	Cp
Cp:
	addiu $2,$0,SC_CP
	syscall
	j	$31
	.end Cp

	.globl Rm
	.ent	Rm
Rm:
	addiu $2,$0,SC_RM
	syscall
	j	$31
	.end Rm

	.globl Mkdir
	.ent	Mkdir
Mkdir:
	addiu $2,$0,SC_MKDIR
	syscall
	j	$31
	.end Mkdir

	.globl Ps
	.ent	Ps
Ps:
	addiu $2,$0,SC_PS
	syscall
	j	$31
	.end Ps

	.globl Pwd
	.ent	Pwd
Pwd:
	addiu $2,$0,SC_PWD
	syscall
	j	$31
	.end Pwd

	.globl Cd
	.ent	Cd
Cd:
	addiu $2,$0,SC_CD
	syscall
	j	$31
	.end Cd

	.globl msgget
	.ent	msgget
msgget:
	addiu $2,$0,SC_MSGGET
	syscall
	j	$31
	.end msgget

	.globl msgsnd
	.ent	msgsnd
msgsnd:
	addiu $2,$0,SC_MSGSND
	syscall
	j	$31
	.end msgsnd
	
	.globl msgrcv
	.ent	msgrcv
msgrcv:
	addiu $2,$0,SC_MSGRCV
	syscall
	j	$31
	.end msgrcv

/* dummy function to keep gcc happy */
        .globl  __main
        .ent    __main
__main:
        j       $31
        .end    __main

