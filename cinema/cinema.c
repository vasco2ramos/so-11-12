#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <cinema.h>
#include <buffer.h>
#include <server.h>

#define RESERVA 1
#define PRERESERVA 2
#define CONFIRMA 3
#define CANCELA 4
#define MOSTRACINEMA 5

sthread_sem_t buffer_sem;

int init_cinema(unsigned int num_rows, unsigned int num_cols, int num_server_threads, int buf_size) {
	// Variáveis necessárias
	int i, ids[num_server_threads], timeslice = 300;
	buffer_sem = sthread_sem_init(buf_size);
	// Inicializa x threads para o servidor
	sthread_init(&timeslice);
	sthread_t thr[num_server_threads];
	
	// Inicializa mutex buffer
	buffer_lock = sthread_mutex_init();
	current_request=-1;
	
	// Inicializa a sala de cinema
	init_movie_theatre(num_rows,num_cols);
	
	// Inicializa o buffer
	buffer_lock = sthread_mutex_init();
	initialize_buffer(buf_size);
	for (i=0; i<num_server_threads; i++) {
		thr[i]=sthread_create(server_thread,(void*)ids[i], 1, 0);
		if (thr[i] == NULL) {
			printf("sthread_create failed\n");
			exit(-1);
		}
		sthread_yield();
	}
	return -1;
}

/* Makes a reservation code 1 */
int reserva(Coords_t* seats,int dim) {
	sthread_sem_wait(buffer_sem);
	// Variáveis necessárias
	int answer, params[4];
	sthread_sem_t sem = sthread_sem_init(0);
	request *req;
	// Prepara o pedido mensagem
	params[0]=dim;
	// Envia para o buffer o pedido
	req = new_request(RESERVA,params, seats, &sem,&answer);
	sthread_mutex_lock(buffer_lock);
	current_request++;
	request_buffer[current_request]=req;
	sthread_mutex_unlock(buffer_lock);
	// Espera pela resposta
	sthread_sem_wait(sem);
	// Sinaliza o buffer dos pedidos
	sthread_sem_post(buffer_sem);
	// devolve o resultado do pedido enviado para o buffer
	return answer;
}

int prereserva(Coords_t* seats,int dim, int segundos) {
	sthread_sem_wait(buffer_sem);
	// Prepara o pedido mensagem
	int answer, params[4];
	sthread_sem_t sem = sthread_sem_init(0);
	params[0]=dim;
	params[1]=segundos;
	// Envia para o buffer o pedido
	request *req = new_request(PRERESERVA,params, seats, &sem,&answer);
	sthread_mutex_lock(buffer_lock);
	current_request++;
	request_buffer[current_request]=req;
	sthread_mutex_unlock(buffer_lock);
	// Espera pela resposta
	sthread_sem_wait(sem);
	// Sinaliza o buffer dos pedidos
	sthread_sem_post(buffer_sem);
	// devolve o resultado do pedido enviado para o buffer
	return answer;
}

int confirma_prereserva(int reservation_id){
	sthread_sem_wait(buffer_sem);
	// Prepara o pedido mensagem
	int answer;
	sthread_sem_t sem = sthread_sem_init(0);
	int params[4];
	params[2]=reservation_id;
	// Envia para o buffer o pedido
	request *req = new_request(CONFIRMA,params, NULL, &sem,&answer);
	sthread_mutex_lock(buffer_lock);
	current_request++;
	request_buffer[current_request]=req;
	sthread_mutex_unlock(buffer_lock);
	// Espera pela resposta
	sthread_sem_wait(sem);
	// Sinaliza o buffer dos pedidos
	sthread_sem_post(buffer_sem);
	// devolve o resultado do pedido enviado para o buffer
	return answer;
}


int cancela(int reservation_id){
	sthread_sem_wait(buffer_sem);
	// Prepara o pedido mensagem
	int answer, params[4];
	sthread_sem_t sem = sthread_sem_init(0);
	params[2]=reservation_id;
	// Envia para o buffer o pedido
	request *req = new_request(CANCELA,params, NULL, &sem,&answer);
	sthread_mutex_lock(buffer_lock);
	current_request++;
	request_buffer[current_request]=req;
	sthread_mutex_unlock(buffer_lock);
	// Espera pela resposta
	sthread_sem_wait(sem);
	// Sinaliza o buffer dos pedidos
	sthread_sem_post(buffer_sem);
	// devolve o resultado do pedido enviado para o buffer
	return answer;
}


void mostra_cinema() {
	sthread_sem_wait(buffer_sem);
	// Envia para o buffer o pedido
	sthread_sem_t sem = sthread_sem_init(0);
	request *req = new_request(MOSTRACINEMA,NULL, NULL, &sem,NULL);
	sthread_mutex_lock(buffer_lock);
	current_request++;
	request_buffer[current_request]=req;
	sthread_mutex_unlock(buffer_lock);
	sthread_sem_wait(sem);
	// Sinaliza o buffer dos pedidos
	sthread_sem_post(buffer_sem);
	return;	
}
