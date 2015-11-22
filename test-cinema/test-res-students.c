#include <stdio.h>
#include <stdlib.h>
#include <sthread.h>
#include <cinema.h>	

#define DURATION_PRE 2

int _NUM_CLIENT_THREADS=1;
int _NUM_S_THREADS=1;
int _NUM_ROWS=20;
int _NUM_COLUMNS=20;
int _BUFFER_SIZE=1000;
int _MAX_SEATS=1;
int _NUM_REQS=1;
int _MAX_ATT=200;
int _CONFLICT=0;
int reserve=0;
int snapshot=0;

int tot_ops=0;
int tot_succ=0;
int tot_fail=0;

sthread_mutex_t lock_cl_stats;

void incOp() {
	sthread_mutex_lock(lock_cl_stats);
	tot_ops++;
	sthread_mutex_unlock(lock_cl_stats);
}

void incSucc() {
	sthread_mutex_lock(lock_cl_stats);
	tot_succ++;
	sthread_mutex_unlock(lock_cl_stats);
}

void incFail() {
	sthread_mutex_lock(lock_cl_stats);
	tot_fail++;
	sthread_mutex_unlock(lock_cl_stats);
}


// Parameters accepted by the test program
// 1 - operations performed by clients. Legava values are 0 and 4, where: 0=Reservation & Cancel - 4=Prereservation & Confirm & Cancel
// 2 - _NUM_CLIENT_THREADS
// 3 - _NUM_S_THREADS
// 4 - _BUFFER_SIZE - size of the circular buffer
// 5 - _MAX_SEATS=1 - number of seats requested in each reservation
// 6 - _CONFLICT= 0 no, 1 random
// 7 - _NUM_ROWS - rows in the cinema
// 8 - _NUM_COLUMNS - columns in the cinema
// 9 - _MAX_REQS=1 - number of requests (res or preres) issued by each client
// 10 - _MAX_ATT=200 - ignored in this version of the testing program
// 11 - SNAPSHOT=0 - if snapshot = 1 clients request the visualization of the cinema in between operations

void read_params(int argc, char **argv) {

	
	if (argc==1) {
		printf("USAGE:\n1 - 0: operations \n2 - _NUM_CLIENT_THREADS\n3 - _NUM_S_THREADS\n4 - _BUFFER_SIZE\n5 - _MAX_SEATS\n6 - _CONFLICT\n7 - _NUM_ROWS\n8 - _NUM_COLUMNS\n9 - _MAX_REQS\n10 - _MAX_ATT\n");
		return;
	}
		
	reserve=atoi(argv[1]);
	printf("issuing %d type res\n",reserve);

	if (argc==2) { return; }

	_NUM_CLIENT_THREADS=atoi(argv[2]);
	printf("_NUM_CLIENT_THREADS: %d\n",_NUM_CLIENT_THREADS);

	if (argc==3) { return; }

	_NUM_S_THREADS=atoi(argv[3]);
	printf("_NUM_S_THREADS: %d\n",_NUM_S_THREADS);

	if (argc==4) { return; }

	_BUFFER_SIZE=atoi(argv[4]);
	printf("_BUFFER_SIZE: %d\n",_BUFFER_SIZE);

	if (argc==5) { return; }

	_MAX_SEATS=atoi(argv[5]);
	printf("_MAX_SEATS: %d\n",_MAX_SEATS);

	if (argc==6) { return; }

	_CONFLICT=atoi(argv[6]);
	printf("_CONFLICT: %d\n",_CONFLICT);

	if (argc==7) { return; }

	_NUM_ROWS=atoi(argv[7]);
	printf("_NUM_ROWS: %d\n",_NUM_ROWS);

	if (argc==8) { return; }

	_NUM_COLUMNS=atoi(argv[8]);
	printf("_NUM_COLUMNS: %d\n",_NUM_COLUMNS);

	if (argc==9) { return; }


	_NUM_REQS=atoi(argv[9]);
	printf("_MAX_REQS: %d\n",_NUM_REQS);

	if (argc==10) { return; }


	_MAX_ATT=atoi(argv[10]);
	printf("_MAX_ATT: %d\n",_MAX_ATT);

	if (argc==11) { return; }


	snapshot=atoi(argv[11]);
	printf("snapshot: %d\n",snapshot);

	if (argc==12) { return; }
	
	printf("Too many args:%d",argc);
	exit(-1);
}



Coords_t* rand_seats(int size) {
	int i=0;
	int start_c=rand(),start_r=rand();

	Coords_t* c=(Coords_t*) malloc(sizeof(Coords_t)*size);
	for (;i<size;i++) {
		c[i].row=(start_r++)%_NUM_ROWS;
		c[i].column=(start_c++)%_NUM_COLUMNS;
	}
	return c;
}




Coords_t* det_seats(int c_id, int size) {
	int i=0;
	int offset=_NUM_ROWS*_NUM_COLUMNS/_NUM_CLIENT_THREADS;
	//printf("Offset:%d\n",offset);
	Coords_t* c=(Coords_t*) malloc(sizeof(Coords_t)*size);
	for (;i<size;i++) {
		c[i].column=(c_id*offset+i)%_NUM_ROWS;
		c[i].row=((c_id*offset+i)/_NUM_ROWS)%_NUM_ROWS;
	}
	return c;
}

int getSeats(int cid, Coords_t** c) {

	int dim=1;
	Coords_t* c1;
	if (!_CONFLICT) {
		dim=_MAX_SEATS;
		c1=det_seats(cid,dim);
	}
	else if (_CONFLICT==1) { 
		dim=rand()%_MAX_SEATS;
		if (dim==0) dim=1;
		c1=rand_seats(dim);
	}
	else {
		printf("Illegal _CONFLICT value\n");
		exit(-1);
	}
	
	*c=c1;
	return dim;
}


int OPCc(int cid) {
	int ret,id;
	int dim;
	Coords_t* c;

	dim=getSeats(cid,&c);

	#ifdef DEBUG
	int i=0;
	printf("#C%d submitting prereservation of %d seats\n", cid, dim);
	for (i=0;i<dim;i++) {
		printf("(%d,%d) ",c[i].row,c[i].column);
	}
	printf("\n");
	#endif

	ret=prereserva(c,dim,DURATION_PRE);

	if (ret==-1)
		return ret;
	
	if (snapshot) 
		mostra_cinema();

	id=ret;

	#ifdef DEBUG
	printf("#C%d confirming preres:%d\n", cid,id);
	#endif
	int att=0;
	int ret2=0;
	
	ret2=confirma_prereserva(id);		
	#ifdef DEBUG
	printf("#C%d confirming res %d:%d\n", cid,id,ret2);	
	#endif	

	if (snapshot) 
		mostra_cinema();

	#ifdef DEBUG
	printf("#C%d cancelling preres:%d\n", cid,id);
	#endif
	att=0;
	ret2=0;
	do {
		att++;
		ret2=cancela(id);		
	} while (ret2==-1 && att<_MAX_ATT);
	#ifdef DEBUG
	printf("#C%d cancellig res %d:%d\n", cid,id,ret2);	
	#endif	



	if (snapshot) 
		mostra_cinema();

	return ret2;
}


int ORc(int cid) {
	int ret,id;
	int dim;
	Coords_t* c;

	dim=getSeats(cid,&c);

	#ifdef DEBUG
	int i=0;
	printf("#C%d submitting reservation of %d seats\n", cid, dim);
	for (i=0;i<dim;i++) {
		printf("(%d,%d) ",c[i].row,c[i].column);
	}
	printf("\n");
	#endif

	ret=reserva(c,dim);

	if (ret==-1)
		return ret;
		
	if (snapshot)
		mostra_cinema();

	id=ret;

	#ifdef DEBUG
	printf("#C%d cancelling res:%d\n", cid,id);
	#endif
	int att=0;
	int ret2=0;
	do {
		att++;
		ret2=cancela(id);		
	} while (ret2==-1 && att<_MAX_ATT);
	#ifdef DEBUG
	printf("#C%d cancelled res %d:%d\n", cid,id,ret2);	
	#endif	

	if (snapshot)
		mostra_cinema();

	return ret2;
}




void *client_thread(void *arg) {

	int cid=(int)arg;
	int counter=0;

	while (counter++<_NUM_REQS) {



	switch (reserve) {
			case 0:
				( ORc(cid)==-1?incFail():incSucc());
				break;

			case 4:
				( OPCc(cid)==-1?incFail():incSucc());
				break;
			default:
				printf("Illegal operation - exiting \n");	
				exit(-1);	
		}

	incOp();

	}
	#ifdef DEBUG
	printf("#C%d-S:%d/F:%d\n", cid,tot_succ,tot_fail);	
	#endif	
	return 0;
}
void dump_params() {
		printf("----\nreserve %d\n_NUM_CLIENT_THREADS:%d\n_NUM_S_THREADS:%d\n_NUM_ROWS:%d\n_NUM_COLUMNS:%d\n_BUFFER_SIZE:%d\n_MAX_SEATS:%d\n_NUM_REQS:%d\n_MAX_ATT:%d\n----\n",reserve,_NUM_CLIENT_THREADS,_NUM_S_THREADS,_NUM_ROWS,
_NUM_COLUMNS,_BUFFER_SIZE,_MAX_SEATS,_NUM_REQS,_MAX_ATT);
}


int main(int argc, char **argv) {
	int i;
	int ret;
	int timeslice = 300;

	sthread_init(&timeslice);
	lock_cl_stats = sthread_mutex_init();

	read_params(argc, argv);
	dump_params();

	int ids[_NUM_CLIENT_THREADS];
	sthread_t thr[_NUM_CLIENT_THREADS];

	init_cinema(_NUM_ROWS,_NUM_COLUMNS,_NUM_S_THREADS,_BUFFER_SIZE);

	for (i=0; i<_NUM_CLIENT_THREADS; i++) {
		ids[i]=i;
		thr[i]=sthread_create(client_thread, (void*) ids[i], 1, 0);
		if (thr[i] == NULL) {
			printf("sthread_create failed\n");
			exit(-1);
		}
		    
		sthread_yield();
	}

	for (i=0; i<_NUM_CLIENT_THREADS; i++) {
		sthread_join(thr[i],(void **)&ret);
		if (ret) {
			printf("sthread_joined returned %d\n",ret);
		}
	}    
	

	printf("-GAME OVER-\n");

  	mostra_cinema();
	printf("#Ops: %d\n#Suc: %d\n#Fail:%d\n",tot_ops, tot_succ, tot_fail);

  return 0;
}


