// sshell.c
// Colton Perazzo and András Necz 
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
#define PIPE_MAX 3
#define COMMANDS_MAX 4

#define OUTPUT_KEY 0
#define INPUT_KEY 1

struct command_struct {
        int cmd_id;
        int total_cmds;

        char *full_cmd;
        char *edited_cmd;
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

bool check_if_invalid_command(struct command_struct cmd_struct) {
        if (cmd_struct.program[0] == '>' || cmd_struct.program[0] == '|') {
                return true;
        } else {
                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '|') {
                        return true;
                }
        }
        return false;
}

bool check_if_missing_inputoutput_file(struct command_struct cmd_struct, int mode) {
        bool missing_file = false;
        switch (mode) {
                case OUTPUT_KEY:
                        if (cmd_struct.has_output_file) {
                                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '>') {
                                        missing_file = true;
                                } else {
                                        if(cmd_struct.output_file == NULL) {missing_file = true;} 
                                }
                        }
                default:
                        if (cmd_struct.has_input_file) {
                                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '<') {
                                        missing_file = true;
                                } else {
                                        if (cmd_struct.input_file == NULL) {missing_file = true;}
                                }
                        }
        }
        return missing_file;
}

bool check_if_inputouput_file_is_null(struct command_struct cmd_struct, int mode) {
        bool null_file = false;
        switch (mode) {
                case OUTPUT_KEY:
                       if (cmd_struct.has_output_file) {
                                char *includes_redirect = strchr(cmd_struct.output_file, '/');
                                if (includes_redirect) {
                                        null_file = true;
                                }
                        } 
                default:
                        if (cmd_struct.has_input_file) {
                                if(access(cmd_struct.input_file, F_OK) != 0) {
                                        null_file = true;
                                }
                        }
                        
        }
        return null_file;
}

bool check_if_piping_inputpout_is_mislocated(struct command_struct cmd_struct, int mode) {
        bool mislocated = false;
        switch (mode) {
                case OUTPUT_KEY:
                        if (cmd_struct.has_output_file) {
                                if (cmd_struct.cmd_id != cmd_struct.total_cmds) {
                                        mislocated = true;
                                }
                        }
                default:
                        if (cmd_struct.has_input_file) {
                                if (cmd_struct.cmd_id != 0) {
                                        mislocated = true;
                                }
                        }

        }
        return mislocated;
}

bool sanity_check_cmd(struct command_struct cmd_struct) {
        bool can_cmd_run = true;
        if (check_if_invalid_command(cmd_struct)) {
                fprintf(stderr, "Error: missing command\n");
                return false;
        } else {
                if (cmd_struct.has_output_file) {
                        if (check_if_missing_inputoutput_file(cmd_struct, OUTPUT_KEY)) { 
                                fprintf(stderr, "Error: no output file\n");
                                return false;
                        }
                        if(check_if_piping_inputpout_is_mislocated(cmd_struct, OUTPUT_KEY)) {
                                fprintf(stderr, "Error: mislocated output redirection\n");
                                return false;
                        }
                        if (check_if_inputouput_file_is_null(cmd_struct, OUTPUT_KEY)) {
                                fprintf(stderr, "Error: cannot open output file\n");
                                return false;
                        }
                }
                if (cmd_struct.has_input_file) {
                        if (check_if_missing_inputoutput_file(cmd_struct, INPUT_KEY)) {
                                fprintf(stderr, "Error: no input file\n");
                                return false;
                        }
                        if (check_if_piping_inputpout_is_mislocated(cmd_struct, INPUT_KEY)) {
                                fprintf(stderr, "Error: mislocated input redirection\n");
                                return false;
                        }
                        if (check_if_inputouput_file_is_null(cmd_struct, INPUT_KEY)) {
                                fprintf(stderr, "Error: cannot open input file\n");
                                return false;
                        }
                }
                if (check_if_too_many_args(cmd_struct)) {
                        fprintf(stderr, "Error: too many process arguments\n");
                        return false;
                }

        }
        return true;
}

struct command_struct parse_single_cmd(char *cmd, int num, int total) {
        char *prog = get_program_name(cmd);
        struct command_struct new_cmd;
        new_cmd.cmd_id = num;
        new_cmd.total_cmds = total;
        new_cmd.full_cmd = cmd;
        new_cmd.program = prog;
        new_cmd.args[0] = prog;
        new_cmd.number_of_args = 1;
        new_cmd.has_output_file = false;
        new_cmd.has_input_file = false;
        char *edited_cmd = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(edited_cmd, cmd);
        new_cmd.edited_cmd = edited_cmd;
        char* has_output_file = strchr(cmd, '>');
        if (has_output_file) {
                new_cmd.has_output_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '>')+1;
                new_cmd.output_file = strtok(split_at_output, " ");
                int index = strcspn(cmd, ">");
                edited_cmd[index] = ' ';
        }
        char *has_input_file = strchr(cmd, '<');
        if (has_input_file) {
                new_cmd.has_input_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '<')+1;
                new_cmd.input_file = strtok(split_at_output, " ");
                int index = strcspn(cmd, "<");
                edited_cmd[index] = ' ';
        }
        if (strlen(prog) == strlen(cmd)) { new_cmd.args[1] = NULL; }
        else {
                char *temp_cmd = calloc(strlen(edited_cmd)+1, sizeof(char));
                strcpy(temp_cmd, edited_cmd);
                char *cmd_arg = strtok(temp_cmd, " ");
                while (cmd_arg != NULL) {
                        bool can_add_arg = true;
                        if (!strcmp(cmd_arg, prog)) {
                                can_add_arg = false;
                        } else if (!strcmp(cmd_arg, ">")) {
                                can_add_arg = false;
                        } else if (!strcmp(cmd_arg, "|")) {
                                can_add_arg = false;
                        } else {
                                if (new_cmd.has_output_file) {
                                        if (new_cmd.output_file != NULL) {
                                                if (strcmp(cmd_arg, new_cmd.output_file) == 0) {
                                                        can_add_arg = false;
                                                } 
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
        //printf("%s\n", getcwd(buf, sizeof(buf)));
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
        //int ret = chdir(filename[1]);
	//fprintf(stderr, "+ completed  '%s': [%d]\n", filename, WEXITSTATUS(ret));
	//int error_code = errno;
	
	
	//switch (error_code) {
		//case 2:
			//fprintf(stderr, "Error: command not found\n");  
	//}
	//exit(1);
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
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
                        break;
                }
                //cd in else, since it has 2 arguments
                /*else if (!strcmp(cmd, "cd")) { 
                        const char dot[256] = "..";
                        //printf("dot = %s\n", dot);
                        cd_execution(dot);
                }*/
                else if (!strcmp(cmd, "pwd")) {
                        pwd_execution();
                } 
                else if (!strcmp(cmd, "pushd")) {

                } 
                else if (!strcmp(cmd, "popd")) {

                } 
                else if (!strcmp(cmd, "dirs")) {

                } 
                else {
                        char* has_multiple_commands = strchr(cmd, '|');
                        if (has_multiple_commands) {
                                if (cmd[0] != '|') {
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
                                                        struct command_struct cmd_to_run = parse_single_cmd(command_strings[cmd_s], cmd_s, number_of_commands-1);
                                                        commands[cmd_s] = cmd_to_run;
                                                        bool can_run = sanity_check_cmd(cmd_to_run);
                                                        if (!can_run) {
                                                                invalid_command = true;
                                                        } else {
                                                                if (cmd[strlen(cmd)-1] == '|') {
                                                                        invalid_command = true;
                                                                        fprintf(stderr, "Error: missing command\n");
                                                                }
                                                        }
                                                }
                                                if (!invalid_command) {
                                                        int return_values[number_of_commands];
                                                        int fd_1[2], fd_2[2], fd_3[2];
                                                        pipe(fd_1);
                                                        pid_t pid_1;
                                                        pid_1 = fork();
                                                        if (pid_1 == 0) {
                                                                close(fd_1[0]); // close pipe 1 read
                                                                dup2(fd_1[1], STDOUT_FILENO);
                                                                close(fd_1[1]); // close pipe 1 write
                                                                if (commands[0].has_input_file) { // check input for pipe
                                                                        int fd_input;
                                                                        fd_input = open(commands[0].input_file, O_RDONLY);
                                                                        dup2(fd_input, STDIN_FILENO);
                                                                        close(fd_input);
                                                                }
                                                                execvp(commands[0].program, commands[0].args);
                                                        }
                                                        pipe(fd_2);
                                                        pid_t pid_2;
                                                        pid_2 = fork();
                                                        if (pid_2 == 0) {
                                                                close(fd_1[1]);  
                                                                dup2(fd_1[0], STDIN_FILENO);
                                                                close(fd_1[0]);
                                                                if (number_of_commands > 2) {
                                                                        close(fd_2[0]); // close pipe 2 read
                                                                        dup2(fd_2[1], STDOUT_FILENO);
                                                                        close(fd_2[1]); // close pipe 2 write
                                                                }
                                                                if(commands[1].has_output_file) {
                                                                        if(commands[1].cmd_id == commands[1].total_cmds) {
                                                                                int fd_output;
                                                                                fd_output = open(commands[1].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                                                dup2(fd_output, STDOUT_FILENO);
                                                                                close(fd_output);
                                                                        } 
                                                                }
                                                                execvp(commands[1].program, commands[1].args);
                                                        }
                                                        close(fd_1[1]);
                                                        close(fd_1[0]);
                                                        if (number_of_commands > 2) {
                                                                pipe(fd_3);
                                                                pid_t pid_3;
                                                                pid_3 = fork();
                                                                if (pid_3 == 0) {
                                                                        close(fd_2[1]);
                                                                        dup2(fd_2[0], STDIN_FILENO);
                                                                        close(fd_2[0]);
                                                                        if (number_of_commands > 3) {
                                                                                close(fd_3[0]); // close pipe 3 read
                                                                                dup2(fd_3[1], STDOUT_FILENO);
                                                                                close(fd_3[1]); // close pipe 3 write
                                                                        }
                                                                        if(commands[2].has_output_file) {
                                                                                if(commands[2].cmd_id == commands[2].total_cmds) {
                                                                                        int fd_output;
                                                                                        fd_output = open(commands[2].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                                                        dup2(fd_output, STDOUT_FILENO);
                                                                                        close(fd_output);
                                                                                } 
                                                                        }
                                                                        execvp(commands[2].program, commands[2].args);
                                                                }
                                                                close(fd_2[1]);
                                                                close(fd_2[0]);
                                                                if (number_of_commands > 3) {
                                                                        pid_t pid_4;
                                                                        pid_4 = fork();
                                                                        if(pid_4 == 0) {
                                                                                close(fd_3[1]);
                                                                                dup2(fd_3[0], STDIN_FILENO);
                                                                                close(fd_3[0]);
                                                                                if(commands[3].has_output_file) {
                                                                                        if(commands[3].cmd_id == commands[3].total_cmds) {
                                                                                                int fd_output;
                                                                                                fd_output = open(commands[3].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                                                                dup2(fd_output, STDOUT_FILENO);
                                                                                                close(fd_output);
                                                                                        } 
                                                                                }
                                                                                execvp(commands[3].program, commands[3].args);  
                                                                        }
                                                                        close(fd_3[1]);
                                                                        close(fd_3[0]);
                                                                } else {
                                                                        close(fd_3[1]);
                                                                        close(fd_3[0]); 
                                                                }
                                                        } else {
                                                                close(fd_2[1]);
                                                                close(fd_2[0]);
                                                        }
                                                        int process;
                                                        for (process = 0; process < number_of_commands; process++) {
                                                                int return_value;
                                                                wait(&return_value);
                                                                return_values[process] = return_value;
                                                        }
                                                        if (number_of_commands > 2) {
                                                                fprintf(stderr, "+ completed '%s' [%d][%d][%d]\n", cmd, WEXITSTATUS(return_values[0]), WEXITSTATUS(return_values[1]), WEXITSTATUS(return_values[2]));
                                                        } else { fprintf(stderr, "+ completed '%s' [%d][%d]\n", cmd, WEXITSTATUS(return_values[0]), WEXITSTATUS(return_values[1]));}
                                                }
                                        } else {
                                                fprintf(stderr, "Error: too many pipes\n");  
                                        }
                                } else {
                                        fprintf(stderr, "Error: missing command\n");
                                } 
                        } else {
                                struct command_struct cmd_to_run = parse_single_cmd(cmd, 0, 0);
                                bool can_run = sanity_check_cmd(cmd_to_run);
                                if (can_run) {
                                        if (!strcmp(cmd_to_run.program, "cd")) {
                                                printf("about to execute cd\n"); 
						cd_execution(cmd);
                                                continue;
                                        }
                                        pid_t pid;
                                        pid = fork();
                                        //printf("pid: %i\n",pid);
                                        if (pid > 0) { // parent
                                                //printf("%i\n", pid);
                                                int return_value;
                                                waitpid(pid, &return_value, 0);
                                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(return_value));
                                        } else if (pid == 0) { // child
                                                if (cmd_to_run.has_output_file) {
                                                        int fd_output;
                                                        fd_output = open(cmd_to_run.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                        dup2(fd_output, STDOUT_FILENO);
                                                        close(fd_output);
                                                } else { 
                                                        if (cmd_to_run.has_input_file) {
                                                                int fd_input;
                                                                fd_input = open(cmd_to_run.input_file, O_RDONLY);
                                                                dup2(fd_input, STDIN_FILENO);
                                                                close(fd_input);
                                                        }
                                                }
                                                execvp(cmd_to_run.program, cmd_to_run.args);
                                                int error_code = errno;
                                                //printf("error code: %d\n", errno);
                                                switch (error_code) {
                                                        case 2:
                                                        fprintf(stderr, "Error: command not found\n");  
                                                }
                                                exit(1);
                                        } else { printf("Error: fork cannot be created\n"); break; }
                                }
                        }
                }
        }
        return EXIT_SUCCESS;
}
