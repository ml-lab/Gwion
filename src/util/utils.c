#include <stdio.h>
#include <string.h>
#ifndef __linux__
#include <sys/select.h>
#endif
#include <netdb.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdlib.h>
#define _XOPEN_SOURCE 700
#include <math.h>
#include <string.h>
#include "err_msg.h"
#include "absyn.h"
#include "type.h"
#include "compile.h"

m_uint num_digit(m_uint i) {
  return i ? (m_uint)floor(log10(i) + 1) : 1;
}

m_bool verify_array(Array_Sub array) {
  if(array->err_num) {
    if(array->err_num == 1)
      CHECK_BB(err_msg(UTIL_, array->pos,
            "invalid format for array init [...][...]..."))
    else if(array->err_num == 2)
      CHECK_BB(err_msg(UTIL_, array->pos,
            "partially empty array init [...][]..."))
  }
  return 1;
}

static Type find_typeof(Env env, ID_List path) {
  path = path->ref;
  Value v = nspc_lookup_value2(env->curr, path->xid);
  Type t = (isa(v->m_type, &t_class) > 0) ? v->m_type->d.actual_type : v->m_type;
  path = path->next;
  while(path) {
    CHECK_OO((v = find_value(t, path->xid)))
    t = v->m_type;
    path = path->next;
  }
  return v->m_type;
}

Type find_type(Env env, ID_List path) {
  Nspc nspc;
  Type type;

  if(path->ref)
    return find_typeof(env, path);
  CHECK_OO((type = nspc_lookup_type1(env->curr, path->xid)))
  nspc = type->info;
  path = path->next;

  while(path) {
    S_Symbol xid = path->xid;
    Type t = nspc_lookup_type1(nspc, xid);
    while(!t && type && type->parent) {
      t = nspc_lookup_type2(type->parent->info, xid);
      type = type->parent;
    }
    if(!t)
      CHECK_BO(err_msg(UTIL_, path->pos,
            "...(cannot find class '%s' in nspc '%s')", s_name(xid), nspc->name))
    type = t;
    nspc = type->info;
    path = path->next;
  }
  return type;
 }

Value find_value(Type type, S_Symbol xid) {
  Value value;
  if(!type || !type->info)
    return NULL;
  if((value = nspc_lookup_value2(type->info, xid)))
    return value;
  if(type->parent)
    return find_value(type->parent, xid);
  return NULL;
}

Func find_func(Type type, S_Symbol xid) {
  Func func;
  if((func = nspc_lookup_func2(type->info, xid)))
    return func;
  if(type->parent)
    return find_func(type->parent, xid);
  return NULL;
}

m_uint id_list_len(ID_List list) {
  m_uint len = 0;
  while(list) {
    len += strlen(s_name(list->xid));
    if(list->next)
      len++;
    list = list->next;
  }
  return len + 1;
}

void type_path(char* str, ID_List list) {
  ID_List path = list;

  str[0] = '\0';
  while(path) {
    strcat(str, s_name(path->xid));
    if(path->next)
      strcat(str, ".");
    path = path->next;
  }
}

Type new_array_type(Env env, m_uint depth, Type base_type, Nspc owner_nspc) {
  Type t = new_type(te_array, base_type->name, &t_array);
  t->size = SZ_INT;
  t->array_depth = depth;
  t->d.array_type = base_type;
  t->info = t_array.info;
  ADD_REF(t->info);
  t->owner = owner_nspc;
  return t;
}

static const char escape1[] = "0'abfnrtv";
static const char escape2[] = "\0\'\a\b\f\n\r\t\v";

m_int get_escape(const char c, int linepos) {
  m_uint i = 0;
  while(escape1[i] != '\0') {
    if(c == escape1[i])
      return escape2[i];
    i++;
  }
  CHECK_BB(err_msg(UTIL_, linepos, "unrecognized escape sequence '\\%c'", c))
  return -1;
}

m_int str2char(const m_str c, m_int linepos) {
  if(c[0] == '\\')
    return get_escape(c[1], linepos);
  else
    return c[0];
}
