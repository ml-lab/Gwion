#include "type.h"
#include "instr.h"
#include "import.h"
#include "compile.h"

struct Type_ t_machine   = { "Machine",      0, NULL, te_machine};

static m_str randstring(VM* vm, int length) {
  char *string = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";
  size_t stringLen = 26 * 2 + 10 + 2;
  char *randomString;

  randomString = malloc(sizeof(char) * (length + 1));

  if(!randomString) {
    return (char*)0;
  }

  for(int n = 0; n < length; n++) {
    unsigned int key = sp_rand(vm->sp) % stringLen;
    randomString[n] = string[key];
  }

  randomString[length] = '\0';

  return randomString;
}

static SFUN(machine_add) {
  M_Object obj = *(M_Object*)MEM(SZ_INT);
  if(!obj)
    return;
  m_str str = STRING(obj);
  release(obj, shred);
  if(!str)
    return;
  *(m_uint*)RETURN = compile(shred->vm_ref, str);
}

static SFUN(machine_check) {
  Ast ast;
  FILE* file;
  m_str prefix, filename, s;
  M_Object prefix_obj = *(M_Object*)MEM(SZ_INT);
  M_Object code_obj = *(M_Object*)MEM(SZ_INT * 2);

  *(m_uint*)RETURN = 0;
  if(!prefix_obj)
    prefix = ".";
  else {
    prefix = STRING(prefix_obj);
    release(prefix_obj, shred);
  }
  if(!code_obj)
    return;
  char c[strlen(prefix) + 17];
  filename = randstring(shred->vm_ref, 12);
  sprintf(c, "%s/%s", prefix, filename);
  if(!(file = fopen(c, "w"))) {
    free(filename);
    return;
  }
  fprintf(file, "%s\n", STRING(code_obj));
  release(code_obj, shred);
  fclose(file);
  free(filename);
  if(!(ast = parse(c)))
    return;
  s = strdup(c);
  if(type_engine_check_prog(shred->vm_ref->emit->env, ast, s) < 0)
    return;
  free(s);
  free_ast(ast);
  *(m_uint*)RETURN = 1;
}

static SFUN(machine_compile) {

  m_str prefix, filename;
  FILE* file;
  M_Object prefix_obj = *(M_Object*)MEM(SZ_INT);
  M_Object code_obj = *(M_Object*)MEM(SZ_INT * 2);

  *(m_uint*)RETURN = 0;
  if(!prefix_obj)
    prefix = ".";
  else {
    prefix = STRING(prefix_obj);
    release(prefix_obj, shred);
  }
  if(!code_obj) return;
  char c[strlen(prefix) + 17];
  filename = randstring(shred->vm_ref, 12);
  sprintf(c, "%s/%s.gw", prefix, filename);
  if(!(file = fopen(c, "w"))) {
    free(filename);
    return;
  }
  fprintf(file, "%s\n", STRING(code_obj));
  release(code_obj, shred);
  fclose(file);
  free(filename);
  compile(shred->vm_ref, c);
}

static SFUN(machine_shreds) {
  int i;
  VM* vm = shred->vm_ref;
  VM_Shred sh;
  M_Object obj = new_M_Array(SZ_INT, vector_size(&vm->shred), 1);
  for(i = 0; i < vector_size(&vm->shred); i++) {
    sh = (VM_Shred)vector_at(&vm->shred, i);
    i_vector_set(ARRAY(obj), i, sh->xid);
  }
  vector_add(&shred->gc, (vtype)obj);
  *(M_Object*)RETURN = obj;
}

m_bool import_machine(Env env) {
  DL_Func fun;

  CHECK_BB(import_class_begin(env, &t_machine, env->global_nspc, NULL, NULL))
  dl_func_init(&fun, "void",  "add", (m_uint)machine_add);
  dl_func_add_arg(&fun,       "string",  "filename");
  CHECK_BB(import_fun(env, &fun, ae_flag_static))

  dl_func_init(&fun, "int[]", "shreds", (m_uint)machine_shreds);
  CHECK_BB(import_fun(env, &fun, ae_flag_static))

  dl_func_init(&fun, "int",  "check", (m_uint)machine_check);
  dl_func_add_arg(&fun,      "string",  "prefix");
  dl_func_add_arg(&fun,      "string",  "code");
  CHECK_BB(import_fun(env, &fun, ae_flag_static))

  dl_func_init(&fun, "void", "compile", (m_uint)machine_compile);
  dl_func_add_arg(&fun,      "string",  "prefix");
  dl_func_add_arg(&fun,      "string",  "filename");
  CHECK_BB(import_fun(env, &fun, ae_flag_static))

  CHECK_BB(import_class_end(env))
  return 1;
}