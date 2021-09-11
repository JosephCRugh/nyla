#ifndef NYLA_CODE_GEN_H
#define NYLA_CODE_GEN_H

#include "types_ext.h"
#include <llvm/IR/Module.h>

namespace nyla {
	
	void init_llvm_native_target();

	bool write_obj_file(c_string fname, llvm::Module* llvm_module);

}

#endif