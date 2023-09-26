#ifndef JUDT_ARRAYS_H
#define JUDT_ARRAYS_H

#ifndef uint
typedef unsigned int uint;
#endif
#ifndef uchar
typedef unsigned char uchar;
#endif

#include <inttypes.h>

#ifndef judyslot
//win32 UINT64?
    typedef uint64_t judyslot;
#endif

#ifdef __cplusplus
extern "C"{
#endif

typedef void* Judy;

Judy judy_open (uint max);
void judy_close (Judy judy);

/// alloc a memory from judy, and return the ptr.
/// often used to alloc value for target key.
void *judy_data (Judy judy, uint size);

///insert key，return value ptr。 if not exist also return a ptr.
/// not exist example:
/// judyslot slot = judy_cell(judy, key1, strlen(key1));
/// if(*slot){
///     Value* v = (Value*)(*slot); //do someting
/// }else{
///      Value * v = judy_data(judy, sizeof(Value));
///      *slot = (judyslot)v;
/// }
judyslot *judy_cell (Judy judy, uchar *buff, uint max);

//get value by key, of not exist , return null.
judyslot *judy_slot (Judy judy, uchar *buff, uint max);
judyslot *judy_end (Judy judy);

//used to iterate the judy array
///
/// char key[MAX_LEN];
/// cell = judy_start(...)
/// while(cell){
///     len = judy_key(judy, key, sizeof(key))
///     Value* v = (Value*)(*cell);
///     ...visit v.
///     //judy_del(judy)
///     cell = judy_next(...);
/// }
///
judyslot *judy_start (Judy judy, uchar *buff, uint max);
uint judy_key (Judy judy, uchar *buff, uint max);
judyslot *judy_next (Judy judy);
judyslot* judy_prv (Judy judy);
//can call after judy_cell. or iterate. like 'judy_start/judy_next'
judyslot *judy_del (Judy judy);

#ifdef __cplusplus
}
#endif

#endif // JUDT_ARRAYS_H
