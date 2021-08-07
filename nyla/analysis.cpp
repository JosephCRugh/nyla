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
		m_sym_table.get_declared_function(function_call->name);
	function_call->is_constexpr = false; // TODO: This should be true if the called
	                                     // function is constexpr
	if (called_function == nullptr) {
		produce_error(ERR_FUNCTION_NOT_FOUND,
			error_data::make_str_literal_load(function_call->name.c_str()),
			function_call
		);
		function_call->checked_type = nyla::type::get_error();
		return;
	}
	function_call->called_function = called_function;
	function_call->checked_type = called_function->return_type;
	for (nyla::aexpr* parameter_value : function_call->parameter_values) {
		type_check_expression(parameter_value);
	}
}

void analysis::type_check_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_VARIABLE_DECL:
		type_check_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
		break;
	case AST_VARIABLE:
		type_check_variable(dynamic_cast<nyla::avariable*>(expr));
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
	case AST_ARRAY:
		type_check_array(dynamic_cast<nyla::aarray*>(expr));
		break;
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
			// TODO handle error
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
	

	type_check_expression(binary_op->lhs);

	// Must have already been dealt with.
	if (binary_op->lhs->tag == AST_ERROR || binary_op->rhs->tag == AST_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}

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
				if (dimension_access->variable->name != nyla::name::make("length")) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				if (dimension_access->indexes.size() > 1) {
					produce_error(ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_SINGLE_DIM, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}
				nyla::aexpr* dimension_expr = dimension_access->indexes[0];
				if (dimension_expr->tag != AST_VALUE_INT) {
					produce_error(ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_INT_DIM, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}
				nyla::anumber* dimension_number = dynamic_cast<nyla::anumber*>(dimension_expr);
				if (dimension_number->value_int > lhs_checked_type->array_depths.size()-1) {
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

				if (binary_op->rhs->tag != AST_VARIABLE) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				nyla::avariable* rhs_name = dynamic_cast<nyla::avariable*>(binary_op->rhs);
				if (rhs_name->name != nyla::name::make("length")) {
					produce_error(ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH, nullptr, binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

				if (lhs_checked_type->array_depths.size() > 1) {
					produce_error(ERR_ARRAY_LENGTH_NO_DIM_INDEX_FOR_MULTIDIM_ARRAY, 
						error_data::make_type_load(binary_op->lhs->checked_type), binary_op);
					binary_op->checked_type = nyla::type::get_error();
					return;
				}

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

	if (*lhs_checked_type == *rhs_checked_type) {
		binary_op->checked_type = lhs_checked_type;
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
		}
	} else if (binary_op->op == '=') {
		// to  = from
		// lhs = rhs
		if (!is_assignable_to(binary_op->rhs, lhs_checked_type)) {
			produce_error(ERR_CANNOT_ASSIGN,
				type_mismatch_data::make_type_mismatch(lhs_checked_type, rhs_checked_type),
				binary_op);
			binary_op->checked_type = nyla::type::get_error();
		}
		if (!attempt_assign(binary_op->lhs, binary_op->rhs)) {
			binary_op->checked_type = nyla::type::get_error();
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

void analysis::type_check_variable(nyla::avariable* variable) {
	// Since the variable was declared previously we simple look up
	// it's type.
	nyla::avariable* declared_variable = m_sym_table.get_declared_variable(m_scope, variable);
	
	if (declared_variable == nullptr) {
		produce_error(ERR_UNDECLARED_VARIABLE,
			error_data::make_str_literal_load(variable->name.c_str()),
			variable);
		variable->checked_type = nyla::type::get_error();
		return;
	}
	// Making sure the variable was declared before it was used.
	if (declared_variable->st->spos > variable->st->spos) {
		produce_error(ERR_USE_BEFORE_DECLARED_VARIABLE,
			error_data::make_str_literal_load(variable->name.c_str()),
			variable);
		variable->checked_type = nyla::type::get_error();
		return;
	}

	variable->checked_type = declared_variable->checked_type;
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
		for (nyla::aexpr* array_depth_expr : type->array_depths) {
			// Possible that no number exist between []
			if (array_depth_expr == nullptr) continue;

			type_check_expression(array_depth_expr);
			
			if (!array_depth_expr->is_constexpr) {
				/*produce_error(ERR_ARRAY_SIZE_EXPECTS_CONSTANT_EXPR,
					error_data::make_type_load(type),
					array_depth_expr);*/
				return;
			}
			if (!array_depth_expr->checked_type->is_int()) {
				produce_error(ERR_ARRAY_SIZE_EXPECTS_INT,
					error_data::make_type_load(type),
					array_depth_expr);
				return;
			}
		}
	}
}

void analysis::type_check_array(nyla::aarray* arr) {
	if (arr->checked_type == nyla::type::get_error()) {
		return;
	}

	for (nyla::aexpr* element : arr->elements) {
		type_check_expression(element);
	}
	std::vector<nyla::aexpr*> processed_array_depths;
	for (u64 depth : arr->depths) {
		nyla::anumber* processed_depth =
			nyla::make_node<nyla::anumber>(AST_VALUE_ULONG, arr->st, arr->st);
		processed_depth->value_ulong = depth;
		processed_array_depths.push_back(processed_depth);
	}

	arr->checked_type = nyla::type::get_arr_mixed(processed_array_depths);
}

void analysis::type_check_array_access(nyla::aarray_access* array_access) {
	type_check_variable(array_access->variable);
	if (array_access->variable->checked_type->tag == TYPE_ERROR) {
		array_access->checked_type = nyla::type::get_error();
		return;
	}
	// TODO 
	array_access->checked_type = array_access->variable->checked_type->get_element_type();
	for (u32 i = 0; i < array_access->indexes.size(); i++) {
		type_check_expression(array_access->indexes[i]);
		array_access->indexes[i] = make_cast(array_access->indexes[i], nyla::type::get_ulong());
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
		u64 amount_at_depth = depths[depth];
		// The lower the depth value the more to fill
		u64 amount_to_fill = depths[0];
		for (u32 i = 1; i < depths.size(); i++) {
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
			if (to->get_element_type()->tag == TYPE_CHAR16) {
				return to->ptr_depth == 1; // char16* s = "Hello!";
			}
			
			return false;
		}
		default:
			return false;
		}
	} else if (from->checked_type->is_arr()) {
		return to->is_arr();
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
		// TODO: we should be able to convert the string type
		// to various types such as char16*, char8*, ect..
		rhs->checked_type = lhs->checked_type;
	} else if (rhs_type->is_arr()) {
		
		if (rhs_type->array_depths.size() != lhs_type->array_depths.size()) {
			produce_error(ERR_DIMENSIONS_OF_ARRAYS_MISMATCH,
				type_mismatch_data::make_type_mismatch(lhs_type, rhs_type), rhs);
			return false;
		}

		for (nyla::aexpr* array_depth : lhs->checked_type->array_depths) {
			if (array_depth != nullptr) {
				produce_error(ERR_ARRAY_SIZE_SHOULD_BE_IMPLICIT, nullptr, lhs);
				return false;
			}
		}
		
		// Here I would flatten the array and assign default
		// values to fill empty space in the array
		nyla::aarray* arr = dynamic_cast<nyla::aarray*>(rhs);

		nyla::aarray* flattened_arr = make_node<nyla::aarray>(AST_ARRAY, arr->st, arr->et);
		u64 flattened_size = arr->depths[0];
		for (u32 i = 1; i < arr->depths.size(); i++) {
			flattened_size *= arr->depths[i];
		}
		std::vector<nyla::aexpr*> array_depths;
		array_depths.reserve(arr->depths.size());
		for (u32 i = 0; i < arr->depths.size(); i++) {
			nyla::anumber* depth_expr = make_node<nyla::anumber>(AST_VALUE_ULONG, arr->st, arr->st);
			depth_expr->value_ulong = arr->depths[i];
			depth_expr->checked_type = nyla::type::get_ulong();
			array_depths.push_back(depth_expr);
		}

		flattened_arr->elements.reserve(flattened_size);
		flatten_array(lhs->checked_type->get_element_type(), arr->depths, arr, flattened_arr);
		flattened_arr->checked_type = lhs->checked_type
			                             ->get_element_type()
			                             ->as_arr(array_depths);

		delete arr; // Deleting the rhs and replacing it with a flattened version of the
		            // array
		rhs = flattened_arr;
		
		lhs->checked_type = rhs->checked_type;

		// Making sure the elements are compatible
		for (nyla::aexpr* element : flattened_arr->elements) {
			if (!is_assignable_to(element, lhs_type->get_element_type())) {
				produce_error(ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY,
					type_mismatch_data::make_type_mismatch(lhs_type->get_element_type(), element->checked_type), element);
				return false;
			}
		}

	} else {
		lhs->checked_type = rhs->checked_type;
	}
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
