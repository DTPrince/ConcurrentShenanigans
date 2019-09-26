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

static pthread_t* threads;
static int* no_threads;
static int NUM_THREADS;
static pthread_barrier_t bar;

//static struct timespec start, end;

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
    t_status.status = static_cast<int*>(malloc(NUM_THREADS*sizeof(int)));

    for (int i = 0; i < NUM_THREADS; i++)
        t_status.status[i] = THREAD_INACTIVE;
    t_status.no_active = 0;

    pthread_barrier_init(&bar, nullptr, NUM_THREADS);
}
void global_cleanup(){
    free(threads);
    free(no_threads);
    free(t_status.status);
    pthread_barrier_destroy(&bar);
}


// semi-intelligently find the partition index to divvy up the work load
int partition(std::vector<int> &my_vec, int lo, int hi)
{
    // Median of 3 might be better for large data sets but rand modulus sounds expensive
//    int pivot = my_vec[static_cast<int>((lo + hi) / 2)];  // TODO: verify compiler translates to binary shift
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

// Threaded wrapper to set up my_quicksort() with all control variables nice and proper for multithreading
// Master thread, essentially. Though this basically creates the binary tree of partial orders
void *my_quicksort_thread(void *data){
    thread_vec *t_vec = static_cast<thread_vec *>(data);
    std::cout << "Parallel quicksort entered. tid: " << t_vec->tid << std::endl;

    int thread_used[2] = {0, 0};
    // exit condition
    if (t_vec->lo < t_vec->hi)
    {
        // split array by oartitioning into two arrays, returning index (pi) of middle
        int pi = partition(*t_vec->vec, t_vec->lo, t_vec->hi);

        // Recursive call depends on whole array to be settled so there are no parallel gains by threading before this
        // not *particularly* happy about allocating in the recursive call and then setting up t_vec but... oh well.
        thread_vec *t_vec_left = new thread_vec;
        t_vec_left->vec = t_vec->vec;        // TODO: double check this does indeed modify as expected (single vector object for program in main())
        t_vec_left->lo = t_vec->lo;
        t_vec_left->hi = pi - 1;

        // Hideous block used to find an idle thread. This should really be managed in a quasi-scheduler
        if (t_vec->tid + 1 < NUM_THREADS){ // Basically and out-of-range segfault check.
            if (t_status.status[t_vec->tid + 1] == THREAD_INACTIVE){ //Check if next thread is available
                t_vec_left->tid = t_vec->tid + 1;
                thread_used[0] = t_vec_left->tid;
                pthread_create(&threads[t_vec_left->tid], nullptr, &my_quicksort_thread, &t_vec_left);
                t_status.status[thread_used[0]] = THREAD_ACTIVE;
//                my_quicksort_thread(t_vec_left); // TODO: remove
            }
            else {
                // find next unused
                // Starting at 1 becasue the master (0) will always be in use because of the double join at the end of my_quicksort_thread
                // Tehre are many other starting options but they all get complicated quickly since threads can free on the other side of the tree
                int i = 1;
                while(t_status.status[t_vec->tid + 1] == THREAD_ACTIVE){
                    i++;
                    if (i == NUM_THREADS){
                        // There are no free threads. Continue sequentially
                        // or loop, hoping for a free thread eventually I 'spose. Bit risky
                        thread_used[0] = 0; // indicate not used
                        break;
                    }
                }
            }
        }
        else {
            // Else start at the beginning and try to find a free thread
            int i = 1;
            while(t_status.status[t_vec->tid + 1] == THREAD_ACTIVE){
                i++;
                if (i == NUM_THREADS){
                    // Same deal
                    thread_used[0] = 0; // indicate not used
                    break;
                }
            }
        }


        // Copy of left determine but applied to right side.
        thread_vec *t_vec_right = new thread_vec;
        t_vec_right->vec = t_vec->vec;        // TODO: double check this does indeed modify as expected (single vector object for program in main())
        t_vec_right->lo = pi + 1;
        t_vec_right->hi = t_vec->hi;

        if (t_vec->tid + 1 < NUM_THREADS){ // Basically and out-of-range segfault check.
            if (t_status.status[t_vec->tid + 1] == THREAD_INACTIVE){ //Check if next thread is available
                t_vec_right->tid = t_vec->tid + 1;
                thread_used[0] = t_vec_right->tid;
                pthread_create(&threads[t_vec_right->tid], nullptr, &my_quicksort_thread, &t_vec_right);
                t_status.status[thread_used[1]] = THREAD_ACTIVE;
//                my_quicksort_thread(t_vec_right); // TODO: remove
            }
            else {
                // find next unused
                // Starting at 1 becasue the master (0) will always be in use because of the double join at the end of my_quicksort_thread
                // Tehre are many other starting options but they all get complicated quickly since threads can free on the other side of the tree
                int i = 1;
                while(t_status.status[t_vec->tid + 1] == THREAD_ACTIVE){
                    i++;
                    if (i == NUM_THREADS){
                        // There are no free threads. Continue sequentially
                        // or loop, hoping for a free thread eventually I 'spose. Bit risky
                        thread_used[1] = 0; // indicate not used
                        break;
                    }
                }
            }
        }
        else {
            // Else start at the beginning and try to find a free thread
            int i = 1;
            while(t_status.status[t_vec->tid + 1] == THREAD_ACTIVE){
                i++;
                if (i == NUM_THREADS){
                    // Same deal
                    thread_used[1] = 0; // indicate not used
                    break;
                }
            }
        }

        // Feels like some real band-aid garbage. I dont want join to be called before the other thread has spawned though.
        if (thread_used[0] > 0) {
            t_vec_left->tid = thread_used[0];
            pthread_create(&threads[t_vec_left->tid], nullptr, &my_quicksort_thread, &t_vec_left);
            t_status.status[thread_used[0]] = THREAD_ACTIVE;
        }
        if (thread_used[1] > 0){
            t_vec_left->tid = thread_used[1];
            pthread_create(&threads[t_vec_right->tid], nullptr, &my_quicksort_thread, &t_vec_right);
            t_status.status[thread_used[1]] = THREAD_ACTIVE;
        }


        // joining here means the parent thread will not terminate until the children threads terminate
        // This is only significant when it comes to freeing threads on a heavily lopsided quicksort tree
        if (thread_used[0] > 0){
            pthread_join(threads[thread_used[0]], &data);
            t_status.status[thread_used[0]] = THREAD_INACTIVE;
        }
        else
            // call sequential sort here to give the other branch a chance to start before getting held up by sequential
            my_quicksort(*t_vec_left->vec, t_vec_left->lo, t_vec_left->hi);
        if (thread_used[1] > 0){
            pthread_join(threads[thread_used[1]], &data);
            t_status.status[thread_used[1]] = THREAD_INACTIVE;
        }
        else
            my_quicksort(*t_vec_right->vec, t_vec_right->lo, t_vec_right->hi);
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
            my_quicksort(*t_vec.vec, t_vec.lo, t_vec.hi);
        }
        else {
            ret = pthread_create(&threads[0], nullptr, &my_quicksort_thread, &t_vec);
            if (ret){
                printf("Error: in creating master thread %d\n", ret);
                return -1;
            }
            t_status.status[0] = THREAD_ACTIVE;
            // Join master
            ret = pthread_join(threads[0], nullptr);
            if (ret){
                printf("Error: in creating master thread %d\n", ret);
                return -1;
            }
            t_status.status[0] = THREAD_INACTIVE;
    //        my_quicksort_thread(&t_vec);
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

    for (std::vector<int>::const_iterator i = file_contents.begin(); i != file_contents.end(); i++) {
		std::cout << *i << std::endl;
	}

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
