#ifndef SERVER_H
#define SERVER_H 1

#include <sthread.h>

sthread_mutex_t **movie_theatre;

void init_movie_theatre(int rows, int cols);
void server_thread();

#endif
