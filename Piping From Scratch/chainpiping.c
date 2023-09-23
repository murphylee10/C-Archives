#include "command.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

void child_exec(int cmd_index, int n, int pipefd[n - 1][2], const command_list *cs)
{
    if ((cmd_index != 0 && dup2(pipefd[cmd_index - 1][0], STDIN_FILENO) == -1) || (cmd_index != n - 1 && dup2(pipefd[cmd_index][1], STDOUT_FILENO) == -1))
    {
        perror("dup error");
        exit(1);
    }

    for (int i = 0; i < n - 1; i++)
    {
        close(pipefd[i][0]);
        close(pipefd[i][1]);
    }
    if (execvp(cs->cmd[cmd_index][0], cs->cmd[cmd_index]) == -1) {
        perror("exec error");
        exit(1);
    }
}

void chain_piping(const command_list *cs) {
    int n = cs->n;
    int pipefd[n - 1][2];
    pid_t p;

    for (int i = 0; i < n - 1; i++) {
        if (pipe(pipefd[i]) == -1) {
            perror("piping failed");
            exit(1);
        }
    }

    for (int i = 0; i < n; i++) {
        if ((p = fork()) == -1){
            perror("fork failed");
            exit(1);
        }
        if (!p) {
            child_exec(i, n, pipefd, cs);
        }
    }

    for (int j = 0; j < n - 1; j++)
    {
        close(pipefd[j][0]);
        close(pipefd[j][1]);
    }
    for (int j = 0; j < n; j++)
        wait(NULL);
}