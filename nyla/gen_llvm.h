#pragma once

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
// Needed for writing .o files
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>

#include "sym_table.h"
#include "gen_llvm.h"
#include "ast.h"

namespace nyla {
	
	extern llvm::LLVMContext* llvm_context;
	extern llvm::IRBuilder<>* llvm_builder;
	extern llvm::Module*      working_llvm_module;

	void init_llvm();

	void init_native_target();

	void clean_llvm();

	bool write_obj_file(c_string fname);

	class llvm_generator {
	public:

		llvm::Function* gen_function(nyla::afunction* function);

		llvm::Type* gen_type(nyla::type* type);

		llvm::Value* gen_expression(nyla::aexpr* expr);

		llvm::Value* gen_return(nyla::areturn* ret);

		llvm::Value* gen_number(nyla::anumber* number);

		llvm::Value* gen_binary_op(nyla::abinary_op* binary_op);

		llvm::Value* gen_variable_decl(nyla::avariable_decl* variable_decl);

		llvm::Value* gen_for_loop(nyla::afor_loop* for_loop);

		llvm::Value* gen_variable(nyla::avariable* variable);

	private:
		nyla::sym_table   m_sym_table;
		llvm::BasicBlock* m_bb;
		llvm::Function*   m_ll_function;
	};
}