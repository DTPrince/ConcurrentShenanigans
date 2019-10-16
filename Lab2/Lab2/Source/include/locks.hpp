#ifndef LOCKS_HPP
#define LOCKS_HPP

#include <atomic>
#include <pthread.h>

#define SEQ_CST             std::memory_order_seq_cst
#define ACQ                 std::memory_order_acquire
#define REL                 std::memory_order_release
#define RELAXED             std::memory_order_relaxed

// Tracks lock types
typedef enum LockType{
    LOCK_TYPE_INVALID,
    LOCK_TYPE_TAS,
    LOCK_TYPE_TTAS,
    LOCK_TYPE_TICKET,
    LOCK_TYPE_MCS,
    LOCK_TYPE_PTHREAD,
    LOCK_TYPE_AFLAG
} LockType;

// Required as a compliment to MCS_Lock.acquire();
// As I'm stubborn, it will now be grandfathered
// into the other acquire() fields. Will be overloaded.
typedef struct MCS_Node{
    MCS_Node *next = nullptr;
    bool locked;    // TODO: this probably has to become atomic.
} MCS_Node;

// --- LockInterface ---
// Attempt to force all locks to share the same traits
// so I can create my lock the same way for all types.
// might be a pipe dream
class LockInterface {
public:
    std::atomic<bool> flag;

    // nomenclature acquire and release are used in favor of
    // lock and unlock for the more sophisticated ticket and MCS locks
    virtual void acquire(MCS_Node *qnode = nullptr) = 0;
    virtual void release(MCS_Node *qnode = nullptr) = 0;

    virtual ~LockInterface();
};

// --- TAS_Lock ---
// All public because these are glorified structs and private serves no benefit at the moment
class TAS_Lock : public LockInterface {
public:
    ~TAS_Lock();

    void acquire(MCS_Node *qnode = nullptr);
    void release(MCS_Node *qnode = nullptr);
};

// --- TTAS_Lock ---
class TTAS_Lock : public LockInterface {
public:
    void acquire(MCS_Node *qnode = nullptr);
    void release(MCS_Node *qnode = nullptr);
};

// --- Ticket_Lock ---
// TODO: reminder that I left memory orders as implicit
// Which I believe defaults to sequential but could
// cause strange problems later in testing. So come bek
class Ticket_Lock : public LockInterface {
public:
    std::atomic<int> serving;
    std::atomic<int> next;

    Ticket_Lock();

    void acquire(MCS_Node *qnode = nullptr);
    void release(MCS_Node *qnode = nullptr);
};

// --- MCS_Lock ---
// This thing slightly ruins the interface because thread_local is
// needed. Which must be declared in the thread.
class MCS_Lock : public LockInterface {
public:

    std::atomic<MCS_Node *> tail;

    thread_local static MCS_Node qnode; // queueueueueue node

    void acquire(MCS_Node *qnode);
    void release(MCS_Node *qnode);
};

// --- Mutex/Pthread Lock ---
// Inheriting interface for consistency (in the future..?)
// Cost is dragging around an atomic bool but whatever
class Mutex_Lock : LockInterface{
public:
    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

    void acquire(MCS_Node *qnode = nullptr);
    void release(MCS_Node *qnode = nullptr);
};


// --- Just a test lock ---
// Testing the Atomic_flag object as a TAS lock.
// It is "non-locking" and does not support loads
// thus cannot make TTAS lock
class AFlag_Lock {
public:
    std::atomic_flag flag = ATOMIC_FLAG_INIT;
    void acquire(MCS_Node *qnode = nullptr);
    void release(MCS_Node *qnode = nullptr);
};


// --- LockBox ---
// This class is made to handle cleanly managing
// lock calls in the program it runs in.
// The result of this is an ugly class that allows
// the syntax `lock.acquire()` called in a thread
// with no extra steps required to determine which
// lock the user (cmd line) intended to call.
// **So long as the Lockbox is initialized correctly**
class LockBox{
public:
    LockType ltype = LOCK_TYPE_INVALID;

    TAS_Lock *tas_lock = nullptr;
    TTAS_Lock *ttas_lock = nullptr;
    Ticket_Lock *ticket_lock = nullptr;
    MCS_Lock *mcs_lock = nullptr;
    Mutex_Lock *mutex_lock = nullptr;
    AFlag_Lock *aflag_lock = nullptr;

    LockBox(LockType lt);
    ~LockBox();

    int acquire(MCS_Node *qnode = nullptr);
    int release(MCS_Node *qnode = nullptr);
};

#endif // LOCKS_HPP
