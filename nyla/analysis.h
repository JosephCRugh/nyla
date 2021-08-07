#pragma once

#include "ast.h"
#include "sym_table.h"
#include "log.h"

namespace nyla {

	class analysis {
	public:

		analysis(nyla::sym_table& sym_table, nyla::log& log)
			: m_sym_table(sym_table), m_log(log) {
		}

		void type_check_file_unit(nyla::afile_unit* file_unit);

		void type_check_function(nyla::afunction* function);

		void type_check_function_call(nyla::afunction_call* function_call);

		void type_check_expression(nyla::aexpr* expr);

		void type_check_return(nyla::areturn* ret);

		void type_check_number(nyla::anumber* number);

		void type_check_binary_op(nyla::abinary_op* binary_op);

		void type_check_unary_op(nyla::aunary_op* unary_op);

		void type_check_variable_decl(nyla::avariable_decl* variable_decl);

		void type_check_type_cast(nyla::atype_cast* type_cast);

		void type_check_variable(nyla::avariable* variable);

		void type_check_for_loop(nyla::afor_loop* for_loop);

		void type_check_type(nyla::type* type);

		void type_check_array(nyla::aarray* arr);

		void type_check_array_access(nyla::aarray_access* array_access);

	private:

		void flatten_array(nyla::type* assign_type, std::vector<u64> depths,
			               nyla::aarray* arr, nyla::aarray*& out_arr, u32 depth = 0);

		bool is_assignable_to(nyla::aexpr* from, nyla::type* to);

		bool attempt_assign(nyla::aexpr*& lhs, nyla::aexpr*& rhs);

		bool only_works_on_ints(u32 op);

		bool only_works_on_numbers(u32 op);

		bool is_comp_op(u32 op);

		nyla::atype_cast* make_cast(nyla::aexpr* value, nyla::type* cast_to_type);

		void produce_error(error_tag tag, error_data* data,
			               nyla::ast_node* node);

		nyla::aexpr* gen_default_value(nyla::type* type);

		nyla::afunction*      m_function;
		nyla::ascope*         m_scope;
		nyla::sym_table&      m_sym_table;
		nyla::log&            m_log;

	};
}