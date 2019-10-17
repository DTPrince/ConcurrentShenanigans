#ifndef BARRIERS_HPP
#define BARRIERS_HPP

#include <atomic>
#include <pthread.h>

#define SEQ_CST             std::memory_order_seq_cst
#define ACQ                 std::memory_order_acquire
#define REL                 std::memory_order_release
#define RELAXED             std::memory_order_relaxed

typedef enum BarrierType{
    BARRIER_TYPE_INVALID,
    BARRIER_TYPE_SENSE,
    BARRIER_TYPE_STD
} BarrierType;

class BarrierInterface {
public:
    int num_threads;    //global variable in main cpp file

    virtual void wait();

    virtual ~BarrierInterface();
};

class SenseReverse_Barrier : BarrierInterface {
public:
    std::atomic<int> count;
    std::atomic<int> sense;

    thread_local static bool my_sense;

    SenseReverse_Barrier(int n = 0);

    void wait();
};

class Barrier : BarrierInterface {
public:
    pthread_barrier_t barrier;

    Barrier(int n = 0);

    void wait();
};


// I didnt know what to call it so I mimicked lockBox
class BarrierBox {
public:
    BarrierType btype = BARRIER_TYPE_INVALID;

    Barrier *std_barrier;
    SenseReverse_Barrier *sr_barrier;

    BarrierBox(BarrierType barrierType, int n = 0);
    ~BarrierBox();

    void wait();
};


#endif // BARRIERS_HPP
