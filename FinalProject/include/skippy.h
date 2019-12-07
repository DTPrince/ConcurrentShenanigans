//
// Created by d on 12/5/19.
//

#ifndef SKIPPY_H
#define SKIPPY_H

#include <vector>
#include <pthread.h>
#include <cstdlib>
#include <atomic>

using namespace std;

// Skip List Node
class SLNode {
public:
    // point to pointer so array allocation is possible
    // Array is used to store levels.
    SLNode ** next;

    // Built around ints because this isn't storing any real data,
    // just marking accesses
    int key;
    // T data;

    // Might as well use the class created for this exact purpose
    std::atomic_flag aflag = ATOMIC_FLAG_INIT;

    SLNode(int, int);
};

class Skippy {
public:
    // levels of "express lanes" above level 0
    int max_level;
    int c_level;    // current level

    // Fraction of nodes that also share a higher-level
    // node.
    float portion;

    // TODO: handle head insertion/removal
    SLNode * head;

    std::vector<int> get_range(int, int);

    void display(); // for debugging, really.

    void insert(int);
    SLNode * get(int);
    int get_randomLevel();
    static SLNode * createSLNode(int, int);

    Skippy(int, float); // max level and fraction of elements that
                        // share an upper level
};

/*
 *  h: ->  3
 *  ___________________________________
 *  0:     3  6  7  9  12 17 19 21 25 26
 *  1:     3  6  -- -- 12 17 -- -- 25 n
 *  2:     -- 6  -- -- 12 17 -- -- 25 n
 *  3:     -- -- -- -- 12 17 -- -- 25 n
 */
#endif //SKIPPY_H
