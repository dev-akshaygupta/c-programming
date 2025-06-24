#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

/*
  Function Declarations for builtin shell commands:
 */
int shh_cd(char **args);
int shh_help(char **args);
int shh_exit(char **args);
int shh_launch(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_str[] = {
    "cd",
    "help",
    "exit"
};

int (*builtin_func[]) (char **) = {
    &shh_cd,
    &shh_help,
    &shh_exit
};

int shh_num_builtins() {
    return sizeof(builtin_str)/sizeof(char *);
}


/*
  Builtin function implementations.
*/
int shh_cd(char **args) {
    if (args[1] == NULL) {
        fprintf(stderr, "shh: expected argument to \"cd\"\n");
    } else {
        if (chdir(args[1]) != 0){
            perror("shh");
        }
    }
    return 1;
}

int shh_help(char **args) {
    int i;
    printf("AkG's SHH \n");
    printf("Type program names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (i=0; i<shh_num_builtins(); i++) {
        printf("    %s\n", builtin_str[i]);
    }

    printf("Use the man command for information on other programs.\n");
    return 1;
}

int shh_exit(char **args) {
    return 0;
}

int shh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork();
    if (pid == 0){
        // Child process
        if (execvp(args[0], args) == -1) {
            perror("shh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        // Error forking
        perror("shh");
    } else {
        // Parent process
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int shh_execute(char **args) {
    int i;

    if (args[0] == NULL) {
        // An empty command was entered
        return 1;
    }

    for (i=0; i<shh_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0){
            return (*builtin_func[i])(args);
        }
    }

    return shh_launch(args);
}


/**
 * @brief Splits a given input line into an array of tokens based on delimiters.
 *
 * This function takes a string and splits it into tokens using the delimiters
 * defined in SHH_TOK_DELIM (" \t\n\r\a"). It dynamically allocates memory for
 * the array of tokens and resizes it as needed. The resulting array is
 * terminated by a NULL pointer.
 *
 * @param line The input string to be split. This string will be modified.
 * @return A NULL-terminated array of strings (tokens). The caller is responsible
 *         for freeing the memory allocated for the array.
 *
 * @note If memory allocation fails, the function prints an error message to stderr
 *       and exits the program.
 */
#define SHH_TOK_BUFSIZE 1024
#define SHH_TOK_DELIM " \t\n\r\a"
char **shh_split_line(char *line) {
    int bufsize = SHH_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(sizeof(char*) * bufsize);
    char *token;

    if (!tokens) {
        fprintf(stderr, "shh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    /**
     * Tokenizes the input line into an array of strings using the specified delimiters.
     * 
     * The function uses strtok to split the input line into tokens, storing each token
     * in a dynamically allocated array. If the number of tokens exceeds the current buffer
     * size, the buffer is reallocated to accommodate more tokens. The array is terminated
     * with a NULL pointer.
     *
     * Error handling is performed for memory allocation failures, printing an error message
     * and exiting if realloc fails.
     *
     * Returns:
     *   A NULL-terminated array of token strings.
     */
    token = strtok(line, SHH_TOK_DELIM);
    while(token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += SHH_TOK_BUFSIZE;
            tokens = realloc(tokens, sizeof(char*) * bufsize);
            if (!tokens) {
                fprintf(stderr, "shh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, SHH_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

/**
 * Reads a line of input from standard input (stdin), dynamically allocating and resizing
 * the buffer as needed to accommodate the entire line.
 *
 * The function reads characters one by one using getchar(), storing them in a buffer.
 * - If the character is EOF or a newline ('\n'), the buffer is null-terminated and returned.
 * - Otherwise, the character is added to the buffer.
 * - If the buffer exceeds its current size, it is reallocated with additional space.
 * - If memory allocation fails at any point, an error message is printed and the program exits.
 *
 * Returns:
 *   char* - A pointer to the dynamically allocated buffer containing the input line.
 *           The caller is responsible for freeing this memory.
 */
#define SHH_RL_BUFSIZE 1024
char *shh_read_line(void) {
    int bufsize = SHH_RL_BUFSIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char)*bufsize);
    int c;

    // Check if memory allocation failed and handle the error by printing a message and exiting.
    if (!buffer) {
        fprintf(stderr, "shh: allocation error\n");
        exit(EXIT_FAILURE);
    }

    /**
     * Reads a line of input from stdin, dynamically allocating and resizing the buffer as needed.
     *
     * This loop reads characters one by one using getchar(), storing them in a buffer.
     * - If the character is EOF or newline ('\n'), the buffer is null-terminated and returned.
     * - Otherwise, the character is added to the buffer.
     * - If the buffer exceeds its current size, it is reallocated with additional space.
     * - If memory allocation fails, an error message is printed and the program exits.
     *
     * Returns:
     *   A pointer to the dynamically allocated buffer containing the input line.
     */
    while (1){
        // Read a character
        c = getchar();

        if (c == EOF || c == '\n'){
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize){
            bufsize += SHH_RL_BUFSIZE;
            buffer = realloc(buffer, bufsize);
            if (!buffer){
                fprintf(stderr, "shh: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}

/**
 * @brief Main loop of the shell.
 *
 * Continuously prompts the user for input, reads a line from standard input,
 * splits the line into arguments, and executes the command. The loop continues
 * until the executed command signals to exit (status becomes 0).
 *
 * Memory allocated for the input line and arguments is freed after each iteration.
 */
void shh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("> ");
        line = shh_read_line();
        args = shh_split_line(line);
        status = shh_execute(args);

        free(line);
        free(args);
    }   while(status);
}

int main(int argc, char **argv) {
    
    // Load config files, if any.
    
    // Run command loop.
    shh_loop();


    // Perform proper shutdown/cleanup.


    return EXIT_SUCCESS;
}