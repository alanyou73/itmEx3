

#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
#include <dirent.h>
#include <asm/errno.h>
#include <errno.h>


/*void checkDir(char* path)
{
    DIR* dir = opendir(path);
    if (dir)
    {
        //printf("%c",path[strlen(path)-1]);
        if(path[strlen(path)-1] != '/')
        {
            printf("302");
        }
        printf("dir exist");

        closedir(dir);
    }
    else
    {
        printf("404");
    }
}


int main () {

    checkDir("1/"); //
    return 0;
}*/