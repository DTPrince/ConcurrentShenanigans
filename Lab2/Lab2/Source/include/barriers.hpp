#ifndef BARRIERS_HPP
#define BARRIERS_HPP

#include <atomic>
#include <pthread.h>

#define SEQ_CST             std::memory_order_seq_cst
#define ACQ                 std::memory_order_acquire
#define REL                 std::memory_order_release
#define RELAXED             std::memory_order_relaxed

typedef enum BarrierType{
    BARRIER_TYPE_SENSE_REV,
    BARRIER_TYPE_STD
} BarrierType;

class BarrierInterface {
public:


    std::atomic<int> count;
    std::atomic<int> sense;

    int num_threads;    //global variable in main cpp file

    virtual void wait();

    virtual ~BarrierInterface();
};

class SenseReverse_Barrier : BarrierInterface {
public:
    SenseReverse_Barrier(){ num_threads = 0; }
    SenseReverse_Barrier(int n){ num_threads = n; }

    void wait(){
        thread_local bool my_sense = 0;
        if (my_sense == 0)
            my_sense = 1;
        else
            my_sense = 0;

        int count_cpy = count.fetch_add(1);

        if (count_cpy == num_threads){
            count.store(0, RELAXED);
            sense.store(my_sense);
        }
        else{
            while(sense.load() != my_sense);    //my_sense might be a bit not right
        }
    }
};

class Barrier : BarrierInterface {
public:
    Barrier(){ num_threads = 0; }
    Barrier(int n){ num_threads = n; }

    int wait_for = num_threads; // by default

    void wait(){
        count++;
        while (count.load() < wait_for){
            // nada
        }
    }
};

#endif // BARRIERS_HPP
