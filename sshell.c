// sshell.c
// Colton Perazzo and Andras 
// ECS 150, University of California, Davis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 17

struct command_struct {
        char *program; // date, ls, cd, exit
        char *args[ARGS_MAX];
        char *output_file;
        int number_of_args;
};

char *get_program_name(char *cmd) {
        char *temp_prog = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(temp_prog, cmd);
        char *prog = strtok(temp_prog, " ");
        return prog;
}

bool check_if_too_many_args(struct command_struct cmd_struct) {
        if (cmd_struct.number_of_args > ARGS_MAX - 1) {
                return true;
        }
        return false;
}

void setup_multiple_cmds() {

}

struct command_struct setup_single_cmd(char *cmd) {
        char *prog = get_program_name(cmd);
        struct command_struct new_cmd;
        new_cmd.program = prog;
        new_cmd.args[0] = prog;
        new_cmd.number_of_args = 1;
        if (strlen(prog) == strlen(cmd)) { new_cmd.args[1] = NULL; }
        else {
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *cmd_arg = strtok(temp_cmd, " ");
                while (cmd_arg != NULL) {
                        if (strcmp(cmd_arg, prog)) {
                                if (new_cmd.number_of_args > (ARGS_MAX - 1)) {
                                        break;
                                }
                                new_cmd.args[new_cmd.number_of_args] = cmd_arg;
                                new_cmd.number_of_args = new_cmd.number_of_args + 1;
                        }
                        cmd_arg = strtok(NULL, " ");
                }
                new_cmd.args[new_cmd.number_of_args] = NULL;
        }
        return new_cmd;
}

int main(void) {
        char cmd[CMDLINE_MAX];
        while (1) {
                char *new_line;
                printf("sshell@ucd$ ");
                fflush(stdout);
                fgets(cmd, CMDLINE_MAX, stdin);
                if (!isatty(STDIN_FILENO)) {
                        printf("%s", cmd);
                        fflush(stdout);
                }
                new_line = strchr(cmd, '\n');
                if (new_line)
                        *new_line = '\0';
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } else if (!strcmp(cmd, "cd")) {
                        // cd command
                } else if (!strcmp(cmd, "pwd")) {
                        // pwd command
                } else {
                        char* has_multiple_commands = strchr(cmd, '|');
                        if (has_multiple_commands) {

                        } else {
                                struct command_struct cmd_to_run = setup_single_cmd(cmd); // todo: add piepline to support multiple cmds
                                //printf("num args struct %d\n", cmd_to_run.number_of_args);
                                if (!check_if_too_many_args(cmd_to_run)) {
                                        pid_t pid;
                                        pid = fork();
                                        if (pid > 0) { // parent
                                                int return_value;
                                                waitpid(pid, &return_value, 0);
                                                fprintf(stderr, "+ completed '%s': [%d]\n", cmd, WEXITSTATUS(return_value));
                                        } else if (pid == 0) { // child
                                                execvp(cmd_to_run.program, cmd_to_run.args);
                                                int error_code = errno;
                                                switch (error_code) {
                                                        case 2:
                                                        fprintf(stderr, "Error: command not found\n");  
                                                }
                                                exit(1);
                                        } else { printf("Error: fork cannot be created\n"); }   
                                } else {
                                        fprintf(stderr, "Error: too many process arguments\n");
                                } 
                        }
                }
        }
        return EXIT_SUCCESS;
}
