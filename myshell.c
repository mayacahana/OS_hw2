//
// Created by mayacahana
//
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
//This function get an arglist a list of char* arguments provided by the user
//
int process_arglist(int count, char** arglist);
int prepare(void);
int finalize(void);
int get_pipe_index(char** arglist, int count);
void execv_child_proc(char** arglist, int* pipe_fd, int std);

void execv_child_proc(char** arglist, int* pipe_fd, int std){
    if (pipe_fd != NULL){
        if (std) {
            close(pipe_fd[1]);
            if (dup2(pipe_fd[0],0) == -1){
                perror("Error in dup2: ");
                exit(1);
            }
            close(pipe_fd[0]);
        } else {
            close(pipe_fd[0]);
            if (dup2(pipe_fd[1],1) == -1){
                perror("Error in dup2: ");
                exit(1);
            }
            close(pipe_fd[1]);
        }
    }
    //execvp
    if (execvp(arglist[0], arglist) == -1){
        fprintf(stderr, "execvp failed. %s\n", strerror(errno));
        exit(1);
    }
    
}

int process_arglist(int count, char** arglist){
    if (count <= 0){
        fprintf(stderr, "Count is less than 0. \n");
        return 1;
    }
    pid_t child_pid;
    bool is_pipe;
    int pipe_index;
    struct sigaction siga;
    memset(&siga, 0, sizeof(siga));
    siga.sa_handler = SIG_DFL;
    // checking if the last arg of the arglist is the & terminator
    bool run_background = false;
    is_pipe = (pipe_index = get_pipe_index(arglist,count)) != -1;

    if (strcmp(arglist[count-1], "&") == 0){
        //run the child process int the background
        run_background = true;
        //remove & from arglist
        arglist[count-1] = NULL;
    }
    //is_pipe = (pipe_index = get_pipe_index(arglist,count)) != -1;
    // create two child processes of the original shell, a hierarchy where one
    // process is the parent of the prev one 
    pid_t child_a, child_b;
    bool pipe_done = false;
    int n = 0, fd[2] = {0};
    if (is_pipe) {
        if(pipe(fd) < 0){
            fprintf(stderr, "create pipe failed. %s \n", strerror(errno));
            exit(1);
        }
    }
    // now fork the first
    if ((child_a = fork()) < 0){
        fprintf(stderr, "fork() failed. %s \n", strerror(errno));
        exit(1);
    }
    // we got the child process
    if (child_a == 0){
        if(!is_pipe){
            if(run_background){
                execv_child_proc(arglist, NULL, 0);
            } else {
                sigaction(SIGINT, &siga, NULL);
                execv_child_proc(arglist,NULL,0);
            }
        } else { //piped command
            arglist[pipe_index] = NULL;
            sigaction(SIGINT, &siga, NULL);
            execv_child_proc(arglist, fd, 0);
        }
    }
    if (run_background)
        return 1;
    if (is_pipe) {
        if ((child_b = fork()) < 0) {
            fprintf(stderr, "fork() failed. %s \n", strerror(errno));
            exit(1);
        }
        if(child_b == 0){
            sigaction(SIGINT, &siga, NULL);
            execv_child_proc(&(arglist[pipe_index+1]), fd, 1);
        }
        close(fd[0]);
        close(fd[1]);
    }
    //wait
    int status = 0;
    if (is_pipe){
        //while the waitpid does not return the proccess id of our children
        //Assumption: no background proccess!!
        if (waitpid(child_a, &status, 0) == child_a && waitpid(child_b,&status,0)==child_b){ 
            //just wait
            return 1;
        }
    } else {
        if ((waitpid(child_a,&status,0) == child_a)){
            //just wait
            return 1;
        }
    }
    return 1;
}

int prepare(void){
    struct sigaction sigign;
    memset(&sigign, 0, sizeof(sigign));
    //set the deafult to sig ignore
    sigign.sa_handler = SIG_IGN;
    if (sigaction(SIGCHLD, &sigign, NULL) != 0){
        perror("Sigint error:");
        exit(1);
    }
    if (sigaction(SIGINT, &sigign, NULL)){
        perror("Sigint error:");
        exit(1);
    }
    return 0;

}

int finalize(void){
  //get back to the settings of prepare
  return 0;
}
// Checking if the coomand is a piped command.
// looking for the '|' index to return its index or -1 in case not found
int get_pipe_index(char** arglist, int count){
    int i=0;
    for (i; i < count; i++){
        if(strcmp(arglist[i],"|") == 0){
            //assuming no more than a single pipe is provided
            // and it's correctly placed
            return i;
        }
    }
    return -1;
}

