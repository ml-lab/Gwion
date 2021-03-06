#include <string.h>
#include "env.h"
#include "type.h"

static m_uint template_size(Env env, Class_Def c, Type_List call) {
  ID_List base = c->types;
  m_uint size = strlen(c->type->name) + 2;
  while(base) {
    Type t = find_type(env, call->list);
    size += strlen(t->name) + 1;
    call = call->next;
    base = base->next;
  }
  return size;
}

static m_bool template_name(Env env, Class_Def c, Type_List call, m_str str) {
  ID_List base = c->types;
  str[0] = '\0';
  strcat(str, c->type->name);
  while(base) {
    Type t = find_type(env, call->list);
    strcat(str, "@");
    strcat(str, t->name);
    call = call->next;
    base = base->next;
  }
  return 1;
}

static ID_List template_id(Env env, Class_Def c, Type_List call) {
  m_uint size = template_size(env, c, call);
  char name[size];
  ID_List list;

  CHECK_BO(template_name(env, c, call, name))
  list = new_id_list(name, call->pos);
  return list;
}

static m_bool template_match(ID_List base, Type_List call) {
  while(base) {
    if(!call)
      return -1;
    base = base->next;
    call = call->next;
  }
  if(call)
    return -1;
  return 1;
}

Class_Def template_class(Env env, Class_Def def, Type_List call) {
  ID_List name;

  CHECK_BO(template_match(def->types, call))
  name = template_id(env, def, call);
  return new_class_def(def->decl, name, def->ext, def->body, call->pos);
}

m_bool template_push_types(Env env, ID_List base, Type_List call) {
  nspc_push_type(env->curr);
  while(base) {
    Type t = find_type(env, call->list);
    nspc_add_type(env->curr, base->xid, t);
    call = call->next;
    base = base->next;
  }
  return 1;
}
