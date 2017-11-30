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

int process_arglist(int count, char** arglist){
    pid_t child_pid;
    bool is_pipe;
    int pipe_index;
    struct sigaction siga;
    // checking if the last arg of the arglist is the & terminator
    bool run_background = false;
    if (strcmp(arglist[count -1], "&") == 0){
        //run the child process int the background
        run_background = true;
        //remove & from arglist
        arglist[count-1] = NULL;
    }
    is_pipe = (pipe_index = get_pipe_index(arglist,count)) != -1;
    // create two child processes of the original shell, a hierarchy where one
    // process is the parent of the prev one 
    pid_t child_a, child_b;
    bool pipe_done = false;
    int n = 0, fd[2];
    while (n < 2){ 
        if(is_pipe && !pipe_done){
            //pipe fd
            if(pipe(fd) < 0){
                fprintf(stderr, "pipe failed. %s \n", strerror(errno));
                exit(1);
            }
            // rmv | from arglist
            arglist[pipe_index] = NULL;
            pipe_done = true;
        }
        // now fork
        if ((child_pid = fork()) < 0){
            fprintf(stderr, "fork() failed. %s \n", strerror(errno));
            exit(1);
        }
        //first proccess 
        if (child_pid == 0){
            if (n == 0){
                child_a = child_pid;
            }
            //TODO: set sigint handler 
            if (is_pipe)
                dup2(fd[1-n],1-n);
            //execvp
            if (execvp(arglist[0+n*(pipe_index+1)], arglist+n*(pipe_index+1)) == -1){
                fprintf(stderr, "execvp failed. %s\n", stderror(errno));
                exit(1);
            }
            n=2; //exit while if you child
        }
        n++;
    }
    //parent case
    int status;
    if (child_pid > 0){
        // if is_pipe is true we need to wait for our children
        if (is_pipe){
            //while the waitpid does not return the proccess id of our children
            //Assumption: no background proccess!!
            while(waitpid(child_pid, &status, 0) != child_pid && waitpid(child_a,&status,0)!=child_a){
                //just wait
            }
        } else {
            while(!run_background && (wait(&status)!=child_pid)){
                //just wait
            }
        }
    }
    return 1;
}

int prepare(void){

}

int finalize(void){

}

int get_pipe_index(char** arglist, int count){
    int i=0;
    for (i; i<count; i++){
        if(strcmp(arglist[i],"|") == 0){
            //assuming no more than a single pipe is provided
            // and it's correctly placed
            return i;
        }
    }
    return -1;
}