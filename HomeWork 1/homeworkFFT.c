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

// In my implementation of the paralel version of the FFT I used this sequential
// implementation, provided by Rosetta Code. The sequenatial implementation can
// be found here https://rosettacode.org/wiki/Fast_Fourier_transform#C or below
/* *** START OF CODE FROM ROSSETA CODE ***
void _fft(cplx buf[], cplx out[], int n, int step)
{
	if (step < n) {
		_fft(out, buf, n, step * 2);
		_fft(out + step, buf + step, n, step * 2);

		for (int i = 0; i < n; i += 2 * step) {
			cplx t = cexp(-I * PI * i / n) * out[i + step];
			buf[i / 2]     = out[i] + t;
			buf[(i + n)/2] = out[i] - t;
		}
	}
}

void fft(cplx buf[], int n)
{
	cplx out[n];
	for (int i = 0; i < n; i++) out[i] = buf[i];

	_fft(buf, out, n, 1);
}
*** END OF CODE FROM ROSSETA CODE *** */

// Number of elements. Declared globally to not be needed to send it as a
// parameter to FT functions
int N;

// Struct to store variables needed by FFT function as I have not found other
// way to send multiple parameters to the thread function
struct vars_for_fft {
	double complex * buffer;
	double complex * out_vec;
	int step;
};

// Here I am going to store all the parameters used in the FFT functions
// The name is sugestive enough, those are going to be all my parameters
struct vars_for_fft parameters;

// The number of threads we are going to use while paralising the algorithm
int num_of_threads;

// Paralel variant fo the Fast Fourier Transformation
void * _fft(void * var) {
	// Get the struct that holds all the params
	struct vars_for_fft data = *(struct vars_for_fft*) var;

	// Get values of the parameters stored in the structure
	double complex * buffer = data.buffer;
	double complex * out_vec = data.out_vec;
	int step = data.step;

	// Prepare struct that's going to be passed to left recursive tree
	struct vars_for_fft data1;
	data1.buffer = data.out_vec;
	data1.out_vec = data.buffer;
	data1.step = 2 * data.step;

	// Prepare struct that's going to be passed to right recursive tree
	struct vars_for_fft data2;
	data2.buffer = data.out_vec + data.step;
	data2.out_vec = data.buffer + data.step;
	data2.step = 2 * data.step;

	// Nothing changed compared to the original Rosseta Code, besides the fact
	// that _fft takes a struct as parameter. This is done to assure we can
	// create threads execute this function using Pthread API (so function
	// must have just one parameter)
	if (step < N) {
		_fft(&(data1));
		_fft(&(data2));
		for (int i = 0; i < N; i += 2 * step) {
			double complex t = cexp(-I * M_PI * i / N) * out_vec[i + step];
			buffer[i / 2]     = out_vec[i] + t;
			buffer[(i + N)/2] = out_vec[i] - t;
		}
	}

	return 0;
}

// Driver function to filter based on the number of threads and call the _fft
void fft() {
	// We start with step = 1
	parameters.step = 1;

	// If we only have on thread, do it in the same manner as a seqeuantial way
	if(num_of_threads == 1) {
		_fft(&(parameters));
	} else if(num_of_threads == 2) {
		// Prepare struct that's going to be passed to first thread
		struct vars_for_fft data1;
		data1.buffer = parameters.out_vec;
		data1.out_vec = parameters.buffer;
		data1.step = 2 * parameters.step;

		// Prepare struct that's going to be passed to second thread
		struct vars_for_fft data2;
		data2.buffer = parameters.out_vec + parameters.step;
		data2.out_vec = parameters.buffer + parameters.step;
		data2.step = 2 * parameters.step;

		// Initialise two threads
		pthread_t tid[num_of_threads];

		// Create two threads and pass the two arguments, as we would do if
		// we had just one thread using recursivity. Nothing changes
		pthread_create(&(tid[0]), NULL, _fft, &(data1));
		pthread_create(&(tid[1]), NULL, _fft, &(data2));

		// Using join to wait until both threads finish execution
		for(int i = 0; i < num_of_threads; i++) {
			pthread_join(tid[i], NULL);
		}

		// Replicate the for loop inside _loop that is not computed for the
		// values calculated by the 2 threads.
		for (int i = 0; i < N; i += 2 * parameters.step) {
			double complex t = cexp(-I * M_PI * i / N) *
										parameters.out_vec[i + parameters.step];
			parameters.buffer[i / 2]     = parameters.out_vec[i] + t;
			parameters.buffer[(i + N)/2] = parameters.out_vec[i] - t;
		}
	} else if(num_of_threads == 4) {
		// Prepare struct that's going to be passed to 1st thread
		struct vars_for_fft data1;
		data1.buffer = parameters.buffer;
		data1.out_vec = parameters.out_vec;
		data1.step = 4 * parameters.step;

		// Prepare struct that's going to be passed to 2nd thread
		struct vars_for_fft data2;
		data2.buffer = parameters.buffer + 2 * parameters.step;
		data2.out_vec = parameters.out_vec + 2 * parameters.step;
		data2.step = 4 * parameters.step;

		// Prepare struct that's going to be passed to 3rd thread
		struct vars_for_fft data3;
		data3.buffer = parameters.buffer + parameters.step;
		data3.out_vec = parameters.out_vec + parameters.step;
		data3.step = 4 * parameters.step;

		// Prepare struct that's going to be passed to 4rd thread
		struct vars_for_fft data4;
		data4.buffer = parameters.buffer + 3 * parameters.step;
		data4.out_vec = parameters.out_vec + 3 * parameters.step;
		data4.step = 4 * parameters.step;

		pthread_t tid[num_of_threads];

		// Create 4 threads and pass a struct as an argument for each tread
		// Same logic applied before for two threads
		pthread_create(&(tid[0]), NULL, _fft, &(data1));
		pthread_create(&(tid[1]), NULL, _fft, &(data2));
		pthread_create(&(tid[2]), NULL, _fft, &(data3));
		pthread_create(&(tid[3]), NULL, _fft, &(data4));

		// Using join to wait until both threads finish execution
		for(int i = 0; i < num_of_threads; i++) {
			pthread_join(tid[i], NULL);
		}

		// Replicate the three loops inside _fft that were not executed because
		// I unrolled the recursivity loop to be able to execute it using
		// the four threads. The order of the for loops is very important.
		// Imagine that those are placed on a stack and I extract them, so the
		// loops that are deeper in the normal recursivity will have to be done
		// first. The role of the loops is to put together the results computed
		// during two separate threads (or recursive calls, as in the original
		// algorithm from Rosseta Code)
		/*
						O            1st
					 /     \
					/       \
				   O         O       2nd
				 /  \       /  \
				/    \     /    \
			   O      O   O      O   3rd
		*/

		// The for loop that is linked to the 3rd level of recursivity of the
		// right recursive tree. This for loop is linked to this call from
		// the original algorithm: _fft(out + step, buf + step, n, step * 2);
		for (int i = 0; i < N; i += 4 * parameters.step) {
			double complex t = cexp(-I * M_PI * i / N) *
									parameters.buffer[i + 3 * parameters.step];
			parameters.out_vec[i / 2 + parameters.step] =
									parameters.buffer[i + parameters.step] + t;
			parameters.out_vec[(i + N) / 2 + parameters.step] =
									parameters.buffer[i + parameters.step] - t;
		}

		// The out and buff vectors are changed between them on each new level
		// of recursivity, as before. This for loop is linked to the first step
		// of the left recursive tree or to this call from the original
		// algorithm: _fft(out, buf, n, step * 2);
		for (int i = 0; i < N; i += 4 * parameters.step) {
			double complex t = cexp(-I * M_PI * i / N) *
									parameters.buffer[i + 2 * parameters.step];
			parameters.out_vec[i / 2]     = parameters.buffer[i] + t;
			parameters.out_vec[(i + N) / 2] = parameters.buffer[i] - t;
		}

		// The original for loop used to sum the values computed during the
		// two recursive calls of the original Rosseta Code algorithm.
		// It is linked to this call: _fft(buf, out, n, 1);
		for (int i = 0; i < N; i += 2 * parameters.step) {
			double complex t = cexp(-I * M_PI * i / N) *
										parameters.out_vec[i + parameters.step];
			parameters.buffer[i / 2] = parameters.out_vec[i] + t;
			parameters.buffer[(i + N) / 2] = parameters.out_vec[i] - t;
		}
	}
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

	// Number of elements
	int err = fscanf(input, "%d", &N);

	// Fcanf returns the number of input items successfully matched and assigned
	// if the value returned is 0 then we have an error
	if(err == 0) {
		printf("Some error occured! \n");
	}

	// Alloc the buffer and output vectors dynamically
	parameters.buffer = (double complex *)malloc(N * sizeof(double complex));
	parameters.out_vec = (double complex *)malloc(N * sizeof(double complex));

	// Auxiliary variable to store values read before converting to complex num
	double read_value;

	// Read input from file and initialise buffer and out vector
	for(int i = 0; i < N; ++i) {
		int err = fscanf(input, "%lf \n", &read_value);
		// Fcanf returns the number of input items successfully matched and
		// assigned if the value returned is 0 then we have an error
		if(err == 0) {
			printf("Some error occured! \n");
		}
		// Copy value to input vector, converting it to a complex number
		parameters.buffer[i] = read_value + 0.0 * I;

		// Initialise output vector with input vector values
		parameters.out_vec[i] = parameters.buffer[i];
	}

	// Close input file as we do not have anything more to read
	fclose(input);

	// Compute value of FFT in a paralel way
	fft();

	// Write the results of the Fourier Transformation to the output file
	fprintf(output, "%d \n", N);
	for(int i = 0; i < N; ++i) {
		// The results consists in two parts, one real and one complex
		fprintf(output, "%lf %lf \n", creal(parameters.buffer[i]),
									cimag(parameters.buffer[i]));
	}

	// Close output file as we are done with writing to it
	fclose(output);

	// Free memory used and return 0 for success
	free(parameters.buffer);
	free(parameters.out_vec);
	return 0;
}
