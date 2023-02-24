/* Program:     Thread-based Batch Job Dispatcher
   Author:      Samuel Goncalves
   Date:        September 19, 2022
   File name:   asgn6-goncalvess2.c
   Compile:     gcc -o asgn6-goncalvess2 asgn6-goncalvess2.c
   Run:         ./asng6-goncalvess2

   The program accepts n number of jobs specified by the user followed by
   n number of commands <= 4 and a start time (n seconds away),then puts 
   the jobs into a singly-linked list in a non-decreasing order of 
   submit time + start time, then dispatches them. The scheduling, 
   dispatching, and execution is carried out by three POSIX threads 
   appropriately named, scheduler, dispatcher, and executer. The input 
   ends once a user enters "Ctrl+C" on the keyboard.
*/

#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>

struct JOB {
   char *command[5]; 
   long submitTime;       
   long startTime;        
   struct JOB *next;      
};
typedef struct JOB Job;

struct LIST {
   int numOfJobs;        
   Job *firstJob;        
   Job *lastJob;         
};
typedef struct LIST List;

void scheduleJobs(List *list);
void dispatchJobs(List *list);
void *dispatchJob(void *jobPtr);
int isJobReady(List *list, long currTime);

List* createList();
Job* createJob();

void appendJob(List *list, Job *jobPtr);
void insertJob(List *list, Job *jobPtr);
void insertJobFirst(List *list, Job *jobPtr);
void insertJobLast(List *list, Job *jobPtr);
void insertJobBetween(List *list, Job *jobPtr);

Job *deleteFirstJob(List *list);
void freeJobStruct(Job *jobPtr);
void deallocJobCommands(Job *jobPtr);

Job *previousJob(List *list, Job *jobPtr);
int greaterSubmitTime(Job *jobPtr1, Job *jobPtr2);
int greaterSubAndStart(Job *jobPtr1, Job *jobPtr2);
int timePrecedence(Job *jobPtr1, Job *jobPtr2);
int totalJobTime(Job *jobPtr);

void printJobs(List *list);
void printJob(Job *jobPtr);

int main(){
   List* list = createList();
   pthread_t scheduler, dispatcher;
   
   pthread_create(&scheduler, NULL, (void*)scheduleJobs, (void*)list);
   pthread_create(&dispatcher, NULL, (void*)dispatchJobs, (void*)list);

   pthread_join(scheduler, NULL);
   pthread_join(dispatcher, NULL);
}

void scheduleJobs(List *list){
   Job*jobPtr = (Job *) malloc(sizeof(Job));
   char inputSymbol;
   
   while (1){ 
      scanf("%c", &(inputSymbol));
      if (inputSymbol == '+'){
         Job* jobPtr = createJob();
         insertJob(list, jobPtr);
      }
      else if ((inputSymbol == 'p')){
         printJobs(list);
      }
      else if ((inputSymbol == '-')){
         jobPtr = deleteFirstJob(list);
         printJob(jobPtr);
         freeJobStruct(jobPtr);
      }
   }
}

void dispatchJobs(List *headerPtr){
   Job *nextJob = NULL;
   long currentSysTime;
   pthread_t executer;
   
   while (1){
      currentSysTime = time(NULL);
      
      if (isJobReady(headerPtr, currentSysTime) == 0){
         printf("Current System Time: %ld\n", currentSysTime);
         nextJob = deleteFirstJob(headerPtr);
         pthread_create(&executer, NULL, (void*)dispatchJob, (void*)nextJob);
      }
      else{
         sleep(1);
      }
   }
   freeJobStruct(nextJob);
}

void *dispatchJob(void *jobPtr){
   Job *jPtr = (Job *)jobPtr;
   int cpid;
   int status = 0;
   
   cpid = fork();
   
   if (cpid != 0){
      waitpid(cpid, &status, WEXITED);
   }
   else{
      execvp(jPtr->command[0], jPtr->command);
      exit(EXIT_SUCCESS);
   }
   printJob(jPtr);
   printf("Exit status: %d\n\n", WEXITSTATUS(status));
   return (void*)jPtr;
}

int isJobReady(List *list, long currTime){
   Job *jobPtr = NULL;
   
   if (list->numOfJobs != 0){
      jobPtr = list->firstJob;
      
      if (currTime >= jobPtr->submitTime + jobPtr->startTime){
         return 0;
      }
   }
   else return 1;
}

List* createList() {
   List* list = (List*) malloc(sizeof(List));
      
   list->firstJob = NULL;
   list->lastJob = NULL;
   list->numOfJobs = 0;

   return list;
}

Job* createJob() { 
   Job*jobPtr = (Job *) malloc(sizeof(Job)); 
   jobPtr->submitTime = time(NULL);
   int wordCount = 0;
   int i = 0;

   scanf("%d", &wordCount);
   if (wordCount < 5){
      for (i = 0; i < wordCount; i++){
         jobPtr->command[i] = (char *) malloc(25*sizeof(char));
         scanf("%s", jobPtr->command[i]);
      }
      scanf("%ld", &(jobPtr->startTime));
   }
   jobPtr->command[wordCount] = NULL;
   jobPtr->next = NULL;

   return jobPtr;
}

void appendJob(List *list, Job *jobPtr){
   
   if (list->firstJob == NULL){
      list->firstJob = jobPtr;
      list->lastJob = jobPtr;
   }
   else {
      list->lastJob->next = jobPtr;
      jobPtr->next = NULL;
      list->lastJob = jobPtr;
   }
   list->numOfJobs = list->numOfJobs + 1;
}

void insertJob(List *list, Job *jobPtr){
   
   if (list->numOfJobs == 0){
      appendJob(list, jobPtr);
   }
   else if (timePrecedence(jobPtr, list->firstJob) == 1){
      insertJobFirst(list, jobPtr);
   }
   else if (timePrecedence(jobPtr, list->lastJob) == 0){
      insertJobLast(list, jobPtr);
   }
   else {
      insertJobBetween(list, jobPtr);
   }
}

void insertJobFirst(List *list, Job *jobPtr){
   
   jobPtr->next = list->firstJob;
   list->firstJob = jobPtr;
   list->numOfJobs = list->numOfJobs + 1;
}

void insertJobLast(List *list, Job *jobPtr){
   
   jobPtr->next = NULL;
   list->lastJob->next = jobPtr;
   list->lastJob = jobPtr;
   list->numOfJobs = list->numOfJobs + 1;
}

void insertJobBetween(List *list, Job *jobPtr){
   Job*prevPtr = (Job *) malloc(sizeof(Job));
   
   prevPtr = previousJob(list, jobPtr);
   jobPtr->next = prevPtr->next;
   prevPtr->next = jobPtr;
   list->numOfJobs = list->numOfJobs + 1;
}

Job *deleteFirstJob(List *list){
   Job*jobPtr = (Job *) malloc(sizeof(Job));
   
   if (list->firstJob != NULL){
      jobPtr = list->firstJob;
      list->firstJob = list->firstJob->next;
      list->numOfJobs = list->numOfJobs - 1;
      
      return jobPtr;
   }
   else {
      return NULL;
   }
}

void freeJobStruct(Job *jobPtr){ 
   deallocJobCommands(jobPtr);
   free(jobPtr);
}

void deallocJobCommands(Job *jobPtr){
   int i = 0;
   
   while (jobPtr->command[i] != NULL){
      free(jobPtr->command[i]);
      i = i + 1;
   }
}

Job *previousJob(List *list, Job *jobPtr){
   Job*prevPtr = (Job *) malloc(sizeof(Job));
   prevPtr = list->firstJob;
   int i;
   int count = -1;
   
   while (timePrecedence(jobPtr, prevPtr) == 0){
      prevPtr = prevPtr->next;
      count = count + 1;
   }
   prevPtr = list->firstJob;
   for (int i = 0; i != count; i++){
      prevPtr = prevPtr->next;
   }
   return prevPtr;
}

int greaterSubmitTime(Job *jobPtr1, Job *jobPtr2){
   
   if (jobPtr1->submitTime > jobPtr2->submitTime){
      return 0;
   }
   else if (jobPtr1->submitTime < jobPtr2->submitTime){
      return 1;
   }
}

int greaterSubAndStart(Job *jobPtr1, Job *jobPtr2){
   
   if (totalJobTime(jobPtr1) > totalJobTime(jobPtr2)){
      return 0;
   }
   else if (totalJobTime(jobPtr1) < totalJobTime(jobPtr2)){
      return 1;
   }
}

int timePrecedence(Job *jobPtr1, Job *jobPtr2){
   
   if ((greaterSubAndStart(jobPtr1, jobPtr2) == 0) 
      || (greaterSubmitTime(jobPtr1, jobPtr2)) == 1){
      return 0;
   }
   else {
      return 1;
   }
}

int totalJobTime(Job *jobPtr){
   int totalJobTime;
   
   totalJobTime = ((jobPtr->submitTime) + (jobPtr->startTime));
   
   return totalJobTime;
}

void printJobs(List *list){
   int i = 0;
   
   printf("\nNumber of jobs: %d\n", list->numOfJobs);
      
   if (list->numOfJobs != 0){
      Job *jobPtr = list->firstJob;
      for (i = 0; i < list->numOfJobs; i++){
         int jobCount = i + 1;
         printf("Job %d:\n", jobCount);
         printJob(jobPtr);
         jobPtr = jobPtr->next;
      }
   }
   else {
      printf("List empty!\n");
   }
}

void printJob(Job *jobPtr) {
   int i = 0;
   
   printf("Program name: ");
   while (jobPtr->command[i] != NULL){
      printf("%s ", jobPtr->command[i]);
      i++;
   }
   printf("\nSubmit time: %ld\nStart time: %ld\n", 
      jobPtr->submitTime, jobPtr->startTime);
}


