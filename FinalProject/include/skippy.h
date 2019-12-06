//
// Created by d on 12/5/19.
//

#ifndef SKIPPY_H
#define SKIPPY_H

#include <vector>

class SkipListNode {
public:
    SkipListNode * next;

    int data;
    int level;
};

class Skippy {
public:
    int max_level;
    std::vector<SkipListNode*> highways;
    SkipListNode * head;

    std::vector<int> get_range(int, int);

    void put();
    int get(int);

};


#endif //SKIPPY_H
