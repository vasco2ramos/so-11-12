#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <cinema.h>	
#define NUM_ROWS 10
#define NUM_COLUMNS 10
#define NUM_CLIENT_THREADS 1
#define BUFFER_SIZE 100
#define NUM_S_THREADS 300
#define MAX_SEATS 3
#define NUM_REQS 1
#define MAX_ATT 200


Coords_t* rand_seats(int size) {
	int i=0;
	int start_c=rand(),start_r=rand();

	Coords_t* c=(Coords_t*) malloc(sizeof(Coords_t)*size);
	for (;i<size;i++) {
		c[i].row=(start_r++)%NUM_ROWS;
		c[i].column=(start_c++)%NUM_COLUMNS;
	}
	return c;
}


void *client_thread(void *arg) {

	int cid=(int)arg;

	int counter=0;
	int i,ret,reserve=0;
	int success=0,failure=0;
	while (counter++<NUM_REQS) {

		int dim=rand()%MAX_SEATS;
		if (dim==0) dim=1;

		Coords_t* c;
		reserve=0;
		int id=-1;

		if ((rand()%100)<50)
			reserve=1;

		c=rand_seats(dim);

		printf("#C%d submitting %sreservation of %d seats\n", cid,reserve?"":"pre", dim);
		for (i=0;i<dim;i++) {
			printf("(%d,%d) ",c[i].row,c[i].column);
		}
		printf("\n");

		if (reserve) {
			ret=reserva(c,dim);
			(ret==-1?failure++:success++);
			if (ret!=-1)
				id=ret;
			}
		else	{
			ret=prereserva(c,dim,100);
			(ret==-1?failure++:success++);
			if (ret!=-1)
				id=ret;
			}

		mostra_cinema();

		printf("Client_thread #C%d received res_id:%d\n", id,ret);

		if (!reserve && id!=-1 && (rand()%100)<100) {
			#ifdef DEBUG			
			printf("#C%d confirming preres %d\n", cid,id);
			#endif
			ret=confirma_prereserva(id);
			(ret==-1?failure++:success++);
			#ifdef DEBUG
			printf("#C%d confirming preres %d returned %d\n", cid,id,ret);
			#endif	
		}

		if (id!=-1 && (rand()%100)<100) {
			#ifdef DEBUG
			printf("#C%d cancelling res:%d\n", cid,id,ret);
			#endif
			int att=0;
			int ret2=0;
			do {
				att++;
				ret2=cancela(id);		
				(ret2==-1?failure++:success++);
			} while (ret2==-1 && att<MAX_ATT);
			#ifdef DEBUG
			printf("#C%d cancelled res %d:%d\n", cid,id,ret);	
			#endif	
		}

		//mostra_cinema();

	}
	printf("#C%d-S:%d/F:%d\n", cid,success,failure);	
	return 0;
}

int main(int argc, char **argv) {
	int i;
	int ret;
	int timeslice = 300;
	
	sthread_init(&timeslice);
	init_cinema(NUM_ROWS,NUM_COLUMNS,NUM_S_THREADS,BUFFER_SIZE);
	sthread_t thr[NUM_CLIENT_THREADS];

	for (i=0; i<NUM_CLIENT_THREADS; i++) {
		thr[i]=sthread_create(client_thread, (void*)i, 1, 0);
		if (thr[i] == NULL) {
			printf("sthread_create failed\n");
			exit(-1);
		}
		    
		sthread_yield();
	}

	for (i=0; i<NUM_CLIENT_THREADS; i++) {
		sthread_join(thr[i],(void**)&ret);
		if (ret) {
			printf("sthread_joined returned %d\n",ret);
		}
	}    
	

  	mostra_cinema();
  return 0;
}


