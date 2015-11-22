#include <buffer.h>
#include <stdlib.h>
#include <stdio.h>
#include <sthread.h>


void initialize_buffer(int max){
	request_buffer = (request **)malloc(max * sizeof(request*));
}


request* new_request(int code, int *parameters, Coords_t *seats, sthread_sem_t *semaphore, int *res){
	request *req;
	req = (request*) malloc(sizeof(request));
	req->request_code = code;
	req->params = parameters;
	req->semaphore = semaphore;
	req->seats = seats;
	req->response = res;
	return req;
}
