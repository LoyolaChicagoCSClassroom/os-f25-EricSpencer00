/*
 * Simple shell (nsh) for POSIX systems
 * - prints a prompt
 * - reads a line with getline
 * - splits the line into argv[] by whitespace (simple tokenizer)
 * - forks and execv the command (searches PATH via execvp)
 * - waits for the child to exit
 *
 * Build: gcc -o nsh src/nsh.c
 * Run:   ./nsh
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_ARGS 64

int main(int argc, char **argv) {
    char *line = NULL;
    size_t bufsize = 0;

    while (1) {
        // 1. Print prompt
        printf("$ ");
        fflush(stdout);

        // 2. Read a line
        ssize_t linelen = getline(&line, &bufsize, stdin);
        if (linelen == -1) {
            // EOF (Ctrl-D) or error
            printf("\n");
            break;
        }

        // Trim trailing newline
        if (linelen > 0 && line[linelen-1] == '\n') {
            line[linelen-1] = '\0';
            linelen--;
        }

        // Skip empty lines
        if (linelen == 0) continue;

        // Tokenize the line into args (whitespace separated)
        char *argvec[MAX_ARGS];
        int argcount = 0;
        char *saveptr = NULL;
        char *tok = strtok_r(line, " \t", &saveptr);
        while (tok && argcount < (MAX_ARGS-1)) {
            argvec[argcount++] = tok;
            tok = strtok_r(NULL, " \t", &saveptr);
        }
        argvec[argcount] = NULL;

        if (argcount == 0) continue;

        // Built-in: exit
        if (strcmp(argvec[0], "exit") == 0) {
            break;
        }

        // Fork
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // Child: execute command using execvp (searches PATH)
            execvp(argvec[0], argvec);
            // If execvp returns, it failed
            perror("exec");
            _exit(127);
        } else {
            // Parent: wait for child
            int status = 0;
            pid_t w = waitpid(pid, &status, 0);
            if (w == -1) {
                perror("waitpid");
            }
            // Optionally report exit status
            // if (WIFEXITED(status)) printf("child exit %d\n", WEXITSTATUS(status));
        }
    }

    free(line);
    return 0;
}
