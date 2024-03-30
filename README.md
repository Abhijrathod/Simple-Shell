# simple shell (ISU CS352 - Operating Systems, 2020)
A C program that implements a simple shell/command-line interpreter. It allows users to execute commands, handle background and foreground processes, handle input/output redirection, and pipe commands together. 



## Compile and Run
compile: 
```
gcc -o simple_shell PA1.c
```
run: 
```
./simple_shell
```

## Supported Commands

### basic commands 
`ls`: List files and directories in the current directory.

`pwd`: Print the current working directory.

`echo Hello, World!`: Print the text "Hello, World!".

`date`: Display the current date and time.


### redirection
`ls > files.txt`: Redirect the output of the ls command to a file named files.txt.

`cat < files.txt`: Redirect the contents of the files.txt file as input to the cat command.


### piping
`ls | grep txt`: List files and pipe the output to grep to search for "txt".

`ls | sort`: List files and pipe the output to sort to sort the file names.


### background execution
`sleep 5 &`: Execute the sleep command in the background for 5 seconds.

`ls -R / &`: List files and directories in the root directory in the background.


### job management
`sleep 10`: Execute the sleep command for 10 seconds in the foreground. Press Ctrl+Z to stop it and put it in the background.

`jobs`: Display the list of background jobs.

`bg 0`: Resume the job at index 0 in the background.


### exiting the shell
`exit`: Exit the shell.

## How it works

#### `main()`
The main loop of the shell continuously prompts the user for input, tokenizes the input into a Cmd struct, and then performs various actions based on the input. It supports exiting the shell, displaying job information, resuming stopped jobs, and executing commands in the foreground or background.

#### Global Variables
Defines global variables and arrays to manage jobs, the current foreground job, the foreground process PID, and the number of jobs.


#### Memory Management
The program dynamically allocates memory for the Cmd structs representing jobs. It also deallocates this memory at the end of the program.


#### Struct Definition - Cmd
Cmd to holds information about a command, including the command line, tokenized command, arguments, symbols, process ID, and status flags (done, printed, stopped). It is used to represent each command to be executed.

#### Signal Handler 
sigtstpHandler handles the SIGTSTP signal (usually triggered by Ctrl+Z), which suspends the currently running foreground process and adds it to the list of jobs in a stopped state.

### `parseCmd()`
Tokenizes a command line input into individual arguments and symbols, storing them in the Cmd struct for later use.

#### `findSymbol()`
Searches for the index of a specific symbol within the Cmd struct's symbols array.

#### `executeCommand()`
Handles the execution of a command. It checks for input/output redirection and piping, and if neither is found, it executes the command using the execvp function.

#### `backgroundCmd()`
Executes a command in the background by forking a child process and adding it to the list of jobs. The parent process displays the job's information.

#### `foreGroundCmd()`
Executes a command in the foreground by forking a child process. The parent process waits for the child process to finish before displaying the prompt again.

#### `processCheck()`
Checks the status of background jobs. It iterates through the list of jobs, checks if any have terminated, and displays relevant information about them.
# Simple-Shell
#
