#ifndef NYLA_ANALYSIS_H
#define NYLA_ANALYSIS_H

#include "ast.h"
#include "log.h"
#include "compiler.h"
#include "llvm_gen.h"

namespace nyla {

	class analysis {
	public:

		analysis(nyla::compiler& compiler, nyla::log& log,
			     nyla::sym_table* sym_table, nyla::afile_unit* file_unit);

		void check_file_unit();

		nyla::afile_unit* get_file_unit() { return m_file_unit; }

	private:

		void check_module(nyla::amodule* nmodule);

		// Prevent allocating a field which results in allocating
		// of the module trying to be allocated for. Checking for circular
		// fields
		bool check_circular_fields(nyla::avariable_decl* original_field, sym_module* sym_module, u32 unique_module_key);

		void check_expression(nyla::aexpr* expr);

		void check_function(nyla::afunction* function);

		void check_variable_decl(nyla::avariable_decl* variable_decl);

		void check_return(nyla::areturn* ret);

		void check_number(nyla::anumber* number);

		void check_binary_op(nyla::abinary_op* binary_op);

		void check_unary_op(nyla::aunary_op* unary_op);

		void check_ident(bool static_context, sym_scope* lookup_scope, nyla::aident* ident);

		void check_for_loop(nyla::afor_loop* for_loop);
		void check_while_loop(nyla::awhile_loop* while_loop);
		void check_loop(nyla::aloop_expr* loop);

		void check_type_cast(nyla::atype_cast* type_cast);

		void check_function_call(bool static_call,
								 sym_module* lookup_module,
			                     nyla::afunction_call* function_call,
			                     bool is_constructor);
		sym_function* find_best_canidate(const std::vector<sym_function*>& canidates,
			                             nyla::afunction_call* function_call,
			                             bool is_constructor);
		void check_function_call(sym_function* called_function,
			                     nyla::afunction_call* function_call);

		void check_array_access(bool static_context, sym_scope* lookup_scope, nyla::aarray_access* array_access);

		void check_array(nyla::aarray* arr, u32 depth = 0);

		void check_var_object(nyla::avar_object* var_object);

		void check_dot_op(nyla::adot_op* dot_op);

		void check_if(nyla::aif* ifstmt);

		// Make sure to push/pop the sym_scope around calls to this
		void check_scope(const std::vector<nyla::aexpr*>& stmts, bool& comptime);

		// Utility stuff

		bool is_assignable_to(nyla::type* to, nyla::type* from);

		nyla::aexpr* make_cast(nyla::aexpr* value, nyla::type* to_type);

		void attempt_assignment(nyla::type* to_type, nyla::aexpr*& value);
		bool attempt_array_assignment(nyla::type* to_element_type, nyla::aarray* arr);

		bool is_lvalue(nyla::aexpr* expr);

		bool compare_arr_size(nyla::aarray* arr,
			                  const std::vector<u32>& computed_arr_dim_sizes,
			                  u32 depth = 0);
		bool compare_arr_size(nyla::astring* str,
			                  const std::vector<u32>& computed_arr_dim_sizes,
			                  u32 depth = 0);

		// Creates a new ast_node
		template<typename node>
		node* make(ast_tag tag, nyla::ast_node* anode) {
			node* n = new node;
			n->tag      = tag;
			n->line_num = anode->line_num;
			n->spos     = anode->spos;
			n->epos     = anode->epos;
			return n;
		}

		nyla::compiler&  m_compiler;
		nyla::sym_table* m_sym_table;
		nyla::log&       m_log;

		  // Current scope to lookup values in
		nyla::sym_scope*  m_sym_scope  = nullptr;
		  // Current local module to lookup functions in
		nyla::sym_module* m_sym_module = nullptr;
		nyla::afile_unit* m_file_unit  = nullptr;
		nyla::afunction*  m_function   = nullptr;

		llvm_generator m_llvm_generator;

		bool m_checking_globals = false;
		bool m_checking_fields  = false;

	};
}

#endif