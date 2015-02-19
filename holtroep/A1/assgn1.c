#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include "genmatvec.c"
#include "matvecres.c"

//Funcation that calculates how many rows each process needs to calculate
int calcRows(int n, int p, int my_rank)
{
	int rows = n / p;
	int overflow = n % p;

	if(my_rank < overflow)
		rows ++;

	return rows;
}

int main(int argc, char* argv[])
{
	int my_rank;		//rank of the current process
	int p;			//number of processes
	int source;		//rank of the sernder
	int destination;	//rank of the destination
	int tag = 0;		//tag
	int m = atoi(argv[1]);	//Width of matrix
	int n = atoi(argv[2]);	//Height of matrix/length of vector
	double *A;		//Matrix	
	double *b;		//Vector
	double *y;		//Output Vector
	int i, j;		//loop variables
	int rows;		//number of this process will have to calculate
	MPI_Status status;	//Status

	

	MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	//error on input
	if(m < 0 || n < 0)
	{
		if(my_rank == 0)
			printf("Error: Matrix cannot have negative size \n");
		MPI_Finalize();
		return 0;	
	}
	if(m == 0 || n == 0)
	{
		if(my_rank == 0)
			printf("Error: Matrix cannot have size 0 \n");
		MPI_Finalize();
		return 0;	
	}
	

	MPI_Comm_size(MPI_COMM_WORLD, &p);


	rows = calcRows(n, p, my_rank);

	//allocate matrix and y based on current rank
	if(my_rank == 0)
	{
		A = malloc(m * n * sizeof(double));
		y = malloc(m * sizeof(double));
	}	
	else
	{
		A = malloc(rows * n * sizeof(double));
		y = malloc(rows * sizeof(double));	
	}

	b = malloc(n * sizeof(double));

	//generate data
	if(my_rank == 0)
	{
		genMatrix(m, n, A); 	//Generate Matrix

		genVector(n, b); 	//Generate Vector
	}

	//boradcast the vector
	MPI_Bcast(b, n, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	//send Matrix data
	if(my_rank == 0)
	{

		int startIndex = rows * n;
		for(i = 1; i < p; i ++)
		{
			int r = calcRows(n, p, i);
			
			MPI_Send(&A[startIndex], n * r, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
			
			startIndex += n*r;
		}
	}
	else
	{
		MPI_Recv(A, n*rows, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &status);
	}

		
	//calculate dot product
	int currentRow = -1;
	for(i = 0; i < rows*n; i++)
	{
		if(i % n == 0)
			currentRow++;
		y[currentRow] += A[i] * b[i % n];
	}

	if(my_rank != 0)
	{
		MPI_Send(y, rows, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD);
	}


	//aggregate data
	int yindex = rows; //index of y that will be calculated next
	if(my_rank == 0)
	{
		for(i = 1; i < p; i++)
		{
			int r = calcRows(n, p, i);
			MPI_Recv(&y[yindex], r, MPI_DOUBLE, i, tag, MPI_COMM_WORLD, &status);
			printf("received %d row(s) from %d \n", r, i);
			
			yindex += r;
		}

		getResult(m, n, A, b, y);
	}

	free(A);
	free(b);
	free(y);

	MPI_Finalize();
	return 0;
}

