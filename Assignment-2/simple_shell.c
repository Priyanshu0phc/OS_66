#include "header.h"
//Above header file have all imported libraries, as like we did in previous assingment task..



typedef struct CommandHistory { //Structure to store Command history, will store timings, command, and pid for the
//child process that will execute this command..
    char command[MAX_COMMAND_LENGTH]; //As we have to mention a size at compile time so we are using 1024 bytes only..
    pid_t pid; //A structure predifined, to store pid of the process that will execute this command..
    struct timeval start_time; //A structure defined to store start-time of process..
    struct timeval end_time; // structure used to store end time of process..
	//We calculate total running time by subtracting end time to start time.
} CommandHistory;	//short hand notation used.

CommandHistory history[MAX_HISTORY];	//Global array for storing history Objects of CommandHistory type.
int history_count = 0;	//Takes count of number of commands executed till now.


//We declare all the function signature beforehand to avoid any kind of precedence errors.


char* read_user_input();
int create_process_and_run(char* command, int background);
int launch(char* command);
void shell_loop();
void parse_command(char* input, char** args);
int has_pipe(char* input);
int execute_pipe_command(char* input);
void show_history();
void show_execution_details();



int execute_pipe_command(char* input) {
    char* commands[MAX_ARGS];  // Array to hold the individual commands
    char* args[MAX_ARGS];      // Array to hold arguments for each command
    int pipe_fd[2], prev_pipe_fd[2];  // Pipe file descriptors
    int num_commands = 0;
    pid_t pid;
    int status;
    gettimeofday(&history[history_count].start_time, NULL);
    char original_input[MAX_COMMAND_LENGTH]; // Store original command input
    strcpy(original_input, input); // Save the full piped command

    // Tokenize the input based on the pipe symbol '|' and split into command
    char* token=strtok(input, "|");
    while(token!= NULL){
        commands[num_commands++]=token;
        token=strtok(NULL,"|");
    }

    // Iterate over all commands
    for (int i=0;i<num_commands;i++){
        // Create a pipe for each command except the last one
        if (i<num_commands-1 && pipe(pipe_fd)==-1){
            perror("Piping failed");
            return 0; //this will make loop to terminate
        }

        // Fork the process for each command
        pid=fork();
        if(pid==0){  // Child process
            // Parse the command and its arguments
            parse_command(commands[i],args);
            // If this is not the first command, set up the input from the previous pipe
            if (i>0){
                dup2(prev_pipe_fd[0], STDIN_FILENO);
                close(prev_pipe_fd[0]);
                close(prev_pipe_fd[1]);
            }

            // If this is not the last command, set up the output to the current pipe
            if (i<num_commands-1) {
                close(pipe_fd[0]);  // Close the reading end of the current pipe
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
            }

            // Execute the current command
            if (execvp(args[0],args)==-1) {
                perror("execvp failed");
                exit(EXIT_FAILURE);  // Exit the child if execvp fails
            }
        } else if(pid<0){  // Forking failed so we exit out main loop
            perror("Fork failed");
            return 0;
        }

        // Parent process:
        // Close the previous pipe
        if(i> 0) {
            close(prev_pipe_fd[0]);
            close(prev_pipe_fd[1]);
        }

        // Save the current pipe as the previous pipe for the next iteration
        if (i <num_commands- 1) {
            prev_pipe_fd[0]=pipe_fd[0];
            prev_pipe_fd[1]=pipe_fd[1];
        }
    }

    // Wait for the last command to finish and return its exit status
    waitpid(pid,&status,0);

    // This has been explained in code two times already in code below..
    if (history_count<MAX_HISTORY) {
        strcpy(history[history_count].command, original_input);
        history[history_count].pid = pid;
        gettimeofday(&history[history_count].end_time, NULL);
        history_count++;
    }

    return 1;
}
void shell_loop(){
// Main Loop Responsible for continuos prompts..
    char* command; //A char sequence pointer, to store command entered by user.
    int status; // To store execution status, or to potentially terminate looping.

    do{
        printf("SimpleShell> "); //Display Prompt..
        command = read_user_input();	//Take user input as command and point commamd referance to that user input..
	//printf("%s",command);
	//printf("\n");
	if(command==NULL){
	//Reading not successful So terminate..
	perror("Reading Not Successful !");
	exit(1);
	}
        if (strcmp(command, "history") == 0) { 	//if user typed history..
            show_history();  // Displays the history of executed commands
        }
	else if(strcmp(command,"exit")==0) { //if exit command is typed, follow prolouge defined..
            // Store the "exit" command in history
            if (history_count<MAX_HISTORY){	//Space available for CommandHistory Object to store..
                strcpy(history[history_count].command, "exit"); //Latest CommandHistory Object from Global array which 
		//currently have random Values, is getting Values to be stored..
                history[history_count].pid = getpid();// Store pid, this time no child process as process is going to terminate..
		history_count++; //Last time counter update..
            }

	//If Alredy 100(space) for Previous command is completed, don't store now, just print those first 100 Command details..
	    gettimeofday(&history[history_count].start_time, NULL); //Storing Current time into Object start-time..
	    gettimeofday(&history[history_count].end_time, NULL);//Storing current time into Object end-time,as pocess going to be terminated as expected
            show_execution_details();  // Shows execution details and exits the loop..
            status = 0; //To break looping
        } else if(has_pipe(command)) { //If There is pipe used in Command..
            status=execute_pipe_command(command);  // Handles commands with pipes
        } else{ //If no pipe is used and not either history or exit command, follow normal execution..
            status=launch(command);  // Launches the command entered by the user
        }

        free(command);  // Free the dynamically allocated memory for the command
    } while(status);  // Continue the loop until status==0..
}


char* read_user_input(){
	//called on every iteration of shell loop, to have user input in a null terminated string(Char Sequence here in C)...
    char* input=malloc(MAX_COMMAND_LENGTH * sizeof(char));	//Allocate Maximum size(1024) as stated in header.h file, as we should know size at compile time 
	//that's why we have to do so..
    if (!input){	//if malloc fails, it return null which can be detected here..
        perror("Unable to allocate buffer");	//Error Message to be shown..
        exit(1);	//Terminate whole shell program here not just loop.
    }
	//If Dynamic space allocation is successful.
    if (fgets(input,MAX_COMMAND_LENGTH,stdin)!=NULL) { //Read upto 1024 bytes from stdin file(user input), and store it to space pointed by input referance variable.
	//fget returns null on failue of reading operation.
	//fget reads till \n or \0 character are not found or till all MAX_COMMAND_LENGTH-1 characters are not read, at the last char it adds '\0' to mention end..
        int length=strlen(input); //Captures the length of input string..
        if (input[length - 1]=='\n') { // if last character is  '\n' replace it with '\0' character..
            input[length - 1]='\0'; // Replacement..
        }
        else
	{
	perror("Command Length Exceeded than 1024 characters");  //If last character is not new line character, then generate error(as whole command is not stored)..
	exit(1); //Exit with error code: 1.
	}
	return input; //Else return input string successfully..
    }

    else return NULL; //if reading op. not successful, return null.
}

int create_process_and_run(char* command,int background){
    pid_t pid; //Pid holder for child proccess which will execute code..
    int status; //to store status.
    struct timeval start_time,end_time; //will be used to calculate time for which process will run.
    char* args[MAX_ARGS]; //placeholders arguments
    parse_command(command,args);  // Directly parse the command into arguments and stored into args array.
    gettimeofday(&start_time,NULL);  // Record the start time, in start_time referance variable.
    pid = fork();  // Create a new process
    if (pid==0){  // Child process
        // Directly execute the command with execvp(), as done in slides.
        if(execvp(args[0],args)==-1) { //use of exec function, to replace the program image(address space) to make it execute new program.
            perror("execvp failed");
	    return 0;
        }
    }
    else if(pid<0) { //fork failed will return negative.
        perror("fork failed");
        return 0;
    }
	else{  // Parent process
        if (!background) {	//if background flag is zero than don't move it to back and make parent process to wait till child process terminates.
            waitpid(pid,&status,0);  // Wait for the child process to finish if not running in background
        } else{
            printf("[Running in background] PID: %d\n", pid);
	//Here background flag is one, move this to background and take prompt again.
        }
        gettimeofday(&end_time,NULL);  // Record the end time
	// If Space available for command history to be stored, store the command and if full ignore this storing
        if (history_count<MAX_HISTORY) {
            strcpy(history[history_count].command, command);  // Save the command in history
            history[history_count].pid = pid;
            history[history_count].start_time = start_time;
            history[history_count].end_time = end_time;
            history_count++; //Increaing History Count..
        }
	//History prolouge above..
    }
    return 1;
}
int launch(char* command) {
    int background = 0;	  //flag to wheather, command is & terminated, should be moved to background or not. CHECK-POINT

    int length = strlen(command); //length of command captured.
    if (command[length-1]=='&') {	//if & is there move process to background.
	background = 1;//enable flag.
        command[length-1]='\0'; //remove that & char, and terminate it.
    }
	//Passes background flag to main execution function...
    return create_process_and_run(command,background); //call execution function.
}


void parse_command(char* input, char** args) {
	//Tokenizes all the string into arguments, at the args array which can we used.
    char* token = strtok(input," ");
    int i = 0;
    while (token!=NULL){
        args[i++]=token;//Store token to array.
        token = strtok(NULL," "); //Extract token from input string
    }
    args[i]=NULL; // Last argument in array is NULL, to specify termination.
}


int has_pipe(char* input) {

	// Checking if input string has that '|' char..
    return strchr(input,'|')!=NULL;}
void show_history() {
	//To be called when history command is given..
	//Iterate over global history array that stores, all the previous exected commands..
    for (int i = 0; i < history_count; i++) {  	// No. of Commands executed till Now->history_count
        printf("%d: %s\n", i + 1, history[i].command);	//Display command attribute from CommandHistory Object..
    }
}

void show_execution_details() {
    printf("\nExecution Details of Commands:\n");
    for (int i = 0; i < history_count; i++) {
        struct timeval start_time = history[i].start_time;
        struct timeval end_time = history[i].end_time;

        double duration = (end_time.tv_sec - start_time.tv_sec) +
                          (end_time.tv_usec - start_time.tv_usec) / 1e6;
        printf("Command: %s\n", history[i].command);
        printf("PID: %d\n", history[i].pid);
        printf("Start Time: %ld.%06ld\n", start_time.tv_sec, start_time.tv_usec);
        printf("End Time: %ld.%06ld\n", end_time.tv_sec, end_time.tv_usec);
        printf("Duration: %.6f seconds\n\n", duration);
    }
}



int main(int argc, char **argv) {
//Main Execution Starts from here..
// We call Shell Loop as stated in lecture slides by sir..
    shell_loop();
    return EXIT_SUCCESS;
}


