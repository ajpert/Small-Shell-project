#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <limits.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <err.h>
#include "token-parser.h"
#include <signal.h>

bool background;
bool foreground_mode = false;

void expand_string(char** str) { //expand if $$
    char strmaker_1[2048];
    char strmaker_2[2048];
    char full_string[2048];
    int pid_num = getpid();
    while(strstr(*str,"$$") != NULL) { //while there still instances of $$ in the string, replace
        strncpy(strmaker_1,*str,strstr(*str,"$$")-(*str));
        strcpy(strmaker_2,strstr(*str,"$$") + 2);
        sprintf(full_string,"%s%i%s",strmaker_1,pid_num,strmaker_2);
        fflush(stdout);
        free(*str);
        (*str) = calloc(strlen(full_string) + 1, sizeof(char));
        strcpy(*str,full_string);
        memset(full_string,0,strlen(full_string));
        memset(strmaker_2,0,strlen(strmaker_2));
        memset(strmaker_1,0,strlen(strmaker_1));
    }
}

void current_status(int status) {
    //most of this was taken from
    //https://canvas.oregonstate.edu/courses/1884949/pages/exploration-process-api-monitoring-child-processes?module_item_id=21839532
    if(WIFEXITED(status)) { // if exited normally
        printf("exit value %i\n",WEXITSTATUS(status));
        fflush(stdout);
    }
    else { // if terminated
        printf("terminated by signal %i\n", WTERMSIG(status));
        fflush(stdout);
    }
    fflush(stdout);
}


void file_helper(char *in_file, char *out_file, bool background) {
    //most of this code is adapted from 
    //https://canvas.oregonstate.edu/courses/1884949/pages/exploration-processes-and-i-slash-o?module_item_id=21839541
    int file_check;
    if (in_file != NULL) { //if there is an infile
        file_check = open(in_file, O_RDONLY);
        if (file_check == -1) {
            fprintf(stderr, "cannot open %s for input\n",in_file);
            fflush(stdout);
            exit(1);
        }
        else if (dup2(file_check, STDIN_FILENO) == -1) {
            fprintf(stderr, "cannot redirect %s for input\n",in_file);
            fflush(stdout);
            exit(1);
        }
        close(file_check);
    }
    else if(in_file == NULL && background) { //if there is no infile and it is running in the background
        file_check = open("/dev/null", O_RDONLY);
        dup2(file_check,STDIN_FILENO);
        close(file_check);
    }
    if (out_file != NULL) { //if there is an outfile
        file_check = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (file_check == -1) {
            fprintf(stderr, "cannot open/create %s for output\n",out_file);
            fflush(stdout);
            exit(1);
        }
        else if (dup2(file_check, STDOUT_FILENO) == -1) { //if there is no outfile and it is running in the background
            fprintf(stderr, "cannot redirect %s for output\n",out_file);
            fflush(stdout);
            exit(1);
        }
        close(file_check);
    }
    else if(out_file == NULL && background) {
        file_check = open("/dev/null", O_WRONLY);
        dup2(file_check,STDOUT_FILENO);
        close(file_check);
    }

}
void change_directorys(char **args, char* home_directory, int argument_counter)
{
    //adapted from shell.c from Lecture 7 examples
    if (argument_counter == 1) { //if cd is the only thing in the command line
        chdir(home_directory);
    }
    else {
        if (chdir(args[1]) == -1) { //change to specified directory, or tell user if it does not exist
            printf("the directory %s does not exist\n", args[1]);
            fflush(stdout);
        }
    }
    fflush(stdout);
}
void cleanup(char** args, char** in_file, char** out_file, int argument_counter) {
    int i;
    for(i = 0; i < argument_counter; i++) {
        free(args[i]);
    }
    free(*out_file);
    free(*in_file);
    *out_file = NULL;
    *in_file = NULL;
}
void fork_child() {

}
void options() {

}
void handle_sigstp(int signo) {
    char* message1 = "\nEntering foreground-only mode (& is now ignored)\n:";
    char* message2 = "\nExiting foreground-only mode\n:";
    //if in foreground mode, switch off, if not, switch on and let user know
    if(!foreground_mode) {
        write(STDOUT_FILENO, message1, 52);
        foreground_mode = true;
    }
    else {
        write(STDOUT_FILENO, message2, 32);
        foreground_mode = false;
    }
    fflush(stdout);
}

int main() {
    background = false;
    struct sigaction SIGSTP_action = {0};
    struct sigaction SIGINT_action = {0};
    char* home_directory = getenv("HOME"); //used for cd
    char **toks = NULL;
    size_t toks_size;
    size_t num_toks;
    char* in_file = NULL;
    char* out_file = NULL;  
    int status = 0;
    size_t i;
    char* args[512]; //num arguments max
    pid_t processes[1000]; //used to store and clean up processes in exit
    int num_processes = 0;
    int argument_counter = 0;
    pid_t pid;
    pid_t back_pid;

    
    
   
    //cntrl signals adapted from modules on canvas
    //https://canvas.oregonstate.edu/courses/1884949/pages/exploration-signal-handling-api?module_item_id=21839540
    //cntrl z handler
    /*SIGSTP_action.sa_handler = handle_sigstp;
    sigfillset(&SIGSTP_action.sa_mask);
    SIGSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGSTP_action, NULL);*/
    signal(SIGTSTP, &handle_sigstp);

    //cntrl c stopper 
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    while(1) {
        
        //get Input
        printf(": ");
        fflush(stdout); 
        num_toks = readTokens(&toks,&toks_size);//get inputs and store to toks
        fflush(stdout);

        if(num_toks == 0 || toks[0][0] == '#') { 
            //if blank line or comment do nothing
        }
        else { 
            //parse the tokens
            fflush(stdout);
            for (i = 0; i < num_toks; i++) { // go through all tokens
                if (strcmp(toks[i], "<") == 0) { // set infile
                    (in_file) = calloc(strlen(toks[++i]) + 1, sizeof(char));
                    strcpy(in_file, toks[i]);
                }
                else if (strcmp(toks[i], ">") == 0) { // set outfile
                    (out_file) = calloc(strlen(toks[++i]) + 1, sizeof(char));
                    strcpy(out_file, toks[i]);
                }
                else { // set arguments
                    args[argument_counter] = calloc(strlen(toks[i]) +1, sizeof(char));
                    strcpy(args[argument_counter++], toks[i]);
                    if (strstr(toks[i], "$$") != NULL) { // expand if needed
                        expand_string(&args[argument_counter - 1]);
                    }
                }
            }
            if (strcmp(toks[num_toks - 1], "&") == 0) { // if last token is &
                if (foreground_mode == false) { // if it is not in foreground mode, toggle background
                    background = true;
                }

                argument_counter--;
            }
            args[argument_counter] = NULL; // for execvp later

            //running background
            if(background) {
                background = true;
                fflush(stdout);
            }
            //exit
            if(strcmp(args[0], "exit") == 0) {
                status = 0;;
                for(i = 0; i < num_processes; i++) { //kill all lingering processes and print
                    kill(processes[i], SIGTERM);
                    wait(&status);
                    if (WIFSIGNALED(status)){
                        if (WTERMSIG(status) == SIGTERM) {
                            printf("background pid %i is done: ", processes[i]);
                            fflush(stdout);
                            current_status(status);
                        }
                    }
                }
                for (i = 0; i < toks_size; ++i)
                {
                    free(toks[i]);
                }
                free(toks);
                exit(0);
            }
            //change directorys
            else if(strcmp(args[0], "cd") == 0) {
                change_directorys(args,home_directory,argument_counter);
            }
            //status
            else if(strcmp(args[0], "status") == 0) {
                current_status(status);
            }
           

            else { //fork and exec, modified from shell.c

                pid = fork();
                

                switch(pid) {
                    case -1: //oopsies fork go bad
                        fprintf(stderr,"oops, looks like we dropped a fork");
                        break;
                    case 0 : //child process activity

                        //ignore sigstp
                        signal(SIGTSTP,SIG_IGN);

                        //if background, ignore cntrl c
                        if(background == false) {
                            SIGINT_action.sa_handler = SIG_DFL;
                            sigfillset(&SIGINT_action.sa_mask);
                            SIGINT_action.sa_flags = 0;
                            sigaction(SIGINT, &SIGINT_action, NULL);
                        }
                        
                        file_helper(in_file,out_file,background);

                        if(execvp(args[0], args)) { //if it cant execute, send an error and exit
                            fprintf(stderr,"%s: no such file or directory\n", args[0]);
                            fflush(stdout);
                            exit(1);
                        }
                        break;
                    default: //parental unit decends 
                        //adapted from, https://canvas.oregonstate.edu/courses/1884949/pages/exploration-process-api-monitoring-child-processes?module_item_id=21839532
                        if(!background) { //if not background, wait for the child to finish
                            waitpid(pid,&status,0);
                            if(WIFSIGNALED(status)) {
                                printf("terminated by signal: %i\n", WTERMSIG(status));
                                fflush(stdout);
                            }
                        }
                        else { //if background, tell user the background pid
                            processes[num_processes++] = pid; //save the process num
                            waitpid(pid,&status,WNOHANG); //need this for kill -15 to print right after for some reason
                            printf("background pid is %i\n", pid);
                            fflush(stdout);
                        }    
                        break;   
                }
                
                
            }
            
            

        }
            //from man page
            //while waitpit is returning process that ended, print those out
            //pid will get an actual pid if a processes ended in the background
            //those that die get stored as a "zombie" pid, and this while loop picks them up
            int end_process_num;
            while((pid = waitpid(-1,&status,WNOHANG)) > 0) {
                    for(i = 0; i < num_processes; i++) { //remove from list of processes
                        if(processes[i] == pid) {
                            end_process_num = i;
                        }
                    }
                    for(i = end_process_num; i < (num_processes - i); i++) {
                        processes[i] = processes[i + 1];
                    }
                    num_processes--;

                    printf("background pid %i is done: ", pid);
                    fflush(stdout);
                    current_status(status);
            }  



            //cleanup
            free(out_file);
            free(in_file);
            out_file = NULL;
            in_file = NULL;
            argument_counter = 0;
            background = false;
            fflush(stdout);
    }
        

}



