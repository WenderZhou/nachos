#include "syscall.h"

int cmp(char* s, char* t, int length)
{
    int i;
    for(i = 0; i < length; i++)
        if(s[i] != t[i])
            return 0;
    return 1;
}

void append(char* path, char* src)
{
    int i, j;
    for(i = 0; path[i] != '\0'; i++);
    for(j = 0; src[j] != '\0'; i++, j++)
        path[i] = src[j];
    path[i] = '\0';
}

int len(char* str)
{
    int i;
    for(i = 0; str[i] != '\0'; i++);
    return i;
}

void remove(char* path)
{
    int length;
    length = len(path);
    if(length == 1)
        return;
    for(; path[length - 1] != '/'; length--);
    path[length - 1] = '\0';
}

void cpy(char* src, char* dst)
{
    int i;
    for(i = 0; src[i] != '\0'; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

int
main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    char path[60];
    char newpath[60];
    int i;
    char* src;
    char* dst;
    int strlen;

    path[0] = '~';
    path[1] = '\0';

    prompt[0] = '$';
    prompt[1] = ' ';

    while( 1 )
    {
        Write(path, len(path), output);
	    Write(prompt, 2, output);

	    i = 0;
	
    	do {
	
	        Read(&buffer[i], 1, input); 

    	} while( buffer[i++] != '\n' );

	    buffer[--i] = '\0';

        if(cmp(buffer,"ls", 2) == 1)
        {
            Ls();
        }
        else if(cmp(buffer, "touch", 5) == 1)
        {
            src = buffer + 6;
            Create(src);
        }
        else if(cmp(buffer, "mv", 2) == 1)
        {
            src = buffer + 3;
            for(dst = &buffer[3]; *dst != ' '; dst++);
            dst++;
            Mv(src, dst);
        }
        else if(cmp(buffer, "cp", 2) == 1)
        {
            src = buffer + 3;
            for(dst = &buffer[3]; *dst != ' '; dst++);
            dst++;
            Cp(src, dst);
        }
        else if(cmp(buffer, "rm", 2) == 1)
        {
            src = buffer + 3;
            Rm(src);
        }
        else if(cmp(buffer, "echo", 4) == 1)
        {
            src = buffer + 5;
            for(strlen = 0; src[strlen] != '\0'; strlen++);
            Write(src, strlen, output);
        }
        else if(cmp(buffer, "mkdir", 5) == 1)
        {
            src = buffer + 6;
            Mkdir(src);
        }
        else if(cmp(buffer, "cd", 2) == 1)
        {
            append(path, "/");
            append(path, buffer + 3);
        }
        else if(cmp(buffer, "pwd", 3) == 1)
        {

        }
        else if(cmp(buffer, "ps", 2) == 1)
        {
            Ps();
        }
        else if(cmp(buffer, "kill", 4) == 1)
        {
            
        }
        else if(cmp(buffer, "exit", 4) == 1)
        {
            Exit(0);
        }

	    // if( i > 0 ) {
	    // 	newProc = Exec(buffer);
	    // 	Join(newProc);
    	// }
    }
}

