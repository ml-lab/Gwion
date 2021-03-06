typedef struct {
  Type_Decl t;
  struct Var_Decl_List_ list;
  struct Var_Decl_ var;
  struct Exp_ exp;
  m_uint array_depth;
} DL_Var;

typedef struct {
  m_str name;
  m_str type;
} DL_Value;

typedef struct {
  m_str    name;
  m_str    type;
  m_uint   addr;
  m_uint   narg;
  DL_Value args[DLARG_MAX];
}  DL_Func;

typedef struct {
  Operator op;
  m_str ret, lhs, rhs;
 } DL_Oper;

typedef struct {
  m_uint n;
  m_str* list;
}Templater;

struct Importer_{
  Env env;
  DL_Var var;
  DL_Func func;
  DL_Oper oper;
  void* addr;
  Templater templater;
};
