//
// Created by d on 12/5/19.
//

#include "skippy.h"

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
    float r_num = rand()/RAND_MAX;
    int lvl = 0;
    while (r_num < portion && lvl < max_level){
        lvl++;
        // roll again
        r_num = rand()/RAND_MAX;
    }
    return lvl;
}

// wrapper for SLNode constructor, really
SLNode * Skippy::createSLNode(int k, int mLvl){
    SLNode * slnode = new SLNode(k, mLvl);
    return slnode;
}

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
            for (int i = c_level + 1; i < rand_level; i--){
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
        for (int i = 0; i < rand_level; i++){
            i_node->next[i] = stitcher[i]->next[i];
            stitcher[i]->next[i] = i_node;
        }
    }
}

// This function will start in the same way as insert() by finding the node and then
// moving along the base row, accumulating the integers in a vector before returning.
// Results are strictly: lower <= results <= max.
std::vector<int> Skippy::get_range(int lower, int upper) {
    SLNode * crawler = head;

    for (int i = c_level; i >= 0; i--) {
        while (crawler->next[i] != nullptr &&
               crawler->next[i]->key <= lower) {
            crawler = crawler->next[i];
        }
    }
    crawler = crawler->next[0];
    std::vector<int> range;
    range.reserve(upper - lower);

    // crawler is now below 'lower' on thew base row. Just accumulate until next[0]->key > upper
    while (crawler->next[0]->key < upper){
        range.push_back(crawler->key);
        crawler = crawler->next[0];
    }

    return range;
}