//
// Created by d on 12/5/19.
//

#include "skippy.h"
#include <iostream>

SLNode::SLNode(int k, int lvl) {
    this->key = k;

    next = new SLNode * [lvl + 1];
    // initialize.  Cant memset nnullptr so whatever.
    for (int i = 0; i < lvl + 1; i++){
        next[i] = nullptr;
    }
}

Skippy::Skippy(int mlevel, float p) {
    max_level = mlevel;
    portion = p;
    c_level = 0;

    head = new SLNode(-1, max_level);    // assign -1 for flagging though
                                                // at the moment there is no need
}

/*
 * The purpose of this function is to generate a random level
 * based on how many levels are allowed (max) and the desired
 * portion of elements should have upper levels associated
 *
 * This is done by interating over the number of max levels
 * and rolling the dice to see if a fraction higher than
 * this->portion is returned. So based on probability.
 */
int Skippy::get_randomLevel(){
    float r_num = (float)rand()/RAND_MAX;
    int lvl = 0;
    while (r_num < portion && lvl < max_level){
        lvl++;
        // roll again
        r_num = (float)rand()/RAND_MAX;
    }
    return lvl;
}

// wrapper for SLNode constructor, really
SLNode * Skippy::createSLNode(int k, int mLvl){
    SLNode * slnode = new SLNode(k, mLvl);
    return slnode;
}

/*
 * Skritchity-skratchity
 * Lock must be acquired before any modifications
 * Locks on both the previous and current node must be held
 * before any changes
 * ***SO LONG AS*** there is no ->prev pointer, the ->next lock need not be held.
 * as you cannot change the previous one without acquiring the lock
 *
 * Steps to add something:
 * Lock head, lock next, check next. (there might exist a solution to only lock after checking
 *                                      but I do not want to have to mess with the possibility
 *                                      of next changing between check and lock by another thread)
 * Unlock head, move next, lock next, check next
 * repeat. (loop until next[i]->key > key) -- code already exists
 *
 */

// I would like to not have to carry around a 'prev' variable for locks if possible.
// Hoping to just use ->next[i] as a reverse-previous access.
void Skippy::insert(int key) {
    // traversal node to find insert point
    SLNode * crawler = head;

    // must be max level because there is
    // no promise that we won't reach the
    // max level on insert
    SLNode * stitcher[max_level + 1];   // holds intermediary values to update with
    for (int i = 0; i < max_level + 1; i ++){
        stitcher[i] = nullptr;
    }

    // Now to find the location desired to insert between or at the end.
    // Inserting first node is handled as inserting on end of head, not as
    // a replacement for it.
    for (int i = c_level; i >= 0; i--) {
        while (crawler->next[i] != nullptr &&
               crawler->next[i]->key < key) {
            // update 'forwarding' pointers as search progresses
            crawler = crawler->next[i];
        }
        stitcher[i] = crawler;
    }

    // Shifting crawler forward after finding node means it can now
    // acceptably be nullptr and is no longer safe to reference without check
    crawler = crawler->next[0];

    // Here check if (first cond) we are at the end OR (second cond) the key has already been inserted
    if (crawler == nullptr || crawler->key != key){
        int rand_level = get_randomLevel();

        // If the random level is a greater depth than the current depth
        // then the head needs to be updated to point to the new node
        if (rand_level > c_level){
            for (int i = c_level + 1; i < rand_level + 1; i++){
                stitcher[i] = head;
            }
            // There is a new level depth now.
            c_level = rand_level;
        }

        // Now that the structure has been massaged into shape, create new node proper
        SLNode * i_node = createSLNode(key, rand_level);

        // insert by adjusting surrounding pointers.
        // recall that 'stitcher' was left in the position
        // just prior to insert point after accumulating
        // all previous pointers.
        for (int i = 0; i <= rand_level; i++){
            i_node->next[i] = stitcher[i]->next[i];
            stitcher[i]->next[i] = i_node;
        }
    }
}

int Skippy::pinsert(int key) {
//    int key = *(int *)k;

    // traversal node to find insert point
    SLNode * crawler = head;
    // Tracks previous node to manage hand-over-hand locking
    SLNode * previous = nullptr;
    // acquire lock or stall thread while waiting
    while (crawler->aflag.test_and_set()) {}

    // must be max level because there is
    // no promise that we won't reach the
    // max level on insert
    SLNode * stitcher[max_level + 1];   // holds intermediary values to update with
    for (int i = 0; i < max_level + 1; i ++){
        stitcher[i] = nullptr;
    }

    // Now to find the location desired to insert between or at the end.
    // Inserting first node is handled as inserting on end of head, not as
    // a replacement for it.
    for (int i = c_level; i >= 0; i--) {
        while (crawler->next[i] != nullptr &&
               crawler->next[i]->key < key) {
            // update 'forwarding' pointers as search progresses

            // clear previous flag
            if (previous != nullptr)
                previous->aflag.clear();
            previous = crawler;
            crawler = crawler->next[i];
            // acquire next flag
            while (crawler->aflag.test_and_set()) {}
        }
        stitcher[i] = crawler;
    }

    // Shifting crawler forward after finding node means it can now
    // acceptably be nullptr and is no longer safe to reference without check
    if (previous != nullptr)
        previous->aflag.clear();
    previous = crawler;
    crawler = crawler->next[0];
    // acquire next flag
    if (crawler != nullptr)
        while (crawler->aflag.test_and_set()) {}

    // Here check if (first cond) we are at the end OR (second cond) the key has already been inserted
    if (crawler == nullptr || crawler->key != key){
        int rand_level = get_randomLevel();

        // If the random level is a greater depth than the current depth
        // then the head needs to be updated to point to the new node
        if (rand_level > c_level){
            for (int i = c_level + 1; i < rand_level + 1; i++){
                stitcher[i] = head;
            }
            // There is a new level depth now.
            c_level = rand_level;
        }

        // Now that the structure has been massaged into shape, create new node proper
        SLNode * i_node = createSLNode(key, rand_level);

        // insert by adjusting surrounding pointers.
        // recall that 'stitcher' was left in the position
        // just prior to insert point after accumulating
        // all previous pointers.
        for (int i = 0; i <= rand_level; i++){
            i_node->next[i] = stitcher[i]->next[i];
            stitcher[i]->next[i] = i_node;
        }
    }

    // Make sure no locks are held on release.
    // This has to be called here in case of fisrt insert/append to end
//    if (previous != nullptr)
    previous->aflag.clear();
    if (crawler != nullptr)
        crawler->aflag.clear();

    return 0;
}

// This function will start in the same way as insert() by finding the node and then
// moving along the base row, accumulating the integers in a vector before returning.
// Results are strictly: lower <= results <= max.
std::vector<int>* Skippy::get_range(int lower, int upper) {
    SLNode * crawler = head;

    for (int i = c_level; i >= 0; i--) {
        while (crawler->next[i] != nullptr &&
               crawler->next[i]->key < lower) {
            crawler = crawler->next[i];
        }
    }
    crawler = crawler->next[0];
    std::vector<int> * range = new std::vector<int>;
    range->reserve(upper - lower);

    // crawler is now below 'lower' on thew base row. Just accumulate until next[0]->key > upper
    while (crawler->key < upper || crawler->next == nullptr){
        range->push_back(crawler->key);
        crawler = crawler->next[0];
    }

    return range;
}

// TODO: add actual locks to this. *Should* be easier than insert, though similar
// There is an argument to be made to lock the whole chain of values while accumulating
// them but I do not think the benefits outweigh the cost. There would never be a
// promise that the returned range is valid for any time past when it is returned.
// More to the point, I think it is out of the scope of comparing hand over hand
// locking on a list and left to a complete implementation of a skip list to be
// used on a real work-load bearing server.
std::vector<int>* Skippy::pget_range(int lower, int upper) {

    SLNode * crawler = head;

    for (int i = c_level; i >= 0; i--) {
        while (crawler->next[i] != nullptr &&
               crawler->next[i]->key < lower) {
            crawler = crawler->next[i];
        }
    }
    crawler = crawler->next[0];
    auto * range = new std::vector<int>;
    range->reserve(upper - lower);

    // crawler is now below 'lower' on thew base row. Just accumulate until next[0]->key > upper
    while (crawler->key < upper || crawler->next == nullptr){
        range->push_back(crawler->key);
        crawler = crawler->next[0];
    }

    pthread_exit((void *)range);
}

// Just iterate over list but hit every node from bottom up
void Skippy::display() {
    std::cout << "Skip list contents\n";
    for (int i = 0; i <= c_level; i++) {
        SLNode * slnode = head->next[i];
        std::cout << "Level: " << i << " : ";
        while (slnode != nullptr){
            std::cout << slnode->key << " ";
            slnode = slnode->next[i];
        }
        std::cout << std::endl;
    }
}