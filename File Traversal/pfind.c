#define _GUN_SOURCE
#define _DEFAULT_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <dirent.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

// A linked list (LL) node to store a queue entry
struct QNode {
    char* key;
    struct QNode* next;
};

// The queue, front stores the front node of LL and rear stores the
// last node of LL
struct Queue {
    struct QNode *front, *rear;
};

pthread_mutex_t lock;
struct Queue* q;
pthread_t *threads;
pthread_cond_t notEmpty;
int exit_code,error;
int num_threads;
int sleeping_threads,threads_errored;
int files_found;
int signaled=0;
char* term;
struct sigaction my_action;

// A utility function to create a new linked list node. got help from -https://www.geeksforgeeks.org/queue-linked-list-implementation/
struct QNode* newNode(char * path)
{
    struct QNode* temp = (struct QNode*)malloc(sizeof(struct QNode));
    if(temp==NULL){
        pthread_mutex_unlock(&lock);
        fprintf(stderr,"Could not malloc " );
     
        threads_errored++;
        sleeping_threads++;
       if( sleeping_threads==num_threads ) {error=1;  pthread_cond_broadcast(&notEmpty);} //everyone will die due to this error
        
        pthread_exit((void*)0);}

    temp->key=path;
    temp->next = NULL;
    return temp;
}

// A utility function to create an empty queue - got help from -https://www.geeksforgeeks.org/queue-linked-list-implementation/
struct Queue* createQueue()
{
    struct Queue* q = (struct Queue*)malloc(sizeof(struct Queue));
    q->front = q->rear = NULL;
    return q;
}

// The function to add a key k to q- got help from https://www.geeksforgeeks.org/queue-linked-list-implementation/
void enQueue(struct Queue* q, char* path)
{
    pthread_testcancel();
    pthread_mutex_lock(&lock);
    
    // Create a new LL node
    struct QNode* temp = newNode(path);

    // If queue is empty, then new node is front and rear both
    if (q->rear == NULL) {
        q->front = q->rear = temp;
        pthread_mutex_unlock(&lock);
        return;
    }

    // Add the new node at the end of queue and change rear
    q->rear->next = temp;
    q->rear = temp;
    pthread_cond_signal(&notEmpty);
    pthread_mutex_unlock(&lock);
    
    pthread_testcancel();

}

// Function to remove a key from given queue q- got help from https://www.geeksforgeeks.org/queue-linked-list-implementation/
struct QNode* deQueue(struct Queue* q)
{
   
    pthread_testcancel();
    if(signaled==1){ pthread_exit((void*)0); }
    pthread_mutex_lock(&lock);
   
    sleeping_threads++;
    while (q->front == NULL){
        
   
        if(sleeping_threads== num_threads){ // if all threads are sleeping, each thread spold wake up and exit
           
            pthread_cond_broadcast(&notEmpty);
            pthread_mutex_unlock(&lock);
            exit_code=0;
            pthread_exit((void*)0);

        }
        
        pthread_cond_wait(&notEmpty,&lock);
        
    }
    sleeping_threads--;
    // Store previous front and move front one node ahead
    struct QNode* temp = q->front;
    q->front = temp->next;

    // If front becomes NULL, then change rear also as NULL
    if (q->front == NULL)
        q->rear = NULL;
    
    pthread_mutex_unlock(&lock);
    pthread_testcancel();
    return temp;
    

}

void *search_files(void *t) // this fuction is called by each thread
{

    while(1){
       pthread_testcancel();
       if(signaled==1){ pthread_exit((void*)0); }
       pthread_testcancel();
        struct QNode* n = deQueue(q);
        
         pthread_testcancel();
    

/// here we  search for the files in the directory
        struct dirent *de;  // Pointer for directory entry

       
        DIR *dr = opendir(n->key);

        if (dr == NULL)  // open failed
        {
            fprintf(stderr,"Could not open current directory\n" );
            free(n->key);
            free(n);
            threads_errored++;
            sleeping_threads++;
            if( sleeping_threads==num_threads ) {error=1; pthread_cond_broadcast(&notEmpty); } //everyone will die due to this error
            
            pthread_exit((void*)0);
        }


        while ((de = readdir(dr)) != NULL){
            pthread_testcancel();
       
            if(de->d_type==DT_DIR && (strcmp(de->d_name,".") != 0 )&& (strcmp(de->d_name,"..") != 0) ){ // we got a directory which isnt "." ot ".."
                  pthread_testcancel();
                char * newPath=(char*)malloc(strlen(n->key)+strlen(de->d_name)+2);
                strcpy(newPath,n->key);
                strcat(newPath,"/");
                enQueue( q, strcat(newPath,de->d_name));
            }
            else if((strcmp(de->d_name,".") != 0 )&& (strcmp(de->d_name,"..") != 0)){  // we got a file which isnt "." ot ".."
                 pthread_testcancel();
                    if( strstr(de->d_name,term)!=NULL){ // there was a match to the symbol
                         pthread_testcancel();
                        files_found++;
                      
                        char * dest=(char*)malloc(strlen(n->key)+strlen(de->d_name)+2);
                         if(dest==NULL){ //malloc failed 
                              
                                fprintf(stderr,"Could not malloc " );
                                sleeping_threads++;
                                if( sleeping_threads==num_threads ) {error=1; pthread_cond_broadcast(&notEmpty);}
                            
                               threads_errored++;
                               pthread_exit((void*)0);}

                        strcpy(dest,n->key);
                        strcat(dest,"/");
                       printf("%s\n", strcat(dest ,de->d_name));
                         
                        free(dest);
                        pthread_testcancel();

                    }

            }
            
        }
       pthread_testcancel();
        closedir(dr);

         free(n->key);
         free(n);
         pthread_testcancel();

    }

}
void handler () {// thread got SIGINT
       signaled=1;
        sleeping_threads= num_threads;
       for(int i = 0; i < num_threads; ++i){
           pthread_cancel(threads[i]);
         
       }
      pthread_cond_broadcast(&notEmpty);
   }

    int main (int argc, char *argv[])
    {
        char * root;
        int rc;
        long t;
        void *status;
        num_threads=atoi(argv[3]);
        memset(&my_action, 0,sizeof(my_action));
        my_action.sa_handler = handler;
        sigaction(SIGINT,&my_action,NULL);
        term=argv[2];
        struct stat statbuf;
       
      if(argc!=4){   fprintf(stderr,"you need to enter 3 arguments " ); return -1;}   // check num of arguments 
         if (stat(argv[1], &statbuf) != 0){printf("arg 1 isnt a searchable directory " );
                 return  1;}
      if( S_ISDIR(statbuf.st_mode)==0){printf("arg 1 isnt a searchable directory " ); return -1;}    // check if arg 1 is a directory
     if (access(argv[1], R_OK) != 0)  // check permissions 
      {
          /* It's  not readable by the current user. */
          printf("arg 1 isnt a searchable directory \n" ); 
          return  1;
     }

        // --- Initialize mutex ---------------------------
        pthread_cond_init (&notEmpty, NULL);
        rc = pthread_mutex_init( &lock, NULL );
        if( rc )
        {
            printf("ERROR in pthread_mutex_init(): "
                   "%s\n", strerror(rc));
            exit( -1 );
        }
        threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
        if(threads == NULL){
            printf("ERROR in malloc. Error code: \n");
            return  1;
        }
        q = createQueue();
        root=(char*)malloc(strlen(argv[1])+1);
        strcpy(root,argv[1]);
        enQueue( q, root);
        for(t=0; t<num_threads; t++) // create threads 
        {
            
            rc = pthread_create(&threads[t], NULL, search_files, (void *)t);
            if (rc)
            {
                printf("ERROR in pthread_create(): %s\n", strerror(rc));
                exit(-1);
            }
        }
        

        for(t=0; t<num_threads; t++) // this makes sure all threads will first finish
        {
            rc = pthread_join(threads[t], &status);
            if (rc)
            {
                printf("ERROR in pthread_join(): %s\n", strerror(rc));
                exit(-1);
            }
          
        }

        if(signaled){  // if there was  a SIGINT
            printf("Search stopped, found %d files\n",files_found);
        }else{
            printf("Done searching, found %d files\n",files_found);
        }
        pthread_mutex_destroy( &lock );
        
        pthread_cond_destroy(&notEmpty);
        free(threads);
        free(q);
        if(threads_errored==num_threads || error==1){
            return 1;
        }
        else{//signaled or idle
            return 0;
        }

    }
