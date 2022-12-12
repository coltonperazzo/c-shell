# SSHELL : Simple Shell

## Introduction

The goal of this program "sshell" (or simple shell) is to understand the
importance of UNIX syscalls in an operating system by creating our own
simple shell using the C language. This program accepts an inputted
command from the user and executes said command with any given arguments
that are provided. 

As like any other UNIX-like operating system, this simple shell offers a 
wide range of syscalls that other popular terminals such as bash and zsh offer.
In addition provides the functionability to pipe up to **four commands**
through *three pipe signs* and supports input and output redirection.

## Implementation

The implementation of this simple shell follows a few basic steps:
1. Gathering information as to what command is being executed.
2. Parsing the inputted command from the user into a struct with it's 
arguments, input/output file, and other useful variables.
3. Figuring out if we are piping with multiple commands or running a
single command.
4. Sanity checking the provided command(s) to ensure they are free of
any parsing or library errors.
5. Using a piping system consisted of either one, two, or three pipes
to connect up to four commands together and using the dup2() function
to redirect STDIN and STDOUT.
6. Using a fork()+exec()+wait() method to execute the commands by using
execvp() to automatically search through the $PATH variable if
provided a syscall.
7. Gathering the exit statues of the executed command to relay back
to the user terminal.

## Gather Information

First and foremost, we gather just exactly what command is being ran by the
user. At the beginning of the program; and after we are able to grab the
full inputted command (ex: echo hello world), `strcmp` compares the inputted
command to the various built-in commands (i.e., exit, pwd, popd, dirs). If our 
program realizes that one of these built-in commands is ran, it'll execute
the code connected to it, for example if we run the command "exit", it'll
simply print `fprintf(stderr, "Bye...\n");` and break us out of the shell.

If the inserted input is not one of these first few built-in commands, we then
consider the two following test cases:
1. We are piping with multiple commands.
2. We are using the other built-in commands (i.e., cd, pushd).
3. We are running a single command with no piping.

## Parsing Inputted Command

Secondly, once information is gathered on what command we are running, 
`parse_single_cmd()` parses through a single command (ex: date -u) and builds
a "parsed" data structure called `struct command_struct`. This is 
intended so we don't to parse the inputted (original) command line again.
This parsed data structure contains all the necessary information for
the program to run each of the provided commands. 

It includes pointers to the actual provided command,program name 
(ex: date) and an array of args called `args[]` that contain the
provided args for the program (ex: -u). `strtok()` parses through each space
in the provided command (ex: date -u) and adds the char to the `args[]` array
if its able to. In this case `-u` would be added into the `args[]` array at
`index[1]` since `index[0]` is reserved for the program name. It should
be noted that the *last index* of the `args[]` array must be `NULL`.

## Deciding Piping or Single Command

Thirdly, we need to decide if we are piping multiple commands together
or if we are running a single command, `strchr()` parses through the 
inputted command for a `|` character. If this is found, the program assumes
we are piping. Otherwise we are running a syscall or one of the other
built-in functions (i.e., cd, pushd).

## Sanity Checking

Once our data structure is built with all the neccessary information, 
`sanity_check_cmd()` can now perform various functions to ensure that
the provided command can actually run.

Function `check_if_invalid_command()` goes through the first character of
the program name to ensure it is not an invalid character. It also ensures
that the last character of the full inputted command is not a pipe character. 
In these circumstances, it would result in an invalid command.

Function `check_if_missing_inputoutput_file()` defines whether or not an
actual file was provided after the `<` or `>` characters. With the help of
some simple code such as:
`cmd_struct.full_cmd[strlen(cmd_struct.full_cmd)-1] == '>'`
where cmd_struct.full_cmd is the full inputted command, we can get the last
character of this inputted command to ensure that there atleast exists
something beyond in this point (the output file). The same process occurs
for the input files.

Function `check_if_piping_inputpout_is_mislocated()` ensures that we are using
the input and output redirection at the correct spots. If we are currently 
use a pipe, we can only output at the last command and input at the first
command. Using the parsed data structure which contains two ints, `cmd_id` and
`total_cmds`, `check_if_piping_inputpout_is_mislocated()` compares these two
integers to ensure that the command is located in the correct spot to run
a file direction in the fully inputted command.

Function `check_if_inputouput_file_is_null()` performs a typical check of the
provided `output_file` or `input_file` character pointer from the parsed
data structure. It `access()`'es each of the files to ensure it can be
correctly accessed by the terminal.

Function `check_if_too_many_args()` analyzes the `args[]` array to ensure that
the size of it doesn't pass the defined `ARGS_MAX` constant.

# Piping

The `|` character denotes a pipe in our inputted command. Piping is 
possible through a lengthy process of closing file descriptor with `close()`
and copying file descriptors with `dup2()`. We will have `n-1` pipes to create
where `n` is equal to the number of commands. It should be noted that our
program does not create a pipe until the pipe is needed. This is purposely
done to ensure the closure of the file descriptors does not get out of hand.

In the case we are piping two commands together (ex: echo hello | wc -l), we
need to create a single pipe and one `fd` array (let's say `fd_1[2]`).
Each pipe is given its own fork with a child process (the parent is not 
necessary here). In the child process for the first command our program
does not need to read from the pipe, therefore `close(fd_1[0])` will
close the pipe 1 read. Our programs goal is to write into this pipe which
must be achieved through `dup2()` as mentioned above. In order to write 
into the pipe `STDOUT_FILENO` must be redirected into `fd_1[1]`. This is
possible by `dup2(fd_1[1], STDOUT_FILENO)`. After completion of this 
function the pipe 1 write is no longer needed and can be closed with 
`close(fd_1[1])`. And with that- we have written the output of the
first command into the first pipe.

The process for reading from a pipe is similar, instead of closing the
read pipe at the start, we close the write pipe. And instead of redirecting
`STDOUT_FILENO` our program redirects `STDIN_FILENO` instead. If we have a case
where we are piping with three or more commands, our program opens up another 
pipe and repeats a similar process to the first command where the read pipe is
closed first, the `STDOUT_FILENO` is redirected and then the write pipe is 
closed.

It's important to consider closing our file descriptors that are associated
with the pipes. Without doing so our program may exhibit behavior where the
pipelines hang and do not fully complete. After a pipe is finished being used
our program closes both the read and write file descriptors.

# Fork()+Exec()+Wait()

When running a single command a `fork()+exec()+wait()` approach is used. Our
program first creates a fork with a child and parent process. Our child process
will handle running the command while the parent waits for the child process
to exit so we can gather the exit status. As our simple shell supports input
and output redirection, our program must take these into account. `open()` 
allows our program to access files in `O_WRONLY` or `O_RDONLY` mode. If we are
outputting to a file, we must be able to write to it. If we are inputting from
a file we must be able to read from it. In order to correctly input and output
from files `dup2()` is used to copy file descriptors into the correct
`STDIN` and `STDOUT` fd's. If given `fd = open(file_name, O_WRONLY, 0644)`, 
the following function `dup2(fd, STDOUT_FILENO)` would copy new file 
descriptor `STDOUT_FILENO` to the old file descriptor `fd`, essentially
writing the output into the file. The process is the same for getting the
input, however we set the new file descriptor to `STDIN_FILENO`.

After the above steps, `execvp()` automatically searches programs in `$PATH`
with the parsed data structue program name and args. An example of this call
would be: `execvp(cmd_to_run.program, cmd_to_run.args)`, where program can
refer to date, and args can refer to [-u]. After `execvp()` is ran, our
child process exits with whatever execvp returns. In our parent process we
*wait* until we can gather the exit status.

# Gathering Exit Status

Exit status is obtained through waiting for the child process to exit. If
performing a single command this is obtained through `waitpid()` inside
the parent process. If provideda pid (where pid = fork()), it will 
suspened the calling process until the child process ends. `waitpid()` 
provides a full *raw* status which can be piped into `WEXITSTATUS()` 
to achieve the exit status from the process completion. If piping is
occuring, the process of obtained the exit status is completed by a 
*for-loop* function that iterates over the number of commands waiting
for each to terminate with the `wait()` function. Each given raw status
is fed into an `arr[]` which is then used to relay back to the terminal
user after all the processes in the pipe have been executed.

## License
This work is distributed under the [GNU All-Permissive
License](https://spdx.org/licenses/FSFAP.html).

Copyright 2022, Colton Perazzo and András Necz 
