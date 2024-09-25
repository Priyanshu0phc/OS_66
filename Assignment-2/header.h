#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 100
#define MAX_HISTORY 100
#define MAX_COMMAND_LENGTH 1024
