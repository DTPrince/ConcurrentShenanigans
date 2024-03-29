// Derek Prince
// ECEN - 5033 Concurrent Programming
// Lab 0 - Text Merge/Quick Sort
// Due 9/6/19

#include <string>
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

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

void my_quicksort(std::vector<int> &my_vec, int lo, int hi) 
{	
	std::cout << "Quicksort top.\n";
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

int main(int argc, char* argv[]) 
{
	cxxopts::Options options(argv[0], "Text parser for lab 1.");

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
			std::cout << "No input file given" << std::endl;
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
	my_quicksort(file_contents, 0, file_contents.size() - 1);

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
