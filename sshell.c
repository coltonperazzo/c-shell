// sshell.c
// Colton Perazzo and Andras
// ECS 150, University of California, Davis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 16

// command structure 

struct command_struct {
    char *program; //date, ls, cd, exit
    char *args[ARGS_MAX];
};

// end command structure

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
                char* temp_prog = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_prog, cmd);
                char *prog = strtok(temp_prog, " ");
                struct command_struct new_cmd;
                new_cmd.program = prog;
                if (strlen(prog) == strlen(cmd)) { new_cmd.args[0] = NULL; 
                } else {
                        new_cmd.args[0] = prog;
                        int i = 1;
                        char *cmd_arg = strtok(cmd, " ");
                        while (cmd_arg != NULL) {
                                if (strcmp(cmd_arg, prog)) {
                                        new_cmd.args[i] = cmd_arg;
                                        i++;
                                }
                                cmd_arg = strtok(NULL, " ");
                        }
                        new_cmd.args[i] = NULL; // set last arg to NULL
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
                pid_t pid;
                pid = fork();
                // needs to handle errors if program isnt vallid
                if (pid > 0) { // parent
                        int return_value;
                        waitpid(pid, &return_value, 0);
                        fprintf(stderr, "+ completed '%s': [%d]\n", cmd, WEXITSTATUS(return_value));
                } else if (pid == 0) { // child
                        execvp(cmd_to_run.program, cmd_to_run.args);
                        perror("execv");
                        exit(1);
                } else { printf("inital process error out"); }   
        }
        return EXIT_SUCCESS;
}

    