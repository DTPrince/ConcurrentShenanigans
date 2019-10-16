#include "include/locks.hpp"

// --- Making the compliler happy ---
LockInterface::~LockInterface(){}


// --- TAS_Lock ---
TAS_Lock::~TAS_Lock(){}

void TAS_Lock::acquire(MCS_Node *qnode){
    bool expect_false = false;
    // using CAS instead of TAS since one is a real member function and one is not
    // Expect false so that it will only swap with `true` (and thus acquiring lock)
    // once the lock is free.
    // Invert becasue flag.CAS(...) will return false here when flag is locked (value of true)
    while(!flag.compare_exchange_weak(expect_false, true));    // nada
}

void TAS_Lock::release(MCS_Node *qnode){
    flag.store(false);
}


// --- TTAS_Lock ---
void TTAS_Lock::acquire(MCS_Node *qnode){
    bool expect_false = false;
    // same as above but with load check.
    while(flag.load() == true || !flag.compare_exchange_weak(expect_false, true));
}

void TTAS_Lock::release(MCS_Node *qnode){
    flag.store(false);
}


// --- Ticket_Lock ---
Ticket_Lock::Ticket_Lock(){
    serving.store(0);
    next.store(0);
}

void Ticket_Lock::acquire(MCS_Node *qnode){
    thread_local int my_num = next.fetch_add(1);
    while (serving.load() != my_num);   // wait
}

void Ticket_Lock::release(MCS_Node *qnode){
    // overloaded in std::atomic
    serving++;
}


// --- MCS_Lock ---
void MCS_Lock::acquire(MCS_Node *qnode){
    // place new node at end of queue and store address of previous last node
    // for checking and also to update next-> ptr
    MCS_Node *prev = tail.exchange(qnode);   // `this` required to reference correct thread_local

    // recall that prev took the address of the last node.
    // meaning if it is null then there is no last node and the queue is empty.
    if (prev != nullptr){
        qnode->locked = true;

        prev->next = qnode;

        while (qnode->locked);   // wait for the lock to free, This is done from
                // `prev->next.locked = true;` in MCS_Lock::release();

    }
}

void MCS_Lock::release(MCS_Node *qnode){
    // grab successor node to set up exit and wake thread.
    MCS_Node *succ = qnode->next;

    // check if there is a waiting thread
    if (succ == nullptr){
        // TODO: discover why tf this wont `compare_exchange_strong`
        if (tail.load() == qnode){
            tail.store(nullptr);
            // None waiting, set null and return early.
            return;
        }
    }
    // wait for qnode.next to sync with tail (see mfukar in creadits)
    while (succ == nullptr);

    // wake next thread
    succ->locked = false;
}


// --- Mutex_Lock ---
void Mutex_Lock::acquire(MCS_Node *qnode){
    // Does wait for lock though it took some convincing
    // to remind me
    pthread_mutex_lock(&lock);
}

void Mutex_Lock::release(MCS_Node *qnode){
    pthread_mutex_lock(&lock);
}


// --- AFlag_Lock ---
void AFlag_Lock::acquire(MCS_Node *qnode){
    // default SEQ_CST
    flag.test_and_set();
}

void AFlag_Lock::release(MCS_Node *qnode){
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

int LockBox::acquire(MCS_Node *qnode){
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
        mcs_lock->acquire(qnode);
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

int LockBox::release(MCS_Node *qnode){
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
        mcs_lock->release(qnode);
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
