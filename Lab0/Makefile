CXX = g++
CLANG = clang++
CXXFLAGS = -Wall -Werror -std=c++0x
LLVMFLAGS = -Weverything -std=c++11 -stdlib=libc++
OBJ = mysort.o
OUTPUT = mysort

vpath %.h ./Source/include
vpath %.hpp ./Source/include
vpath %.cpp ./Source

all : gcc

gcc : $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(OUTPUT) $(OBJ)

mysort.o : main.cpp cxxopts.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

#Nope.
clang : $(OBJ)
	$(CLANG) $(LLVMFLAGS) $(OBJ) -o $(OUTPUT)


.PHONY : clean
clean :
	rm *.o

uninstall :
	rm mysort

# Creates a log to keep track of things changed
 #log : main.cpp $(OBJ)
 #	lpt -p $?
 #	touch log