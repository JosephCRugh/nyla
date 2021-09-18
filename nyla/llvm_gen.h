#ifndef NYLA_LLVM_H
#define NYLA_LLVM_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/IRBuilder.h>
#include <functional>

#include "compiler.h"
#include "ast.h"
#include "words.h"

namespace nyla {

	llvm::Constant* get_ll_int1(bool tof);
	llvm::Constant* get_ll_int8(s32 value);
	llvm::Constant* get_ll_uint8(u32 value);
	llvm::Constant* get_ll_int16(s32 value);
	llvm::Constant* get_ll_uint16(u32 value);
	llvm::Constant* get_ll_int32(s32 value);
	llvm::Constant* get_ll_uint32(u32 value);
	llvm::Constant* get_ll_int64(s64 value);
	llvm::Constant* get_ll_uint64(u64 value);

	class llvm_generator {
	public:

		~llvm_generator();

		llvm_generator(nyla::compiler& compiler, llvm::Module* llvm_module,
			           nyla::afile_unit* file_unit, bool print);

		void gen_file_unit();

		void gen_type_declarations();
		void gen_body_declarations();

		void gen_global_initializers(sym_function* sym_main_function,
			                         const std::vector<nyla::avariable_decl*>& initializer_expressions);
		void gen_startup_function_calls(sym_function* sym_main_function,
			                            const std::vector<llvm::Function*>& ll_startup_functions);

		void gen_module(nyla::amodule* nmodule);

		void gen_function_declaration(nyla::afunction* function);
		llvm::Value* gen_global_variable(nyla::avariable_decl* global);
		llvm::Constant* gen_global_module(nyla::avariable_decl* static_module);

		void gen_function_body(nyla::afunction* function);

		llvm::Value* gen_expression(nyla::aexpr* expr);
		// Since gen_expression returns lvalues for variables
		// array access, and dot operations, this function
		// will load those values
		llvm::Value* gen_expr_rvalue(nyla::aexpr* expr);

		llvm::Value* gen_variable_decl(nyla::avariable_decl* variable_decl);
		
		llvm::Value* gen_return(nyla::areturn* ret);

		llvm::Value* gen_number(nyla::anumber* number);

		llvm::Value* gen_null(nyla::aexpr* expr);

		llvm::Value* gen_binary_op(nyla::abinary_op* binary_op);

		llvm::Value* gen_unary_op(nyla::aunary_op* unary_op);

		llvm::Value* gen_ident(nyla::aident* ident);

		llvm::Value* gen_function_call(llvm::Value* ptr_to_struct, nyla::afunction_call* function_call);

		llvm::Value* gen_array(nyla::aarray* arr);
		llvm::Value* gen_string(nyla::astring* str);
		void gen_global_const_array(nyla::type* element_type,
			                        const std::vector<llvm::Constant*>& ll_element_values,
			                        llvm::Value* ll_arr_ptr);

		llvm::Value* gen_array_access(llvm::Value* ll_location, nyla::aarray_access* array_access);

		llvm::Value* gen_type_cast(nyla::atype_cast* type_cast);

		llvm::Value* gen_object(llvm::Value* ptr_to_struct, nyla::aobject* object);

		llvm::Value* gen_for_loop(nyla::afor_loop* for_loop);
		llvm::Value* gen_while_loop(nyla::awhile_loop* while_loop);
		llvm::Value* gen_loop(nyla::aloop_expr* loop_expr);

		llvm::Value* gen_dot_op(nyla::adot_op* dot_op);
		llvm::Value* gen_dot_op_on_field(sym_variable* sym_variable,
			                             llvm::Value* ll_location,
			                             bool is_last_index);

		llvm::Value* gen_if(nyla::aif* ifstmt);

		llvm::Value* gen_new_type(nyla::anew_type* new_type);

		llvm::Type* gen_type(nyla::type* type);

		llvm::Value* gen_allocation(sym_variable* sym_variable);
		llvm::Value* gen_precomputed_array_alloca(nyla::type* type, const std::vector<u32>& dim_sizes, u32 depth = 0);

		llvm::Value* gen_array_alloca(nyla::type* element_type, u32 num_elements);
		llvm::Value* get_arr_ptr(nyla::type* element_type, llvm::Value* arr_alloca);

	private:

		nyla::word get_word(u32 word_key);

		void gen_default_value(sym_variable* sym_variable, nyla::type* type, bool default_initialize);
		llvm::Constant* gen_default_value(nyla::type* type);
		void gen_default_array(sym_variable* sym_variable,
			nyla::type* type,
			llvm::Value* ll_arr_alloca,
			u32 depth = 0);

		llvm::Value* gen_malloc(llvm::Type* ll_type_to_alloc, u64 total_mem_size);
		llvm::Value* gen_malloc(llvm::Type* ll_type_to_alloc,
			                    u64 total_mem_size,
			                    llvm::Value* ll_array_size);

		// Unconditionally branches as long as the last statement
		// was not also a branch
		void branch_if_not_term(llvm::BasicBlock* ll_bb);

		nyla::afile_unit* m_file_unit = nullptr;

		nyla::compiler&    m_compiler;
		llvm::Module*      m_llvm_module;
		llvm::IRBuilder<>* m_llvm_builder;
		nyla::afunction*   m_function = nullptr;
		llvm::Function*    m_ll_function;
		// Current loop exit (Needed for breaks)
		// TODO: in the future a stack of these could exist
		// instead allowing the user to break out of multi-layered
		// loops
		llvm::BasicBlock* m_ll_loop_exit = nullptr;

		// Needed to determine context for getting an lvalue of
		// a variable
		bool m_initializing_module = false;
		
		bool m_initializing_globals = false;

		// Print the IR to console or not
		bool m_print;

	};
}

#endif