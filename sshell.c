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
#define ARGS_MAX 16

// command structure 

struct command_struct {
    char *program; // date, ls, cd, exit
    char *args[ARGS_MAX];
    char *output_file;
};

// end command structure

// useful functions

char *get_program_name(char *cmd) {
        char *temp_prog = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(temp_prog, cmd);
        char *prog = strtok(temp_prog, " ");
        return prog;
}

int get_number_of_args(char *cmd) { // redo this
        int current_arg = 1;
        char *prog = get_program_name(cmd);
        char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(temp_cmd, cmd);
        char *cmd_arg = strtok(temp_cmd, " ");
        while (cmd_arg != NULL) {
                if (strcmp(cmd_arg, prog)) {
                        current_arg++;
                }
                cmd_arg = strtok(NULL, " ");
        } 
        return current_arg;
}

// end useful functions

// error management functions

bool check_if_too_many_args(char *cmd) {
        if (get_number_of_args(cmd) > ARGS_MAX) {
                return true;
        }
        return false;
}

bool check_if_valid_program(char *cmd) {

}

bool check_for_valid_output_file(struct command_struct cmd_struct) {
        // check struct
}

// end error management

void setup_cmd_struct(char *cmd) {
        // create all structs for cmds and return back
        //struct command_struct cmd_str;
        //cmd_str = cmd;

        // should move some stuff from parse_cmd into here
}

void pwd_cmd() {

}

void cd_cmd() {

}

void parse_args_from_cmd() {

}

struct command_struct parse_cmd(char *cmd) {
        char* has_multiple_commands = strchr(cmd, '|');
        if (has_multiple_commands) {
                // loop through all commands and break at "|"
                printf("multiple cmds\n");
        } else {
                char *prog = get_program_name(cmd);
                struct command_struct new_cmd;
                new_cmd.program = prog;

                char* has_output_file = strchr(cmd, '>');
                if (has_output_file) {
                        printf("output file detected\n");
                }

                if (strlen(prog) == strlen(cmd)) { new_cmd.args[0] = NULL; 
                } else {
                        // todo: redo this as irs the same as get_number_of_args
                        int current_arg = 1;
                        new_cmd.args[0] = prog; // set first arg to program name
                        char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                        strcpy(temp_cmd, cmd);
                        char *cmd_arg = strtok(temp_cmd, " ");
                        while (cmd_arg != NULL) {
                                if (strcmp(cmd_arg, prog)) {
                                        if (current_arg == ARGS_MAX - 1) { break; } // break to save last arg for null
                                        new_cmd.args[current_arg] = cmd_arg;
                                        current_arg++;
                                }
                                cmd_arg = strtok(NULL, " ");
                        }
                        new_cmd.args[current_arg] = NULL;
                }
                return new_cmd;
        }
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
                }
                struct command_struct cmd_to_run = parse_cmd(cmd); // todo: add piepline to support multiple cmds
                if (!check_if_too_many_args(cmd)) {
                        pid_t pid;
                        pid = fork();
                        // needs to handle errors if program isnt valid
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
        return EXIT_SUCCESS;
}

    