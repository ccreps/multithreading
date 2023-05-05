/* Project 2 Multithreading Applications

Written by: Collin Creps
Date: March 8th 2022
Update History:
    03/08/2022 - Implemented processing the size passed arguments from user.
    03/09/2022 - Restrutured handling of arguments using strcmp and added the time logging for array. Implemented all parameter gathering / error checking.
    03/10/2022 - Debugged missing arguments and the conversion from pointers to actual char values that can be handled.
    03/12/2022 - Running tests on capturing the user input correctly, and implemented an isSorted function / isDigit function
    03/15/2022 - Added partitioning to main function and tracking the lo and hi indicies of each segment
    03/16/2022 - Added the median handling for partition, started quicksort function partition, stopping at the alternate sorting methods
    03/17/2022 - Debugged quicksort, created insertion / shell sort. All functioning properly. Implementing the handling for multithreading
                 Handled several segfaults which arrived with the multithreading
    03/18/2022 - Worked through multiple segfaults, threading errors, and quicksort errors
    03/19/2022 - Commented out the code, began first test with args from Lab 2 addendum failed. Debugged shell sort because of segfault and reran for success. isSorted
                 never actually got called because of missing parentheses so it was always evaluating to tru.
    03/20/2022 - Began Test 3 of addendum and harvested data from test 1 completion.

Decription: Project 2 has the objective of sorting an array of 32-bit integers using the Quick sort, Insertion sort, and Shell sort (Hibbard's Sequence)
    algorithms. There are a number of arguments the user can provide (some optional and some not) which determine how the program will process the sorting.
    We are allowed to run the sorting on up to 4 threads or less which is determined by the user.
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <errno.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

//define global array
uint32_t *array;

//create the struct for holding global array indexes
struct segmentData {
    uint32_t lo;
    uint32_t hi;
    uint32_t size;
};

//create the array of structures to hold the different segments
struct segmentData *segments;

//set all of the user args as global vars
uint32_t size; //defines size of the array and number of ints to be sorted
int seed = 1; //default for seed
int threshold = 10; //default for threshold
int pieces = 10; //default for pieces
int maxThread = 4; //default for maxThreads
char alternate = 's'; //deafult for alternate sorting
bool multiThread = true; //default for multiThread
bool median = false; //default for median of three rule

//prints out the accepted arguments for the program.
int help() {
    printf("Here are valid parameters for the program:\n");
    printf("-n SIZE\n");
    printf("[-s THRESHOLD]\n");
    printf("[-a ALTERNATE] can be S, s, i, or I\n");
    printf("[-r SEED] -1 seeds with clock\n");
    printf("[-m MULTITHREAD] can be n, N, y, or Y\n");
    printf("[-p PIECES]\n");
    printf("[-t MAXTHREADS] can be up to 4\n");
    printf("[-m3 MEDIAN] can be n, N, y, or Y\nAll arguments are integer values except ALTERNATE, MEDIAN and MULTITHREAD\n");
    exit(0);
}

//Checks if the argument is truly an integer, or if it contains characters. Exits if it is not.
bool intCheck(char size[]) {

    //check for a combination of an int and char (not valid)
    for (int i = 0; i < strlen(size); i++) {

        if (isdigit(size[i]) == 0) {
            fprintf(stderr, "The value is not a positive integer, but type uint is required for this argument.\n");
            help();
        }

    }
    return true;
}

//performs a comparison between array values to confirm the final array is sorted returns a true or false value
bool isSorted() {

    for(int i = 0; i < size; i++) {

        if (array[i] < array[i - 1]) {
            return false;
        }
    }
    return true;
}

/*takes a segment from the arrray and will recursively quicksort itself if THRESHOLD is 1
if THRESHOLD is SIZE, the function will only operarte with the alternate sort algorithm
The "hybrid" sort comes from a combination of SIZE and THRESHOLD where the array will partition & quicksort
until a segment hits threshold and then use the alternate sorting algorithm to finish.*/
void quickSort(struct segmentData seg, int threshold) {

    //if the segment is 0 or 1 then it is considered already sorted
    if (seg.size <= 1) {
        return;
    }
    
    //if size < threshold then use the alternate sorting method
    if (seg.size <= threshold) {

        //use shell sort
        if (alternate == 's') {

            int k = 1;

            while (k <= seg.size) {
                k *=2;
            }

            //set our k increment
            k = (k/2)-1;

            do {

                //for each comb position
                for (int i = seg.lo; i < (seg.hi + 1 - k); i++) {
                    
                    //tooth to tooth is k
                    for (int j = i; j >= seg.lo; j -= k) {

                        if (j < 0) {
                            break;
                        }
                        //move upstream
                        if (array[j] <= array[j + k]){
                            break;
                        }

                        else {
                            uint32_t shellTemp = array[j];
                            array[j] = array[j + k];
                            array[j + k] = shellTemp;
                        }
                    }
                }

                k /= 2;

            } while (k > 0);

        }

        //use insertion sort
        else {

            int next;
            int comp; //stores the value we compare with
            int prev;

            for (next = seg.lo + 1; next < seg.hi + 1; next++) {
                comp = array[next];
                prev = next - 1;

                //swap the values if prev is greater than comp
                while (prev >= seg.lo && array[prev] > comp) {
                    array[prev + 1] = array[prev];
                    prev = prev - 1;
                }

                array[prev + 1] = comp;
            }
        }
    }
    
    //enter the quicksorting algorithm
    else {

        //set the indexes for our access to the array
        uint32_t i = seg.lo;
        uint32_t j = seg.hi + 1;
        uint32_t X = array[seg.lo];
        uint32_t loHolder = seg.lo; //holds the initial lo value of the segment passed
        uint32_t hiHolder = seg.hi; //holds the initial hi value of the segment passed
        uint32_t med; //holds the median variable for median of three

        //handles the m3 parameter if it is true to us a middle value as the pivot instead of the first value which is default
        if (median) {

            //get middle index of segment
            uint32_t k = (seg.lo + seg.hi) / 2;
            //set A,B,C to lo, middle, high respectively
            uint32_t A = array[seg.lo]; uint32_t B = array[k]; uint32_t C = array[seg.hi];

            //A is inbetween B and C so median gets set to lo
            if ((A >= B && A <= C) || (A>=C && A<=B)) {
                med = seg.lo;
            }

            //B is inbetween A and C, set median to k
            else if ((B >= A && B <= C) || (B >= C && B <= A)) {
                med = k;
            }

            else {
                //median gets set to hi
                med = seg.hi;
            }

            //swap the pivot with the lo index
            uint32_t hold = array[seg.lo];
            array[seg.lo] = array[med];
            array[med] = hold;
            X = array[seg.lo];
        }

        do {

            //increment i and decrement j which start at lo and hi respectively until they cross
            do i++; while (array[i] < X);
            do j--; while (array[j] > X);
            if (i < j) {uint32_t temp = array[i]; array[i] = array[j]; array[j] = temp;}
            else break;

        } while (1);

        //swap the pivot with j so that all values to the left are < j and all to the right are > j
        array[seg.lo] = array[j];
        array[j] = X;

        //if the lower half is smaller that upper half, quicksort on lo to pivot-1 then pivot+1 to hi
        if ((j - seg.lo) < (seg.hi -j)) {

            seg.hi = j - 1;
            seg.lo = loHolder;
            seg.size = j - seg.lo;
            quickSort(seg, threshold);

            seg.lo = j + 1;
            seg.hi = hiHolder;
            seg.size = seg.hi - j;
            quickSort(seg, threshold);
        }

        //otherwise quicksort from pivot+1 to hi first then lo to pivot-2
        else {
            seg.lo = j+1;
            seg.hi = hiHolder;
            seg.size = seg.hi - j;
            quickSort(seg, threshold);
            seg.lo = loHolder;
            seg.hi = j - 1;
            seg.size = j - seg.lo;
            quickSort(seg, threshold);
        }
    }

    return;

}

//call quicksort on the segment of current thread runs for created threads
void *runner(void *param) {

    //convert from a pointer to struct so that I can call quicksort on it
    struct segmentData *paramSeg = param;
    struct segmentData pointerToStruct;

    //set the values for the newly initialized struct
    pointerToStruct.lo = paramSeg->lo;
    pointerToStruct.hi = paramSeg->hi;
    pointerToStruct.size = paramSeg->size;

    //quicksort on that specific segment
    quickSort(pointerToStruct, threshold);
}

/*Creates maxThread threads. When one finished it will create a new thread for the next segment with the terminated threads tid
Will enter the runner upon creation to start sorting the new segment and return back to main once all threads have terminated*/
void thread() {

    int count = 0; //keeps track of the segment to be run
    int status; //stores the int return value from pthreads_tryjoin_np()

    //create maxThread threads for the segments to run on
    pthread_t *tid = malloc (sizeof(pthread_t) * maxThread);
    //allocate memory for the attributes of each thread
    pthread_attr_t *attr = malloc (sizeof(pthread_attr_t) * maxThread);
    
    //initializes the first maxThread threads and increments the overall count for each created
    for(int threads = 0; threads < maxThread; threads++) {
        pthread_attr_init(&attr[threads]);
        pthread_create(&tid[threads], &attr[threads], &runner, &segments[count]);
        count++;
    }

    //keeps tabs on each of the created threads and throws next segment into the one that finishes up
    for (int polling = 0; polling < maxThread; polling++) {

        //update the staus for the polled thread upon a new loop iteration
        status = pthread_tryjoin_np(tid[polling], NULL);

        //thread is still running
        if (status == EBUSY) {
            usleep(50);
        }

        //thread has completed
        else if (status == 0) {

            //create a new thread in place of the completed thread
            pthread_create(&tid[polling], &attr[polling], &runner, &segments[count]);
            //increment count so we only run on pieces segments
            count++;

        }

        //there was an error with the thread
        else if (status == ETIMEDOUT) {

            fprintf(stderr, "Call timed out before thread finished.");
            EXIT_FAILURE;

        }

        //Pieces threads have been run
        if (count == pieces) {

            for (int i = 0; i < maxThread; i++) {

                do {
                    
                    //check the status of the thread UNTIL it has finished
                    status = pthread_tryjoin_np(tid[i], NULL);

                    //sleep the cpu clock to allow for thread to complete then poll again
                    if (status == EBUSY) {
                        usleep(50);
                    }

                } while (status != 0);

            }

            //set polling to something so I exit this function and do not reset with next if
            polling = maxThread;

        }

        //set polling back to 0 upon the next for loop iteration if we havent sorted all pieces
        if (polling == maxThread-1) {

            polling = -1;

        }
    }

    //threading has completed, return to main
    return;

}


int main(int argc, char* argv[]) {

    struct timeval startCreate; //wall clock time start
    struct timeval endCreate; //wall clock time end
    float timeIntervalCreate; //wall clock time interval

    struct timeval startPopulate;
    struct timeval endPopulate;
    float timeIntervalPopulate;

    struct timeval startScramble;
    struct timeval endScramble;
    float timeIntervalScramble;

    struct timeval startSort;
    struct timeval endSort;
    float timeIntervalSort;

    //start at the arg after program name and iterate through them all
    for (int i = 1; i < argc-1; i++) {

        //looks for the SIZE argument
        if (strcmp("-n", argv[i]) == 0) {

            if (intCheck(argv[i + 1])) { 

                //retrieves the size of the array
                size = atoi(argv[i + 1]);
                i++;

            }

            continue;
        }

        //checks for THRESHOLD argument
        if (strcmp("-s", argv[i]) == 0) {

            if (intCheck(argv[i + 1])) {

                //fetch the threshold set by user
                threshold = atoi(argv[i + 1]);
                i++;

            }

            continue;
        }

        //checks for ALTERNATE argument
        if (strcmp("-a", argv[i]) == 0) {

            if (strcmp(argv[i + 1], "s") == 0 || strcmp(argv[i + 1], "S") == 0) {

                //sets alternate to user passed arg
                alternate = 's';
                i++;

            }

            //if alternate is not specified as shell, it is insertion
            else if (strcmp(argv[i + 1], "i") == 0 || strcmp(argv[i + 1], "I") == 0) {
                alternate = 'i';
                i++;
            }

            //notify user of valid alternate sorts
            else {
                fprintf(stderr, "Argument for ALTERNATE parameter must be s, S, i, or I\n");
                help();
            }

            continue;
        }

        //checks for SEED argument
        if (strcmp("-r", argv[i]) == 0) {

            if(strcmp(argv[i + 1], "-1") == 0) {

                //sets seed to clock if user specifies -1
                seed = clock();
                i++;

            }

            else {

                if(intCheck(argv[i + 1])) {

                    //sets seed to user passed arg
                    seed = atoi(argv[i+1]);
                    i++;

                }

            }

            continue;
        }

        //checks for MULTITHREAD argument
        if (strcmp("-m", argv[i]) == 0) {

            //if no, set the boolean value false
            if (strcmp(argv[i + 1], "n") == 0 || strcmp(argv[i + 1], "N") == 0) {

                multiThread = false;
                i++;

            }

            //if yes boolean is true
            else if (strcmp(argv[i + 1], "y") == 0 || strcmp(argv[i + 1], "Y") == 0) {

                multiThread = true;
                i++;

            }

            //invalid user arg, let them know
            else {

                fprintf(stderr, "Argument for MULTITHREAD parameter must be n, N, y, or Y\n");
                help();

            }

            continue;
        }

        //checks for PIECES argument
        if (strcmp("-p", argv[i]) == 0) {

            if(intCheck(argv[i + 1])) {

                //retrieve pieces if it is a valid int
                if (atoi(argv[i + 1]) < size) {

                    pieces = atoi(argv[i + 1]);
                    i++;

                }

                else {
                    printf("Cannot divide the list into pieces larger than size of the array.\n");
                    exit(0);
                }

            }

            continue;
        }

        //checks for MAXTHREADS argument
        if (strcmp("-t", argv[i]) == 0) {

            if(intCheck(argv[i + 1])) {

                if(atoi(argv[i + 1]) <= 4) {

                    maxThread = atoi(argv[i + 1]);
                    i++;

                }
            }

            continue;
        }

        //checks for MEDIAN argument
        if (strcmp("-m3", argv[i]) == 0) {

            if(strcmp(argv[i + 1], "y") == 0 || strcmp(argv[i + 1], "Y") == 0) {

                median = true;
                i++;

            }

            else if (strcmp(argv[i + 1], "n") == 0 || strcmp(argv[i + 1], "N") == 0) {

                median = false;
                i++;

            }

            else {

                printf("Median must be specified by y, Y, n, or N\n");

            }

            continue;
        }

        else {

            //prints out a list of valid parameters and arg values
            help();

        }

    }

    //cannot have more threads than pieces to run on
    if (maxThread > pieces) {

        printf("MAXTHREADS must be smaller than PIECES.");
        exit(0);

    }

    //allocate memory for the segments to be held, and set the initial segment up
    segments = malloc(pieces * sizeof(struct segmentData));
    segments[0].lo = 0; segments[0].hi = size-1; segments[0].size = size;

    //initialize segments data to 0
    for (int i = 1; i < pieces; i ++) {

        segments[i].hi = 0;
        segments[i].lo = 0;
        segments[i].size = 0;

    }

    //wall time taken to create the array in memory
    gettimeofday(&startCreate, NULL);

    //allocate memory for SIZE 32-bit integers
    array = malloc(size * sizeof(uint32_t));

    gettimeofday(&endCreate, NULL);
    timeIntervalCreate = (endCreate.tv_sec - startCreate.tv_sec);
    timeIntervalCreate += 1e-6 * (endCreate.tv_usec - startCreate.tv_usec);

    gettimeofday(&startPopulate, NULL);

    //fill the array from 0 - SIZE-1 with the index
    for(uint32_t j = 0; j < size; j++) {

        array[j] = j; //initializes array with values from 0 through SIZE-1

    }

    //wall time taken to fill the array
    gettimeofday(&endPopulate, NULL);
    timeIntervalPopulate = (endPopulate.tv_sec - startPopulate.tv_sec);
    timeIntervalPopulate += 1e-6 * (endPopulate.tv_usec - startPopulate.tv_usec);
    
    gettimeofday(&startScramble, NULL);

    //sets random with seed specified by user. Default is 1.
    srand(seed);
    
    //scramble the array
    for (int k = size-1; k > 0; k--) {

        int index = random() % (k + 1);
        int temp = array[k]; array[k] = array[index]; array[index] = temp;

    }

    //wall time taken to scramble the array given a user specified seed
    gettimeofday(&endScramble, NULL);
    timeIntervalScramble = (endScramble.tv_sec - startScramble.tv_sec);
    timeIntervalScramble += 1e-6 * (endScramble.tv_usec - startScramble.tv_usec);

    //enters the first partition section to get the segments split up (see quickSort for comments)

    uint32_t i = 0; //start i at beginning of array
    uint32_t j = size; //start j at end of array
    uint32_t X = array[0]; //set a pivot holder
    uint32_t med; //median of three var

    //split into pieces, pass a piece to a sorting function on a thread using the high and lo indicies, store it, sort the subarrays
    for (int count = 0; count <= pieces-2; count++) { //loop until segments is populated

        if (median) {

            uint32_t k = (segments[0].lo + segments[0].hi) / 2;
            uint32_t A = array[segments[0].lo]; uint32_t B = array[k]; uint32_t C = array[segments[0].hi];

            if ((A >= B && A <= C) || (A>=C && A<=B)) {
                med = segments[0].lo;
            }

            else if ((B >= A && B <= C) || (B >= C && B <= A)) {
                med = k;
            }

            else {
                med = segments[0].hi;
            }

            uint32_t hold = array[segments[0].lo];
            array[segments[0].lo] = array[med];
            array[med] = hold;
            X = array[segments[0].lo];
        }

        do {

            do i++; while (array[i] < X);
            do j--; while (array[j] > X);
            if (i < j) {int temp = array[i]; array[i] = array[j]; array[j] = temp;}
            else break;

        } while (1);

        //swap the pivot back into place with the lo index of the seg
        array[segments[0].lo] = array[j];
        array[j] = X;
        
        segments[count + 1].lo = i;
        //needs to be size-1 for first iteration, then be segments[0].hi
        segments[count + 1].hi = segments[0].hi;
        //update our struct in the segment array with the other split end
        segments[count + 1].size = segments[count + 1].hi - segments[count + 1].lo + 1;

        //update our struct in the segment array
        segments[0].lo = segments[0].lo;
        segments[0].hi = j;
        segments[0].size = segments[0].hi - segments[0].lo + 1;

        //always will place the largest segment at 0 index
        for (int i = 0; i < pieces; i++) {

            if (segments[i].size > segments[0].size) {

                struct segmentData temp = segments[0];
                segments[0]  = segments[i];
                segments[i] = temp;

            }

        }

        //set indexes again to updated largest segment
        i = segments[0].lo;
        j = segments[0].hi + 1;
        X = array[segments[0].lo];
    }

    //sorts the segments so that they are in size order
    for(int i = 1; i < pieces; i++) {

        struct segmentData key = segments[i];
        int j = i - 1;
        
        while (j >= 0 && segments[j].size < key.size) {
            segments[j + 1] = segments[j];
            j = j-1;

        }

        segments[j + 1]= key;
    }

    //cputimer
    clock_t cpuStart = clock();

    //wall clock time
    gettimeofday(&startSort, NULL);

    //multithreaded execution calls threads which in turn will call quickSort on each thread
    if (multiThread) {

        thread();

    }

    //run sorting method on the whole array if it is not multithreaded
    else {

        for (int segCount = 0; segCount < pieces; segCount++) {

            quickSort(segments[segCount], threshold);

        }

    }

    clock_t cpuEnd = clock();

    gettimeofday(&endSort, NULL);
    timeIntervalSort = (endSort.tv_sec - startSort.tv_sec);
    timeIntervalSort += 1e-6 * (endSort.tv_usec - startSort.tv_usec);

    //if array is properly sorted, success!
    if (isSorted()) {
        //print the time stamps for everything
        printf("Array Creation: %0.3f seconds Array Population: %0.3f seconds Array Scramble: %0.3f seconds CPU Time: %0.3f Wall Time: %0.3f\n",
        timeIntervalCreate, timeIntervalPopulate, timeIntervalScramble, (double) (cpuEnd - cpuStart) / CLOCKS_PER_SEC, timeIntervalSort);
        printf("Array is sorted properly.\n");
        return 0;
    }

    //array did not sort properly
    else {
        printf("failure");
        exit(1);
    }
}