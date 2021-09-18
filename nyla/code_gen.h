#ifndef NYLA_CODE_GEN_H
#define NYLA_CODE_GEN_H

#include "types_ext.h"
#include <llvm/IR/Module.h>
#include <llvm/Target/TargetMachine.h>

namespace nyla {
	
	void init_llvm_native_target();

	llvm::TargetMachine* create_llvm_target_machine();

	bool write_obj_file(c_string fname, llvm::Module* llvm_module, llvm::TargetMachine* target_machine);

}

#endif