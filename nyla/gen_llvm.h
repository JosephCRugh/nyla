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

		void gen_file_unit(nyla::afile_unit* file_unit, bool print_functions = false);

		llvm::Function* gen_function(nyla::afunction* function);

		llvm::Value* gen_function_call(nyla::afunction_call* function_call);

		llvm::Type* gen_type(nyla::type* type);

		llvm::Value* gen_expression(nyla::aexpr* expr);

		llvm::Value* gen_return(nyla::areturn* ret);

		llvm::Value* gen_number(nyla::anumber* number);

		llvm::Value* gen_binary_op(nyla::abinary_op* binary_op);

		llvm::Value* gen_unary_op(nyla::aunary_op* unary_op);

		llvm::Value* gen_variable_decl(nyla::avariable_decl* variable_decl);

		llvm::Value* gen_for_loop(nyla::afor_loop* for_loop);

		llvm::Value* gen_identifier(nyla::aidentifier* identifier);

		llvm::Value* gen_variable(nyla::avariable* variable);

		llvm::Value* gen_type_cast(nyla::atype_cast* type_cast);

		llvm::Value* gen_array(nyla::aarray* arr, llvm::Value* ptr_to_arr);

		llvm::Value* gen_array_access(nyla::aarray_access* array_access);

	private:

		llvm::AllocaInst* gen_alloca(nyla::avariable* variable);

		llvm::Value* calc_array_size(nyla::type* arr_type, nyla::avariable* variable);

		void calc_array_size(nyla::avariable* arr_alloc_reference, nyla::avariable* variable);

		nyla::sym_table   m_sym_table;
		llvm::BasicBlock* m_bb;
		llvm::Function*   m_ll_function;
	};
}