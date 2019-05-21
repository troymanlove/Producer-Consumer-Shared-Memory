/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.c
 * Author: Troy Manlove
 *
 * Created on March 4, 2019, 2:11 PM
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

/*
 * Struct that has the components of shared memory contains:
 * mutex
 * circular buffer
 * 2 condition variables
 * 
 */


typedef struct{
    pthread_mutex_t mutex;
    pthread_cond_t PtoC;
    pthread_cond_t CtoP;
    int front;
    int back;
    int count;
    int notDone;
    char buffer[1024];
}sharedSegment;

/**
 * Producer Thread takes an input of a filename 
 * 
 */
void producerThread(const char* file,sharedSegment* ss,int id){
  
    printf("producerThread started\n");
    //open the file for ReadOnly mode
    FILE *fp;
    fp = fopen(file,"r");
    if (fp==NULL){
        printf("Input file not found.\n");
        shmctl(id,IPC_RMID,NULL);
        shmdt(ss);
        
        exit(1);
    }
    
    
    //while file is not empty 
    //this reads the next character from the input file and places it into its appropriate spot in the buffer
    pthread_mutex_lock(&(ss->mutex));
    while(fscanf(fp,"%c",&ss->buffer[ss->back]) != EOF){
    //lock mutex
        
    //if size is at 1024 then buffer is full so shout at consumer and wait for return
                
        ss->count = (ss->count)+ 1;
        if((ss->count) == 1024){
            //shout
            //printf("Waiting on Consumer\n");
            pthread_cond_signal(&ss->PtoC);
            pthread_cond_wait(&ss->CtoP,&ss->mutex);
        }

        //increment the back unit of the queue
        ss->back = ((ss->back) + 1) % 1024;


    //END LOOP
    }
        //unlock mutex
        pthread_mutex_unlock(&(ss->mutex));
    //make notDone =0
    ss->notDone =0;
    pthread_cond_signal(&ss->PtoC);
    printf("Producer Thread is done\n");
}

void consumerThread(const char * file,sharedSegment* ss){
    
    //open file to write only mode
    printf("Consumer thread started\n");
    FILE *fp;
    fp = fopen(file,"w");

    //while notDone != 0 || queue size >0
    pthread_mutex_lock(&(ss->mutex));

    while(ss->notDone !=0 || ss->count != 0){
    //lock mutex
                while(ss->count == 0){
            //printf("Waiting on Producer\n");
            pthread_cond_signal(&ss->CtoP);
            pthread_cond_wait(&ss->PtoC,&ss->mutex);
        }
        //place two elements in the buffer at once, places next item first then current item then increments the thing
        if(ss->count > 1){
        fprintf(fp,"%c",ss->buffer[(ss->front +1) % 1024]);
        fprintf(fp,"%c",ss->buffer[ss->front]);
        ss->front = ((ss->front) +2) % 1024;
        ss->count = (ss->count) - 2;
        }
        
    //if notDone == 0 && size >0 then only one element left so place into output file
        if(ss->notDone == 0 && ss->count ==1){
            fprintf(fp,"%c",ss->buffer[ss->front]);
            ss->front = ((ss->front) +1) %1024;
            ss->count = (ss->count) -1;
        }

    //unlock mutex
       
    //END LOOP
    }
   // pthread_cond_signal(&ss->CtoP);
     pthread_mutex_unlock(&(ss->mutex));
    printf("Consumer Thread is done\n");
    fclose(fp);
}

int main(int argc, char** argv) {

    key_t key;
    pid_t pid;
    int identifier;
    sharedSegment* seg;
    pthread_condattr_t condattr;
    pthread_mutexattr_t mutattr;
    //By just giving a value to key we can bypass using ftok()
    key = 1234;
    if (argc < 3){
        printf("Need both an input and output file specified\n");
        exit(1);
    }
    //create shared memory should be:
    // shmget(create)  -> initilize 
    if ((identifier = shmget(key,sizeof(seg),0666| IPC_CREAT | IPC_EXCL)) != -1){
        
        seg = (sharedSegment*) shmat(identifier, 0,0);
        printf("smhget created succesfully, now initiliziing\n");
        
        seg->notDone=1;
        seg->front=0;
        seg->back =0;
        //printf("seg values init\n");
        pthread_mutexattr_init(&mutattr);
 
        pthread_mutexattr_setpshared(&mutattr,PTHREAD_PROCESS_SHARED);
        //printf("mutexattr_setpshared\n");
        pthread_mutex_init(&(seg->mutex),&mutattr);
    
        pthread_condattr_init(&condattr);
        pthread_condattr_setpshared(&condattr,PTHREAD_PROCESS_SHARED);
        //printf("condattr_setpshared\n");
        pthread_cond_init(&(seg->CtoP),&condattr);
        pthread_cond_init(&(seg->PtoC),&condattr);
        //printf("cond_init\n");    
    }
    else{
        //already exists so just attach 
        identifier = shmget(key,0,0);
        seg = (sharedSegment*) shmat(identifier, 0,0);
        printf("failed to create shared memory as it is already created. Attaching to segment\n");

    }

    //fork()
    pid = fork();
   
    /*
     switch (pid): -1 : error
     * 0: child and do producerThread
     * default: parent and do consumerThread
     */
    switch(pid){
        case(-1):
            printf("error with fork() in main\n");
            //kill program
            return (EXIT_SUCCESS);
            break;
        case(0): 
            consumerThread(argv[2],seg);
            exit(0);
            break;
        default: 
            producerThread(argv[1],seg,identifier);
            //wait for child
            pid = wait(NULL);       
    }
            
        
    
    printf("Both are done terminating process.\n");
    if(shmctl(identifier,IPC_RMID,NULL) != 0){
        printf("Failed to destroy the shared memory segment\n");
    }
    else{
        printf("memory marked for removal\n");
    }
    shmdt(seg);
    return (EXIT_SUCCESS);
}

