#include "defs.h"
#include "type.h"
#include "err_msg.h"
#include "lang.h"
#include "import.h"

static struct Type_ t_invalid_var_name = { "invalid_var_name", SZ_INT, &t_object };

MFUN(test_mfun){}
IMPORT
{
  CHECK_BB(importer_class_ini(importer, &t_invalid_var_name, NULL, NULL))

	importer_item_ini(importer,"int[]", "int_array");
  importer_item_end(importer, 0, NULL); // import array var
  importer_func_ini(importer, "float[][]", "f", (m_uint)test_mfun);
  CHECK_BB(importer_func_end(importer, 0))
  importer_func_ini(importer, "float[][]", "g", (m_uint)test_mfun);
  CHECK_BB(importer_func_end(importer, 0))

  CHECK_BB(importer_class_end(importer))
  return 1;
}
