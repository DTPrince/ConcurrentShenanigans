// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 3 - Text Merge/Quick Sort with OpenMP
// Due 11/1/19

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

#include <omp.h>

#include "include/cxxopts.hpp"


// semi-intelligently find the partition index to divvy up the work load
int partition(std::vector<int> &my_vec, int lo, int hi)
{
	//select pivot as value of upper
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

	// This partition function 'weighs' the vector to see how uneven or pre-sorted
	// the array is. As i only increments when there is a swap, it is a semi-direct way of
	// labeling how many times the array was swapped with respect to the highest location
	// (which will be the highest number in the array after quicksort finishes)
	// In other words, the highest position is supposed to be the highest number
	// so if i is still -1 by the time it iterates over all of it then we know
	// the upper location is already sorted and pi = 0
	return (i + 1);
}


// This does not have a way to determine the remaining work would be faster in a single thread
// because I wanted to see the effects of scaling OMP with the sort size, including skewed
// overhead/work ratios.
void my_quicksort(std::vector<int> &my_vec, int lo, int hi) 
{	
	// exit condition
	if (lo < hi)
	{
		// split array by oartitioning into two arrays, returning index (pi) of middle
		int pi = partition(my_vec, lo, hi);

        // now that split has been found, quicksort the two partitions
        // I have chosen tasks as there is no guarantee that the vector is even remotely balanced
        // -> so tasks allow them to run on without being barrier'd
        #pragma omp task default(none) shared(my_vec) firstprivate(lo, pi)
        // partition index (pi) could be shared but I just copied it provately instead
        // so that I owuld not have contention on accessing it. More testing needs to be
        // done to determine the effects, if any
        {
            my_quicksort(my_vec, lo, pi - 1);
        }
        #pragma omp task default(none) shared(my_vec) firstprivate(hi, pi)
        {
            my_quicksort(my_vec, pi + 1, hi);
        }
	}
}

int main(int argc, char* argv[]) 
{
    cxxopts::Options options(argv[0], "Text parse and sort with OpenMP.");

	std::string input, output;

	options.add_options()
		("n, name", "My name (for grading purposes)", cxxopts::value<bool>())
		("i,input", "input file name", cxxopts::value<std::string>(), "FILE")
		("o, output", "Output file name", cxxopts::value<std::string>(), "FILE")
		("h, help", "Display help options")
		;

	std::string ofile = "";
	std::string ifile = "";

	try 
	{
		//Check which known opts were passed
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

		if (result.count("o"))
		{
			ofile = result["o"].as<std::string>();
		}
		else
		{	// default name if none given
			ofile = "output.txt";
		}

		if (result.count("i"))
		{
			ifile = result["i"].as<std::string>();
		}
		else
        {
            std::cout << "No input file given\n\n";
            std::cout << options.help({ "", "Group" }) << std::endl;
            return 0;
		}
	}
	// Catch unknown options and close nicely
	catch(std::exception& e) 
	{
		std::cout << e.what();
	}

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

    // --- Sort data ---
    // define the quicksort as a parallel region for omp
    # pragma omp parallel default(none) shared(file_contents)
    {
        // tell omp to not wait for other sections to finish
        // as there are no other sections
        # pragma omp single nowait
        {
            my_quicksort(file_contents, 0, file_contents.size() - 1);
        }
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
