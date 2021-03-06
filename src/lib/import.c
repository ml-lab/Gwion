#include <string.h>
#include <ctype.h>
#include "err_msg.h"
#include "type.h"
#include "traverse.h"
#include "import.h"
#include "importer.h"


#define CHECK_EB(a) if(!a) { CHECK_BB(err_msg(TYPE_, 0, "import error: import_xxx invoked between ini/end")) }
#define CHECK_EO(a) if(!a) { CHECK_BO(err_msg(TYPE_, 0, "import error: import_xxx invoked between ini/end")) }

struct Path {
  m_str path, curr;
  m_uint len;
};

static ID_List templater_def(Templater* templater) {
  m_uint i;
  ID_List list[templater->n];
  list[0] = new_id_list(templater->list[0], 0);
  for(i = 1; i < templater->n; i++) {
    list[i] = new_id_list(templater->list[i], 0);
    list[i - 1]->next = list[i];
  }
  return list[0];
}

static Type_List templater_call(Templater* templater) {
  m_uint i;
  Type_List list[templater->n];
  for(i = templater->n; --i;) {
    ID_List id = new_id_list(templater->list[i - 1], 0);
    list[i - 1] = new_type_list(id, i ? list[i - 1] : NULL, 0);
  }
  return list[0];
}

void importer_template_stop(Importer importer) {
  importer->templater.n = 0;
  importer->templater.list = NULL;
}

void importer_template_init(Importer importer, m_uint n, const m_str* list) {
  importer->templater.n = n;
  importer->templater.list = (m_str*)list;
}

static void dl_func_init(DL_Func* a, const m_str t, const m_str n, m_uint addr) {
  a->name = n;
  a->type = t;
  a->addr = addr;
  a->narg = 0;
}

m_int importer_func_ini(Importer importer, const m_str t, const m_str n, m_uint addr) {
  dl_func_init(&importer->func, t, n, addr);
  return 1;
}

static void dl_func_func_arg(DL_Func* a, const m_str t, const m_str n) {
  a->args[a->narg].type = t;
  a->args[a->narg++].name = n;
}

m_int importer_func_arg(Importer importer, const m_str t, const m_str n) {
  if(importer->func.narg == DLARG_MAX - 1)
    CHECK_BB(err_msg(UTIL_, 0,
          "to many arguments for function '%s'.", importer->func.name))
  dl_func_func_arg(&importer->func, t, n);
  return 1;
}

static m_bool check_illegal(char* curr, char c, m_uint i) {
  if(isalnum(c) || c == '_' || (i == 1 && c== '@'))
    curr[i - 1] = c;
  else
    return -1;
  return 1;
}

static m_bool name_valid(m_str a) {
  m_uint i, len = strlen(a);
  for(i = 0; i < len; i++) {
    char c = a[i];
    if((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
        || (c == '_') || (c >= '0' && c <= '9'))
      continue;
    CHECK_BB(err_msg(UTIL_,  0, "illegal character '%c' in name '%s'...", c, a))
  }
  return 1;
}

static void path_valid_inner(m_str curr) {
  m_int j, size = strlen(curr);
  for(j = (size / 2) + 1; --j;) {
    char s = curr[j];
    curr[j] = curr[size - j - 1];
    curr[size - j - 1] = s;
  }
}

static m_bool path_valid(ID_List* list, struct Path* p) {
  char last = '\0';
  m_uint i;
  for(i = p->len; --i;) {
    char c = p->path[i - 1];
    if(c != '.' && check_illegal(p->curr, c, i) < 0)
      CHECK_BB(err_msg(UTIL_,  0,
            "illegal character '%c' in path '%s'...", c, p->path))
    if(c == '.' || i == 1) {

      if((i != 1 && last != '.' && last != '\0') ||
          (i ==  1 && c != '.')) {
        path_valid_inner(p->curr);
        *list = prepend_id_list(p->curr, *list, 0);
        memset(p->curr, 0, p->len + 1);
      } else
        CHECK_BB(err_msg(UTIL_,  0,
              "path '%s' must not ini or end with '.'", p->path))
    }
    last = c;
  }
  return 1;
}

static ID_List str2list(m_str path, m_uint* array_depth) {
  m_uint len = strlen(path);
  ID_List list = NULL;
  m_uint depth = 0;
  char curr[len + 1];
  struct Path p = { path, curr, len };
  memset(curr, 0, len + 1);


  while(p.len > 2 && path[p.len - 1] == ']' && path[p.len - 2] == '[') {
    depth++;
    p.len -= 2;
  }
  if(path_valid(&list, &p) < 0) {
    if(list)
      free_id_list(list);
    return NULL;
  }
  CHECK_OO(list)
  strncpy(curr, path, p.len);
  list->xid = insert_symbol(curr);
  *array_depth = depth;
  return list;
}

static m_bool mk_xtor(Type type, m_uint d, e_native_func e) {
  VM_Code* code = e == NATIVE_CTOR ? &type->info->pre_ctor : &type->info->dtor;
  m_str name = type->name;
  m_str filename = e == NATIVE_CTOR ? "[ctor]" : "[dtor]";

  SET_FLAG(type, e == NATIVE_CTOR ? ae_flag_ctor : ae_flag_dtor);
  *code = new_vm_code(NULL, SZ_INT, 1, name, filename);
  (*code)->native_func = (m_uint)d;
  (*code)->native_func_type = e;
  return 1;
}

m_int importer_add_type(Importer importer, Type type) {
  if(type->name[0] != '@')
    CHECK_BB(name_valid(type->name));
  CHECK_BB(env_add_type(importer->env, type))
  return type->xid;
}

static m_bool import_class_ini(Env env, Type type, f_xtor pre_ctor, f_xtor dtor) {
  type->info = new_nspc(type->name, "global_nspc");
  type->info->parent = env->curr;
  if(pre_ctor)
    mk_xtor(type, (m_uint)pre_ctor, NATIVE_CTOR);
  if(dtor)
    mk_xtor(type, (m_uint)dtor,     NATIVE_DTOR);
  if(type->parent) {
    type->info->offset = type->parent->obj_size;
    vector_copy2(&type->info->obj_v_table, &type->parent->info->obj_v_table);
  }
  type->owner = env->curr;
  SET_FLAG(type, ae_flag_checked);
  CHECK_BB(env_push_class(env, type))
  return 1;
}

m_int importer_class_ini(Importer importer, Type type, f_xtor pre_ctor, f_xtor dtor) {
  if(type->info)
    CHECK_BB(err_msg(TYPE_, 0, "during import: class '%s' already imported...", type->name))
  if(importer->templater.n) {
    type->def = calloc(1, sizeof(struct Class_Def_));
    type->def->types = templater_def(&importer->templater);
    SET_FLAG(type, ae_flag_template);
  }
  CHECK_BB(importer_add_type(importer, type))
  CHECK_BB(import_class_ini(importer->env, type, pre_ctor, dtor))
  return type->xid;
}

static m_int import_class_end(Env env) {
  if(!env->class_def)
    CHECK_BB(err_msg(TYPE_, 0, "import: too many class_end called..."))
  env->class_def->obj_size = env->class_def->info->offset;
  CHECK_BB(env_pop_class(env))
  return 1;
}

m_int importer_class_end(Importer importer) {
  return import_class_end(importer->env);
}

static void dl_var_new_array(DL_Var* v) {
  v->t.array = new_array_sub(NULL, 0);
  v->t.array->depth = v->array_depth;
  v->var.array = new_array_sub(NULL, 0);
  v->var.array->depth = v->array_depth;
}

static void dl_var_set(DL_Var* v, ae_flag flag) {
  v->list.self = &v->var;
  v->t.flag = flag;
  v->exp.exp_type = ae_exp_decl;
  v->exp.d.exp_decl.type = &v->t;
  v->exp.d.exp_decl.list = &v->list;
  v->exp.d.exp_decl.is_static = ((flag & ae_flag_static) == ae_flag_static);
  v->exp.d.exp_decl.self = &v->exp;
  if(v->array_depth)
    dl_var_new_array(v);
}

static void dl_var_release(DL_Var* v) {
  if(v->array_depth) {
    free_array_sub(v->t.array);
    free_array_sub(v->var.array);
  }
  free(v->t.xid);
}

m_int importer_item_ini(Importer importer, const m_str type, const m_str name) {
  DL_Var* v = &importer->var;
  CHECK_EB(importer->env->class_def)
  memset(v, 0, sizeof(DL_Var));
  if(!(v->t.xid = str2list(type, &v->array_depth)))
    CHECK_BB(err_msg(TYPE_, 0, "... during var import '%s.%s'...",
          importer->env->class_def->name, name))
  v->var.xid = insert_symbol(name);
  return 1;
}

m_int importer_item_end(Importer importer, const ae_flag flag, const m_uint* addr) {
  DL_Var* v = &importer->var;
  CHECK_EB(importer->env->class_def)
  dl_var_set(v, flag);
  v->var.addr = (void*)addr;
  if(importer->templater.n)
    v->exp.d.exp_decl.types = templater_call(&importer->templater);
  if(traverse_decl(importer->env, &v->exp.d.exp_decl) < 0)
    v->var.value->offset = -1;
  dl_var_release(v);
  return v->var.value->offset;
}

static Array_Sub make_dll_arg_list_array(Array_Sub array_sub,
  m_uint* array_depth, m_uint array_depth2) {
  m_uint i;
  if(array_depth2)
    *array_depth = array_depth2;
  if(*array_depth) {
    array_sub = new_array_sub(NULL, 0);
    for(i = 1; i < *array_depth; i++)
      array_sub = prepend_array_sub(array_sub, NULL);
  }
  return array_sub;
}

static Arg_List make_dll_arg_list(DL_Func * dl_fun) {
  Arg_List arg_list    = NULL;
  m_int i = 0;

  for(i = dl_fun->narg + 1; --i; ) {
    m_uint array_depth = 0, array_depth2 = 0;
    Array_Sub array_sub = NULL;
    Type_Decl* type_decl = NULL;
    Var_Decl var_decl    = NULL;
    DL_Value* arg = &dl_fun->args[i-1];
    ID_List type_path2, type_path = str2list(arg->type, &array_depth);
    if(!type_path) {
      if(arg_list)
        free_arg_list(arg_list);
      CHECK_BO(err_msg(TYPE_,  0, "...at argument '%i'...", i + 1))
    }
    type_decl = new_type_decl(type_path, 0, 0);
    if((type_path2 = str2list(arg->name, &array_depth2)))
      free_id_list(type_path2);
    if(array_depth && array_depth2) {
      free_type_decl(type_decl);
      if(arg_list)
        free_arg_list(arg_list);
      CHECK_BO(err_msg(TYPE_, 0, "array subscript specified incorrectly for built-in module"))
    }
    array_sub = make_dll_arg_list_array(array_sub, &array_depth, array_depth2);
    var_decl = new_var_decl(arg->name, array_sub, 0);
    arg_list = new_arg_list(type_decl, var_decl, arg_list, 0);
  }
  return arg_list;
}

static Func_Def make_dll_as_fun(DL_Func * dl_fun, ae_flag flag) {
  Func_Def func_def = NULL;
  ID_List type_path = NULL;
  Type_Decl* type_decl = NULL;
  m_str name = NULL;
  Arg_List arg_list = NULL;
  m_uint i, array_depth = 0;

  flag |= ae_flag_func | ae_flag_builtin;
  if(!(type_path = str2list(dl_fun->type, &array_depth)) ||
      !(type_decl = new_type_decl(type_path, 0, 0)))
    CHECK_BO(err_msg(TYPE_, 0, "...during @ function import '%s' (type)...", dl_fun->name))
  if(array_depth) {
    Array_Sub array_sub = new_array_sub(NULL, 0);
    for(i = 1; i < array_depth; i++)
      array_sub = prepend_array_sub(array_sub, NULL);
    type_decl = add_type_decl_array(type_decl, array_sub, 0);
  }
  name = dl_fun->name;
  arg_list = make_dll_arg_list(dl_fun);
  func_def = new_func_def(flag, type_decl, name, arg_list, NULL, 0);
  func_def->d.dl_func_ptr = (void*)(m_uint)dl_fun->addr;
  return func_def;
}

static Func_Def import_fun(Env env, DL_Func * mfun, ae_flag flag) {
  CHECK_OO(mfun) // probably deserve an err msg
  CHECK_BO(name_valid(mfun->name));
  CHECK_EO(env->class_def)
  return make_dll_as_fun(mfun, flag);
}

m_int importer_func_end(Importer importer, ae_flag flag) {
  Func_Def def = import_fun(importer->env, &importer->func, flag);

  CHECK_OB(def)
  if(importer->templater.n) {
    def = calloc(1, sizeof(struct Class_Def_));
    def->types = templater_def(&importer->templater);
    SET_FLAG(def, ae_flag_template);
  }
  if(traverse_func_def(importer->env, def) < 0) {
    free_func_def(def);
    return -1;
  }
  return 1;
}

static Type get_type(Env env, const m_str str) {
  m_uint depth = 0;
  ID_List list = str2list(str, &depth);
  Type  t = list ? find_type(env, list) : NULL;
  if(list)
    free_id_list(list);
  return t ? (depth ? new_array_type(env, depth, t, env->curr) : t) : NULL;
}

static m_int import_op(Env env, DL_Oper* op,
                const f_instr f, const m_bool global) {
  Type lhs = op->lhs ? get_type(env, op->lhs) : NULL;
  Type rhs = op->rhs ? get_type(env, op->rhs) : NULL;
  Type ret = get_type(env, op->ret);
  struct Op_Import opi = { op->op, lhs, rhs, ret, f, NULL, global};
  return env_add_op(env, &opi);
}

m_int importer_oper_ini(Importer importer, const m_str l, const m_str r, const m_str t) {
  importer->oper.ret = t;
  importer->oper.rhs = r;
  importer->oper.lhs = l;
  return 1;
}

m_int importer_oper_end(Importer importer, Operator op, const f_instr f, const m_bool global) {
  importer->oper.op = op;
  return import_op(importer->env, &importer->oper, f, global);
}

m_int importer_add_value(Importer importer, const m_str name, Type type, const m_bool is_const, void* value) {
  return env_add_value(importer->env, name, type, is_const, value);
}
