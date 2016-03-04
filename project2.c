/* program MovieTheaterSim */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

#define MAX_CUSTOMERS 50
#define MAX_AGENTS 2
#define MAX_TAKERS 1
#define MAX_WORKERS 1

/********* Global Semaphores *********/
sem_t asem;
sem_t tsem;
sem_t csem;
sem_t custsem[MAX_CUSTOMERS];
sem_t theaterOpen;

/********* Semeaphore Queues *********/
struct semNode
{
	sem_t sem;
	struct semNode *next;
};
struct semNode *ticketFront = NULL;
struct semNode *ticketBack = NULL;
struct semNode *concessionFront = NULL;
struct semNode *concessionBack = NULL;
int ticketCount = 0;
int concessionCount = 0;

/********* Queue Functions *********/
void enqueueSem(sem_t *data, int *count, struct semNode **back, struct semNode **front)
{
	if (*back == NULL)
	{
		*back  = (struct semNode*) malloc(1*sizeof(struct semNode));
		(*back)->next = NULL;
		(*back)->sem = *data;
		*front = *back;
	}
	else
	{
		struct semNode *temp = (struct semNode*) malloc(1*sizeof(struct semNode));
		(*back)->next = temp;
		temp->sem = *data;
		temp->next = NULL;
		*back = temp;
	}

	(*count)++;
}

void dequeue(int *count, struct semNode **back, struct semNode **front)
{
	struct semNode *tempFront = (struct semNode*) malloc(1*sizeof(struct semNode));
	tempFront = *front;

	if (tempFront == NULL)
	{
		printf("Error: Trying to dequeue from empty queue!\n");
		exit(1);
	}
	else
	{
		if (tempFront->next != NULL)
		{
			tempFront = tempFront->next;
			free(*front);
			*front = tempFront;
		}
		else
		{
			free(*front);
			*front = NULL;
			*back = NULL;
		}
		(*count)--;
	}
}

void empty(struct semNode **back, struct semNode **front)
{
	if (*back == NULL && *front == NULL)
		printf("Queue is empty\n");
	else
		printf("Queue is not empty\n");
}

void queuesize(int *count)
{
	printf("Queue size is %d\n", *count);
}

/********* Thread Functions *********/
void* Customer(void *custID)
{
	int custVal, theaterVal;
	sem_getvalue(&theaterOpen, &theaterVal);
	sem_wait(&custsem[(int) custID]);	//Block Customers until theater is open
	if(theaterVal == 4)
	{
		sem_wait(&theaterOpen);	//Decrement theaterOpen semaphore so no other customers come in.
		printf("Theater is now open!\n");
		int i;
		
		// Alert all customers who are blocked that the theater is now open
		for (i = 0; i < MAX_CUSTOMERS; i++)
		{
			sem_getvalue(&custsem[i], &custVal);
			if (custVal == -1)
			{
				sem_post(&custsem[i]);
			}
		}
	}
	printf("Customer %d created\n", (int) custID);
	pthread_exit(NULL);	
}

void* BoxOfficeAgent(void *agentID)
{
	printf("Box office agent %d created\n", (int) agentID);
	sem_post(&theaterOpen);
	pthread_exit(NULL);
}

void* TicketTaker(void *takerID)
{
	printf("Ticket taker %d created\n", (int) takerID);
	sem_post(&theaterOpen);
	pthread_exit(NULL);
}

void* ConcessionStandWorker(void *workerID)
{
	printf("Concession stand worker %d created\n", (int) workerID);
	sem_post(&theaterOpen);
	pthread_exit(NULL);
}

/********* Functions *********/
void initSems(sem_t *sem, int val)
{
	if (sem_init(sem, 0, val) == -1)
	{
		perror("SEM_INIT_ERROR:");
		exit(1);
	}
	return;
}

void checkThreadError(int *rc)
{
	if (*rc)
	{
		printf("ERROR: pthread_create returned %d\n", *rc);
		exit(1);
	}
}

/********* Beginning main function *********/
int main(int argc, char **argv)
{	
	int i;		//Loop Counter
	int rc; 	//Error checker for threads
	void *status; 	//Status of joins

	pthread_t agents[MAX_AGENTS];		// Box Office Agents threads
	pthread_t customers[MAX_CUSTOMERS];	// Customers threads
	pthread_t ticketTakers[MAX_TAKERS];	// Ticket Taker thread
	pthread_t workers[MAX_WORKERS];		// Concession Stand Worker thread

	/* Initialize all semaphores */
	initSems(&theaterOpen, 0);
	initSems(&tsem, MAX_TAKERS);
	initSems(&csem, MAX_WORKERS);
	initSems(&asem, MAX_AGENTS);
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&custsem[i], 0);	
	}

	/* First create BoxOfficeAgentsm TickerTaker, and ConcessionStandWorker threads */
	for (i = 0; i < MAX_AGENTS; i++)
	{
		rc = pthread_create(&agents[i], NULL, BoxOfficeAgent, (void *) i);
		checkThreadError(&rc);
	}
	for (i = 0; i < MAX_TAKERS; i++)
	{
		rc = pthread_create(&ticketTakers[i], NULL, TicketTaker, (void *) i);
		checkThreadError(&rc);
	}
	for (i = 0; i < MAX_WORKERS; i++)
	{
		rc = pthread_create(&workers[i], NULL, ConcessionStandWorker, (void *) i);
		checkThreadError(&rc);
	}

	/* Create customer threads */
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		rc = pthread_create(&customers[i], NULL, Customer, (void *) i);
		checkThreadError(&rc);
	}	
	/* Join all customer threads */
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		rc = pthread_join(customers[i], &status);
		checkThreadError(&rc);
		printf("Customer %d joined with status %d\n", i, (int) status);
	}
	
	/* Exit main thread */
	pthread_exit(NULL);

} /********* End main function *********/
