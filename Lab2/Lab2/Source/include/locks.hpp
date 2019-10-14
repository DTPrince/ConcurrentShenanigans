#ifndef LOCKS_HPP
#define LOCKS_HPP

#define LOCK_TYPE_TAS       0
#define LOCK_TYPE_TTAS      1
#define LOCK_TYPE_TICKET    2
#define LOCK_TYPE_MCS       3

typedef struct {

} tas_lock;

typedef struct {

} ttas_lock;

typedef struct {

} ticket_lock;

typedef struct {

} mcs_lock;

#endif // LOCKS_HPP
