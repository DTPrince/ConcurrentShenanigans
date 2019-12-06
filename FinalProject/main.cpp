#include <iostream>
#include <exception>
#include <pthread.h>
#include <atomic>

#include "include/cxxopts.hpp"


int main(int argc, char* argv[]) {
    std::cout << "Hello, World!" << std::endl;

    cxxopts::Options options(argv[0], "Text parse and sort for lab 2.");

    std::string input, output;

    options.add_options()(
            "n, name", "My name (for grading purposes)", cxxopts::value<bool>())(
            "i,input", "input file name", cxxopts::value<std::string>(), "FILE")(
            "o, output", "Output file name", cxxopts::value<std::string>(), "FILE")(
            "t, threads", "Number of threads to be used", cxxopts::value<int>(), "NUM_THREADS")(
            "h, help", "Display help options");

    std::string ofile = "";
    std::string ifile = "";

    // --- Opt parse block ---
    try {
        options.parse_positional({ "input" });
        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help({ "", "Group" }) << std::endl;
            return 0;
        }

        if (result.count("name")) {
            std::cout << "Derek Prince" << std::endl;
            return 0;
        }

        // Output file name
        if (result.count("o")) {
            ofile = result["o"].as<std::string>();
        } else { // default name if none given
            ofile = "output.txt";
        }

        // Input file name
        if (result.count("i")) {
            ifile = result["i"].as<std::string>();
        } else {
            std::cout << "No input file given" << std::endl;
            return (0);
        }
         int NUM_THREADS;
        // Extract thread number information
        if (result.count("threads")) {
            NUM_THREADS = result["t"].as<int>();
            if (NUM_THREADS > 150) {
                printf("Error: Too many threads\n");
                return (-1);
            }
        } else
            NUM_THREADS = 5;
    }
        // Catch unknown options and close nicely
    catch (std::exception& e) {
        std::cout << e.what() << std::endl;
        return -1;
    }
    // -- END Opt parse block ---

    return 0;

}
