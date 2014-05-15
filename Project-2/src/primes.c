#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "CircularQueue.h"

int DEBUG_MODE = 0;
int QUEUE_SIZE = 10;

int writeIndex;
unsigned long* primesList;
int primesListSize;

long n;
sem_t termSem;
pthread_mutex_t primesListAccessControlMutex;

int compare(const void * a, const void * b) {
	return *(unsigned long*) a - *(unsigned long*) b;
}

void addNumToPrimesList(int num) {
	pthread_mutex_lock(&primesListAccessControlMutex);

	primesList[writeIndex++] = num;
	primesListSize++;

	if (DEBUG_MODE)
		printf("Added %d to primes list.\n", num);

	pthread_mutex_unlock(&primesListAccessControlMutex);
}

void printPrimesList() {
	int i;
	for (i = 0; i < primesListSize; i++)
		printf("%ld ", primesList[i]);
	printf("\n");
}

void* filterThread(void* arg) {
	CircularQueue* inputCircularQueue = arg;

	QueueElem num = queue_get(inputCircularQueue);
	if (num > sqrt(n)) {
		do {
			// adding num to primes list
			addNumToPrimesList(num);

			// reading next element from the circular queue
			num = queue_get(inputCircularQueue);
		} while (num != 0);

		// signaling termination semaphore
		if (DEBUG_MODE)
			printf("Signaling semaphore: num > sqrt(n)\n");
		sem_post(&termSem);
	} else {
		int temp = num;

		// creating output circular queue
		CircularQueue* outputCircularQueue;
		queue_init(&outputCircularQueue, QUEUE_SIZE);

		// creating filter thread
		pthread_t ft;
		pthread_create(&ft, NULL, filterThread, outputCircularQueue);

		do {
			// reading next element from the circular queue
			num = queue_get(inputCircularQueue);

			if (num % temp != 0)
				queue_put(outputCircularQueue, num);
		} while (num != 0);

		// adding first number to primes list
		addNumToPrimesList(temp);

		// placing num (that by now should be equal to zero) at the end of the queue to terminate the sequence
		queue_put(outputCircularQueue, num);
	}

	// destroying input circular queue
	queue_destroy(inputCircularQueue);

	return NULL ;
}

void* initThreadFunc(void* arg) {
	if (DEBUG_MODE)
		printf("> Starting initial thread\n");

	// adding number 2 to primes list
	addNumToPrimesList(2);

	if (n > 2) {
		// creating output circular queue
		CircularQueue* outputCircularQueue;
		queue_init(&outputCircularQueue, QUEUE_SIZE);

		// creating filter thread
		pthread_t ft;
		pthread_create(&ft, NULL, filterThread, outputCircularQueue);

		// placing odd numbers in the output circular queue
		int i;
		for (i = 3; i <= n; i += 2)
			queue_put(outputCircularQueue, i);

		// placing zero at the end of the queue to terminate the sequence
		queue_put(outputCircularQueue, 0);
	} else {
		// signaling termination semaphore
		if (DEBUG_MODE)
			printf("Signaling semaphore: n = 2\n");
		sem_post(&termSem);
	}

	return NULL ;
}

int main(int argc, char** argv) {
	char* pEnd;

	// validating number of arguments
	if (argc <= 1) {
		printf("\n");
		printf("Wrong number of arguments.\n");
		printf("Usage: primes <n>\n");
		printf("Usage: primes <n> <queue size>\n");
		printf("Usage: primes <n> <queue size> <debug mode>\n");
		printf("\n");

		return -1;
	}

	// processing and validating <debug mode> paramater
	if (argc == 4) {
		DEBUG_MODE = strtol(argv[3], &pEnd, 10);
		if (argv[3] == pEnd || (DEBUG_MODE != 0 && DEBUG_MODE != 1)) {
			printf("\n");
			printf("Invalid argument.\n");
			printf("<debug mode> must be 0 or 1.\n");
			printf("\n");

			return -1;
		}

		if (DEBUG_MODE)
			printf("DEBUG_MODE value overrided to: %d\n", DEBUG_MODE);
	}

	// processing and validating <queue size> paramater
	if (argc >= 3) {
		QUEUE_SIZE = strtol(argv[2], &pEnd, 10);
		if (argv[2] == pEnd || QUEUE_SIZE <= 0) {
			printf("\n");
			printf("Invalid argument.\n");
			printf("<queue size> must be an integer greater than 0.\n");
			printf("\n");

			return -1;
		}

		if (DEBUG_MODE)
			printf("QUEUE_SIZE value overrided to: %d\n", QUEUE_SIZE);
	}

	// processing and validating <n> paramater
	n = strtol(argv[1], &pEnd, 10);
	if (argv[1] == pEnd || n < 2) {
		printf("\n");
		printf("Invalid argument.\n");
		printf("<n> must be an integer greater than or equal to 2.\n");
		printf("\n");

		return -1;
	}

	// displaying received data info
	if (DEBUG_MODE) {
		printf("---------------\n");
		printf("Primes info:\n");
		printf("n: %ld\n", n);
		printf("queue size: %d\n", QUEUE_SIZE);
		printf("debug mode: %d\n", DEBUG_MODE);
		printf("---------------\n");
	}

	// initializing shared data
	writeIndex = 0;
	int allocationSize = 1.2 * n / log(n);
	primesList = (unsigned long*) malloc(allocationSize * sizeof(unsigned long));
	primesListSize = 0;

	// initializing termination semaphore
	if (sem_init(&termSem, 0, 0) != 0)
		return -1;

	// initializing primes list access control mutex
	if (pthread_mutex_init(&primesListAccessControlMutex, NULL) != 0)
		return -1;

	// starting initial thread
	pthread_t initThread;
	pthread_create(&initThread, NULL, initThreadFunc, NULL);

	// waiting for termination semaphore
	sem_wait(&termSem);

	// sorting primes list
	if (DEBUG_MODE)
		printf("Sorting primes list\n");
	qsort(primesList, primesListSize, sizeof(unsigned long), compare);

	// displaying primes list
	printf("\n");
	printf("The list of primes between 2 and %ld is:\n", n);
	printPrimesList();
	printf("\n");

	if (DEBUG_MODE)
		printf("Program Terminated.\n");
	return 0;
}
