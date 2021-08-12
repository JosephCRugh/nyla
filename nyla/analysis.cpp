#include "analysis.h"

#include <assert.h>

using namespace nyla;

#include <unordered_set>
#include <unordered_map>

void analysis::type_check_file_unit(nyla::afile_unit* file_unit) {
	for (nyla::afunction* function : file_unit->functions) {
		if (!function->is_external)
			type_check_function(function);
	}
}

void analysis::type_check_function(nyla::afunction* function) {
	m_function = function;
	m_scope = function->scope;
	for (nyla::aexpr* expr : function->scope->stmts) {
		type_check_expression(expr);
	}
	m_scope = m_scope->parent;
	m_function = nullptr;
}

void nyla::analysis::type_check_function_call(nyla::afunction_call* function_call) {
	nyla::afunction* called_function =
		m_sym_table.get_declared_function(function_call->ident->name);
	function_call->is_constexpr = false; // TODO: This should be true if the called
	                                     // function is constexpr
	if (called_function == nullptr) {
		produce_error(ERR_FUNCTION_NOT_FOUND,
			error_data::make_str_literal_load(function_call->ident->name.c_str()),
			function_call
		);
		function_call->checked_type = nyla::type::get_error();
		return;
	}
	function_call->called_function = called_function;
	function_call->checked_type = called_function->return_type;
	for (u32 i = 0; i < function_call->parameter_values.size(); i++) {
		nyla::aexpr*& parameter_value = function_call->parameter_values[i];
		type_check_expression(parameter_value);
		attempt_pass_arg(parameter_value, called_function->parameters[i]->checked_type);
	}
}

void analysis::type_check_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_VARIABLE_DECL:
		type_check_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
		break;
	case AST_RETURN:
		type_check_return(dynamic_cast<nyla::areturn*>(expr));
		break;
	case AST_TYPE_CAST:
		type_check_type_cast(dynamic_cast<nyla::atype_cast*>(expr));
		break;
	case AST_FOR_LOOP:
		type_check_for_loop(dynamic_cast<nyla::afor_loop*>(expr));
		break;
	case AST_VALUE_BYTE:
	case AST_VALUE_SHORT:
	case AST_VALUE_INT:
	case AST_VALUE_LONG:
	case AST_VALUE_UBYTE:
	case AST_VALUE_USHORT:
	case AST_VALUE_UINT:
	case AST_VALUE_ULONG:
	case AST_VALUE_FLOAT:
	case AST_VALUE_DOUBLE:
		type_check_number(dynamic_cast<nyla::anumber*>(expr));
		break;
	case AST_ARRAY: {
		nyla::aarray* arr = dynamic_cast<nyla::aarray*>(expr);
		type_check_array(arr, arr->depths);
		break;
	}
	case AST_VALUE_BOOL:
		expr->checked_type = nyla::type::get_bool();
		break;
	case AST_VALUE_STRING:
		expr->checked_type = nyla::type::get_string();
		break;
	case AST_BINARY_OP:
		type_check_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
		break;
	case AST_UNARY_OP:
		type_check_unary_op(dynamic_cast<nyla::aunary_op*>(expr));
		break;
	case AST_FUNCTION_CALL:
		type_check_function_call(dynamic_cast<nyla::afunction_call*>(expr));
		break;
	case AST_ERROR:
		expr->checked_type = nyla::type::get_error();
		break;
	case AST_ARRAY_ACCESS:
		type_check_array_access(dynamic_cast<nyla::aarray_access*>(expr));
		break;
	case AST_VALUE_NULL:
		expr->checked_type = nyla::type::get_null();
		break;
	case AST_VARIABLE: // Already assumed type checked from declaration
		break;
	case AST_IDENTIFIER:
		type_check_identifier(dynamic_cast<nyla::aidentifier*>(expr));
		break;
	default:
		assert(!"Failed to implement a type check");
		break;
	}
}

void analysis::type_check_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		// Ensuring that the return type of the function
		// is void.
		if (m_function->return_type->tag != TYPE_VOID) {
			produce_error(ERR_SHOULD_RETURN_VALUE, nullptr, ret);
		}
	} else {
		type_check_expression(ret->value);
		

		if (is_assignable_to(ret->value, m_function->return_type)) {
			// TODO: need to provide better assignable functionality than
			// just type assigning
			ret->value->checked_type = m_function->return_type;
		} else {
			produce_error(ERR_CANNOT_ASSIGN,
				type_mismatch_data::make_type_mismatch(m_function->return_type, ret->value->checked_type),
				ret->value);
			ret->value->checked_type = nyla::type::get_error();
		}
	}
}

void analysis::type_check_number(nyla::anumber* number) {
	// Nothing really to do since numbers already have types assigned.
	// Just mapping the tag to the proper type.
	switch (number->tag) {
	case AST_VALUE_BYTE:   number->checked_type = nyla::type::get_byte();   break;
	case AST_VALUE_SHORT:  number->checked_type = nyla::type::get_short();  break;
	case AST_VALUE_INT:    number->checked_type = nyla::type::get_int();    break;
	case AST_VALUE_LONG:   number->checked_type = nyla::type::get_long();   break;
	case AST_VALUE_UBYTE:  number->checked_type = nyla::type::get_ubyte();  break;
	case AST_VALUE_USHORT: number->checked_type = nyla::type::get_ushort(); break;
	case AST_VALUE_UINT:   number->checked_type = nyla::type::get_uint();   break;
	case AST_VALUE_ULONG:  number->checked_type = nyla::type::get_ulong();  break;
	case AST_VALUE_FLOAT:  number->checked_type = nyla::type::get_float();  break;
	case AST_VALUE_DOUBLE: number->checked_type = nyla::type::get_double(); break;

	default:
		assert(!"Haven't implemented type mapping for value.");
		break;
	}
}

void analysis::type_check_binary_op(nyla::abinary_op* binary_op) {
	
	// Must have already been dealt with.
	if (binary_op->lhs->tag == AST_ERROR || binary_op->rhs->tag == AST_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}

	type_check_expression(binary_op->lhs);

	nyla::type* lhs_checked_type = binary_op->lhs->checked_type;

	if (lhs_checked_type->tag == TYPE_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}

	if (binary_op->op == '.') {
		if (binary_op->lhs->checked_type->is_arr()) {
			// TODO: need to check lhs is a variable?
			
			///  Tree with dimension qualifier
			//    . op
			//    / \
			//  lhs  rhs
			//  /     \
			// arr.length [n]
            //  ^          ^--- Dimension
			//  |
			// is array -> then check dimension

			if (binary_op->rhs->tag == AST_ARRAY_ACCESS) {
				nyla::aarray_access* dimension_access =
					dynamic_cast<nyla::aarray_access*>(binary_op->rhs);
				nyla::aidentifier* ident = dynamic_cast<nyla::aidentifier*>(dimension_access->ident);

				if (ident->name != nyla::name::make("length")) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				if (dimension_access->next) {
					produce_error(ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_SINGLE_DIM, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}
				nyla::aexpr* dimension_expr = dimension_access->index;
				if (dimension_expr->tag != AST_VALUE_INT) {
					produce_error(ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_INT_DIM, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}
				nyla::anumber* dimension_number = dynamic_cast<nyla::anumber*>(dimension_expr);
				if (dimension_number->value_int > lhs_checked_type->arr_depth-1) {
					produce_error(ERR_ARRAY_LENGTH_OPERATOR_INVALID_DIM_INDEX,
						error_data::make_type_load(binary_op->lhs->checked_type), binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				delete binary_op->rhs;
				// TODO delete dimension_access->variable;
				binary_op->rhs = dimension_number;
				binary_op->op = TK_ARRAY_LENGTH;
				binary_op->checked_type = nyla::type::get_ulong();
			} else {
				
				if (binary_op->rhs->tag != AST_IDENTIFIER) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				nyla::aidentifier* rhs_name = dynamic_cast<nyla::aidentifier*>(binary_op->rhs);
				if (rhs_name->name != nyla::name::make("length")) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				if (lhs_checked_type->arr_depth > 1) {
					produce_error(ERR_ARRAY_LENGTH_NO_DIM_INDEX_FOR_MULTIDIM_ARRAY, 
						error_data::make_type_load(binary_op->lhs->checked_type), binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				// Replacing the rhs operatation with a request for the array's length
				delete binary_op->rhs;
				binary_op->rhs = gen_default_value(nyla::type::get_int()); // Generating value 0
				binary_op->op = TK_ARRAY_LENGTH;
				binary_op->checked_type = nyla::type::get_ulong();
			}
			return;
		} else {
			// TODO field access, ect..
		}
	}

	type_check_expression(binary_op->rhs);
	// TODO: this will need a number of modifications once modules
	// added
	nyla::type* rhs_checked_type = binary_op->rhs->checked_type;

	if (rhs_checked_type->tag == TYPE_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}

	if (binary_op->op == '.') {
		
	} if (only_works_on_ints(binary_op->op)) {
		// TODO: convert to the larger integer size
		if (!lhs_checked_type->is_int()) {
			// TODO: produce error
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
		if (!lhs_checked_type->is_int()) {
			// TODO: produce error
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
	} else if (only_works_on_numbers(binary_op->op)) {
		if (!lhs_checked_type->is_number()) {
			produce_error(ERR_OP_CANNOT_APPLY_TO,
				op_applies_to_data::make_applies_to(binary_op->op, binary_op->rhs->st),
				binary_op);
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
		if (!rhs_checked_type->is_number()) {
			produce_error(ERR_OP_CANNOT_APPLY_TO,
				op_applies_to_data::make_applies_to(binary_op->op, binary_op->rhs->st),
				binary_op);
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
		if (lhs_checked_type != rhs_checked_type) {
			if (lhs_checked_type->is_int() && rhs_checked_type->is_int()) {
				if (lhs_checked_type->get_mem_size() > rhs_checked_type->get_mem_size()) {
					// Converting rhs to lhs type
					binary_op->rhs = make_cast(binary_op->rhs, lhs_checked_type);
					binary_op->checked_type = lhs_checked_type; // Since the lhs is prefered.
				} else {
					assert(lhs_checked_type->get_mem_size() < rhs_checked_type->get_mem_size());
					// Converting lhs to rhs type
					binary_op->lhs = make_cast(binary_op->lhs, rhs_checked_type);
					binary_op->checked_type = rhs_checked_type; // Since the rhs is prefered.
				}
			} else if (lhs_checked_type->is_float() && rhs_checked_type->is_int()) {
				binary_op->rhs = make_cast(binary_op->rhs, lhs_checked_type);
				binary_op->checked_type = lhs_checked_type; // Since the lhs is prefered.
			} else if (lhs_checked_type->is_int() && rhs_checked_type->is_float()) {
				binary_op->lhs = make_cast(binary_op->lhs, rhs_checked_type);
				binary_op->checked_type = rhs_checked_type; // Since the rhs is prefered.
			} else if (lhs_checked_type->is_float() && rhs_checked_type->is_float()) {
				if (lhs_checked_type->get_mem_size() > rhs_checked_type->get_mem_size()) {
					binary_op->rhs = make_cast(binary_op->rhs, lhs_checked_type);
					binary_op->checked_type = lhs_checked_type; // Since the lhs is prefered.
				} else {
					assert(lhs_checked_type->get_mem_size() < rhs_checked_type->get_mem_size());
					binary_op->lhs = make_cast(binary_op->lhs, rhs_checked_type);
					binary_op->checked_type = rhs_checked_type; // Since the rhs is prefered.
				}
			}
		} else {
			binary_op->checked_type = lhs_checked_type;
		}
	} else if (binary_op->op == '=') {
		// to  = from
		// lhs = rhs
		if (!is_assignable_to(binary_op->rhs, lhs_checked_type)) {
			produce_error(ERR_CANNOT_ASSIGN,
				type_mismatch_data::make_type_mismatch(lhs_checked_type, rhs_checked_type),
				binary_op);
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
		if (!attempt_assign(binary_op->lhs, binary_op->rhs)) {
			binary_op->checked_type = nyla::type::get_error();
			return;
		}
		binary_op->checked_type = binary_op->lhs->checked_type;
	}

	// If it is a comparison operator it
	// gets auto converted into a bool type.
	if (is_comp_op(binary_op->op)) {
		binary_op->checked_type = nyla::type::get_bool();
	}
	if (!binary_op->lhs->is_constexpr || !binary_op->rhs->is_constexpr) {
		binary_op->is_constexpr = false;
	}
}

void analysis::type_check_unary_op(nyla::aunary_op* unary_op) {
	type_check_expression(unary_op->factor);

	if (unary_op->factor->checked_type->tag == TYPE_ERROR) {
		unary_op->checked_type = nyla::type::get_error();
		return;
	}

	if (only_works_on_numbers(unary_op->op)) {
		if (!unary_op->factor->checked_type->is_number()) {
			produce_error(ERR_OP_CANNOT_APPLY_TO,
				op_applies_to_data::make_applies_to(unary_op->op, unary_op->factor->st),
				unary_op);
			unary_op->checked_type = nyla::type::get_error();
			return;
		}
		unary_op->checked_type = unary_op->factor->checked_type;
	}
}

void analysis::type_check_variable_decl(nyla::avariable_decl* variable_decl) {
	type_check_type(variable_decl->variable->checked_type);
	if (variable_decl->assignment != nullptr) {
		type_check_expression(variable_decl->assignment);
	}
}

void analysis::type_check_type_cast(nyla::atype_cast* type_cast) {
	type_check_expression(type_cast->value);
}

void analysis::type_check_identifier(nyla::aidentifier* identifier) {
	
	// Since the variable was declared previously we simple look up
	// it's type.
	nyla::avariable* declared_variable = m_sym_table.get_declared_variable(m_scope, identifier);

	if (declared_variable == nullptr) {
		produce_error(ERR_UNDECLARED_VARIABLE,
			error_data::make_str_literal_load(identifier->name.c_str()),
			identifier);
		identifier->checked_type = nyla::type::get_error();
		return;
	}

	// Making sure the variable was declared before it was used.
	if (declared_variable->st->spos > identifier->st->spos) {
		produce_error(ERR_USE_BEFORE_DECLARED_VARIABLE,
			error_data::make_str_literal_load(identifier->name.c_str()),
			identifier);
		identifier->checked_type = nyla::type::get_error();
		return;
	}

	identifier->checked_type = declared_variable->checked_type;
	identifier->variable = declared_variable;
}

void nyla::analysis::type_check_for_loop(nyla::afor_loop* for_loop) {
	m_scope = for_loop->scope;
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		type_check_expression(var_decl);
	}
	type_check_expression(for_loop->cond);
	if (for_loop->cond->checked_type->tag != TYPE_BOOL) {
		if (for_loop->cond->checked_type->tag != TYPE_ERROR) {
			produce_error(ERR_EXPECTED_BOOL_COND, nullptr, for_loop->cond);
		}
	}
	for (nyla::aexpr* expr : for_loop->scope->stmts) {
		type_check_expression(expr);
	}
	m_scope = m_scope->parent;
}

void analysis::type_check_type(nyla::type* type) {
	// TODO: if the type is undetermined then it needs to now
	// be determined here.
	if (type->is_arr()) {
		// Possible that no number exist between []
		if (type->dim_size != nullptr) {
			type_check_expression(type->dim_size);
			if (!type->dim_size->checked_type->is_int()) {
				produce_error(ERR_ARRAY_SIZE_EXPECTS_INT,
					error_data::make_type_load(type),
					type->dim_size);
				return;
			}
		}
		
		type_check_type(type->elem_type);
	}
}

void analysis::type_check_array(nyla::aarray* arr, const std::vector<u64>& depths, u32 depth) {
	if (arr->checked_type == nyla::type::get_error()) {
		return;
	}
	
	for (nyla::aexpr* element : arr->elements) {
		if (element->tag == AST_ARRAY) {
			type_check_array(dynamic_cast<nyla::aarray*>(element), depths, depth + 1);
		} else {
			type_check_expression(element);
		}
	}

	nyla::anumber* processed_dim_size =
		nyla::make_node<nyla::anumber>(AST_VALUE_ULONG, arr->st, arr->st);
	processed_dim_size->value_ulong = depths[depth];
	processed_dim_size->checked_type = nyla::type::get_ulong();

	nyla::type* fst_arr_type = nyla::type::get_arr(nullptr, processed_dim_size);
	nyla::type* cur_arr_type = fst_arr_type;
	for (u32 i = 1; i < depths.size(); i++) {
		processed_dim_size =
			nyla::make_node<nyla::anumber>(AST_VALUE_ULONG, arr->st, arr->st);
		processed_dim_size->value_ulong = depths[i];
		processed_dim_size->checked_type = nyla::type::get_ulong();

		nyla::type* arr_type = nyla::type::get_arr(nullptr, processed_dim_size);
		cur_arr_type->elem_type = arr_type;
		cur_arr_type = arr_type;
	}
	cur_arr_type->elem_type = nyla::type::get_mixed();

	arr->checked_type = fst_arr_type;
	arr->checked_type->calculate_arr_depth();

}

void analysis::type_check_array_access(nyla::aarray_access* array_access, u32 depth) {
	type_check_expression(array_access->index);
	array_access->index = make_cast(array_access->index, nyla::type::get_ulong());

	if (array_access->next) {
		type_check_array_access(dynamic_cast<nyla::aarray_access*>(array_access->next), depth + 1);
		if (array_access->next->checked_type->tag == TYPE_ERROR) {
			array_access->checked_type = nyla::type::get_error();
			return;
		}
		array_access->checked_type = array_access->next->checked_type;
		array_access->ident = array_access->next->ident;
	} else {
		type_check_identifier(dynamic_cast<nyla::aidentifier*>(array_access->ident));
		if (array_access->ident->checked_type->tag == TYPE_ERROR) {
			array_access->checked_type = nyla::type::get_error();
			return;
		}
		array_access->checked_type = array_access->ident
			                                     ->checked_type
			                                     ->get_array_at_depth(depth + 1);
	}
	
}

void analysis::flatten_array(nyla::type* assign_type, std::vector<u64> depths,
	                         nyla::aarray* arr, nyla::aarray*& out_arr, u32 depth) {
	bool at_bottom = false;
	u64 processed = 0;
	for (nyla::aexpr* element : arr->elements) {
		if (element->tag == AST_ARRAY) {
			nyla::aarray* inner_arr = dynamic_cast<nyla::aarray*>(element);
			flatten_array(assign_type, depths, inner_arr, out_arr, depth + 1);
			delete inner_arr;
		} else {
			at_bottom = true;
			type_check_expression(element);
			out_arr->elements.push_back(element);
		}
		++processed;
	}
	if (at_bottom) {
		u64 amount_at_bottom = depths[depth];
		for (u64 i = processed; i < amount_at_bottom; i++) {
			out_arr->elements.push_back(gen_default_value(assign_type));
		}
	} else {
		
		// Filling algorithm
		// [i][j][k]
		//	^  ^  k-processed
		//	|  |
		//	|  k
		// i*k

		u64 amount_at_depth = depths[depth];

		u64 amount_to_fill = depths[depths.size() - 1];
		for (u32 i = depths.size() - 2; i > depth; i--) {
			amount_to_fill *= depths[i];
		}
		for (u64 i = processed; i < amount_at_depth; i++) {
			for (u64 j = 0; j < amount_to_fill; j++) {
				out_arr->elements.push_back(gen_default_value(assign_type));
			}
		}
	}
}

bool analysis::is_assignable_to(nyla::aexpr* from, nyla::type* to) {
	if (from->checked_type->tag == TYPE_ERROR || to->tag == TYPE_ERROR) {
		return true; // Assumed that the error for this has already been handled
	}
 
	if (from->checked_type->tag == TYPE_STRING) {
		switch (to->tag) {
		case TYPE_PTR: {
			if (to->get_ptr_base_type()->tag == TYPE_CHAR16) {
				return to->ptr_depth == 1;
			}
			return false;
		}
		case TYPE_ARR: {
			if (to->get_array_base_type()->tag == TYPE_CHAR16) {
				return to->arr_depth == 1;
			}
			return false;
		}
		default:
			return false;
		}
	} else if (from->checked_type->is_arr()) {
		return to->is_arr() || to->is_ptr();
	} else if (from->checked_type->tag == TYPE_NULL) {
		return to->is_ptr();
	} else {
		return *from->checked_type == *to; // TODO: probably too strict
	}
}

bool nyla::analysis::attempt_assign(nyla::aexpr*& lhs, nyla::aexpr*& rhs) {

	nyla::type* lhs_type = lhs->checked_type;
	nyla::type* rhs_type = rhs->checked_type;
	if (lhs->checked_type->tag == TYPE_ERROR) return true;
	if (rhs->checked_type->tag == TYPE_ERROR) return true;

	// lhs       = rhs
	// int[][] a = { {1, 2}, {7, 8} }

	if (rhs_type->tag == TYPE_STRING) {
		nyla::astring* str = dynamic_cast<nyla::astring*>(rhs);
		switch (lhs_type->tag) {
		case TYPE_ARR: {

			nyla::type* arr_type = lhs->checked_type;
			while (arr_type->tag == TYPE_ARR) {
				if (arr_type->dim_size != nullptr) {
					produce_error(ERR_ARRAY_SIZE_SHOULD_BE_IMPLICIT, nullptr, lhs);
					return false;
				}
				arr_type = arr_type->elem_type;
			}

			if (lhs_type->get_array_base_type()->tag == TYPE_CHAR16) {

				nyla::aarray* str_as_arr =
					make_node<nyla::aarray>(AST_ARRAY, rhs->st, rhs->et);
				str_as_arr->depths.push_back(str->lit.length());
				for (c8& ch : str->lit) {
					nyla::anumber* expr_ch =
						make_node<nyla::anumber>(AST_VALUE_SHORT, rhs->st, rhs->et);
					expr_ch->checked_type = nyla::type::get_char16();
					expr_ch->value_int = ch & 0xFF;
					str_as_arr->elements.push_back(expr_ch);
				}

				nyla::anumber* dim_size = make_node<nyla::anumber>(AST_VALUE_ULONG,
					lhs->st, lhs->et);
				dim_size->value_ulong = str->lit.length();
				dim_size->checked_type = nyla::type::get_ulong();
				str_as_arr->checked_type = nyla::type::get_arr(
					nyla::type::get_char16(), dim_size);
				str_as_arr->checked_type->calculate_arr_depth();

				delete rhs;
				rhs = str_as_arr;

				lhs->checked_type = rhs->checked_type;
				get_variable(lhs)->checked_type = lhs->checked_type;
			}
			break;
		}
		case TYPE_PTR: {
			// TODO
			break;
		}
		}

	} else if (rhs->tag == AST_ARRAY) {
		
		if (rhs_type->arr_depth != lhs_type->arr_depth) {
			produce_error(ERR_DIMENSIONS_OF_ARRAYS_MISMATCH,
				type_mismatch_data::make_type_mismatch(lhs_type, rhs_type), rhs);
			return false;
		}

		nyla::type* arr_type = lhs->checked_type;
		while (arr_type->tag == TYPE_ARR) {
			if (arr_type->dim_size != nullptr) {
				produce_error(ERR_ARRAY_SIZE_SHOULD_BE_IMPLICIT, nullptr, lhs);
				return false;
			}
			arr_type = arr_type->elem_type;
		}

		// Here I would flatten the array and assign default
		// values to fill empty space in the array
		nyla::aarray* arr = dynamic_cast<nyla::aarray*>(rhs);

		nyla::aarray* flattened_arr = make_node<nyla::aarray>(AST_ARRAY, arr->st, arr->et);
		u64 flattened_size = arr->depths[0];
		for (u32 i = 1; i < arr->depths.size(); i++) {
			flattened_size *= arr->depths[i];
		}

		flattened_arr->elements.reserve(flattened_size);
		flatten_array(lhs->checked_type->get_array_base_type(), arr->depths, arr, flattened_arr);
		flattened_arr->checked_type = rhs->checked_type;
		flattened_arr->checked_type->set_array_base_type(lhs->checked_type->get_array_base_type());

		delete rhs; // Deleting the rhs and replacing it with a flattened version of the
		            // array
		rhs = flattened_arr;
		lhs->checked_type = rhs->checked_type;
		get_variable(lhs)->checked_type = lhs->checked_type;

		// Making sure the elements are compatible
		for (nyla::aexpr* element : flattened_arr->elements) {
			if (!is_assignable_to(element, lhs_type->get_array_base_type())) {
				produce_error(ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY,
					type_mismatch_data::make_type_mismatch(lhs_type->get_array_base_type(), element->checked_type), element);
				return false;
			}
		}

	} else if (rhs_type->tag == TYPE_NULL) {
		// Converting RHS into numberic value of zero for the pointer
		delete rhs;
		rhs = make_cast(gen_default_value(nyla::type::get_int()), lhs->checked_type);
	} else if (rhs_type->is_arr()) {
		if (lhs_type->is_ptr()) {
			// T[] b;
			// T* ptr a = b;
			rhs->checked_type = lhs->checked_type;
		} else {
			nyla::avariable* variable     = get_variable(lhs);
			variable->arr_alloc_reference = get_variable(rhs);
			lhs->checked_type = rhs->checked_type;
		}
	} else {
		lhs->checked_type = rhs->checked_type;
	}
}

bool analysis::attempt_pass_arg(nyla::aexpr*& argument, nyla::type* param_type) {
	if (argument->tag == AST_VALUE_NULL) { // null to zero value
		delete argument;
		argument = make_cast(gen_default_value(nyla::type::get_int()), param_type);
	}
	return true;
}

bool analysis::only_works_on_ints(u32 op) {
	switch (op) {
	default:
		return false;
	}
}

bool analysis::only_works_on_numbers(u32 op) {
	switch (op) {
	case '+': case '-': case '*': case '/':
	case '<': case '>':
		return true;
	default:
		return false;
	}
}

bool analysis::is_comp_op(u32 op) {
	switch (op) {
	case '<': case '>':
		return true;
	default:
		return false;
	}
}

nyla::atype_cast* analysis::make_cast(nyla::aexpr* value, nyla::type* cast_to_type) {
	nyla::atype_cast* type_cast =
		nyla::make_node<nyla::atype_cast>(AST_TYPE_CAST, value->st, value->et);
	type_cast->value        = value;
	type_cast->checked_type = cast_to_type;
	return type_cast;
}

void analysis::produce_error(error_tag tag, error_data* data,
	                         nyla::ast_node* node) {
	m_log.error(tag, data, node->st, node->et);
}

nyla::aexpr* analysis::gen_default_value(nyla::type* type) {
	switch (type->tag) {
	case TYPE_ARR:
		assert(!"Cannot generate a default value for an array");
		break;
	case TYPE_BYTE: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_BYTE, type->st, type->et);
		value->checked_type = type;
		value->value_int = 0;
		return value;
	}
	case TYPE_SHORT: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_SHORT, type->st, type->et);
		value->checked_type = type;
		value->value_int = 0;
		return value;
	}
	case TYPE_INT: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_INT, type->st, type->et);
		value->checked_type = type;
		value->value_int = 0;
		return value;
	}
	case TYPE_LONG: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_LONG, type->st, type->et);
		value->checked_type = type;
		value->value_long = 0;
		return value;
	}
	case TYPE_UBYTE: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_UBYTE, type->st, type->et);
		value->checked_type = type;
		value->value_uint = 0;
		return value;
	}
	case TYPE_USHORT: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_USHORT, type->st, type->et);
		value->checked_type = type;
		value->value_uint = 0;
		return value;
	}
	case TYPE_UINT: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_UINT, type->st, type->et);
		value->checked_type = type;
		value->value_uint = 0;
		return value;
	}
	case TYPE_ULONG: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_ULONG, type->st, type->et);
		value->checked_type = type;
		value->value_ulong = 0;
		return value;
	}
	case TYPE_FLOAT: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_FLOAT, type->st, type->et);
		value->checked_type = type;
		value->value_float = 0.0F;
		return value;
	}
	case TYPE_DOUBLE: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_DOUBLE, type->st, type->et);
		value->checked_type = type;
		value->value_double = 0.0;
		return value;
	}
	case TYPE_CHAR16: {
		nyla::anumber* value = make_node<nyla::anumber>(AST_VALUE_CHAR16, type->st, type->et);
		value->checked_type = type;
		value->value_int = 0;
		return value;
	}
	default:
		assert(!"Failed to implement default value for type");
		break;
	}
	return nullptr;
}

nyla::avariable* analysis::get_variable(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_IDENTIFIER:
		return dynamic_cast<nyla::aidentifier*>(expr)->variable;
	case AST_VARIABLE:
		return dynamic_cast<nyla::avariable*>(expr);
	case AST_ARRAY_ACCESS:
		return dynamic_cast<nyla::aarray_access*>(expr)->ident->variable;
	}
	assert(!"Should only be called when trying to get an already allocated lvalue");
	return nullptr;
}
