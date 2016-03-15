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
#define NUM_ITEMS 4
#define ITEM_SIZE 35

/********* Global Semaphores *********/
sem_t createdAgent; //Semaphore for agent creation
sem_t createdTaker; //Semaphore for ticket taker creaton
sem_t theaterOpen;  //Semaphore to signal that the theater has opened
sem_t boughtTicket[MAX_CUSTOMERS]; //Semaphore to signal that a customer has boguht a movie
sem_t toreTicket[MAX_CUSTOMERS];
sem_t orderTaken[MAX_CUSTOMERS];
sem_t ticketOrdered; //Semphore to signal that the customer has ordered a movie ticket
sem_t fOrderReady;
sem_t aMutex, tMutex, sMutex;
sem_t agentCoord;
sem_t ticketReady;

/********* Semeaphore Queues *********/

struct agentNode //This queue represents the line for the customers at the box office
{
	int ID;
	char *movieTitle;
	struct agentNode *next;
};
struct agentNode *aFront = NULL;
struct agentNode *aBack = NULL;
int aCount = 0;

struct takerNode //This queue represents the line for the customers at the ticket taker
{
	int ID;
	struct takerNode *next;
};
struct takerNode *tFront = NULL;
struct takerNode *tBack = NULL;
int tCount = 0;

struct standNode //This queue represents the line for the customers at the concession stand
{
	int ID;
	char *food;
	struct standNode *next;
};
struct standNode *sFront = NULL;
struct standNode *sBack = NULL;
int sCount = 0;

/******** Global Variables *********/

int isTheaterOpen = 0;	//Used to signal if the theater is ready for business
int movieCounter = 0;	//Counts the number of movies read from movies.txt
char **movieTitles; 	//Char ** to hold movie titles
int *ticketCount; 		//int * to hold ticket counts to movie titles.
int indexes[MAX_CUSTOMERS]; 	//Array to respresent each customer and the movie that they bought.
int shouldExit[MAX_CUSTOMERS]; 	//Array to signal if a customer should go home if the movie
								//is sold out.

/********* Queue Functions *********/

/****************************************************
 * Description: Adds a enw customer to the queue at
 * the box office so that the next box office agent
 * may serve them
 *
 * Parameters: ID - ID of customer
 *	       *title = title of movie the customer
 *	                is trying to order
 * Return: nothing
 ***************************************************/
void enqueueAgent(int ID, char *title)
{
	if (aBack == NULL)
	{
		aBack = (struct agentNode*) malloc(1*sizeof(struct agentNode));
		aBack->next = NULL;
		aBack->ID = ID;
	 	aBack->movieTitle = title;
		aFront = aBack;
	}
	else
	{
		struct agentNode *temp = (struct agentNode*) malloc(1*sizeof(struct agentNode));
		aBack->next = temp;
		temp->ID = ID;
		temp->movieTitle = title;
		temp->next = NULL;
		aBack = temp;
	}

	aCount++;
}

/****************************************************
 * Description: removes an element from the front of
 * the queue and updated the pointers accordingly
 * 
 * Parameters: **title - reference to movie title that
 * a customer is trying to watch
 *
 * Return: error if dequeueing from an empty queue or
 * returns the id of the customer if successful
 ***************************************************/
int dequeueAgent(char **title)
{
	int id;
	struct agentNode *tempFront = (struct agentNode*) malloc(1*sizeof(struct agentNode));
	tempFront = aFront;

	if (tempFront == NULL)
	{
		printf("Error: Trying to dequeue from empty queue!\n");
		exit(1);
	}
	else
	{
		*title = aFront->movieTitle;
		id = aFront->ID;
			
		if (tempFront->next != NULL)
		{
			tempFront = aFront->next;
			free(aFront);
			aFront = tempFront;

		}
		else
		{
			free(aFront);
			aFront = NULL;
			aBack = NULL;
		}
		aCount--;
	}

	return id;
}

/****************************************************
 * Description: Adds a new customer to the queue at
 * the ticket taker so that the ticket taker
 * may serve them
 *
 * Parameters: ID - ID of customer
 *	       
 * Return: nothing
 ***************************************************/
void enqueueTaker(int ID)
{
	if (tBack == NULL)
	{
		tBack = (struct takerNode*) malloc(1*sizeof(struct takerNode));
		tBack->next = NULL;
		tBack->ID = ID;
		tFront = tBack;
	}
	else
	{
		struct takerNode *temp = (struct takerNode*) malloc(1*sizeof(struct takerNode));
		tBack->next = temp;
		temp->ID = ID;
		temp->next = NULL;
		tBack = temp;
	}

	tCount++;
}

/****************************************************
 * Description: removes an element from the front of
 * the queue and updated the pointers accordingly
 *
 * Return: error if dequeueing from an empty queue or
 * returns the id of the customer if successful
 ***************************************************/
int dequeueTaker()
{
	int id;
	struct takerNode *tempFront = (struct takerNode*) malloc(1*sizeof(struct takerNode));
	tempFront = tFront;

	if (tempFront == NULL)
	{
		printf("Error: Trying to dequeue from empty queue!\n");
		exit(1);
	}
	else
	{
		id = tFront->ID;

		if (tempFront->next != NULL)
		{
			tempFront = tFront->next;
			free(tFront);
			tFront = tempFront;

		}
		else
		{
			free(tFront);
			tFront = NULL;
			tBack = NULL;
		}
		tCount--;
	}

	return id;
}

/****************************************************
 * Description: Adds a new customer to the queue at
 * the concessions stand worker so that the worker
 * may serve them
 *
 * Parameters: ID - ID of customer
 * 				*food - name of food item the customer
 *				is ordering
 *	       
 * Return: nothing
 ***************************************************/
void enqueueStand(int ID, char *food)
{
	if (sBack == NULL)
	{
		sBack = (struct standNode*) malloc(1*sizeof(struct standNode));
		sBack->next = NULL;
		sBack->ID = ID;
		sBack->food = food;
		sFront = sBack;
	}
	else
	{
		struct standNode *temp = (struct standNode*) malloc(1*sizeof(struct standNode));
		sBack->next = temp;
		temp->ID = ID;
		temp->food = food;
		temp->next = NULL;
		sBack = temp;
	}

	sCount++;
}

/****************************************************
 * Description: removes an element from the front of
 * the queue and updated the pointers accordingly
 *
 * Parameters: **food - reference of food item the 
 * customer  is trying to order.
 *
 * Return: error if dequeueing from an empty queue or
 * returns the id of the customer if successful
 ***************************************************/
int dequeueStand(char **food)
{
	int id;
	struct standNode *tempFront = (struct standNode*) malloc(1*sizeof(struct standNode));
	tempFront = sFront;

	if (tempFront == NULL)
	{
		printf("Error: Trying to dequeue from empty queue!\n");
		exit(1);
	}
	else
	{
		id = sFront->ID;
		*food = sFront->food;
		
		if (tempFront->next != NULL)
		{
			tempFront = sFront->next;
			free(sFront);
			sFront = tempFront;

		}
		else
		{
			free(sFront);
			sFront = NULL;
			sBack = NULL;
		}
		sCount--;
	}

	return id;
}

/********* Thread Helper Functions *********/

/****************************************************
 * Description: Used to implement functionality of
 * a customer buying a ticket to a movie
 *
 * Return: title - movie title
 ***************************************************/
char* purchase(int custID)
{
	char *title = NULL;
	int randIdx = rand() % (movieCounter - 1);
	indexes[custID] = randIdx;
	title = movieTitles[randIdx];
	return title;
}

/****************************************************
 * Description: Used to implement functionality of
 * a box office agent selling a movie ticket to a
 * customer. This functions takes 90/60 seconds to
 * complete.
 *
 * Return: nothing
 ***************************************************/
void sellTicket(int agentID, char *title, int id)
{
	sleep(90/60); //Sleep for 90/60 seconds to sell the ticket with time constraints
	if (ticketCount[indexes[id]] > 0)
	{
		printf("Box office agent %d sold ticket for %s to customer %d\n", agentID, title, id);
		(ticketCount[indexes[id]])--;
	}
	else
	{
		printf("Movie %s is sold out. Customer %d is going home.\n", title, id);
		shouldExit[id] = 1;
	}
}

/****************************************************
 * Description: Functionality of ticket taker taking
 * a customer's movie ticket and tearing it
 *
 * Return: nothing
 ***************************************************/
void tearTicket(int id)
{
	sleep(15/60);
	printf("Ticket taken from customer %d.\n", id);
}

/****************************************************
 * Description: Calculates a random chance that a
 * customer will enter the concession stand
 *
 * Return: random number either 0 or 1
 ***************************************************/
int calcStandChance()
{
	return (int)rand() % 2;
}

/****************************************************
 * Description: Functionality of customer ordering
 * food at the concession stand
 *
 * Return: food - food item the customer ordered
 ***************************************************/
char* enterCStand(int id)
{
	int idx = (int)rand() % NUM_ITEMS;
	char *food;
	
	switch(idx)
	{
		case 0:
		{
			food = "Popcorn";
			break;
		}
		case 1:
		{
			food = "Candy";
			break;
		}
		case 2:
		{
			food = "Hotdog";
			break;
		}
		case 3:
		{
			food = "Burger";
			break;
		}
		case 4:
		{
			food = "Cornydog";
			break;
		}
	}
	printf("Customer %d in line to buy %s.\n", id, food);
	
	/* Check to see if freeing memory here later will mess up the program */
	
	return food;
}

/****************************************************
 * Description: Functionality of the concession stand
 * worker taking a customers order and filling it out
 *
 * Return: nothing
 ***************************************************/
void fillOrder(int id, char *food)
{
	sleep(180/60);
	printf("%s given to customer %d\n", food, id);
}


/********* Thread Functions *********/

/****************************************************
 * Description: Customer thread function to execute
 * the actions of a customer as it goes through the
 * theater simulation
 *
 * Parameters: *custID - ID of thread or customer
 *
 * Return: pthread_exit(NULL)
 ***************************************************/
void* Customer(void *custID)
{
	/* Wait for the theater to be open, the first customer who sees that it is
	 * open will output that it is open. Afterwards, it will signal to the next
	 * blocked customer that the theater is now open */
	int id = (int) custID; 
	sem_wait(&theaterOpen);
	
	if (!isTheaterOpen)
	{
		isTheaterOpen = 1;
		printf("The theater is now open!\n");
	}
	
	sem_post(&theaterOpen);	
	
	char *title = purchase(id);
	printf("Customer %d created, buying ticket to %s\n", id, title);

	/* Enqueue the customer so that he may communicate to the box office agent
	 * the information of his movie and his ID */
	sem_wait(&aMutex);
	enqueueAgent(id, title);	//Enqueue the customer ID to communicate to the agent
								//That the customer is ordering tickets.
	sem_post(&ticketOrdered);	//Tell the box office agent that the customer has bought a ticket
	sem_post(&aMutex);
	
	sem_wait(&boughtTicket[id]); 	//Wait for the agent to signal that the ticket has been bought.
	if (shouldExit[id])	   	   		//Check to see if the ticket was sold out.
	{
		printf("Customer %d is going home.\n", id);
		pthread_exit(NULL);
	}
	
	/* Now the customer is going to enter the queue for the ticket taker */
	sem_wait(&tMutex);
	enqueueTaker(id);
	printf("Customer %d in line to see ticket taker.\n", id);
	sem_post(&ticketReady);
	sem_post(&tMutex);
	
	sem_wait(&toreTicket[id]);
	
	/*The customer now either sees their movie or visits the concession stand
	 with a probabilistic chance */
	 int chance = calcStandChance();
	 
	 if (chance == 0)
	 {
		 char *food = enterCStand(id);
		 
		 sem_wait(&sMutex);
		 enqueueStand(id, food);
		 sem_post(&fOrderReady);
		 sem_post(&sMutex);
		 
		 sem_wait(&orderTaken[id]);
	 }
	 
	 printf("Customer %d enters the theater to see %s.\n", id, title);

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
		
		sem_wait(&aMutex);
		char *title;
		int id = dequeueAgent(&title); //Dequeue customer to receive his id
		sem_post(&aMutex);
		
		/* Allow 2 agents to sell tickets at once */
		sem_wait(&agentCoord);
		printf("Box office agent %d serving customer %d\n", (int) agentID, id);
		sellTicket((int) agentID, title, id); //Sell a ticket

		sem_post(&agentCoord); 		 //Let the next agent know he can sell the next ticket
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
	
	while(1)
	{
		/*The ticket taker is going to wait for a customer to be ready */
		sem_wait(&ticketReady);
		
		sem_wait(&tMutex);
		char *food;
		int id = dequeueTaker(&food); //Dequeue customer to receive his id and food
		sem_post(&tMutex);
		tearTicket(id);
		
		sem_post(&toreTicket[id]);
	}
	
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
	
	while(1)
	{
		sem_wait(&fOrderReady);
		
		sem_wait(&sMutex);
		char *food;
		int id = dequeueStand(&food);
		sem_post(&sMutex);
		
		printf("Order for %s taken from customer %d\n", food, id);
		fillOrder(id, food);
		sem_post(&orderTaken[id]);
	}
	
	pthread_exit(NULL);
}

/********* Functions *********/

/****************************************************
 * Description: initializes a sempahore to a given value 
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
 * Description: initializes all semaphores with use
 * of the helper function initSems()
 * 
 * Return: nothing
 ***************************************************/
void initAllSems()
{
	int i;
	initSems(&theaterOpen, 0);
	initSems(&createdAgent, 0);
	initSems(&createdTaker, 0);
	initSems(&ticketOrdered, 0);
	initSems(&aMutex, 1);
	initSems(&tMutex, 1);
	initSems(&sMutex, 1);
	initSems(&agentCoord, 2);
	initSems(&fOrderReady, 0);
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&orderTaken[i], 0);
	}
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&toreTicket[i], 0);
	}
	for (i = 0; i < MAX_CUSTOMERS; i++)
	{
		initSems(&boughtTicket[i], 0);
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
	int i, j;		//Loop Counter
	int rc; 		//Error checker for threads
	void *status; 	//Status of joins
	FILE *file;		//Pointer to movies.txt file
	movieTitles = (char **) malloc(DEF_ARR_S*sizeof(char *)); 
	ticketCount = (int *) malloc (DEF_ARR_S*sizeof(int));

	pthread_t agents[MAX_AGENTS];		// Box Office Agents threads
	pthread_t customers[MAX_CUSTOMERS];	// Customers threads
	pthread_t ticketTakers[MAX_TAKERS];	// Ticket Taker thread
	pthread_t workers[MAX_WORKERS];		// Concession Stand Worker thread
	
	/* Initialize all semaphores */
	initAllSems();
	
	srand(time(NULL));  		//Seed random number generator

 	/* Parse movies.txt file to figure out the movie theater titles and ticket counts */	
	for (i = 0; i < DEF_ARR_S; i++) //First allocate space for array of pointers
	{
		movieTitles[i] = (char*) malloc(DEF_ARR_S*sizeof(char));
	}
	
	openFile(&file); //Now open and parse file
	parseFile(&file, &movieTitles, &ticketCount);

	/* First create BoxOfficeAgentsm TickerTaker, and ConcessionStandWorker threads 
	 * I would create a function for this procedure, but have been lazy */
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
