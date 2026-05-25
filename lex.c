#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "main.h"


int stack[STACK_MAX];
int stack_ptr = 0;
int print_next_token_as_literal = 0;

#define LOOP_STACK_MAX 32
int loop_stack[LOOP_STACK_MAX];
int loop_ptr = 0;

InterpreterState current_state = STATE_EXECUTE;
DictionaryEntry *dictionary_head = NULL;

// Temporary compilation buffers
char *building_word_name = NULL;
char *compiled_tokens[512];
int compiled_token_count = 0;

int skip_mode = 0;          // tracks if we are skipping code inside a false if block
int if_nested_count = 0;    // tracks nested if statements so we skip to the correct endif
int *current_word_index_ptr = NULL;


DictionaryEntry* find_word(const char *name) {
    DictionaryEntry *current = dictionary_head;
    while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void add_forth_word(const char *name, char **words, int count) {
    DictionaryEntry *new_entry = malloc(sizeof(DictionaryEntry));
    new_entry->name = strdup(name);
    new_entry->word_count = count;
    new_entry->words = malloc(sizeof(char*) * count);

    for (int i = 0; i < count; i++) {
        new_entry->words[i] = strdup(words[i]);
    }

    new_entry->next = dictionary_head;
    dictionary_head = new_entry;
}

void push(int value) {
    if (stack_ptr >= STACK_MAX) {
        printf("Po Error: Stack Overflow!\n");
        exit(1);
    }
    stack[stack_ptr++] = value;
}

int pop() {
    if (stack_ptr <= 0) {
        printf("Po Error: Stack Underflow!\n");
        exit(1);
    }
    return stack[--stack_ptr];
}

void loop_push(int val) {
    if (loop_ptr < LOOP_STACK_MAX) {
        loop_stack[loop_ptr++] = val;
    } else {
        printf("Po Runtime Error: Loop stack overflow!\n");
        exit(1);
    }
}

int loop_pop() {
    if (loop_ptr > 0) {
        return loop_stack[--loop_ptr];
    }
    printf("Po Runtime Error: Loop stack underflow!\n");
    exit(1);
    return 0;
}

int is_number(const char *str) {
    if (*str == '-' || *str == '+') str++;
    if (*str == '\0') return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

// this decides what the token is.
void process_token(const char *lexeme) {
    if (print_next_token_as_literal && current_state == STATE_EXECUTE && !skip_mode) {
        printf("%s", lexeme);
        print_next_token_as_literal = 0;
        return;
    }

    Token t;
    t.lexeme = strdup(lexeme);

    if (strcmp(lexeme, ":") == 0) {
        t.type = TOKEN_COL;
    } else if (strcmp(lexeme, ";") == 0) {
        t.type = TOKEN_SEMCOL;
    } else if (strcasecmp(lexeme, "IF") == 0) {   // strcasecmp ignores UPPER/lowercase
        t.type = TOKEN_IF;
    } else if (strcasecmp(lexeme, "ELSE") == 0) {
        t.type = TOKEN_ELSE;
    } else if (strcasecmp(lexeme, "ENDIF") == 0) {
        t.type = TOKEN_ENDIF;
    } else if (is_number(lexeme)) {
        t.type = TOKEN_NUM;
        t.value = atoi(lexeme);
    } else {
        t.type = TOKEN_WORD;
        t.value = 0;
    }

    if (skip_mode) {
        if (t.type == TOKEN_IF) {
            if_nested_count++;
        }
        else if (t.type == TOKEN_ENDIF) {
            if (if_nested_count > 0) {
                if_nested_count--;
            } else {
                skip_mode = 0;
            }
        }
        else if (t.type == TOKEN_ELSE && if_nested_count == 0) {
            skip_mode = 0;
        }

        free(t.lexeme);
        return;
    }

    if (current_state == STATE_COMPILE) {
        if (t.type == TOKEN_SEMCOL) {
            add_forth_word(building_word_name, compiled_tokens, compiled_token_count);
            free(building_word_name);
            building_word_name = NULL;
            for (int i = 0; i < compiled_token_count; i++) {
                free(compiled_tokens[i]);
            }
            compiled_token_count = 0;
            current_state = STATE_EXECUTE;
        }
        else if (building_word_name == NULL) {
            building_word_name = strdup(t.lexeme);
        }
        else {
            compiled_tokens[compiled_token_count++] = strdup(t.lexeme);
        }
    } else {
        if (t.type == TOKEN_COL) {
            current_state = STATE_COMPILE;
        } else {
            // trigger immediate string print mode
            if (strcmp(t.lexeme, ".\"") == 0) {
                print_next_token_as_literal = 1;
            } else {
                interpret_token(t);
            }
        }
    }

    free(t.lexeme);
}

// execute the token, depending on wether it's built-in or if it's user defined
void interpret_token(Token t) {
    if (t.type == TOKEN_NUM) {
        push(t.value);
    }
    else if (t.type == TOKEN_IF) {
        int condition = pop();
        if (condition == 0) {
            // the condition was false. Turn on skip_mode to skip to else or endif
            skip_mode = 1;
            if_nested_count = 0;
        }
    }
    else if (t.type == TOKEN_ELSE) {
        // If we hit an else while executing normally, it means we just finished
        // running the true block. We need to skip the false block
        skip_mode = 1;
        if_nested_count = 0;
    }
    else if (t.type == TOKEN_ENDIF) {
        // Do nothing! endif just acts as like how the closing brace would in C ( "}" )
    }
    else if (t.type == TOKEN_WORD) {
        if (strcmp(t.lexeme, "+") == 0) {
            int b = pop();
            int a = pop();
            push(a + b);
        }
        else if (strcmp(t.lexeme, "-") == 0) {
            int b = pop();
            int a = pop();
            push(a - b);
        }
        else if (strcmp(t.lexeme, "*") == 0) {
            int b = pop();
            int a = pop();
            push(a * b);
        }
        else if (strcmp(t.lexeme, "/") == 0) {
            int b = pop();
            int a = pop();
            push(a / b);
        }
        else if (strcmp(t.lexeme, ".") == 0) {
            printf("%d", pop());
        } else if (strcmp(t.lexeme, "=") == 0) {
            int b = pop();
            int a = pop();
            if (a == b) {
                push(1);
            } else {
                push(0);
            }
        }
        else if (strcmp(t.lexeme, ">") == 0) {
            int b = pop();
            int a = pop();
            if (a > b) {
                push(1);
            } else {
                push(0);
            }
        }
        else if (strcmp(t.lexeme, "<") == 0) {
            int b = pop();
            int a = pop();
            if (a < b) {
                push(1);
            } else {
                push(0);
            }
        }
        else if (strcmp(t.lexeme, "cr") == 0) {
            printf("\n");
        }
        else if (strcmp(t.lexeme, "dup") == 0) {
            int a = pop();
            push(a);
            push(a);
        }
        else if (strcmp(t.lexeme, "drop") == 0) {
            pop();
        }
        else if (strcmp(t.lexeme, "swap") == 0) {
            int b = pop();
            int a = pop();
            push(b);
            push(a);
        }
        else if (strcmp(t.lexeme, "i") == 0) {
            if (!skip_mode) {
                // peek at the top of the loop stack without removing it
                if (loop_ptr > 0) {
                    push(loop_stack[loop_ptr - 1]);
                } else {
                    printf("Po Runtime Error: 'i' used outside of a loop!\n");
                    exit(1);
                }
            }
        }
        else if (strcmp(t.lexeme, ".\"") == 0) {
            // check if we are executing a compiled custom word right now
            if (current_word_index_ptr != NULL) {
                (*current_word_index_ptr)++;
            }
            printf("Po Error: String operator misplacement.\n");
        }
        else if (strcmp(t.lexeme, "inp") == 0) {
            char input_buffer[32];
            fflush(stdout);

            if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
                input_buffer[strcspn(input_buffer, "\n")] = '\0';

                if (is_number(input_buffer)) {
                    push(atoi(input_buffer));
                } else {
                    printf("\nPo Runtime Error: 'input' expected a numeric value.\n");
                    exit(1);
                }
            }
        }
        else {
            DictionaryEntry *custom_word = find_word(t.lexeme);
            if (custom_word != NULL) {
                int do_index = -1;

                for (int i = 0; i < custom_word->word_count; i++) {
                    char *current_token = custom_word->words[i];

                    // handle string printing
                    if (strcmp(current_token, ".\"") == 0 && i + 1 < custom_word->word_count) {
                        if (!skip_mode) printf("%s", custom_word->words[i + 1]);
                        i++;
                    }
                    else if (strcmp(current_token, "do") == 0) {
                        if (!skip_mode) {
                            int start = pop();
                            int limit = pop();
                            loop_push(limit);
                            loop_push(start);
                            do_index = i;
                        }
                    }
                    else if (strcmp(current_token, "loop") == 0) {
                        if (!skip_mode) {
                            int current_index = loop_pop();
                            int limit = loop_pop();

                            current_index++;

                            if (current_index < limit) {
                                loop_push(limit);
                                loop_push(current_index);
                                i = do_index; // jump the execution pointer back to do
                            }
                        }
                    }
                    // normal token playback
                    else {
                        process_token(current_token);
                    }
                }
            } else {
                printf("Po Error: Unknown word '%s'\n", t.lexeme);
            }
        }
    }
}
