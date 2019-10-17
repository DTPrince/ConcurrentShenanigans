#include <atomic>
#include "include/locks.hpp"
#include "include/cpu_relax.hpp"

// --- Making the compliler happy ---
LockInterface::~LockInterface(){}


// --- TAS_Lock ---
TAS_Lock::~TAS_Lock(){}

void TAS_Lock::acquire(){
    bool expect_false = false;
    // using CAS instead of TAS since one is a real member function and one is not
    // Expect false so that it will only swap with `true` (and thus acquiring lock)
    // once the lock is free.
    // Invert becasue flag.CAS(...) will return false here when flag is locked (value of true)
    while(!flag.compare_exchange_weak(expect_false, true));    // nada
}

void TAS_Lock::release(){
    flag.store(false);
}


// --- TTAS_Lock ---
void TTAS_Lock::acquire(){
    bool expect_false = false;
    // same as above but with load check.
    while(flag.load() == true || !flag.compare_exchange_weak(expect_false, true));
}

void TTAS_Lock::release(){
    flag.store(false);
}


// --- Ticket_Lock ---
Ticket_Lock::Ticket_Lock(){
    serving.store(0);
    next.store(0);
}

void Ticket_Lock::acquire(){
    // This became atomic because running more iterations
    // in the counter bench than there were threads would
    // cause unexpewcted flogging of my_num when the compiler
    // launches threads seemingly at random and the fetch/++
    // would be done in unexpected orders. Meaning a thread
    // could be both the released and the next in line, causing
    // weird shit to happen with my_num accesses.
    thread_local std::atomic<int> my_num;
    my_num.store(next.fetch_add(1));// = next.fetch_add(1);
    while (serving.load() != my_num);   // wait
}

void Ticket_Lock::release(){
    // overloaded in std::atomic
    serving++;
}



// --- MCS_Lock ---
thread_local MCS_Node MCS_Lock::qnode;

MCS_Lock::MCS_Lock(){
    tail.store(nullptr);
}

void MCS_Lock::acquire(){
    // place new node at end of queue and store address of previous last node
    // for checking and also to update next-> ptr
    MCS_Node *prev = tail.exchange(&this->qnode);


    if (prev != nullptr){
        this->qnode.locked = true;
        prev->next = &this->qnode;

        // wait for lock to free from MCS_Lock::release();
        while(this->qnode.locked){
            // cpu_relax comes from ifknot and mfukar suggesting using it. See credits.
            cpu_relax();
        }
    }

}

// Over-commented for my sanity
void MCS_Lock::release(){
    // https://libfbp.blogspot.com/2018/01/c-mellor-crummey-scott-mcs-lock.html

    // check if there is a next node waiting
    if (this->qnode.next == nullptr){
        // Check to see if this node is the last one waiting and return if so
        MCS_Node *temp = &this->qnode;
        // 4-argument compare_exchange_strong required to use nullptr argument
        if (tail.compare_exchange_strong(temp, nullptr, REL, RELAXED)){
            return;
        }
        // Otherwise the next node is still being set up so wait for it
        while (this->qnode.next == nullptr);
    }

    // Signal next node that the lock is free
    this->qnode.next->locked = false;

    // This line makes a world of a difference
    this->qnode.next = nullptr;
}


// --- Mutex_Lock ---
Mutex_Lock::Mutex_Lock(){
    lock = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_init(&lock, nullptr);
}

Mutex_Lock::~Mutex_Lock(){
    pthread_mutex_destroy(&lock);
}

void Mutex_Lock::acquire(){
    // Does wait for lock though it took some convincing
    // to remind me
    pthread_mutex_lock(&this->lock);
}

void Mutex_Lock::release(){
    pthread_mutex_unlock(&this->lock);
}


// --- AFlag_Lock ---
void AFlag_Lock::acquire(){
    // default SEQ_CST
    flag.test_and_set();
}

void AFlag_Lock::release(){
    flag.clear();
}


// --- LockBox ---
LockBox::LockBox(LockType lt){
    ltype = lt;
    switch(ltype){
    case (LOCK_TYPE_TAS):{
        tas_lock = new TAS_Lock();
        break;
    }
    case (LOCK_TYPE_TTAS):{
        ttas_lock = new TTAS_Lock();
        break;
    }
    case (LOCK_TYPE_TICKET):{
        ticket_lock = new Ticket_Lock();
        break;
    }
    case (LOCK_TYPE_MCS):{
        mcs_lock = new MCS_Lock();
        break;
    }
    case (LOCK_TYPE_PTHREAD):{
        mutex_lock = new Mutex_Lock();
        break;
    }
    case (LOCK_TYPE_AFLAG):{
        aflag_lock = new AFlag_Lock();
        break;
    }
    case (LOCK_TYPE_INVALID):
    default:
        // handle as desired... For now just return -1.
        break;
    }
}

LockBox::~LockBox(){
    switch(ltype){
    case (LOCK_TYPE_TAS):{
        delete tas_lock;
        break;
    }
    case (LOCK_TYPE_TTAS):{
        delete ttas_lock;
        break;
    }
    case (LOCK_TYPE_TICKET):{
        delete ticket_lock;
        break;
    }
    case (LOCK_TYPE_MCS):{
        delete mcs_lock;
        break;
    }
    case (LOCK_TYPE_PTHREAD):{
        delete mutex_lock;
        break;
    }
    case (LOCK_TYPE_AFLAG):{
        delete aflag_lock;
        break;
    }
    case (LOCK_TYPE_INVALID):
    default:
        // handle as desired... For now just return -1.
        break;
    }
}

int LockBox::acquire(){
    switch(ltype){
    case (LOCK_TYPE_TAS):{
        tas_lock->acquire();
        break;
    }
    case (LOCK_TYPE_TTAS):{
        ttas_lock->acquire();
        break;
    }
    case (LOCK_TYPE_TICKET):{
        ticket_lock->acquire();
        break;
    }
    case (LOCK_TYPE_MCS):{
        mcs_lock->acquire();
        break;
    }
    case (LOCK_TYPE_PTHREAD):{
        mutex_lock->acquire();
        break;
    }
    case (LOCK_TYPE_AFLAG):{
        aflag_lock->acquire();
        break;
    }
    case (LOCK_TYPE_INVALID):
    default:
        // handle as desired... For now just return -1.
        return -1;
        break;
    }
    return 0;
}

int LockBox::release(){
    switch(ltype){
    case (LOCK_TYPE_TAS):{
        tas_lock->release();
        break;
    }
    case (LOCK_TYPE_TTAS):{
        ttas_lock->release();
        break;
    }
    case (LOCK_TYPE_TICKET):{
        ticket_lock->release();
        break;
    }
    case (LOCK_TYPE_MCS):{
        mcs_lock->release();
        break;
    }
    case (LOCK_TYPE_PTHREAD):{
        mutex_lock->release();
        break;
    }
    case (LOCK_TYPE_AFLAG):{
        aflag_lock->release();
        break;
    }
    case (LOCK_TYPE_INVALID):
    default:
        // handle as desired... For now just return -1.
        return -1;
    }
    return 0;
}
