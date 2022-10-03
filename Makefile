DEV=-Wall -Wextra -Werror

all: sshell

sshell: sshell.c
	gcc sshell.c -o sshell 