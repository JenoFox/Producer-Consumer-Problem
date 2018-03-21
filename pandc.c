#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>

/* variables */
int *buffer;		// bounded buffer
static int n;		// buffer size
static int numP;	// number of producers
static int numC;	// number of consumers
static int x;		// number of unique items per producer
static int waitP;	// wait time for producers
static int waitC;	// wait time for consumers
pthread_t *proTid;	// producer pthread ID array
pthread_t *conTid;	// consumer pthread ID array
int *produced;		// array to hold all items produced
int *consumed;		// array to hold all items consumed

int timeRunning = 0;	// variable for total runtime
int item = 0;		// initialize item created by producer
int item2;		// consumed item
int item3 = 0;		// to check number of items consumed, used in if to check consumers at end
int counter = 0;	// initialize counter used to check if space available in buff

/* mutex */
pthread_mutex_t mutex;

/* semaphores */
sem_t full;
sem_t empty;

/* function declarations */
void *producerFunc(void *param); // function for producers
void *consumerFunc(void *param); // function for consumers
int enqueue(int addedNum);	 // function to produce items into buffer
int dequeue();			 // function to consume items from buffer

int main(int argc, char *argv[]) {
/* CHECK ARGUMENTS */
	if(argc != 7) {
		fprintf(stderr, "Incorrect # args! \nCorrect Format:./main.out <INT> <INT> <INT> <INT> <INT> <INT>\n");
		return -1;
	}
/* INITIALIZE VARIABLES */
	n = atoi(argv[1]);
	buffer = (int*)calloc(n, 4);
	numP = atoi(argv[2]);
	numC = atoi(argv[3]);
	x = atoi(argv[4]);
	waitP = atoi(argv[5]);
	waitC = atoi(argv[6]);
	proTid = (pthread_t*)calloc(numP, sizeof(pthread_t));
	conTid = (pthread_t*)calloc(numC, sizeof(pthread_t));
	produced = (int*)calloc((numP * x), 4);
	consumed = (int*)calloc((numP * x), 4);
	struct timeval currentTime, elapsedTime;	// used for end total runtime
	time_t current_time = time(NULL);		// get current time
	char * c_time_string;				// string for time and date format

/* LOCKS */
	if(pthread_mutex_init(&mutex, NULL)!= 0){
		fprintf(stderr, "Error initializing mutex.\n");
		return -1;
	}
	if(sem_init(&full, 0, 0)!= 0){
		fprintf(stderr, "Error initializing sem full.\n");
		return -1;
	}
	if(sem_init(&empty, 0, n)!= 0){
		fprintf(stderr, "Error initializing sem empty.\n");
		return -1;
	}
/* THREAD CREATION */
	gettimeofday(&currentTime, NULL);
	c_time_string = ctime(&current_time);
	fprintf (stderr, "\nCurrent Time: %s\n", c_time_string);
	for(int i = 0; i < numP; i++) {
		proTid[i] = i;
		if(pthread_create(&proTid[i], NULL, producerFunc, (void*)proTid[i]) < 0)	// create producers
			fprintf(stderr, "Error creating PRODUCER thread.\n");	// print if error in producer	
	}
	for(int i = 0; i < numC; i++) {
		conTid[i] = i;
		if(pthread_create(&conTid[i], NULL, consumerFunc, (void*)conTid[i]) < 0)	// create consumers
			fprintf(stderr, "Error creating CONSUMER thread.\n");	// print if error in consumer
	}
/* THREAD JOINING */
	for(int i = 0; i < numP; i++){
		pthread_join(proTid[i], NULL);
	}
	for(int i = 0; i < numC; i++){
		pthread_join(conTid[i], NULL);
	}
/* CHECK ITEMS PRODUCED ARE SAME AS CONSUMED */
	fprintf(stderr, "\nProducer Array \t Consumer Array\n");
	for(int i = 0; i < (numP * x); i++){
		fprintf(stderr,"%d\t\t | %d\n", produced[i], consumed[i]);
	}
	if(memcmp(produced,consumed, numP * x) == 0)
		fprintf(stderr, "\nConsume and Produce Arrays Match!\n");
	else
		fprintf(stderr, "Consume and Produced Arrays ***DO NOT*** Match!\n");
/* STATE TOTAL RUNTIME */
	gettimeofday(&elapsedTime, NULL);
	current_time = time(NULL);
	c_time_string = ctime(&current_time);
	fprintf (stderr, "\nEnd Time: %s\n", c_time_string);
	fprintf(stderr, "Total Runtime: %d seconds.\n\n", (elapsedTime.tv_sec - currentTime.tv_sec));

	return 0;	
}

void *producerFunc(void *param){
	int id = (int)param;
	fprintf(stderr, "Producer %d created.\n", id);
	int counter = 1; 	// counter to check for number of items produced
	while(1){
		if(counter > x){
			fprintf(stderr, "Producer %d has finished producing.\n", id);
			break;
		}
		sleep(waitP);		// sleep for designated PTime
		sem_wait(&empty);		// check empty lock
		pthread_mutex_lock(&mutex);	// check mutex

		item++;				// get new item, done after lock as to not have any race conditions
		if(enqueue(item) < 0)		// check if enqueue worked
			fprintf(stderr, "Error adding item, Buffer Full.\n");
		else
			fprintf(stderr, "Producer %d produced item: %d\n", id, item);
		
		pthread_mutex_unlock(&mutex);	// unlock mutex
		sem_post(&full);		// incremenet full due to item added

		counter++;			// counter to check that producer only produces correct amount
	}
	pthread_exit(0);
}

void *consumerFunc(void *param){
	int id = (int)param;
	fprintf(stderr, "Consumer %d created.\n", id);
	while(1){
		item3++;			// keep track of total items consumed
		if(item3 > (numP * x)){
			fprintf(stderr, "Consumer %d has finished consuming.\n", id);
			break;
		}
		sleep(waitC);		// sleep for designated CTime
		sem_wait(&full);		// check full lock
		pthread_mutex_lock(&mutex);	// check mutex

		if(dequeue() < 0)
			fprintf(stderr, "Error consuming item, Buffer Empty.\n");
		else
			fprintf(stderr, "Consumer %d consumed item: %d\n", id, item2);
		
		pthread_mutex_unlock(&mutex);	// unlock mutex
		sem_post(&empty);		// decrement empty due to item consumed	
	}
	pthread_exit(0);
}

int enqueue(int addedNum){
	if(counter < n){		// check if buff is full
		buffer[counter] = addedNum; // add item if not full
		counter++;		// increment counter
		produced[addedNum] = addedNum; // add item to array of items produced
		return 1;		// flag to show item added without error
	}
	else{
		return -1;		// flag to show buff is full
	}
}

int dequeue(){
	if(counter > 0){		// check if buff is empty
		item2 = buffer[counter-1]; // "remove" item from buffer
		counter--;		// decrement counter
		consumed[item2] = item2; // add item to array of items consumed
		return 1;		// flag to show item consumed without error
	}
	else{
		return -1;		// flag to show buff is full
	}
}
