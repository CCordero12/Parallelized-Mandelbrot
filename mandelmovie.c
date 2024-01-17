#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <string.h>
#include <sys/wait.h> 


char * words[16];
extern int errno;
pid_t pid;
char *delim = " \t\n", *token = NULL;


void show_help()
{
	printf("Use: mandelmovie [options]\n");
	printf("Where options are:\n");
    printf("-m <max>        The maximum number of iterations per point. (default=900)\n");
	printf("-x <coord>      X coordinate of image center point. (default=-0.11275)\n");
	printf("-y <coord>      Y coordinate of image center point. (default=0.89223)\n");
	printf("-s <scale>      Scale of the final image in the Mandlebrot coordinates. (default=0.00008)\n");
	printf("-W <pixels>     Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels>     Height of the image in pixels. (default=1000)\n");
	printf("-o <file>       Set output file. (default=mandel.bmp)\n");
	printf("-n <threads>    Sets the number of threads that will process the mandelbrot calculations. (default=1)\n");
    printf("-i <iterations> Number of iterations you want to see. Will reduce the jump in zoom between images. (default = 50)\n");
    printf("-p <processes>  Number of mandel processes running at the same time.\n");
	printf("-h              Show this help text.\n");
}

int main( int argc, char *argv[] )
{
    printf("Welcome to mandelmovie. Put -h in the argument to get help.\n");
    printf("Default values are Christopher Cordero's selected mandel output.\n");
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	char * outfile = "mandel";
    char cmd[5000];
	double xcenter = -0.11275;
	double ycenter = 0.89223;
	double scale = 2;
    double scale_goal = 0.00008;
    double scale_interval = 0.04;
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 900;
    int iterations = 50;
    int cur_iteration = 0;
    int thread_count = 1; 
    int max_processes = 1;
    int cur_process = 0;


	// For each command line argument given,
	// override the appropriate configuration value.


	while((c = getopt(argc,argv,"x:y:s:W:H:m:n:i:p:o:h"))!=-1) {
		switch(c) {
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				scale_goal = atof(optarg);
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
            case 'i':
                iterations = atoi(optarg);
                if(iterations < 1){
                    printf("mandelmovie: Iterations should be an integer greater than 1\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'p':
                max_processes = atoi(optarg);
                if(max_processes < 1){
                    printf("mandelmovie: Processes should be an integer greater than 1\n");
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

    
    //How much the mandel movie will zoom in after each image
    scale_interval = ((scale - scale_goal)/(iterations -1));


    //Iterates the number of times specified
    while(cur_iteration < iterations){
        
        char cur_outfile[300] = "mandel.bmp";

        //Setting up the outfile and the arguments we are passing to the mandel program.
        sprintf(cur_outfile, "%s%d.bmp", outfile, cur_iteration);
        sprintf(cmd, "./mandel -x %lf -y %lf -s %lf -W %d -H %d -m %d -o %s -n %i", xcenter, ycenter, scale, image_width, image_height, max, cur_outfile, thread_count);
        //printf("./mandel -x %lf -y %lf -s %lf -W %d -H %d -m %d -o %s -n %i\n", xcenter, ycenter, scale, image_width, image_height, max, cur_outfile, thread_count);

        //Parsing the cmd string into individual words
        token = strtok(cmd, delim);
        //Each individual word is stored in the word index
        words[0] = token;
        int args = 1;

        while(token){
            token = strtok(0, delim);
            words[args] = token;
            args ++;
        }

        //Forks the process
        int fork_status = fork();
        
        //This first part will only execute if it is the child process resulted from the fork
        if(fork_status == 0){

            //We know this is the child process now, so we run the desired process
            if(execvp(words[0], words)==-1){
                //If it returns -1, the process failed, so we need to exit this child process with a failure
                exit(-1);
            }
            //Exit to return to the parent process
            exit(0);
            
        } else if (fork_status < 0){
            //Incase the fork fails (returns negative value)
            printf("myshell: failed to create child: %s\n", strerror(errno));
        }

        //Increment the current processes count
        cur_process++;

        //If the number of current processes is greater than or equal to the max number, we have to pause here.
        //This will be skipped if it isnt and just begin the next process.
        if(cur_process >= max_processes){
            //We wait here for one of the child processes to end
            pid = wait(NULL);
            if(errno != 0){
                //If we get an error, let the user know which process failed
                printf("myshell: process %i exited abnormally with status %i\n", pid, errno);
            } 
            //Remove 1 from the current process count since one just finished
            cur_process--;
        }
        
        //Continue the loop and adjust the scale
        cur_iteration++;
        scale = (scale - scale_interval);
    }

    return 0;
}
