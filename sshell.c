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

// Constants:
#define CMDLINE_MAX 512
#define ARGS_MAX 17
#define SPACE_CHAR 32
#define PIPE_MAX 3
#define COMMANDS_MAX 4

#define OUTPUT_KEY 0
#define INPUT_KEY 1

// Command struct:
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

/*
Returns a character of the program name parsed from the cmd put into the terminal.
@param char *cmd => command inserted into terminal (ex: echo hello world)
@returns char prog => program name (ex: echo)
*/
char *get_program_name(char *cmd) {
        char *temp_prog = calloc(strlen(cmd)+1, sizeof(char));
        strcpy(temp_prog, cmd);
        char *prog = strtok(temp_prog, " ");
        return prog;
}

/*
Returns a boolean if there is too many pipes in the inserted command.
@param char *cmd => command inserted into terminal (ex: echo hello world)
@returns boolean => true if > PIPE_MAX else false.
*/
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

/*
Returns a boolean if there is too many args in the inserted command.
@param command_struct cmd_struct => struct of the parsed inserted command.
@returns boolean => true if > ARGS_MAX else false.
*/
bool check_if_too_many_args(struct command_struct cmd_struct) {
        if (cmd_struct.number_of_args > ARGS_MAX - 1) {
                return true;
        }
        return false;
}

/*
Returns a boolean if the command is valid or not. Commands starting with ">" or "|" are not.
@param command_struct cmd_struct => struct of the parsed inserted command.
@returns boolean => if command is valid.
*/
bool check_if_invalid_command(struct command_struct cmd_struct) {
        // Check for invalid characters at the start of our command.
        if (cmd_struct.program[0] == '>' || cmd_struct.program[0] == '|') {
                return true;
        } else {
                // Check for invalid character at the end of our command.
                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '|') {
                        return true;
                }
        }
        return false;
}

/*
Returns a boolean if the inserted command has a input or output file. (ex: echo hello > file3 - true, echo hello > - false)
@param command_struct cmd_struct => struct of the parsed inserted command, int mode (0 for output, 1 for input).
@returns boolean => if file does exist in inserted command.
*/
bool check_if_missing_inputoutput_file(struct command_struct cmd_struct, int mode) {
        bool missing_file = false;
        switch (mode) {
                case OUTPUT_KEY:
                        if (cmd_struct.has_output_file) {
                                // Check if the last character of the inserted cmd is the redirect.
                                // if so, we must be missing our file.
                                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '>') {
                                        missing_file = true;
                                } else {
                                        // If output_file was not parsed correctly, return missing.
                                        if(cmd_struct.output_file == NULL) {missing_file = true;} 
                                }
                        }
                default:
                        if (cmd_struct.has_input_file) {
                                // Check if the last character of the inserted cmd is the redirect.
                                // if so, we must be missing our file.
                                if (cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '<') {
                                        missing_file = true;
                                } else {
                                        // If input_file was not parsed correctly, return missing.
                                        if (cmd_struct.input_file == NULL) {missing_file = true;}
                                }
                        }
        }
        return missing_file;
}

/*
Returns a boolean if the output or input file (which exists) can actually be opened/accessed.
@param command_struct cmd_struct => struct of the parsed inserted command, int mode (0 for output, 1 for input).
@returns boolean => if file can be accessed.
*/
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
                                // Check if we can access.
                                if(access(cmd_struct.input_file, F_OK) != 0) {
                                        null_file = true;
                                }
                        }
                        
        }
        return null_file;
}

/*
Returns a boolean if the output or input direction is mislocated (ex: trying to output as first program).
@param command_struct cmd_struct => struct of the parsed inserted command, int mode (0 for output, 1 for input).
@returns boolean => if mislocated or not.
*/
bool check_if_piping_inputpout_is_mislocated(struct command_struct cmd_struct, int mode) {
        bool mislocated = false;
        switch (mode) {
                case OUTPUT_KEY:
                        if (cmd_struct.has_output_file) {
                                // If cmd_id (index of command) is not the last cmd, we can't output.
                                if (cmd_struct.cmd_id != cmd_struct.total_cmds) {
                                        mislocated = true;
                                }
                        }
                default:
                        if (cmd_struct.has_input_file) {
                                // If cmd_id (index of cmd) is not the first cmd, we can't input.
                                if (cmd_struct.cmd_id != 0) {
                                        mislocated = true;
                                }
                        }

        }
        return mislocated;
}

/*
Returns a boolean if the command_struct is actually able to run or not, sanity checks args, output/input file/etc.
@param command_struct cmd_struct => struct of the parsed inserted command.
@returns boolean => if can run.
*/
bool sanity_check_cmd(struct command_struct cmd_struct) {
        bool can_cmd_run = true;
        if (check_if_invalid_command(cmd_struct)) { // Checks if the command is valid (ex: > file).
                fprintf(stderr, "Error: missing command\n");
                return false;
        } else {
                if (cmd_struct.has_output_file) { // Sanity checks all output related errors.
                        if (check_if_missing_inputoutput_file(cmd_struct, OUTPUT_KEY)) { 
                                // Checks if we are missing an output file (ex: echo >).
                                fprintf(stderr, "Error: no output file\n");
                                return false;
                        }
                        if(check_if_piping_inputpout_is_mislocated(cmd_struct, OUTPUT_KEY)) {
                                // Check if we are mislocated our output (ex: echo Hello world > file | cat file).
                                fprintf(stderr, "Error: mislocated output redirection\n");
                                return false;
                        }
                        if (check_if_inputouput_file_is_null(cmd_struct, OUTPUT_KEY)) {
                                // Checks if we can't access the output file (ex: echo hack > /etc/passwd).
                                fprintf(stderr, "Error: cannot open output file\n");
                                return false;
                        }
                }
                if (cmd_struct.has_input_file) { // Sanity checks all input related errors.
                        if (check_if_missing_inputoutput_file(cmd_struct, INPUT_KEY)) {
                                // Checks if we are missing an input file (ex: cat <).
                                fprintf(stderr, "Error: no input file\n");
                                return false;
                        }
                        if (check_if_piping_inputpout_is_mislocated(cmd_struct, INPUT_KEY)) {
                                // Checks if we are mislocated our input (ex: echo Hello world | grep world < file)
                                fprintf(stderr, "Error: mislocated input redirection\n");
                                return false;
                        }
                        if (check_if_inputouput_file_is_null(cmd_struct, INPUT_KEY)) {
                                // Checks if we can't access the input file (ex: grep toto < doesnotexist).
                                fprintf(stderr, "Error: cannot open input file\n");
                                return false;
                        }
                }
                // Checks if we have too many args.
                // (ex: ls 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16)
                if (check_if_too_many_args(cmd_struct)) {
                        fprintf(stderr, "Error: too many process arguments\n");
                        return false;
                }

        }
        return true;
}

/*
Grabs a command from the terminal and parses it for information to put into the struct. 
Parses through the command args and puts them into an array.
Parses through input/ouput file and puts them into their discitinve values.
@param char *cmd => command inserted into terminal (ex: echo hello world), int num (index of command), int total (total num of cmds).
@returns command_struct => struct fully parsed to be used by other functions.
*/
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
        if (has_output_file) { // Parse for output file.
                new_cmd.has_output_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '>')+1;
                new_cmd.output_file = strtok(split_at_output, " ");
                int index = strcspn(cmd, ">");
                edited_cmd[index] = ' ';
        }
        char *has_input_file = strchr(cmd, '<');
        if (has_input_file) { // Parse for input file.
                new_cmd.has_input_file = true;
                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                strcpy(temp_cmd, cmd);
                char *split_at_output = strchr(temp_cmd, '<')+1;
                new_cmd.input_file = strtok(split_at_output, " ");
                int index = strcspn(cmd, "<");
                edited_cmd[index] = ' ';
        }
        if (strlen(prog) == strlen(cmd)) { new_cmd.args[1] = NULL; } // If command has no args, set second arg to NULL (first arg is program name).
        else {
                // If we have args, loop through all the args by splitting at the " " and ensure the arg can be used.
                char *temp_cmd = calloc(strlen(edited_cmd)+1, sizeof(char));
                strcpy(temp_cmd, edited_cmd);
                char *cmd_arg = strtok(temp_cmd, " ");
                while (cmd_arg != NULL) {
                        printf("testing arg: %s\n", cmd_arg);
                        bool can_add_arg = true;
                        if (!strcmp(cmd_arg, prog)) { // If arg is program name, ignore.
                                can_add_arg = false;
                        } else if (!strcmp(cmd_arg, ">")) { // If arg is a ">" character, ignore.
                                can_add_arg = false;
                        } else if (!strcmp(cmd_arg, "|")) { // If arg is a "|" character, ignore.
                                can_add_arg = false;
                        } else {
                                if (new_cmd.has_output_file) {
                                        if (new_cmd.output_file != NULL) {
                                                // If arg is output file, ignore.
                                                if (strcmp(cmd_arg, new_cmd.output_file) == 0) {
                                                        can_add_arg = false;
                                                } 
                                        }
                                }
                        }
                        if (can_add_arg) { 
                                if (new_cmd.number_of_args > (ARGS_MAX - 1)) { // If we've reached the max args, break loop.
                                        break;
                                }
                                char* has_output_file = strchr(cmd_arg, '>');
                                if (has_output_file) {
                                        char *temp_cmd_output = calloc(strlen(cmd_arg)+1, sizeof(char));
                                        strcpy(temp_cmd_output, cmd_arg);
                                        char *cmd_arg_output = strtok(temp_cmd_output, ">");
                                        printf("got: %s\n", cmd_arg_output);
                                        if (strcmp(cmd_arg_output, new_cmd.output_file)) {
                                                printf("adding arg: %s\n", cmd_arg_output);
                                                new_cmd.args[new_cmd.number_of_args] = cmd_arg_output;
                                                new_cmd.number_of_args = new_cmd.number_of_args + 1;
                                        }
                                } else {
                                        new_cmd.args[new_cmd.number_of_args] = cmd_arg;
                                        new_cmd.number_of_args = new_cmd.number_of_args + 1;
                                }
                        }
                        cmd_arg = strtok(NULL, " ");
                }
                new_cmd.args[new_cmd.number_of_args] = NULL;
        }
        return new_cmd;
}

// Structure to create a node with data and the next pointer
struct Node {
    char* dir;
    struct Node *next;  
};


char* pwd_execution() {
        //gotta deal with error handling
        char buf[256];

        if (getcwd(buf, sizeof(buf)) == NULL)
		perror("getcwd() error");
	else {
                printf("%s\n", getcwd(buf, sizeof(buf)));
        }

}

void cd_execution(char* cmd, const char *filename) {	
	int ret = chdir(filename);

	if (ret) {
		fprintf(stderr, "Error: cannot cd into directory\n");
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
        }
        else {
                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
        }
}

int main(void) {
        char cmd[CMDLINE_MAX];
        struct Node* Top = NULL;
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
                else if (!strcmp(cmd, "pwd")) {
                        pwd_execution();
                } 

                else if (!strcmp(cmd, "popd")) {
                        printf("%s\n",Top->dir);
                        struct Node *tmp = Top;
                        char* next_dir = malloc(sizeof(char)*256);
                        strcpy(next_dir, Top->dir); //to store data of top node
                        Top = Top->next;
                        free(tmp->dir);
                        free(tmp); //deleting the node
        
                        
                        char cur_dir[256];
                        if (getcwd(cur_dir, sizeof(cur_dir)) == NULL)
                                perror("getcwd() error");
                        else {
                                getcwd(cur_dir, sizeof(cur_dir));
                        }
                        
                        
                        //comparing the two paths
                        bool common = 1;
                        int i = 0;
                        int greatest_common = 0;
                        int move_from_cur = 0;
                        printf("current: %s, to: %s\n", cur_dir, next_dir);
                        for (i;  i < strlen(cur_dir) || i < strlen(next_dir); i++) {
                                                                
                                if(common) {
                                        if(cur_dir[i] == next_dir[i]) {
                                                if(cur_dir[i] == '/') {
                                                        greatest_common = i;
                                                }
                                                if(i+1 == strlen(cur_dir) || i+1 == strlen(next_dir)) {
                                                        greatest_common = i;
                                                }
                                        }
                                        else {
                                                common = 0;
                                                continue;
                                        }
                                }
                                else {
                                        if (i < strlen(cur_dir)) { //get path to common
                                                if(move_from_cur == 0) move_from_cur = 1;
                                                if (cur_dir[i] == '/') move_from_cur++;
                                        }
                                }  
                        }

                        char *move_to_next = malloc(sizeof(char)*256);
                        printf("j: %i", greatest_common);
                        greatest_common++;
                        int k = 0;
                        for (int j = greatest_common; j < strlen(next_dir); j++) {
                                printf("j: %i, %i, curdir: %c\n", j, k, cur_dir[j]);
                                move_to_next[k] = cur_dir[j];
                                k++;
                        }  
                        for (int j = 0; j < move_from_cur; j++) {
                                chdir("..");
                                printf("\nmove from cur: %i\n", move_from_cur);
                        }
                        
                        printf("\nmovetonext: %s\n", move_to_next);
                        int ret = chdir(move_to_next);
                        if (ret) {
                                fprintf(stderr, "Error: directory stack empty\n");
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
                        }
                        else {
                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
                        }
                        printf("\ncurrent dir: ");
                        pwd_execution();
                }
                
                else if (!strcmp(cmd, "dirs")) {

                } 
                else {
                        // Check if we have a pipe character => "|"
                        char* has_multiple_commands = strchr(cmd, '|');
                        if (has_multiple_commands) { // We must have multiple commands.
                                if (cmd[0] != '|') { // Sanity check that first character is not "|".
                                        // Do we have too many pipes?
                                        if (!check_if_too_many_pipes(cmd)) {
                                                char *temp_cmd = calloc(strlen(cmd)+1, sizeof(char));
                                                strcpy(temp_cmd, cmd);
                                                char *cmd_arg = strtok(temp_cmd, "|");
                                                struct command_struct commands[COMMANDS_MAX]; // Array of commands parsed into structs.
                                                char *command_strings[COMMANDS_MAX];
                                                int number_of_commands = 0; // Number of commands.
                                                while (cmd_arg != NULL) { // Loop through the command splitting at the pipes to get each character.
                                                        // (ex: echo hello world | ls => ["echo hello world", "ls"]).
                                                        if (cmd_arg[0] == SPACE_CHAR) {cmd_arg++;}
                                                        command_strings[number_of_commands] = cmd_arg;
                                                        number_of_commands++;
                                                        cmd_arg = strtok(NULL, "|");
                                                }
                                                int cmd_s;
                                                bool invalid_command = false;
                                                for(cmd_s = 0; cmd_s < number_of_commands; cmd_s++) { // Loop through all the characters to parse them into a struct. 
                                                        struct command_struct cmd_to_run = parse_single_cmd(command_strings[cmd_s], cmd_s, number_of_commands-1);
                                                        commands[cmd_s] = cmd_to_run; // Fill into array.
                                                        bool can_run = sanity_check_cmd(cmd_to_run); // Check if command can run.
                                                        if (!can_run) {
                                                                invalid_command = true;
                                                        } else {
                                                                if (cmd[strlen(cmd)-1] == '|') { // Ensure we aren't missing a command.
                                                                        invalid_command = true;
                                                                        fprintf(stderr, "Error: missing command\n");
                                                                }
                                                        }
                                                }
                                                // Sanity check that ALL commands are valid.
                                                if (!invalid_command) {
                                                        int return_values[number_of_commands];
                                                        int fd_1[2], fd_2[2], fd_3[2];
                                                        pipe(fd_1); // Open first pipe.
                                                        pid_t pid_1;
                                                        pid_1 = fork();
                                                        if (pid_1 == 0) { // Child process.
                                                                close(fd_1[0]); // Close pipe 1 read.
                                                                dup2(fd_1[1], STDOUT_FILENO);
                                                                close(fd_1[1]); // Close pipe 1 write.
                                                                if (commands[0].has_input_file) { // Check input for file.
                                                                        // (this can only happen in first command for pipes)
                                                                        int fd_input;
                                                                        fd_input = open(commands[0].input_file, O_RDONLY); // Open file in READ_ONLY mode.
                                                                        dup2(fd_input, STDIN_FILENO); // Set STDIN to file.
                                                                        close(fd_input); // Close fd.
                                                                }
                                                                // Run command.
                                                                execvp(commands[0].program, commands[0].args);
                                                        }
                                                        pipe(fd_2); // Open second pipe.
                                                        pid_t pid_2;
                                                        pid_2 = fork();
                                                        if (pid_2 == 0) {
                                                                close(fd_1[1]);  // Close pipe 1 write.
                                                                dup2(fd_1[0], STDIN_FILENO); // Set STDIN.
                                                                close(fd_1[0]); // Close pipe 1 read.
                                                                if (number_of_commands > 2) {
                                                                        // If we have more than 2 commands, set up the next pipe.
                                                                        close(fd_2[0]); // Close pipe 2 read.
                                                                        dup2(fd_2[1], STDOUT_FILENO); // Set STDOUT.
                                                                        close(fd_2[1]); // Close pipe 2 write.
                                                                }
                                                                if(commands[1].has_output_file) { // Check output file.
                                                                        // If this command is last in pipe, we can output.
                                                                        if(commands[1].cmd_id == commands[1].total_cmds) {
                                                                                int fd_output;
                                                                                fd_output = open(commands[1].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open file, or create if doesn't exist.
                                                                                dup2(fd_output, STDOUT_FILENO); // Set STDOUT to file.
                                                                                close(fd_output); // Close fd.
                                                                        } 
                                                                }
                                                                // Run command.
                                                                execvp(commands[1].program, commands[1].args);
                                                        }
                                                        // Close first pipe as its no longer needed.
                                                        close(fd_1[1]);
                                                        close(fd_1[0]);
                                                        if (number_of_commands > 2) { // Run if we have more than 2 commands (so atleast 2 pipes).
                                                                pipe(fd_3); // Open third pipe.
                                                                pid_t pid_3;
                                                                pid_3 = fork();
                                                                if (pid_3 == 0) {
                                                                        close(fd_2[1]); // Close pipe 2 write.
                                                                        dup2(fd_2[0], STDIN_FILENO); // Set STDIN.
                                                                        close(fd_2[0]); // Close pipe 2 read.
                                                                        if (number_of_commands > 3) { 
                                                                                // If we have more than 3 commands, set up last pipe.
                                                                                close(fd_3[0]); // Close pipe 3 read.
                                                                                dup2(fd_3[1], STDOUT_FILENO); // Set STDOUT.
                                                                                close(fd_3[1]); // Close pipe 3 write.
                                                                        }
                                                                        if(commands[2].has_output_file) { // Check output file.
                                                                                // (refer to documentation in last command).
                                                                                if(commands[2].cmd_id == commands[2].total_cmds) {
                                                                                        int fd_output;
                                                                                        fd_output = open(commands[2].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                                                        dup2(fd_output, STDOUT_FILENO);
                                                                                        close(fd_output);
                                                                                } 
                                                                        }
                                                                        // Run command.
                                                                        execvp(commands[2].program, commands[2].args);
                                                                }
                                                                // Close second pipe as its no longer needed.
                                                                close(fd_2[1]);
                                                                close(fd_2[0]);
                                                                // If we have more than 3 commands, run next command (so we have max 3 pipes).
                                                                if (number_of_commands > 3) {
                                                                        pid_t pid_4;
                                                                        pid_4 = fork();
                                                                        if(pid_4 == 0) {
                                                                                close(fd_3[1]); // Close pipe 3 write.
                                                                                dup2(fd_3[0], STDIN_FILENO); // Set STDIN.
                                                                                close(fd_3[0]); // Close pipe 3 read.
                                                                                if(commands[3].has_output_file) { // Check output file.
                                                                                        if(commands[3].cmd_id == commands[3].total_cmds) {
                                                                                                int fd_output;
                                                                                                fd_output = open(commands[3].output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                                                                dup2(fd_output, STDOUT_FILENO);
                                                                                                close(fd_output);
                                                                                        } 
                                                                                }
                                                                                // Run command.
                                                                                execvp(commands[3].program, commands[3].args);  
                                                                        }
                                                                        // Close last pipe as its no longer needed.
                                                                        close(fd_3[1]);
                                                                        close(fd_3[0]);
                                                                } else {
                                                                        // Close last pipe if we don't have a fourth command.
                                                                        close(fd_3[1]);
                                                                        close(fd_3[0]); 
                                                                }
                                                        } else {
                                                                // Close second pipe if we don't have a third command.
                                                                close(fd_2[1]);
                                                                close(fd_2[0]);
                                                        }
                                                        int process;
                                                        for (process = 0; process < number_of_commands; process++) { // Loop through all of the proccesses (#commands).
                                                                int return_value;
                                                                wait(&return_value); // Wait for each process to return a status.
                                                                return_values[process] = return_value; // Set status into array.
                                                        }
                                                        // Use array above to print out correct terminal end lines.
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
                                // Create a command, since this is a single command the id is 0 and total is 0.
                                struct command_struct cmd_to_run = parse_single_cmd(cmd, 0, 0); 
                                bool can_run = sanity_check_cmd(cmd_to_run);
                                if (can_run) {
                                        if (!strcmp(cmd_to_run.program, "cd")) { 
						cd_execution(cmd_to_run.program, cmd_to_run.args[1]);
                                                continue;
                                        }
                                        else if (!strcmp(cmd_to_run.program, "pushd")) {
                                                int ret = chdir(cmd_to_run.args[1]);
                                                if (ret) {
                                                        fprintf(stderr, "Error: cannot push directory\n");
                                                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
                                                }
                                                else {
                                                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(ret));
                                                }

                                                struct Node *nodes = (struct Node*)malloc(sizeof(struct Node));

                                                char buf[256];
                                                if (getcwd(buf, sizeof(buf)) == NULL)
                                                        perror("getcwd() error");
                                                else { getcwd(buf, sizeof(buf)); }

                                                nodes->dir = malloc(sizeof(char)*256);
                                                strcpy(nodes->dir, buf);

                                                if (Top == NULL) {
                                                        nodes->next = NULL;
                                                        //printf("top");

                                                }
                                                else {
                                                        //printf("not top");
                                                        nodes->next = Top;
                                                }
                                                Top = nodes;
                                                //printf("%s\n", nodes->dir);
                                                
                                                
                                        }
                                        pid_t pid;
                                        pid = fork();
                                        if (pid > 0) { // Parent process.
                                                int return_value;
                                                // Wait for exit status.
                                                waitpid(pid, &return_value, 0);
                                                fprintf(stderr, "+ completed '%s' [%d]\n", cmd, WEXITSTATUS(return_value));
                                        } else if (pid == 0) { // Child process.
                                                if (cmd_to_run.has_output_file) {
                                                        // If we have an output file, redirect STDOUT.
                                                        int fd_output;
                                                        fd_output = open(cmd_to_run.output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                                                        dup2(fd_output, STDOUT_FILENO);
                                                        close(fd_output); // Close.
                                                } else { 
                                                        if (cmd_to_run.has_input_file) {
                                                                // If we have an input file, redirect STDIN.
                                                                int fd_input;
                                                                fd_input = open(cmd_to_run.input_file, O_RDONLY);
                                                                dup2(fd_input, STDIN_FILENO);
                                                                close(fd_input); // Close.
                                                        }
                                                }
                                                // Run command.
                                                execvp(cmd_to_run.program, cmd_to_run.args);
                                                int error_code = errno; // Gather error if it exists;
                                                switch (error_code) {
                                                        case 2:
                                                                // Command was not valid (ex: windows98)
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
