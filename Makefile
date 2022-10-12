DEV=-Wall -Wextra -Werror -g

all: sshell

sshell: sshell.c
	gcc sshell.c -o sshell

clean: sshell
	rm -f sshell