#include <stdio.h>
#include <stdlib.h>
#include <server.h>
#include <buffer.h>
#include <string.h>
#include <cinema_map.h>
#define PADDING 4

int N_ROWS, N_COLS, reservation_id;

void init_movie_theatre(int rows, int cols){
	// Variáveis Necessárias
	int i, i2; 
	reservation_id=0;
	// Store number of rows and cols
	N_ROWS = rows;
	N_COLS = cols;
	// Alocate room for movie theathre
	init_backend_cinema(rows,cols);
	movie_theatre = (sthread_mutex_t **)malloc(rows * sizeof(sthread_mutex_t*));
	for(i = 0; i < rows; i++){
		movie_theatre[i] = (sthread_mutex_t *)malloc(cols * sizeof(sthread_mutex_t));
		for(i2 = 0; i2< cols; i2++){
			movie_theatre[i][i2] = sthread_mutex_init();
		}
	}
}

void reserva_s(request *req){
	int i, i2, reserved = 0;
	int *pms;
	Coords_t seat;
	status_t *status;
	pms = req->params;
	// - Reserva
	for(i=0;i<pms[0];i++){
		seat = req->seats[i];
		sthread_mutex_lock(movie_theatre[seat.row][seat.column]);
		status = backend_get_state(seat.row, seat.column);
		if(status->state == 0){
			backend_reserva(seat.row, seat.column, reservation_id);
			reserved = 1;
		}
		else if(status->state==1 && difftime(time(NULL), status->exp_time)<0){
			// Pre Reservation has expired
			backend_reserva(seat.row, seat.column, reservation_id);
			reserved = 1;
		}
		else{
			// Backtrack and say its impossible to make reservation
			for(i2=0; i2<i; i2++){
				seat = req->seats[i];
				sthread_mutex_lock(movie_theatre[seat.row][seat.column]);
				backend_cancela(seat.row,seat.column);
				sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
			}
			sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
			*(req->response) = -1;
			reserved = 0;
		}
		sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
	}
	if(reserved == 1){
		*(req->response) = reservation_id;
		reservation_id++;
	}
	// Let the client know the answer is ready
	sthread_sem_post(*(req->semaphore));
}

void prereserva_s(request *req){
	
	int i, i2, reserved = 0;
	int *pms;
	Coords_t seat;
	status_t *status;
	pms = req->params;
	// - Reserva
	
	for(i=0;i<pms[0];i++){
		seat = req->seats[i];
		sthread_mutex_lock(movie_theatre[seat.row][seat.column]);
		status = backend_get_state(seat.row, seat.column);
		if(status->state == 0){
			backend_prereserva(seat.row, seat.column, time(NULL)+pms[1],reservation_id);
			reserved = 1;
		}
		else if(status->state==1 && difftime(time(NULL), status->exp_time)<0){
			// Pre Reservation has expired
			backend_reserva(seat.row, seat.column, reservation_id);
			reserved = 1;
		}
		else{
			// Backtrack and say its impossible to make reservation
			for(i2=0; i2<i; i2++){
				seat = req->seats[i];
				sthread_mutex_lock(movie_theatre[seat.row][seat.column]);
				backend_cancela(seat.row,seat.column);
				sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
			}
			sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
			reserved = 0;
		}
		sthread_mutex_unlock(movie_theatre[seat.row][seat.column]);
	}
	if(reserved == 1){
		*(req->response) = reservation_id;
		reservation_id++;
	}
	else{
		*(req->response) = -1;
	}
	// Let the client know the answer is ready
	sthread_sem_post(*(req->semaphore));
}

void confirma_s(request *req){
	int i, i2, done = 0;
	int *pms;
	status_t *status;
	pms = req->params;
	for(i = 0;i<N_ROWS;i++){
		for(i2=0;i2<N_COLS;i2++){
			sthread_mutex_lock(movie_theatre[i][i2]);
			status = backend_get_state(i,i2);
			if(status->id == pms[2]){
				backend_reserva(i,i2,status->id);
				done = 1;
			}
			sthread_mutex_unlock(movie_theatre[i][i2]);
		}
	}
	if(done==1){
		*(req->response) = 0;
	}
	else{
		*(req->response) = -1;
	}
	// Let the client know the answer is ready
	sthread_sem_post(*(req->semaphore));
}

void cancela_s(request *req){
	int i, i2, done = 0;
	int *pms;
	status_t *status;
	pms = req->params;
	for(i = 0;i<N_ROWS;i++){
		for(i2=0;i2<N_COLS;i2++){
			status = backend_get_state(i,i2);
			sthread_mutex_lock(movie_theatre[i][i2]);
			if(status->id == pms[2]){
				backend_cancela(i,i2);
				done = 1;
			}
			sthread_mutex_unlock(movie_theatre[i][i2]);
		}
	}
	if(done==1){
		*(req->response) = 0;
	}
	else{
		*(req->response) = -1;
	}
	// Let the client know the answer is ready
	sthread_sem_post(*(req->semaphore));
}

void mostra_cinema_s(request *req){
	int i, j, w, qty=0, did=0;
	int res=0;
	int preres=0;
	int expired=0; // expired prereservations
	char seat[5];
	char *seats = malloc(20 * sizeof(char));
	//print map state
  	for (j=0; j<N_COLS*(PADDING+1); j++) 
		printf("-");

	printf("\n");


	for(i = 0;i<N_ROWS;i++) {
		printf("|");
		for (j=0; j<N_COLS; j++) {
		status_t* s=backend_get_state(i,j);
		char c='E'; //'E' is used to encode any unexpected status returned by the back_end (note this should never happen!)
		switch (s->state) {
			case 0: c='F'; break;
			case 1: 
				if(difftime(time(NULL), s->exp_time)>0)
					c='F';
				else
					c='P'; 
				break;
			case 2: c='R'; break;
		}
			printf("%c|",c);
   		}
	printf("\n");
	}

  	for (j=0; j<N_COLS*(PADDING+1); j++) 
		printf("-");

	printf("\n");

	//print map state - including res_id in case seat is (pre-)reserved

	for(i = 0;i<N_ROWS;i++) {
		printf("<");
		for (j=0; j<N_COLS; j++) {
		status_t* s=backend_get_state(i,j);
		char c='E'; //'E' is used to encode any unexpected status returned by the back_end (note this should never happen!)
		switch (s->state) {
			case 0: c='F';break;
			case 1: c='P';
				if(difftime(time(NULL), s->exp_time)>0)
					c='F';
				else
					c='P'; 
				break;
			case 2: c='R';break;
		}
			printf("%c:%3d|",c,s->id);
   		}
	printf("\n");
	}
	for(i = 0;i<N_ROWS;i++) {
		for (j=0; j<N_COLS; j++) {
			status_t* s=backend_get_state(i,j);
			switch (s->state) {
				case 1:
					if(difftime(time(NULL), s->exp_time)>0)
						expired++;
					else
						preres++;
				break;
				case 2: 
					res++; 
				break;
			}
		}
	}
	
	
	printf("#res:%d\n",res);
	printf("#preres:%d\n",preres);
	printf("#expired:%d\n",expired);

	printf("-Reservations:\n");
	while(res >0){
		for(w=0;w<reservation_id;w++){
			for(i = 0;i<N_ROWS;i++) {
				for (j=0; j<N_COLS; j++) {
					status_t* s=backend_get_state(i,j);
					if(s->id == w && s->state == 2) {
						qty++;
						sprintf(seat,"[%d,%d]", i, j);
						seats = strncat(seats, seat, sizeof(seat));
						res--;
					}
				}
			}
			if(qty>0){
				 printf("{ Res:%d #seats:%d - %s }\n", w, qty, seats);
				 qty = 0;
				 did=1;
			}
			free(seats);
			seats = malloc(20 * sizeof(char));
		}
	}
	if(did!=1)
		printf("Empty list.\n");
	else
		did = 0;

	printf("-Prereservations:\n");
	while(preres>0){
		for(w=0;w<reservation_id;w++){
			for(i = 0;i<N_ROWS;i++) {
				for (j=0; j<N_COLS; j++) {
					status_t* s=backend_get_state(i,j);
					if(s->id == w && s->state == 1 && difftime(time(NULL), s->exp_time)<0) {
						printf("{ Preres:%d ",s->id);
						qty++;
						sprintf(seat,"[%d,%d]", i, j);
						seats = strncat(seats, seat, sizeof(seat));
						preres--;
					}
				}
			}
			if(qty>0){
				 printf("#seats:%d - %s }\n", qty, seats);
				 qty = 0;
				 did=1;
			}
			free(seats);
			seats = malloc(20 * sizeof(char));
		}
	}
	if(did!=1)
		printf("Empty list.\n");
	else
		did = 0;
	free(seats);
	sthread_sem_post(*(req->semaphore));
		
}

void server_thread(){
	// Variáveis necessárias
	int rc;
	request *req = NULL;
	while(1){
		
		// espera alguma coisa na fila
		sthread_mutex_lock(buffer_lock);
		if(current_request >= 0){
			req = request_buffer[current_request];
			current_request--;
		}
		sthread_mutex_unlock(buffer_lock);
		if(req != NULL){
			// processa o pedido
			rc = req->request_code;
			if(rc==1){
				// - Reserva
				reserva_s(req);
			}
			else if(rc==2){
				// - Pre Reserva
				prereserva_s(req);
			}
			else if(rc==3){
				// - Confirma Reserva
				confirma_s(req);
			}
			else if(rc==4){
				// - Cancela Reserva
				cancela_s(req);
			}
			else if(rc==5){
				// - Mostra Cinema
				mostra_cinema_s(req);
			}
			else{
			}
			free(req);
			req = NULL;	
		}
	}
}
