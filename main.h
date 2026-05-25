#ifndef MAIN_H
#define MAIN_H

#define STACK_MAX 256

// use extern so multiple files can share these without duplicate errors
extern int stack[STACK_MAX];
extern int stack_ptr;
extern int *current_word_index_ptr;

typedef enum {
    TOKEN_EOF,
    TOKEN_ERROR,

    TOKEN_NUM,
    TOKEN_WORD,

    TOKEN_COL,
    TOKEN_SEMCOL,

    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_ENDIF,

    TOKEN_PRINT_STR
} TokenType;

typedef struct {
    TokenType type;
    char *lexeme;
    int value;
} Token;


typedef struct DictionaryEntry {
    char *name;                     // Name of the word
    char **words;                   // Array of strings representing internal tokens
    int word_count;                 // How many words inside this definition
    struct DictionaryEntry *next;   // Link to the next dictionary item
} DictionaryEntry;

typedef enum {
    STATE_EXECUTE,
    STATE_COMPILE
} InterpreterState;


void push(int value);
int pop();
void process_token(const char *lexeme);
void interpret_token(Token t);

#endif
