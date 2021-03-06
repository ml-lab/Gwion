#include "stdlib.h"
#include "string.h"
#include "defs.h"
#include "symbol.h"
#define SIZE 65347  /* should be prime */

struct S_Symbol_ {
  m_str name;
  S_Symbol next;
};

static S_Symbol hashtable[SIZE];

static S_Symbol mksymbol(const m_str name, S_Symbol next) {
  S_Symbol s = malloc(sizeof(*s));
  s->name = strdup(name);
  s->next = next;
  return s;
}

static void free_symbol(S_Symbol s) {
  if(s->next)
    free_symbol(s->next);
  free(s->name);
  free(s);
}

void free_symbols() {
  int i;
  for(i = SIZE + 1; --i;) {
    S_Symbol s = hashtable[i - 1];
    if(s)
      free_symbol(s);
  }
}

static unsigned int hash(const char *s0) {
  unsigned int h = 0;
  const char *s;
  for(s = s0; *s; s++)
    h = h * 65599 + *s;
  return h;
}

S_Symbol insert_symbol(const m_str name) {
  int index = hash(name) % SIZE;
  S_Symbol sym, syms = hashtable[index];

  for(sym = syms; sym; sym = sym->next)
    if(!strcmp(sym->name, (m_str)name))
      return sym;
  return hashtable[index] = mksymbol(name, syms);
}

m_str s_name(S_Symbol sym) {
  return sym->name;
}

