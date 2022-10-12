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
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512
#define ARGS_MAX 17
#define SPACE_CHAR 32
#define PIPE_MAX 2
#define COMMANDS_MAX 3

struct command_struct {
        char *full_cmd;
        char *program;
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

bool check_if_too_many_pipes(char *cmd) {
        int pipes = 0;
        int i;
        for (i = 0; i < strlen(cmd); i++) {
                if (cmd[i] == '|') {
                        pipes++;
                }
        }
        if (pipes > PIPE_MAX) {
                return true;
        }
        return false;
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
        // todo: echo hello > fails
                fprintf(stderr, "Error: no output file\n");
                can_run = false;
        } else if (check_if_too_many_args(cmd_struct)) { // 3rd leftmost
                fprintf(stderr, "Error: too many process arguments\n");
                can_run = false;
        }
        // todo: add more here
        return can_run;
}

struct command_struct parse_single_cmd(char *cmd) {
        char *prog = get_program_name(cmd);
        struct command_struct new_cmd;
        new_cmd.full_cmd = cmd;
        new_cmd.program = prog;
        new_cmd.args[0] = prog;
        new_cmd.number_of_args = 1;
        new_cmd.has_output_file = false;
        new_cmd.has_input_file = false;

        // parse output
        char* has_output_file = strchr(cmd, '>');
        if (has_output_file) {
                new_cmd.has_output_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '>')+1;
                // todo: spit out an error its empty
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

void cd_execution(const char *filename) {
        printf("%s", filename);
        chdir(filename);
        /*
        printf("here in cd %s\n", filename);
        char buf[256];
        buf[0] = '\"';
        for (int i=1; i<255; i++) {
                if (filename[i-1] == '\0') {
                        buf[i] = '\"';
                        buf[i+1] = '\0';
                        break;
                }
                buf[i] = filename[i-1];
        }
        */
        //printf("%s\n", buf);
        //printf("%i\n", chdir(".."));
        //strcat("\"", buf);
        //printf("2%s\n", buf);
        //strcat(filename, buf);
        //printf("3%s\n", buf);
        //strcat("\"", buf);
        //printf("4%s\n", buf);
        //int ret = 
        //chdir(getcwd(buf, sizeof(buf)));
        //printf("result: %s, %i\n", buf, chdir(buf));
        //chdir("..");
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
                //cd in else, since it has 2 arguments 
                else if (!strcmp(cmd, "cd")) { 
                        const char dot[256] = "..";
                        printf("dot = %s\n", dot);
                        cd_execution(dot);
                }
                else if (!strcmp(cmd, "pwd")) {
                        // pwd command
                        pwd_execution();
                } else if (!strcmp(cmd, "pushd")) {

                } else if (!strcmp(cmd, "popd")) {

                } else if (!strcmp(cmd, "dirs")) {

                } else {
                        char* has_multiple_commands = strchr(cmd, '|');
                        if (has_multiple_commands) {
                                if (!check_if_too_many_pipes(cmd)) {
                                        char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                                        strcpy(temp_cmd, cmd);
                                        char *cmd_arg = strtok(temp_cmd, "|");
                                        struct command_struct commands[COMMANDS_MAX];
                                        char *command_strings[COMMANDS_MAX];
                                        int number_of_commands = 0;
                                        while (cmd_arg != NULL) {
                                                if (cmd_arg[0] == SPACE_CHAR) {cmd_arg++;}
                                                command_strings[number_of_commands] = cmd_arg;
                                                number_of_commands++;
                                                cmd_arg = strtok(NULL, "|");
                                        }
                                        int cmd_s;
                                        bool invalid_command = false;
                                        for(cmd_s = 0; cmd_s < number_of_commands; cmd_s++) {
                                                struct command_struct cmd_to_run = parse_single_cmd(command_strings[cmd_s]);
                                                commands[cmd_s] = cmd_to_run;
                                                bool can_run = sanity_check_cmd(cmd_to_run);
                                                if (!can_run) {
                                                        invalid_command = true;
                                                }
                                        }
                                        if (!invalid_command) {
                                                size_t cmd_size = number_of_commands;
                                                pid_t *shared_return_values = mmap(NULL, cmd_size,
                                                        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED,
                                                        -1, 0);
                                                pid_t pid;
                                                pid = fork();
                                                if (pid > 0) {
                                                        int return_value;
                                                        waitpid(pid, &return_value, 0);
                                                        if (number_of_commands > 2) {
                                                                fprintf(stderr, "+ completed '%s': [%d][%d][%d]\n", cmd, WEXITSTATUS(shared_return_values[2]), WEXITSTATUS(shared_return_values[1]), WEXITSTATUS(return_value));
                                                        } else { fprintf(stderr, "+ completed '%s': [%d][%d]\n", cmd, WEXITSTATUS(shared_return_values[1]), WEXITSTATUS(return_value));}
                                                } else if (pid == 0) {
                                                        int cmd_n, in_pipe;
                                                        int fd[2];
                                                        in_pipe = STDIN_FILENO;
                                                        for (cmd_n = 0; cmd_n < number_of_commands - 1; cmd_n++) {
                                                                pipe(fd);
                                                                pid_t child_pid;
                                                                child_pid = fork();
                                                                if (child_pid > 0) {
                                                                        int return_value;
                                                                        waitpid(child_pid, &return_value, 0);
                                                                        shared_return_values[cmd_n] = return_value;
                                                                } else if (child_pid == 0) {
                                                                        if (in_pipe != STDIN_FILENO) {
                                                                                dup2(in_pipe, STDIN_FILENO);
                                                                                close(in_pipe);
                                                                        }
                                                                        dup2(fd[1], STDOUT_FILENO);
                                                                        close(fd[1]);
                                                                        execvp(commands[cmd_n].program, commands[cmd_n].args);
                                                                        exit(1); 
                                                                }
                                                                close(in_pipe);
                                                                close(fd[1]);
                                                                in_pipe = fd[0];
                                                        }
                                                        if (in_pipe != STDIN_FILENO) {
                                                                dup2(in_pipe, STDIN_FILENO);
                                                                close(in_pipe);
                                                        }
                                                        execvp(commands[cmd_n].program, commands[cmd_n].args);
                                                        exit(1);
                                                }
                                        }
                                 } else {
                                        fprintf(stderr, "Error: too many pipes\n");  
                                }
                        } else {
                                struct command_struct cmd_to_run = parse_single_cmd(cmd);
                                bool can_run = sanity_check_cmd(cmd_to_run);
                                if (can_run) {
                                        printf("%s\n", cmd_to_run.program);
                                        if (!strcmp(cmd_to_run.program, "cd")) {
                                                cd_execution(cmd_to_run.args[1]);
                                                continue;
                                        }
                                        pid_t pid;
                                        pid = fork();
                                        printf("pid: %i\n",pid);
                                        if (pid > 0) { // parent
                                                //printf("%i\n", pid);
                                                int return_value;
                                                waitpid(pid, &return_value, 0);
                                                fprintf(stderr, "+ completed '%s': [%d]\n", cmd, WEXITSTATUS(return_value));
                                        } else if (pid == 0) { // child
                                                if (cmd_to_run.has_output_file) {
                                                        int fd;
                                                        fd = open(cmd_to_run.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                        dup2(fd, 1);
                                                        close(fd);
                                                } else { 
                                                        int stdout = dup(1);
                                                        dup2(stdout, 1);
                                                }
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