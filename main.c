//    Po                  v1.0
//
//    ***********************
//    *  PoLang Interpreter *
//    *    Wesley Sampson   *
//    *   2026  ---   2026  *
//    ***********************
//
//    Hey there, programmer! Please don't delete these comments. Read through them.
//
//    To anyone viewing this code:
//    You may use this code as long as you keep these credits -- so that people viewing
//    whatever extensions you have made to my code -- will know that I was the one who created it.
//    Feel free to also add your name to this code if you wish to modify it.
//    Your name here?   __________________
//
//    Please look through the code! It's actually quite simple.
//    If you want to add on to the already existing instructions, simply
//    add some more strcmp if blocks to interpret_token in lex.c. That's where all of the
//    actual *executing* is being done.
//
//    If you want to add support for different types, structs or anything like that,
//    then make that in main.h
//
//    Thank you for reading.
//
//    You can compile with gcc via:
//    gcc main.c lex.c -o po
//
//    (Not for commercial use)


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

int main(int argc, char *argv[]) {
    const char *delimiters = " \t\n\r";
    // check correct args
    if (argc != 2) {
        printf("Usage:     po filename.pols\n");
        return 1;
    }

    // file opening logic
    char* openFile = argv[1];
    FILE* fptr;

    fptr = fopen(openFile, "rb");

    if (fptr == NULL) {
        printf("Po encountered an error while opening %s\n", openFile);
    } else {
        printf("Successfully opened %s\n", openFile);
    }

    // file loading logic (loads in to fstr)
    fseek(fptr, 0, SEEK_END);
    long fsize = ftell(fptr);
    rewind(fptr);

    char *buffer = (char*)malloc(fsize + 1);
    if (buffer == NULL) {
        printf("Error: Memory allocation failed\n");
        fclose(fptr);
        return 2;
    }

    size_t bytesRead = fread(buffer, 1, fsize, fptr);
    buffer[bytesRead] = '\0';
    fclose(fptr);
    char* fstr = buffer;


    // strtok chops the file content string by whitespace tokens
    char *word = strtok(fstr, delimiters);
    while (word != NULL) {
        if (strcmp(word, "\\") == 0) { // comments
            char *search_start = word + strlen(word) + 1;
            char *next_newline = strchr(search_start, '\n');

            if (next_newline == NULL) {
                break;
            }

            word = strtok(next_newline + 1, delimiters);
        } // intercept String Printing `."`
        else if (strncmp(word, ".\"", 2) == 0) {
            char *str_start = word + 2;

            if (*str_start == '\0') {
                str_start = word + strlen(word) + 1;
            }

            // find the closing quote
            char *str_end = strchr(str_start, '"');
            if (str_end == NULL) {
                printf("Po Error: Unterminated string literal!\n");
                break;
            }

            // isolate the string temporarily by turning the trailing `"` into a null terminator
            *str_end = '\0';

            process_token(".\"");
            process_token(str_start);

            word = strtok(str_end + 1, delimiters);
        }
        else {
            // normal word processing
            process_token(word);
            word = strtok(NULL, delimiters); // grab next word
        }
    }

    // end function and free
    free(buffer);
    return 0;
}
