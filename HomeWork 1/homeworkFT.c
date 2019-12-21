/*
 *  **********************************************************************
 *  DIEACONU CONFIDENTIAL
 *  _____________________
 *
 *   Copyright 2019 Vlad-Stefan Dieaconu
 *   Zero Rights Reserved.
 *
 *  NOTICE:  All information contained herein is, and remains
 *  the property of Vlad-Stefan Dieaconu. You can use it however you want,
 *  it's OPEN-SOURCE, just don't say it was written by you. Give credits!
 *  Dissemination of this information or reproduction of this material
 *  is strictly approved unless prior written permission is denied by me.
 *  #SharingIsCaring #LongLiveOpenSource #FreeInternet
 *
 *  Original Publisher https://github.com/vladstefandieaconu
 *  Date: November 2019
 *  **********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <complex.h>

// Number of elements. Declared globally to not be needed to send it as a
// parameter to FT functions
int N;

// The number of threads we are going to use while paralising the algorithm
int num_of_threads;

// The input and output vectors
double complex * in_vec;
double complex * out_vec;

// Sequential variant of the Fourier Transformation
void seq_FT() {
	// Auxiliary variable used to compute the current X(k)
	complex double aux = 0.0;

	// Variable where I'll store the value of the exponent of e
	double exponent_of_e = 0.0;

	for (int i = 0; i < N; ++i) {
		// Initialisation done to start with fresh parameters for each iteration
		aux = 0.0;
		exponent_of_e = 0.0;
		for (int j = 0; j < N; ++j) {
			// Compute the value of the exponent of e
			exponent_of_e = -1 * 2 * M_PI * j * i / N;

			// Then multiply that value with the original input value and
			// add it to the variable that stores the auxiliaty sum
			aux += in_vec[j] * cexp(exponent_of_e * I);
		}
		out_vec[i] = aux;
	}
}

// Paralel variant fo the Fourier Transformation
void * par_FT(void * var) {
	// The id of the current thread
	int thread_id = *(int*)var;

	// Variables used to delimit the space for every thread (done to paralise
	// the implementation). Instead of having one thread that does the whole
	// FOR loop, we have numerous threads that each do a part of the for loop,
	// based on the start and end computed for that specific thread
	int start = thread_id * ceil((double)N / num_of_threads);
    int end = fmin(N, (thread_id + 1) * ceil((double)N / num_of_threads));

	// Auxiliary variable used to compute the current X(k)
	complex double aux = 0.0;

	// Variable where I'll store the value of the exponent of e
	double exponent_of_e = 0.0;

	for (int i = start; i < end; ++i) {
		// Initialisation done to start with fresh parameters for each iteration
		aux = 0.0;
		exponent_of_e = 0.0;
		for (int j = 0; j < N; ++j) {
			// Compute the value of the exponent of e
			exponent_of_e = -1 * 2 * M_PI * j * i / N;

			// Then multiply that value with the original input value and
			// add it to the variable that stores the auxiliaty sum
			aux += in_vec[j] * cexp(exponent_of_e * I);
		}
		out_vec[i] = aux;
	}
	return 0;
}

int main(int argc, char * argv[]) {
	// Files for input and output
	FILE * input;
	FILE * output;

	// Get command line arguments
	char * in_file = argv[1];
	char * out_file = argv[2];
	num_of_threads = atoi(argv[3]);

	// Open files for reading and writing
	input = fopen (in_file,"r");
	output = fopen (out_file,"w");

	// Get the number of elements
	int err = fscanf(input, "%d", &N);

	// Fcanf returns the number of input items successfully matched and assigned
	// if the value returned is 0 then we have an error
	if(err == 0) {
		printf("Some error occured! \n");
	}

	// Alloc the input and output vectors dynamically
	in_vec = (double complex *)malloc(N * sizeof(double complex));
	out_vec = (double complex *)malloc(N * sizeof(double complex));

	// Auxiliary value to store what I've read before converting to complex num
	double read_value;

	// Get the values used
	for(int i = 0; i < N; ++i) {
		int err = fscanf(input, "%lf \n", &read_value);
		// Fcanf returns the number of input items successfully matched and
		// assigned if the value returned is 0 then we have an error
		if(err == 0) {
			printf("Some error occured! \n");
		}
		// Convert it to complex number
		in_vec[i] = read_value + 0.0 * I;
	}

	// Close input file as we do not have anything more to read
	fclose(input);

	// Compute the simple Fourier Transformation in a sequential way
	// uncoment the following line to run the sequential variant
	//seq_FT();

	// Compute the simple Fourier Transformation in a paralel way using more
	// threads that work simultaneous
	pthread_t tid[num_of_threads];

	// Vector to store IDs of every thread
	int thread_id[num_of_threads];

	// For loop to assign thread IDs
	for(int i = 0; i < num_of_threads; ++i) {
		thread_id[i] = i;
	}

	// For loop to create each thread
	for(int i = 0; i < num_of_threads; ++i) {
		pthread_create(&(tid[i]), NULL, par_FT, &(thread_id[i]));
	}

	// Using join to wait until all threads finish execution
	for(int i = 0; i < num_of_threads; i++) {
		pthread_join(tid[i], NULL);
	}

	// Write the results of the Fourier Transformation to the output file
	fprintf(output, "%d \n", N);
	for(int i = 0; i < N; ++i) {
		// The results consists in two parts, one real and one complex
		fprintf(output, "%lf %lf \n", creal(out_vec[i]), cimag(out_vec[i]));
	}

	// Close output file as we are done with writing to it
	fclose(output);

	// Free memory used and return 0 for success
	free(in_vec);
	free(out_vec);
	return 0;
}
