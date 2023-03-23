/*
name: alim
date: march 21, 2023
pledge: i pledge my honor that i have abided by the stevens honor system
extra credit: done
*/

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

int fprintfx(char*, char, void*);
int fscanfx(char*, char, void*);
int clean();