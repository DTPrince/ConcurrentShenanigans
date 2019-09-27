// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 1 - Concurrent Text Sort with Quicksort and Bucketsort
// Due 9/25/19

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <exception>
#include <pthread.h>

#include "include/cxxopts.hpp"


#define FORK_JOIN 1
#define BUCKET 0
#define ALG_UNDEFINED 10    // arbitrary number. No inclined to use negative numbers at this time to avoid potential unsigned/signed cross-pollination

#define THREAD_INACTIVE true
#define THREAD_ACTIVE false
// Played with this number a bit but it'll depend on the machine so I'm not too woried about the +/-
#define LIST_SZ_LIMIT 12

static pthread_t* threads;
static int* no_threads;
static int NUM_THREADS;
static struct timespec start, end;

// 10 ave -> 207.16
// 11 ave -> 197.6
// 12 ave -> 171

typedef struct{
    std::vector<int> *vec;
    int lo, hi, tid;
} thread_vec;

typedef struct {
    int* status;
    int no_active;
} thread_status;

static thread_status t_status;

void global_init(){
    threads = static_cast<pthread_t*>(malloc(NUM_THREADS*sizeof(pthread_t)));
    no_threads = static_cast<int*>(malloc(NUM_THREADS*sizeof(int)));
}
void global_cleanup(){
    free(threads);
    free(no_threads);
}

// semi-intelligently find the partition index to divvy up the work load
int partition(std::vector<int> &my_vec, int lo, int hi)
{
    // Median of 3 might be better for large data sets but rand modulus sounds expensive
    int pivot = my_vec[hi];

	//pre-emptive decrement
    int i = lo - 1;

	// iterate over sub-array and swap if needed
    for (int j = lo; j < hi; j++)
	{
		if (my_vec[j] < pivot) {
			i++;
			std::swap(my_vec[i], my_vec[j]);
		}
	}
	std::swap(my_vec[i + 1], my_vec[hi]);

    /*
     * This partition function 'weighs' the vector to see how uneven or pre-sorted
     * the array is. As i only increments when there is a swap, it is a semi-direct way of
     * labeling how many times the array was swapped with respect to the highest location
     * (which will be the highest number in the array after quicksort finishes)
     * In other words, the highest position is supposed to be the highest number
     * so if i is still -1 by the time it iterates over all of it then we know
     * the upper location is already sorted and pi = 0
    */
	return (i + 1);
}

// my_qsort from lab0
void my_quicksort(std::vector<int> &my_vec, int lo, int hi)
{
    // exit condition
    if (lo < hi)
    {
        // split array by oartitioning into two arrays, returning index (pi) of middle
        int pi = partition(my_vec, lo, hi);

        // now that split has been found, quicksort the two partitions
        my_quicksort(my_vec, lo, pi - 1);
        my_quicksort(my_vec, pi + 1, hi);
    }
}

// Standard insertion sort to take care of remainder of array sorting when creating threads has too much overhead.
void my_insertionsort(std::vector<int> &vec, int sz){
    int i, key, j;
    for (i = 0; i< sz; i++){
        // grab key to check against
        key = vec[i];

        j = i - 1; // like j = 0 but starts at ~i
        // iterate backwards while index value is greater than key
        while( (j >= 0) && (vec[j] > key) ){
            vec[j+1] = vec[j];
            j--;
        }
        vec[j + 1] = key;
    }
}

//only spawn one new thread. There is no reason to spawn 2 as the main thread will be idle anyhow
// Dont make new threads once the list becomes small enough. It has too much overhead
void *pqsort(void* arg){
    // Master/managing thread
    thread_vec *t_vec = static_cast<thread_vec *>(arg);
    if(t_vec->tid==0){
        clock_gettime(CLOCK_MONOTONIC,&start);
    }

    if(t_vec->lo < t_vec->hi)
    {
        if ( (t_vec->hi - t_vec->lo) < LIST_SZ_LIMIT)
        {
            //sort with straight-forward method and smaller overhead
            my_insertionsort(*t_vec->vec, t_vec->vec->size());
        }
         else
         {
            if (t_vec->tid == NUM_THREADS) {
                // Sort in a single thread.
                // Possibility of checking for old, completed threads here but... Eh. So much could go wrong and this isn't the design for it
                my_quicksort(*t_vec->vec, t_vec->lo, t_vec->hi);
            }
            else {
                // split array by oartitioning into two arrays, returning index (pi) of middle
                int pi = partition(*t_vec->vec, t_vec->lo, t_vec->hi);

                // now that split has been found, quicksort the two partitions
                // Create thread for upper/right side
                thread_vec t_vec_nthread;
                t_vec_nthread.vec = t_vec->vec;
                t_vec_nthread.hi = t_vec->hi;
                t_vec_nthread.lo = pi + 1;
                t_vec_nthread.tid = t_vec->tid + 1;


                // Create upper in parallel
                pthread_create(&threads[t_vec_nthread.tid], nullptr, pqsort, &t_vec_nthread);
                // Run next branch in same thread (as there is nothing to do anymore here)
                my_quicksort(*t_vec->vec, pi + 1, t_vec->hi);

                // rejoin thread
                pthread_join(threads[t_vec_nthread.tid], nullptr);
            }
        }
    }
    if(t_vec->tid==0){
        clock_gettime(CLOCK_MONOTONIC,&end);
    }
    pthread_exit(nullptr);
}

void my_bucketsort();

int main(int argc, char* argv[]) 
{
	cxxopts::Options options(argv[0], "Text parser for lab 1.");

	std::string input, output;

	options.add_options()
		("n, name", "My name (for grading purposes)", cxxopts::value<bool>())
		("i,input", "input file name", cxxopts::value<std::string>(), "FILE")
		("o, output", "Output file name", cxxopts::value<std::string>(), "FILE")
        ("t, threads", "Number of threads to be used", cxxopts::value<int>())
        ("alg", "Algorithm to sort with", cxxopts::value<std::string>())
		("h, help", "Display help options")
		;

	std::string ofile = "";
    std::string ifile = "";
    std::string algorithm = "";
    int algType = ALG_UNDEFINED;

    // --- Opt parse block ---
	try 
	{
		options.parse_positional({ "input" });
		auto result = options.parse(argc, argv);

		if (result.count("help"))
		{
			std::cout << options.help({ "", "Group" }) << std::endl;
			return 0;
		}

		if (result.count("name"))
		{
			std::cout << "Derek Prince" << std::endl;
			return 0;
		}

        // Output file name
		if (result.count("o"))
		{
			ofile = result["o"].as<std::string>();
		}
		else
		{	// default name if none given
			ofile = "output.txt";
		}

        // Input file name
		if (result.count("i"))
		{
			ifile = result["i"].as<std::string>();
		}
		else
		{
			std::cout << "No input file given" << std::endl;
            return(0);
		}

        // Extract algorithm type
        if (result.count("alg")){
            algorithm = result["alg"].as<std::string>();
            if (algorithm == "fj")
                algType = FORK_JOIN;
            else if (algorithm == "bucket")
                algType = BUCKET;
            else {
                printf("Improper algorithm argument supplied, Fork/Join assumed");
                algType = FORK_JOIN;
            }
        }
        else {
            printf("No algorithm selected, exiting\n");
            return 0;
        }

        // Extract thread number information
        if (result.count("threads")){
            NUM_THREADS = result["t"].as<int>();
            if (NUM_THREADS > 150){
                printf("Error: Too many threads\n");
                return(-1);
            }
        }
        else
            NUM_THREADS = 5;
	}
	// Catch unknown options and close nicely
	catch(std::exception& e) 
    {
        std::cout << e.what() << std::endl;
        return -1;
	}
    // -- END Opt parse block ---


	// --- Gather file data ---
	std::ifstream infile(ifile);

    std::vector<int> file_contents;

	if (infile.is_open())
	{
		std::string temp;
		// populate vector w/ contents
		while (infile >> temp)
		{
			// Attempt to convert str to int, catch errors
			try
			{
                file_contents.push_back(atoi(temp.c_str()));
			}
			catch (std::exception& e)
			{
				std::cout << e.what();
			}
		}
	}
	else
		std::cout << "File " << ifile << " did not open.";

	// This is just in case the output file is the same as the input
	infile.close();
    // --- END Gather file data ---

	// --- Sort data ---
    switch (algType) {
    case FORK_JOIN:
        int ret;

        global_init();
        thread_vec t_vec;
        t_vec.vec = &file_contents;
        t_vec.hi = file_contents.size() - 1;
        t_vec.lo = 0;
        t_vec.tid = 0;
        if (NUM_THREADS == 1){  // Not parallel. Avoids checking in the parallel version every loop
            if (t_vec.vec->size() > LIST_SZ_LIMIT){
                clock_gettime(CLOCK_MONOTONIC,&start);
                my_insertionsort(*t_vec.vec, t_vec.vec->size());
                clock_gettime(CLOCK_MONOTONIC,&end);
            }
            else {
                clock_gettime(CLOCK_MONOTONIC,&start);
                my_quicksort(*t_vec.vec, t_vec.lo, t_vec.hi);
                clock_gettime(CLOCK_MONOTONIC,&end);
            }
        }
        else {
            ret = pthread_create(&threads[0], nullptr, &pqsort, &t_vec);
            if (ret){
                printf("Error: in creating master thread %d\n", ret);
                return -1;
            }
            // Join master
            ret = pthread_join(threads[0], nullptr);
            if (ret){
                printf("Error: in creating master thread %d\n", ret);
                return -1;
            }
            global_cleanup();
        }
        break;

    case BUCKET:
        //my_bucketsort();
        break;
    case ALG_UNDEFINED:
        printf("This should have been set or returned before getting here\n");
        return -1;
    default:
        printf("How even\n");
        return -1;
    }

    // Print sorted list
//    for (std::vector<int>::const_iterator i = file_contents.begin(); i != file_contents.end(); i++) {
//		std::cout << *i << std::endl;
//	}

    // Print time
    unsigned long long elapsed_ns;
    elapsed_ns = (end.tv_sec-start.tv_sec)*1000000000 + (end.tv_nsec-start.tv_nsec);
    printf("Elapsed (ns): %llu\n",elapsed_ns);
    double elapsed_s = ((double)elapsed_ns)/1000000000.0;
    printf("Elapsed (s): %lf\n",elapsed_s);

	// --- Write data ---
	std::ofstream outfile(ofile);

	if (outfile.is_open())
	{
		try
		{
            for (std::vector<int>::const_iterator i = file_contents.begin(); i != file_contents.end(); i++)
			{
				outfile << *i << std::endl;
			}
		}
		catch (std::exception& e)
		{
			std::cout << e.what();
		}
	}
	return 0;
}
