
#include "bitmap.h"
#include <pthread.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <string.h>

//Structure containing all the arguments a thread needs to pass to the compute image method
typedef struct thread_args {
		double xmin;
		double xmax;
		double ymin;
		double ymax;
		double scale;
		int image_width;
		int height_min;
		int height_max;
		int max;
		struct bitmap *bm;
		//Threadnum is used for debugging, finding where each thread starts and end with a print (those prints need to be uncommented too)
		//int threadnum;
	}thread_args;

int iteration_to_color( int i, int max );
int iterations_at_point( double x, double y, int max );
//Compute image has to be void * and take void * as an argument due to what pthread_create expects in its arguments
void * compute_image(void *myarguments );


void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>     The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>   X coordinate of image center point. (default=0)\n");
	printf("-y <coord>   Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>   Scale of the image in Mandlebrot coordinates. (default=4)\n");
	printf("-W <pixels>  Width of the image in pixels. (default=500)\n");
	printf("-H <pixels>  Height of the image in pixels. (default=500)\n");
	printf("-o <file>    Set output file. (default=mandel.bmp)\n");
	printf("-n <threads> Sets the number of thread_count that will process the mandelbrot calculations. (default=1)\n");
	printf("-h           Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	const char *outfile = "mandel.bmp";
	double xcenter = 0;
	double ycenter = 0;
	double scale = 4;
	int    image_width = 500;
	int    image_height = 500;
	int    max = 1000;
	int thread_count = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:o:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				if(image_width < 1){
                    printf("mandelmovie: Width should be an integer greater than 1");
                    exit(EXIT_FAILURE);
                }
				break;
			case 'H':
				image_height = atoi(optarg);
				if(image_height < 1){
                    printf("mandelmovie: Height should be an integer greater than 1");
                    exit(EXIT_FAILURE);
                }
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'n':
				thread_count = atoi(optarg);
				if(thread_count < 1){
                    printf("mandelmovie: Thread count should be an integer greater than 1\n");
                    exit(EXIT_FAILURE);
                }
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

	// Display the configuration of the image.
	printf("mandel: x=%lf y=%lf scale=%lf max=%d outfile=%s\n",xcenter,ycenter,scale,max,outfile);

	// Create a bitmap of the appropriate size.
	struct bitmap *bm = bitmap_create(image_width,image_height);

	// Fill it with a dark blue, for debugging
	bitmap_reset(bm,MAKE_RGBA(0,0,255,0));

	// Create the desired number of threads and have them start processing the mandelbrot image, working on seperate parts

	//Creates an array of the created structure that holds all the arguments we need for each thread
	struct thread_args th_args[thread_count];
	//This allows us to create the number of threads we are prompted with
	pthread_t threads[thread_count];

	//How many lines of the image each thread has to process
	int thread_H_increment = image_height/thread_count;
	//The starting line of the image for the thread
	int thread_H_min = 0;
	//The last line that thread will generate. 
	//Starts as imageheight%thread_count so that if we have a number of threads that doesn't divide evenly into our Height, we still get the whole image
	//This approach means the first thread may have to process more of the image than the other threads, but for any realistically beneficial number of threads, it will be a minor difference of less than 10 extra lines.
	int thread_H_max = image_height%thread_count;

	//Loops through to create each thread
	for(size_t i = 0; i<thread_count; i++){
		//Adds to the max height part at the start so it will always be greater than min
		thread_H_max = thread_H_max + thread_H_increment;

		//Filling out the information needed for the mandelbrot process
		th_args[i].xmin = xcenter - scale;
		th_args[i].xmax = xcenter + scale;
		th_args[i].ymin = ycenter - scale;
		th_args[i].ymax = ycenter + scale;
		th_args[i].scale = scale;
		th_args[i].height_min = thread_H_min;
		th_args[i].height_max = thread_H_max;
		th_args[i].image_width = image_width;
		th_args[i].max = max;
		th_args[i].bm = bm;
		//Threadnum is used for debugging to see which threads terminate early
		//th_args[i].threadnum = i;

		//Passes the arguments to create a thread. If it doesnt return 0, we need to return 1 as a failure
		//First is address of the current thread, second defines thread attributes NULL = default
		//Third, we pass the address of the method we are calling, which has to return a void * and take a void * as argument
		//Lastly, we have to pass the address of the thread arguments as a void pointer since pthread_create expects a method that returns void * and takes a void * as an argument.
		if(pthread_create(&threads[i], NULL, &compute_image, (void *) &th_args[i]) != 0){
			return 1;
		}

		//Adds to the height minimum at the end, keeping it less than the max since that will be added to again before the next thread creation
		thread_H_min = thread_H_min + thread_H_increment;
	}

	for(size_t i = 0; i<thread_count; i++){
		if(pthread_join(threads[i], NULL) != 0){
			return 1;
		}
	}
	// Compute the Mandelbrot image
	//compute_image(bm,xcenter-scale,xcenter+scale,ycenter-scale,ycenter+scale,max);

	// Save the image in the stated file.
	if(!bitmap_save(bm,outfile)) {
		fprintf(stderr,"mandel: couldn't write to %s: %s\n",outfile,strerror(errno));
		return 1;
	}

	return 0;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void * compute_image( void *myarguments)
{
	//Converts the void pointer to what we actually need
	struct thread_args *args = myarguments;

	//Used for debugging
	//printf("Thread %i created\n", args->threadnum);

	int i,j;

	int width = bitmap_width(args->bm);
	int height = bitmap_height(args->bm);

	// For every pixel in the image...

	//Only modifications here are using the args from the structure we passed
	//and starting from our desired height minimum (starting line) and ending before our height max (ending line)
	//This allows the threads to calculate different lines of the image horizontally
	for(j = (int) (args->height_min); j < args->height_max; j++) {
		for(i=0;i<width;i++) {
			// Determine the point in x,y space for that pixel.
			double x = args->xmin + i*(args->xmax-args->xmin)/width;
			double y = args->ymin + (int)j*(args->ymax-args->ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,args->max);

			// Set the pixel in the bitmap.
			bitmap_set(args->bm,i,j,iters);
		}
	}

	//Used for debugging
	//printf("Thread %i terminated\n", args->threadnum);
	return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iteration_to_color(iter,max);
}

/*
Convert a iteration number to an RGBA color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/

int iteration_to_color( int i, int max )
{
	int gray = 255*i/max;
	return MAKE_RGBA(gray,gray,gray,0);
}




