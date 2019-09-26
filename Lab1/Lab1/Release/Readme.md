# Lab 1

### Parallel Quicksort and Bucketsort File Text
#### Derek Prince

This is a little something I call "Hot Garbage Thread Soup." 

At present it runs quicksort in parallel and ~on onoccassion~ (maybe 1/2 to 1/3rd of the time) returns a valid sort. As it is currently imcomplete as to the requirements laid out in the Lab1 doc I am submitting this version on time for posterity's sake as I think the code architecture needs a rework as a whole since I wound up making some weakly-held and poorly scheduled almost task-based thread scheme that looks like terrible and maintains R/W civility based on gentlemans (threads) honor that nobody will peek at their neighbors and each master thread in the binary-tree like partial order has a single set of data to modify.

Essentially what I'm saying is that I'm going to resubmit it and this is just for reference before the due date sails by (in 2 minutes).

#### File Descriptions
* Makefile - makefile to automate build with gcc.
* Readme.md - Markdown version of this PDF
* source/mysort.cpp - Entry point of program containing all relevant code.
* source/include/cxxopts.hpp - CLI argument parser library

#### Credits
[cxxopts](https://github.com/jarro2783/cxxopts) for command line parsing because nobody but C++ needs to solve this issue again.

#### Extant Bugs
None *known* bugs but unsupported file formats are largely untested. Letters and leading 0's are have proven to be ignored for all basic testing cases.

A file of only 0s will crash on overflow as it does an infinite recursion downwards. I don't realistically see this being a problem so I didnt limit it.

