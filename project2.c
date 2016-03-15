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
sem_t createdAgent; //Semaphore for agent creation
sem_t createdTaker; //Semaphore for ticket taker creaton
sem_t theaterOpen;  //Semaphore to signal that the theater has opened
sem_t boughtTicket[MAX_CUSTOMERS]; //Semaphore to signal that a customer has boguht a movie
sem_t ticketOrdered; //Semphore to signal that the customer has ordered a movie ticket
sem_t queueMutex1;
sem_t agentCoord;

/********* Semeaphore Queues *********/

struct agentNode //This queue represents the line for the customers at the box office
{
	int ID;
	char *movieTitle;
	struct agentNode *next;
};
struct agentNode *agentFront = NULL;
struct agentNode *agentBack = NULL;
int agentCount = 0;

/******** Global Variables *********/

int isTheaterOpen = 0;	//Used to signal if the theater is ready for business
int movieCounter = 0;	//Counts the number of movies read from movies.txt
char **movieTitles; 	//Char ** to hold movie titles
int *ticketCount; 	//int * to hold ticket counts to movie titles.
int randIdx;
int indexes[MAX_CUSTOMERS]; //Array to respresent each customer and the movie that they bought.
int shouldExit[MAX_CUSTOMERS] = {0}; //Array to signal if a customer should go home if the movie
				     //is sold out.

/********* Queue Functions *********/

/****************************************************
 * Description: enqueue places a semaphore into the
 * queue represented by a linked list of Nodes
 *
 * Parameters: ID - ID of agent
 *	       *title = title of movie the agent
 *	                is trying to order
 * Return: nothing
 ***************************************************/
void enqueueAgent(int ID, char *title)
{
	if (agentBack == NULL)
	{
		agentBack = (struct agentNode*) malloc(1*sizeof(struct agentNode));
		agentBack->next = NULL;
		agentBack->ID = ID;
	 	agentBack->movieTitle = title;
		agentFront = agentBack;
	}
	else
	{
		struct agentNode *temp = (struct agentNode*) malloc(1*sizeof(struct agentNode));
		agentBack->next = temp;
		temp->ID = ID;
		temp->movieTitle = title;
		temp->next = NULL;
		agentBack = temp;
	}

	agentCount++;
}

/****************************************************
 * Description: removes an element from the front of
 * the queue and updated the pointers accordingly
 *
 * Return: error if trying to remove from empty queue
 * or nothing
 ***************************************************/
int dequeueAgent(char **title)
{
	int id;
	struct agentNode *tempFront = (struct agentNode*) malloc(1*sizeof(struct agentNode));
	tempFront = agentFront;

	if (tempFront == NULL)
	{
		printf("Error: Trying to dequeue from empty queue!\n");
		exit(1);
	}
	else
	{
		if (tempFront->next != NULL)
		{
			*title = agentFront->movieTitle;
			id = agentFront->ID;
			tempFront = agentFront->next;
			free(agentFront);
			agentFront = tempFront;

		}
		else
		{
			*title = agentFront->movieTitle;
			id = agentFront->ID;
			free(agentFront);
			agentFront = NULL;
			agentBack = NULL;
		}
		agentCount--;
	}

	return id;
}

/********* Thread Helper Functions *********/

char* purchase(int custID)
{
	char *title = NULL;
	randIdx = rand() % (movieCounter - 1);
	indexes[custID] = randIdx;
	title = movieTitles[randIdx];
	return title;
}

void sellTicket(int agentID, char *title, int id)
{
	sleep(90/60); //Sleep for 90/60 seconds to sell the ticket witht time constraints
	if (ticketCount[indexes[id]] > 0)
	{
		printf("Box office agent %d sold ticket for %s to customer %d\n", agentID, title, id);
		(ticketCount[indexes[id]])--;
	}
	else
	{
		printf("Movie %s is sold out.\n", title);
		shouldExit[id] = 1;
	}
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
	
	char *title = purchase((int) custID);
	printf("Customer %d created, buying ticket to %s\n", (int) custID, title);

	/* Enqueue the customer so that he may communicate to the box office agent
	 * the information of his movie and his ID */
	sem_wait(&queueMutex1);
	enqueueAgent((int) custID, title);//Enqueue the customer ID to communicate to the agent
	                                  //That the customer is ordering tickets.
	sem_post(&ticketOrdered);	  //Tell the box office agent that the customer has bought a ticket
	sem_post(&queueMutex1);
	
	sem_wait(&boughtTicket[(int) custID]); //Wait for the agent to signal that the ticket has been bought.
	if (shouldExit[(int) custID])	   //Check to see if the ticket was sold out.
	{
		printf("Customer %d is going home.\n", (int) custID);
		pthread_exit(NULL);
	}

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
	int i = 0; //Loop counter

	/* Agents are created frst therefore, they do not need
	 * to wait on anyone to be created */
	printf("Box office agent %d created\n", (int) agentID);
	sem_post(&createdAgent);

	while(1)
	{
		/* Once a ticket has been ordered, let the box office agent serve the customer */
		sem_wait(&ticketOrdered);
		
		sem_wait(&queueMutex1);
		char *title;
		int id = dequeueAgent(&title); //Dequeue customer to receive his id
		sem_post(&queueMutex1);
		
		/* Allow 2 agents to sell tickets at once */
		sem_wait(&agentCoord);
		printf("Box office agent %d serving customer %d\n", (int) agentID, id);
		sellTicket((int) agentID, title, id); //Sell a ticket

		sem_post(&agentCoord); //Let the next agent know he can sell the next ticket
		sem_post(&boughtTicket[id]); //Signal to the agent that he has been processed.
	}

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
		movieCounter++; //One line cooresponds to one movie
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
	movieTitles = (char **) malloc(DEF_ARR_S*sizeof(char *)); 
	ticketCount = (int *) malloc (DEF_ARR_S*sizeof(int));

	pthread_t agents[MAX_AGENTS];		// Box Office Agents threads
	pthread_t customers[MAX_CUSTOMERS];	// Customers threads
	pthread_t ticketTakers[MAX_TAKERS];	// Ticket Taker thread
	pthread_t workers[MAX_WORKERS];		// Concession Stand Worker thread

	/* Initialize all semaphores */
	initSems(&theaterOpen, 0);
	initSems(&createdAgent, 0);
	initSems(&createdTaker, 0);
	initSems(&ticketOrdered, 0);
	initSems(&queueMutex1, 1);
	initSems(&agentCoord, 2);
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&boughtTicket[i], 0);
	}

	srand(time(NULL));  		//Seed random number generator

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
	exit(0);

} /********* End main function *********/
