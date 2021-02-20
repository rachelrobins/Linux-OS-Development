#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
int command;
int background;
int pipeMode;


int prepare(void){
/* I used SIG_IGN both for zombies and for background processes. according to
http://www.microhowto.info/howto/reap_zombie_processes_using_a_sigchld_handler.html
zombies can be reaped with SIG_IGN*/
    struct sigaction myaction;
    memset(&myaction,0,sizeof(myaction));
    myaction.sa_handler=SIG_IGN;
     if(sigaction(SIGINT,&myaction,0)==1 )
    {
        fprintf(stderr, "Signal handle registration " "failed. \n"
               );
        return 1;
    }
    if(sigaction(SIGCHLD,&myaction,0)==1 )// handles zombies
    {
        fprintf(stderr, "Signal handle registration " "failed. \n"
               );
        return 1;
    }



      return 0;

}
int process_arglist(int count, char** arglist){
int i;
int pipeSgn=0;
int  pid_first, pid_second;
       int pipefd[2];
    struct sigaction newaction;
    memset(&newaction,0,sizeof(newaction));

    command=0;
    background=0;
    pipeMode=0;
    if(strcmp(arglist[count-1],"&")==0){
        background=1;
    }
    for( i=0; i<count;i++){
        if (strcmp(arglist[i],"|")==0){
            pipeMode=1;
            pipeSgn=i;
        }
    }
    if(background!=1 && pipeMode!=1){
        command=1;
    }

    if(command==1){
        int pid=fork();
        if(pid<0){
            fprintf( stderr, "print failed\n");
            return 0;
        }
        if(pid==0){
//child
            newaction.sa_handler=SIG_DFL;  
            sigaction(SIGINT,&newaction,0);  // send default signal so child process will die under CTRL c  
            if(execvp(arglist[0],arglist)==-1){
                fprintf( stderr, "execvp failed\n");
                exit(1);
            }
        }
        else{
//parent

           waitpid(pid,NULL,0);
        }
    }
    if(background==1){
       arglist[count-1]=NULL;
        int pid=fork();
        if(pid<0){
            fprintf( stderr, "fork failed\n");
            return 0;
        }
        if(pid==0){
//child
           
                     
            if(execvp(arglist[0],arglist)==-1){
                fprintf( stderr, "execvp failed\n");
                exit(1);
               
            }

        }
        else{
//parent
           // no need to wait because process is running on background


        }

    }if(pipeMode==1){ //got insperation from https://brandonwamboldt.ca/how-linux-pipes-work-under-the-hood-1518/
     
       // Create  pipe
       if (pipe(pipefd) == -1) {
           fprintf(stderr,"parent: Failed to create pipe\n");
           return 0;
       }


       pid_second = fork();

       if (pid_second == -1) {
           fprintf( stderr,"fork failed\n");
           return 0;
       } else if (pid_second == 0) {// child
     
           if (dup2(pipefd[0], STDIN_FILENO) == -1) {
               fprintf(stderr, " dup2 failed\n");
               exit(1);
           }

           // Close the pipe
           close(pipefd[0]);
           close(pipefd[1]);

         

       
              newaction.sa_handler=SIG_DFL;  
              sigaction(SIGINT,&newaction,0);   // send default signal so child process will die under CTRL c
           if(execvp(arglist[pipeSgn+1],arglist+pipeSgn+1)==-1){
                           fprintf(stderr,"execvp failed");
                                exit(1);
                       }

           
           fprintf(stderr ,"pipe side 2  failed!\n");// if we came back here, it was due to an error
           exit(1);
       }
            else{// first command of pipe

   
       pid_first = fork();

       if (pid_first == -1) {
           fprintf( stderr,"fork failed\n");
           return 0;
       } else if (pid_first == 0) {
 
           

         
           if (dup2(pipefd[1], STDOUT_FILENO) == -1) {
               fprintf(stderr,"dup2 failed\n");
               exit(1);
           }

           // Close the pipe
           close(pipefd[0]);
           close(pipefd[1]);



           
                arglist[pipeSgn]=NULL;//"cut" arglist for execvp
               newaction.sa_handler=SIG_DFL;  
               sigaction(SIGINT,&newaction,0); // send default signal so child process will die under CTRL c
               
           if(execvp(arglist[0],arglist)==-1){
                           fprintf(stderr,"execvp failed");
                                exit(1);
                       }

         
          fprintf(stderr, "first command in pipe failed!\n");// if we got here it was due to an error
           exit(1);
       }
            else{// we are in parent
       // close pipe
       close(pipefd[0]);
       close(pipefd[1]);

       
             waitpid(pid_first,NULL,0);

        waitpid(pid_second,NULL,0);
        // parent waited for both children to finish
             
       }
            }

    }
    return 1;
}

int finalize(void){
return 0;
}