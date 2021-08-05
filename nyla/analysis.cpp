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
	m_scope = nullptr;
	m_function = nullptr;
}

void nyla::analysis::type_check_function_call(nyla::afunction_call* function_call) {
	nyla::afunction* called_function =
		m_sym_table.get_declared_function(function_call->name);
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
	case AST_VALUE_INT:
	case AST_VALUE_FLOAT:
	case AST_VALUE_DOUBLE:
		type_check_number(dynamic_cast<nyla::anumber*>(expr));
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
		if (is_convertible_to(ret->value->checked_type, m_function->return_type)) {
			ret->value->checked_type = m_function->return_type;
		} else {
			//nyla::g_log->error(ERR_RETURN_TYPE_MISMATCH, ret->line_num,
			//	type_mismatch_data::make_type_mismatch(m_function->return_type, ret->value->checked_type));
		}
	}
}

void analysis::type_check_number(nyla::anumber* number) {
	// Nothing really to do since numbers already have types assigned.
	// Just mapping the tag to the proper type.
	switch (number->tag) {
	case AST_VALUE_INT:    number->checked_type = nyla::type::get_int();    break;
	case AST_VALUE_FLOAT:  number->checked_type = nyla::type::get_float();  break;
	case AST_VALUE_DOUBLE: number->checked_type = nyla::type::get_double(); break;
	default:
		assert(!"Haven't implemented type mapping for value.");
		break;
	}
}


void analysis::type_check_binary_op(nyla::abinary_op* binary_op) {
	type_check_expression(binary_op->lhs);
	type_check_expression(binary_op->rhs);
	// TODO: this will need a number of modifications once modules
	// added
	nyla::type* lhs_checked_type = binary_op->lhs->checked_type;
	nyla::type* rhs_checked_type = binary_op->rhs->checked_type;

	// Must have already been dealt with.
	if (binary_op->lhs->tag == AST_ERROR || binary_op->rhs->tag == AST_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}
	if (lhs_checked_type->tag == TYPE_ERROR || rhs_checked_type->tag == TYPE_ERROR) {
		binary_op->checked_type = nyla::type::get_error();
		return;
	}

	if (lhs_checked_type == rhs_checked_type) {
		binary_op->checked_type = lhs_checked_type;
	}

	if (only_works_on_ints(binary_op->op)) {
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
		bool found_match = true;
		if (rhs_checked_type->tag == TYPE_STRING) {
			switch (lhs_checked_type->tag) {
			case TYPE_PTR_CHAR16:
				if (lhs_checked_type->ptr_depth == 1) {
					binary_op->checked_type = lhs_checked_type;
					// TODO: Generate an array of 16 bit characters
					// out of the String so that the llvm generator
					// generates off of an array instead of the internal
					// string type.
					return;
				}
				break;
			}
			found_match = false;
		} else {
			// At the moment it is very strict and always requires
			// the types be the same. Should probably allow upcasting
			if (!is_convertible_to(lhs_checked_type, rhs_checked_type)) {
				found_match = false;
			}
			binary_op->checked_type = lhs_checked_type;
		}
		if (!found_match) {
			produce_error(ERR_CANNOT_ASSIGN,
				type_mismatch_data::make_type_mismatch(lhs_checked_type, rhs_checked_type),
				binary_op);
		}
	}

	// If it is a comparison operator it
	// gets auto converted into a bool type.
	if (is_comp_op(binary_op->op)) {
		binary_op->checked_type = nyla::type::get_bool();
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
	// TODO: if the type is undetermined then it needs to now
	// be determined here.
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

bool analysis::is_convertible_to(nyla::type* from, nyla::type* to) {
	if (from->tag == TYPE_ERROR || to->tag == TYPE_ERROR) {
		return true; // Assumed that the error for this has already been handled
	}
	return *from == *to; // This may be too strict.
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
		nyla::make_node<nyla::atype_cast>(AST_TYPE_CAST, nullptr, nullptr);
	type_cast->value        = value;
	type_cast->checked_type = cast_to_type;
	return type_cast;
}

void analysis::produce_error(error_tag tag, error_data* data,
	                         nyla::ast_node* node) {
	m_log.error(tag, data, node->st, node->et);
}
