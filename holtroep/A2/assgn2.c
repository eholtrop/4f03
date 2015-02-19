#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <gmp.h>
#include <time.h>
#include "mpi.h"

long primegap(mpz_t start, mpz_t end){
	mpz_t current;
	mpz_t last;
	mpz_t temp;
	long max_gap = 0;

	mpz_init(current);
	mpz_init(last);
	mpz_init(temp);

	mpz_nextprime(last, start);
	mpz_nextprime(current, last);

	do {
		mpz_sub(temp, current, last);
		//gmp_printf("%Zd - %Zd = %Zd\n", current, last, temp);
		if(mpz_get_ui(temp) > max_gap) {
			max_gap = mpz_get_ui(temp);
		}
		
		mpz_set(last, current);
		mpz_nextprime(current, last);
	} while(mpz_cmp(current, end) < 0);

	return max_gap;
}

int main(int argc, char* argv[])
{
	int my_rank;			//the rank of the current process
	int p;				//the total number of processes
	mpz_t global_min, global_max;	//the global max and min that needs to be calculated
	mpz_t local_min, local_max;	//the local max and min for what needs to be calculated
	mpz_t temp;			//used to hold values as needed
	int section_size;		//the size of the area this process needs to calculate
	long local_gap;			//largest gap in the current section
	double max_time, min_time;	//global output for times
	int max_gap;			//global output for gap
	double local_time;		//time this process takes to compute its section
	struct timespec start, end;	//used to figure out time values

	mpz_init(global_min);
	mpz_init(global_max);
	mpz_init(local_min);
	mpz_init(local_max);
	mpz_init(temp);

	//initialize MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	
	mpz_set_ui(global_min, atoi(argv[1]));
	mpz_set_ui(global_max, atoi(argv[2]));
	
	//figuring out section_size for each process
	mpz_sub(temp, global_max, global_min);
	mpz_cdiv_q_ui(temp, temp, p);
	section_size = mpz_get_ui(temp);
	
	//initializing local_min and local_max based on section_size
	mpz_add_ui(local_min, global_min, (section_size * my_rank));
	mpz_add_ui(local_max, local_min, section_size);
	if(mpz_cmp(local_max, global_max) > 0){
		mpz_set(local_max, global_max);
	}

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	
	local_gap = primegap(local_min, local_max);
	
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);

	local_time = ((double)end.tv_sec) - ((double)start.tv_sec) + 
		     ((double)end.tv_nsec/ 1000000000) - ((double)start.tv_nsec / 1000000000);

	printf("%d process time: %f(s)\n", my_rank, local_time);
	
	MPI_Reduce(&local_gap, &max_gap, 1, MPI_INTEGER, MPI_MAX, 0, MPI_COMM_WORLD);
	MPI_Reduce(&local_time, &min_time, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
	MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
	
	if(my_rank == 0){
		printf("Min Time: %f(s)  Max Time: %f(s)  Largest Gap: %d \n",  
				min_time, 
				max_time, 
				max_gap);
	}
	
	MPI_Finalize();
}
