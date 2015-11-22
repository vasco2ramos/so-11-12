#ifndef BUFFER_H
#define BUFFER_H 1

#include <sthread.h>
#include <cinema.h>

typedef struct request{
	/* request code 
	 * 1-reserva 
	 * 2-pre-reserva 
	 * 3-confirma reserva 
	 * 4-cancela 
	 * 5-mostra cinema
	 * */
	int request_code;
	/* params some may be empty array
	 * 0-dimension
	 * 1-seconds
	 * 2-reservation id
	 */
	int *params;
	// Seats array - may be empty
	Coords_t *seats;
	// Semaphore to wait for response
	sthread_sem_t *semaphore;
	// response
	int *response;
	// next buffer item<s
	struct request *next;
} request;

sthread_mutex_t buffer_lock;
request **request_buffer;
int current_request;

/* new_request - allocates memory for request and initializes it */
request* new_request(int code, int *parameters, Coords_t *seats, sthread_sem_t *semaphore, int *response);

void initialize_buffer(int max);
#endif
