// sshell.c
// Colton Perazzo and Necz András 
// ECS 150, University of California, Davis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 17

struct command_struct {
        char *full_cmd;
        char *program; // date, ls, cd, exit
        char *args[ARGS_MAX];
        char *output_file; 
        bool has_output_file;

        char *input_file;
        bool has_input_file;

        int number_of_args;
};

struct stack {
        int top_stack;
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

bool check_if_missing_output_file(struct command_struct cmd_struct) {
        if (cmd_struct.has_output_file) {
                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '>') {
                        return true;
                }
        }
        return false;
}

bool check_if_output_file_does_not_exist(struct command_struct cmd_struct) {
        return false;
}

bool check_if_invalid_commands(struct command_struct cmd_struct) {
        if (cmd_struct.program[0] == '>' || cmd_struct.program[0] == '|') {
                return true;
        } else {
                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '|') {
                        return true;
                }
        }
        return false;
}


bool sanity_check_cmd(struct command_struct cmd_struct) {
        bool can_run = true;
        if (check_if_invalid_commands(cmd_struct)) { // leftmost
                fprintf(stderr, "Error: missing command\n");
                can_run = false;
        } else if (check_if_missing_output_file(cmd_struct)) { // 2nd leftmost
                fprintf(stderr, "Error: no output file\n");
                can_run = false;
        } else if (check_if_too_many_args(cmd_struct)) { // 3rd leftmost
                fprintf(stderr, "Error: too many process arguments\n");
                can_run = false;
        }
        // todo: add more here
        return can_run;
}

void setup_multiple_cmds() {

}

struct command_struct parse_single_cmd(char *cmd) {
        char *prog = get_program_name(cmd);
        struct command_struct new_cmd;
        new_cmd.full_cmd = cmd;
        new_cmd.program = prog;
        new_cmd.args[0] = prog;
        new_cmd.number_of_args = 1;

        // parse output
        char* has_output_file = strchr(cmd, '>');
        if (has_output_file) {
                new_cmd.has_output_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '>')+1;
                new_cmd.output_file = strtok(split_at_output, " ");;
        }

        // parse input
        char *has_input_file = strchr(cmd, '<');
        if (has_input_file) {
                new_cmd.has_input_file = true;
        }

        if (strlen(prog) == strlen(cmd)) { new_cmd.args[1] = NULL; }
        else {
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *cmd_arg = strtok(temp_cmd, " ");
                while (cmd_arg != NULL) {
                        bool can_add_arg = true;
                        if (!strcmp(cmd_arg, prog)) {
                                can_add_arg = false;
                        } else if (!strcmp(cmd_arg, ">")) {
                                can_add_arg = false;
                        } else {
                                if (new_cmd.has_output_file) {
                                        if (strcmp(cmd_arg, new_cmd.output_file) == 0) {
                                                can_add_arg = false;
                                        }
                                }
                        }
                        if (can_add_arg) {
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

int file_name_errors(int err) {

}

void pwd_execution() {
        //gotta deal with error handling
        char buf[256];
        printf("%s\n", getcwd(buf, sizeof(buf)));
        /*
        if (chdir("/tmp") != 0)
                perror("chdir() error()");
        else {
                if (getcwd(cwd, sizeof(cwd)) == NULL)
                        perror("getcwd() error");
                else
                        printf("current working directory is: %s\n", cwd);
        }
        */
}

void cd_execution(const char filename[256]) {
        printf("here in cd");
        int ret = chdir(filename);
        printf("%i\n", ret);
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
                        cd_execution()
                } else if (!strcmp(cmd, "pwd")) {
                        // pwd command
                        pwd_execution();

                } else {
                        //char* has_multiple_commands = strchr(cmd, '|');
                        bool has_multiple_commands = false; //temp;
                        if (has_multiple_commands) {
                                // todo: if multiple commands, ensure only last cmd can output redirect
                        } else {
                                struct command_struct cmd_to_run = parse_single_cmd(cmd); // todo: add piepline to support multiple cmds
                                bool can_run = sanity_check_cmd(cmd_to_run);
                                if (can_run) {
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
                                }
                        }
                }
        }
        return EXIT_SUCCESS;
}
