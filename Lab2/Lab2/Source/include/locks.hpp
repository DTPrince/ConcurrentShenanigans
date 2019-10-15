#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <atomic>

#define SEQ_CST             memory_order_seq_cst
#define ACQ                 memory_order_acquire
#define REL                 memory_order_release
#define RELAXED             memory_order_relaxed

#define LOCK_TYPE_TAS       0
#define LOCK_TYPE_TTAS      1
#define LOCK_TYPE_TICKET    2
#define LOCK_TYPE_MCS       3

// Attempt to force all locks to share the same traits
// so I can create my lock the same way for all types.
// might be a pipe dream
class LockInterface {
public:
    // I wanted to use atomic_flag but it does not support load operations
    // I could just overload TTAS_Lock to use atoimic<bool> but it would provide
    // bad perf comparisons
//    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    std::atomic<bool> flag;
    int counter;

    // nomenclature acquire and release are used in favor of
    // lock and unlock for the more sophisticated ticket and MCS locks
    virtual void acquire() = 0;
    virtual void release() = 0;

    virtual ~LockInterface();
};

// All public because these are glorified structs and private serves no benefit at the moment
class TAS_Lock : public LockInterface {
public:
    void acquire(){
        bool expect_false = false;
        // using CAS instead of TAS since one is a real member function and one is not
        // Expect false so that it will only swap with `true` (and thus acquiring lock)
        // once the lock is free.
        // Invert becasue flag.CAS(...) will return false here when flag is locked (value of true)
        while(!flag.compare_exchange_weak(expect_false, true));    // nada
    }
    void release(){
        flag.store(false);
    }
};

class TTAS_Lock : public LockInterface {
public:
    void acquire(){
        bool expect_false = false;
        // same as above but with load check.
        while(flag.load() == true || !flag.compare_exchange_weak(expect_false, true));
    }
    void release(){
        flag.store(false);
    }
};

// TODO: reminder that I left memory orders as implicit
// Which I believe defaults to sequential but could
// cause strange problems later in testing. So come bek
// TODO: check the warning on no out-of-line virt method
//       definitions; vtable emitt etc etc etc
class Ticket_Lock : public LockInterface {
public:
    std::atomic<int> serving;
    std::atomic<int> next;

    Ticket_Lock(){
        serving.store(0);
        next.store(0);
    }

    void acquire(){
        thread_local int my_num = next.fetch_add(1);
        while (serving.load() != my_num);   // wait
    }
    void release(){
        // overloaded in std::atomic
        serving++;
    }
};

class MCS_Lock : public LockInterface {
public:
    typedef struct MCS_Node{
        MCS_Node *next;
        bool locked;    // this probably has to become atomic.
    } MCS_Node;

    std::atomic<MCS_Node *> tail;

    thread_local static MCS_Node qnode; // queueueueueue node


    inline void acquire(){
        // place new node at end of queue and store address of previous last node
        // for checking and also to update next-> ptr
        MCS_Node *prev = tail.exchange(&this->qnode);   // `this` required to reference correct thread_local

        // recall that prev took the address of the last node.
        // meaning if it is null then there is no last node and the queue is empty.
        if (prev != nullptr){
            qnode.locked = true;

            prev->next = &this->qnode;

            while (qnode.locked);   // wait for the lock to free, This is done from
                                    // `prev->next.locked = true;` in MCS_Lock::release();

        }
    }

    inline void release(){
        // grab successor node to set up exit and wake thread.
        MCS_Node *succ = qnode.next;

        // check if there is a waiting thread
        if (succ == nullptr){
            // TODO: discover why tf this wont `compare_exchange_strong`
            if (tail.load() == &this->qnode){
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
};

#endif // LOCKS_HPP
