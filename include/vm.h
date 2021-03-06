#ifndef __VM
#define __VM

#ifdef USE_DOUBLE
#undef USE_DOUBLE
#endif
#ifndef SOUNDPIPE_H
#include <soundpipe.h>
#endif

#include "defs.h"
#include "oo.h"
#include "map.h"
#include "map_private.h"

typedef enum {
  NATIVE_UNKNOWN,
  NATIVE_CTOR,
  NATIVE_DTOR,NATIVE_MFUN,
  NATIVE_SFUN
} e_native_func;

typedef struct VM_Code_* VM_Code;
struct VM_Code_ {
  Vector instr;
  m_str name, filename;
  m_uint stack_depth;
  m_uint native_func;
  e_native_func native_func_type;
  m_bool need_this;
  struct VM_Object_ obj;
};

typedef struct Shreduler_* Shreduler;
typedef struct {
// BBQ
  unsigned int n_in;
  SPFLOAT* in;
  sp_data* sp; // holds sample rate and out
// !BBQ
  Shreduler shreduler;
  M_Object adc, dac, blackhole;
  Emitter emit;
  void (*wakeup)();
  struct Vector_ shred;
  struct Vector_ ugen;
  struct Vector_ plug;
  pthread_mutex_t mutex;
  m_bool is_running;
} VM;

extern VM* vm;

typedef struct VM_Shred_* VM_Shred;
struct VM_Shred_ {
  VM_Code code;
  VM_Shred parent;
  char* reg;
  char* mem;
  char* _reg;
  char* _mem;
  char* base;
  m_uint pc, next_pc, xid;
  m_str name;
  VM* vm_ref;
  VM_Shred prev, next;
  Vector args; // passed pointer from compile
  M_Object me;
  m_str filename;
  M_Object wait;
  struct Vector_ child;
  struct Vector_ gc, gc1;
  m_float wake_time;
};

struct  Shreduler_ {
  VM* vm;
  VM_Shred list;
  VM_Shred curr;
  m_uint n_shred;
  m_bool loop;
};

VM_Code new_vm_code(Vector instr, m_uint stack_depth, m_bool need_this, m_str name, m_str filename);
void free_vm_code(VM_Code a);

VM_Shred shreduler_get(Shreduler s);
void shreduler_remove(Shreduler s, VM_Shred out, m_bool erase);
VM_Shred shreduler_get(Shreduler s);
m_bool shredule(Shreduler s, VM_Shred shred, m_float wake_time);
void shreduler_set_loop(Shreduler s, m_bool loop);

VM_Shred new_vm_shred(VM_Code code);
void free_vm_shred(VM_Shred shred);

void vm_run(VM* vm);
VM* new_vm(m_bool loop);
void free_vm(VM* vm);
void vm_add_shred(VM* vm, VM_Shred shred);
#endif
