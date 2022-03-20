/**
 * @file main.c
 * @brief Main entry point to the program.
 *
 * @version 0.1
 * @date 2022-03-19
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

/**
 * @brief Print simple help text for the REPL functionality. This is mostly
 * a stub for when there are debugging commands implemented.
 *
 */
static void printHelp()
{
    printf("Command help:\n");
    printf("  .q(uit) to quit program.\n");
    printf("  .h(elp) print this information.\n");
}

/**
 * @brief Accept random correct code and submit it to the interpreter for
 * execution.
 *
 */
static void repl()
{
    char* line = NULL;
    bool quit = false;

    printf("klox REPL. Type '.h(elp)) for a list of commands.\n");
    while(!quit) {
        if(line != NULL) {
            free(line);
        }

        line = readline("klox > ");

        if(line != NULL) {
            if(line[0] != '\0') {
                if(line[0] == '.') {
                    switch(toupper(line[1])) {
                        case 'Q':
                            quit = true;
                            break;
                        case 'H':
                        case '\0':
                            printHelp();
                            break;
                        default:
                            printf("unknown REPL command\n");

                    }
                }
                else {
                    interpret(line);
                }
            }
        }
        else {
            interpret(line);
        }
    }
}


/**
 * @brief Read a file that was specified on the command line and prepare it
 * to be compiled and interpreted.
 *
 * @param path - fully qualified file name
 * @return char* - pointer to the raw text of the file
 *
 */
static char* readFile(const char* path)
{
    FILE* file = fopen(path, "rb");
    if(file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    if(buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n",
                path);
        exit(74);
    }

    size_t bytesRead = fread(buffer, sizeof(char), fileSize,
                             file);
    if(bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

/**
 * @brief Read the file from disk and submit it to the compiler for
 * processing.
 *
 * @param path - fully qualified file name
 *
 */
static void runFile(const char* path)
{
    char* source = readFile(path);
    InterpretResult result = interpret(source);
    free(source); // [owner]

    if(result == INTERPRET_COMPILE_ERROR) {
        exit(65);
    }
    if(result == INTERPRET_RUNTIME_ERROR) {
        exit(70);
    }
}

/**
 * @brief Main entry point to the program.
 *
 * @param argc - number of command line arguments, set by runtime system of host
 * @param argv - array of strings for command arguments set by system
 * @return int - return 0 if no error
 *
 */
int main(int argc, const char* argv[])
{
    initVM();

    if(argc == 1) {
        repl();
    }
    else if(argc == 2) {
        runFile(argv[1]);
    }
    else {
        fprintf(stderr, "Usage: klox [path]\n");
        exit(64);
    }

    freeVM();

    return 0;
}
