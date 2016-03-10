/* Program MovieTheaterSim */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <limits.h>

#define MAX_CUSTOMERS 50
#define MAX_AGENTS 2
#define MAX_TAKERS 1
#define MAX_WORKERS 1
#define DEF_ARR_S 50

/********* Global Semaphores *********/
sem_t createdAgent;
sem_t createdTaker;
sem_t asem[MAX_CUSTOMERS];
sem_t tsem[MAX_CUSTOMERS];
sem_t wsem[MAX_CUSTOMERS];
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

/******** Global Variables *********/
int isTheaterOpen = 0;

/********* Queue Functions *********/

/****************************************************
 * Description: enqueue places a semaphore into the
 * queue represented by a linked list of Nodes
 *
 * Parameters: *data - semaphore to place in the queue
 *             *count - number of elements in the queue
 *             **back - pointer to back of queue
 *             **front - pointer to front of queue
 *
 * Return: nothing
 ***************************************************/
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

/****************************************************
 * Description: removes an element from the front of
 * the queue and updated the pointers accordingly
 *
 * Parameters: *count - number of elements in the queue
 *             **back - back of queue
 *             **front - front of queue
 *
 * Return: error if trying to remove from empty queue
 * or nothing
 ***************************************************/
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

/****************************************************
 * Description: empty checks to see if the queue
 * is empty or not and prints the result
 *
 * Parameters: **back - back of queue
 *             **front - front of queue
 *
 * Return: nothing
 ***************************************************/
void empty(struct semNode **back, struct semNode **front)
{
	if (*back == NULL && *front == NULL)
		printf("Queue is empty\n");
	else
		printf("Queue is not empty\n");
}

/****************************************************
 * Description: outputs the number of elements in the
 * queue
 *
 * Parameters: *count - number of elements int the queue
 *
 * Return: nothing
 ***************************************************/
void queuesize(int *count)
{
	printf("Queue size is %d\n", *count);
}

/********* Thread Functions - Comments need to be updated *********/

/****************************************************
 * Description: Customer thread function to execute
 * the actions of a customer as it goes through the
 * theater simulation
 *
 * Parameters: *custID - ID of thread or customer
 *
 * Return: pthread_exit(NULL) if successful
 ***************************************************/
void* Customer(void *custID)
{
	int custVal, theaterVal;

	/* Wait for the theater to be open, the first customer who sees that it is
	 * open will output that it is open. Afterwards, it will signal to the next
	 * blocked customer that the theater is now open */
	sem_wait(&theaterOpen);
	if (!isTheaterOpen)
	{
		isTheaterOpen = 1;
		printf("The theater is now open!\n");
	}
	sem_post(&theaterOpen);
	
	printf("Customer %d created\n", (int) custID);
	pthread_exit(NULL);	
}

/****************************************************
 * Description: BoxOfficeAgent thread function to
 * execute the actions of a box office agent as it goes
 * through the theater simulation
 *
 * Parameters: *agentID - ID or thread of box office
 * agent
 *
 * Return: pthread_exit(NULL)
 ***************************************************/
void* BoxOfficeAgent(void *agentID)
{
	/* Agents are created frst therefore, they do not need
	 * to wait on anyone to be created */
	printf("Box office agent %d created\n", (int) agentID);
	sem_post(&createdAgent);
	pthread_exit(NULL);
}

/****************************************************
 * Description: TicketTaker thread function to execute
 * the actions of a ticket taker as it goes through the
 * theater simulation
 *
 * Parameters: *takerID - ID or thread of ticket taker
 *
 * Return: pthread_exit(NULL)
 ***************************************************/
void* TicketTaker(void *takerID)
{
	/* The ticket taker will wait for the agents to be created
	 * and then will signal that he has been created */
	sem_wait(&createdAgent);
	printf("Ticket taker %d created\n", (int) takerID);
	sem_post(&createdTaker);
	pthread_exit(NULL);
}

/****************************************************
 * Description: ConcessionStandWorker thread function
 * to execute the actions of a concession stand worker
 * as it goes throught the theater simulation
 *
 * Parameters: *workerID - ID or thread of the 
 * concession stand worker
 *
 * Return: pthread_exit(NULL)
 ***************************************************/
void* ConcessionStandWorker(void *workerID)
{
	/* The worker will wait for the taker to be created 
	 * and then signal that the theater is now open for customers */
	sem_wait(&createdTaker);
	printf("Concession stand worker %d created\n", (int) workerID);
	sem_post(&theaterOpen);
	pthread_exit(NULL);
}

/********* Functions *********/

/****************************************************
 * Description:initializes a sempahore to a given value 
 * indicated by the val parameter
 *
 * Parameters: *sem - semaphore to initialize
 *             val - value to init *sem to
 *
 * Return: error if sem_init fails or nothing if
 * success
 ***************************************************/
void initSems(sem_t *sem, int val)
{
	if (sem_init(sem, 0, val) == -1)
	{
		perror("SEM_INIT_ERROR:");
		exit(1);
	}
}

/****************************************************
 * Description: Checks to see if there was an error
 * when creating or joining threads
 *
 * Parameters: *rc - return from pthread_create
 *
 * Return: error if pthread_create fails or nothing
 ***************************************************/
void checkThreadError(int *rc)
{
	if (*rc)
	{
		printf("ERROR: pthread_create returned %d\n", *rc);
		exit(1);
	}
}

/****************************************************
 * Description: Opens movie.txt file 
 *
 * Parameters: **file - movie.txt file pointer
 *
 * Return: error if could not open file or nothing
 ***************************************************/
void openFile(FILE **file)
{
	*file = fopen("movies.txt", "r");
	if (*file == NULL)
	{
		perror("ERROR: cannot open file");
		exit(1);
	}
}

/****************************************************
 * Description: Parses the file for movie titles and
 * the corresponding ticket amounts for each movie
 * title
 *
 * Parameters: **file - movies.txt file pointer
 *             ***movieTitles - pointer to array of string
 *             	to hold the movie titles
 *             **ticketCount - pointer to array of ints to
 *             hold the number of tickets for each movie
 *
 * Return: error if strtol fails or nothing
 ***************************************************/
void parseFile(FILE **file, char ***movieTitles, int **ticketCount)
{
	int i = 0, j = 0;        //Counters for array access
	char *line = NULL;       //Line to get from the file
	size_t numBytes = 100;   //Number of bytes to read from the file for a line
	ssize_t read;            //Number of bytes read form a line in the file
	char *token, *endptr;    //Token pointer for string token and enptr for use in strtol
	char *delim = "\t\n";    //Delimiters used to signal a token
	int tokenCounter = 0;    //Counter used to determine if the token is a movie name
	                         //Or the number of tickets for the corresponding movie
	int base = 10;           //Base to convert ticket numbers to

	while((read = getline(&line, &numBytes, *file)) != -1) 
	{
		token = strtok(line, delim);
		while(token != NULL)
		{
			if (tokenCounter % 2 == 0)
			{
				strcpy((*movieTitles)[i], token); //Must copys string because equals operator
				                                  //creates one pointer to token for every element in movieTitles.
				i++;
			}
			else
			{
				(*ticketCount)[j] = (int)strtol(token, &endptr, base);
				
				errno = 0; //distingush success/failure after call

				/* Check for various errors */
				if (errno == ERANGE && ((*ticketCount)[j] == LONG_MAX 
					|| (*ticketCount)[j] == LONG_MIN)
					|| (errno != 0 && (*ticketCount)[j] == 0))
				{
					perror("strtol");
					exit(1);
				}

				if (endptr == token)
				{
					fprintf(stderr, "No digits were found\n");
					exit(1);
				}
				
				/* If we got here, strtol successfully parsed a number */
				j++;
			}
			tokenCounter++;
			token = strtok(NULL, delim);
		}
	}


	/* free dynamic memory */
	if (token)
	{
		free(token);
		token = NULL;
	}
	if (line)
	{
		free(line);
		line = NULL;
	}
}

/********* Beginning main function *********/
int main(int argc, char **argv)
{	
	int i, j;	//Loop Counter
	int rc; 	//Error checker for threads
	void *status; 	//Status of joins
	FILE *file;	//Pointer to movies.txt file
	char **movieTitles = (char **) malloc(DEF_ARR_S*sizeof(char *)); //Char ** to hold movie titles
	int *ticketCount = (int *) malloc (DEF_ARR_S*sizeof(int)); //int * to hold ticket counts to movie titles.

	pthread_t agents[MAX_AGENTS];		// Box Office Agents threads
	pthread_t customers[MAX_CUSTOMERS];	// Customers threads
	pthread_t ticketTakers[MAX_TAKERS];	// Ticket Taker thread
	pthread_t workers[MAX_WORKERS];		// Concession Stand Worker thread

	/* Initialize all semaphores */
	initSems(&theaterOpen, 0);
	initSems(&createdAgent, 0);
	initSems(&createdTaker, 0);
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&custsem[i], 0);	
	}
	/*for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&asem[i], 0);	
	}
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&wsem[i], 0);	
	}
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&tsem[i], 0);	
	}*/

 	/* Parse movies.txt file to figure out the movie theater titles and ticket counts */	
	for (i = 0; i < DEF_ARR_S; i++) //First allocate space for array of pointers
	{
		movieTitles[i] = (char*) malloc(DEF_ARR_S*sizeof(char));
	}
	openFile(&file); //Now open and parse file
	parseFile(&file, &movieTitles, &ticketCount);

	/* First create BoxOfficeAgentsm TickerTaker, and ConcessionStandWorker threads 
	 * I would create a function for this procedure, but passing a function name as
	 * shown below is a little more tricky than I though. I was thinking function pointers
	 * would work, but am not familiar enough with them to use them yet. */
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

	/* Technically we should join all threads since a thread is an allocated resource
	 * and must be freed. There are possible memory leaks in this program
	 * due to project specification allowing main to only join customer threads */
	
	/* free dynamic memory */ 
	for (i = 0; i < DEF_ARR_S; i++)
	{
		if (movieTitles[i])
		{
			free(movieTitles[i]);
			movieTitles[i] = NULL;
		}
	}
	if (movieTitles)
	{
		free(movieTitles);
		movieTitles = NULL;
	}
	if (ticketCount);
	{
		free(ticketCount);
		ticketCount = NULL;
	}

	/* Exit main thread */
	pthread_exit(NULL);

} /********* End main function *********/
