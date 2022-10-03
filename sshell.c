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
    char *cmd; //date, ls, cd, exit
    char *args[ARGS_MAX];
};

// end command structure

void setup_cmd_struct(char *cmd) {
        // create all structs for cmds and return back
        //struct command_struct cmd_str;
        //cmd_str = cmd;

}

void parse_args_from_cmd() {

}

struct command_struct parse_cmd(char *cmd) {
        // get all commands and put in array
        // get all args of cmd and put in rray in cmd
        // return
        //printf("test: %s\n", cmd);
        char* has_multiple_commands = strchr(cmd, '|');
        if (has_multiple_commands) {
                // loop through all commands and break at "|"
                printf("multiple cmds\n");
        } else {
                char* temp_string = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_string, cmd);
                char *first_cmd = strtok(temp_string, " ");
                struct command_struct new_cmd;
                new_cmd.cmd = first_cmd;
                if (strlen(first_cmd) == strlen(cmd)) {
                        new_cmd.args[0] = NULL;
                } else {
                        printf("got args\n");
                        new_cmd.args[0] = first_cmd;
                        new_cmd.args[1] = "-u";
                        new_cmd.args[2] = NULL;
                        //parse through the end of line
                        //append NULL at the end of array
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
                char *testargs[] = {cmd, NULL, NULL};
                pid_t pid;
                pid = fork();
                if (pid > 0) { // parent
                        int return_value;
                        waitpid(pid, &return_value, 0);
                        fprintf(stderr, "+ completed '%s': [%d]\n", cmd, WEXITSTATUS(return_value));
                } else if (pid == 0) { // child
                        execvp(cmd_to_run.cmd, cmd_to_run.args);
                        perror("execv");
                        exit(1);
                } else { printf("inital process error out"); }   
        }
        return EXIT_SUCCESS;
}

    