#include <pthread.h>
#include <atomic>

#include "include/barriers.hpp"


void BarrierInterface::wait(){}

BarrierInterface::~BarrierInterface(){}

Barrier::Barrier(int n){
    num_threads = n;
    pthread_barrier_init(&this->barrier, nullptr, num_threads);
}

void Barrier::wait(){
    pthread_barrier_wait(&this->barrier);
}


thread_local bool SenseReverse_Barrier::my_sense;
SenseReverse_Barrier::SenseReverse_Barrier(int n){
    num_threads = n;
}

void SenseReverse_Barrier::wait(){
//    if (this->my_sense == 0)
//        this->my_sense = 1;
//    else
//        this->my_sense = 0;
    this->my_sense = !this->my_sense;

    int count_cpy = count.fetch_add(1);

    if (count_cpy == num_threads - 1){
        count.store(0);
        sense.store(this->my_sense);
    }
    else{
        while(sense.load() != this->my_sense);    //my_sense might be a bit not right
    }
}

BarrierBox::BarrierBox(BarrierType barrierType, int n){
    btype = barrierType;
    switch (btype){
    case BARRIER_TYPE_STD:{
        std_barrier = new Barrier(n);
        break;
    }
    case BARRIER_TYPE_SENSE:{
        sr_barrier = new SenseReverse_Barrier(n);
        break;
    }
    case BARRIER_TYPE_INVALID:
    default:
        break;
    }
}

BarrierBox::~BarrierBox(){
    switch (btype){
    case BARRIER_TYPE_STD:{
        delete std_barrier;
        break;
    }
    case BARRIER_TYPE_SENSE:{
        delete sr_barrier;
        break;
    }
    case BARRIER_TYPE_INVALID:
    default:
        break;
    }
}

void BarrierBox::wait(){
    switch (btype){
    case BARRIER_TYPE_STD:{
        std_barrier->wait();
        break;
    }
    case BARRIER_TYPE_SENSE:{
        sr_barrier->wait();
        break;
    }
    case BARRIER_TYPE_INVALID:
    default:
        break;
    }
}
