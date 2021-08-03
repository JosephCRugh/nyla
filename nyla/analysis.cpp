#include "analysis.h"

#include <assert.h>
#include "log.h"

using namespace nyla;

#include <unordered_set>
#include <unordered_map>

void analysis::type_check_function(nyla::afunction* function) {
	m_function = function;
	for (nyla::aexpr* expr : function->scope->stmts) {
		type_check_expression(expr);
	}
	m_function = nullptr;
}

void nyla::analysis::type_check_expression(nyla::aexpr* expr) {
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
	case AST_BINARY_OP:
		type_check_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
		break;
	}
}

void nyla::analysis::type_check_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		// Ensuring that the return type of the function
		// is void.
		if (m_function->return_type->tag != TYPE_VOID) {
			nyla::g_log->error(ERR_SHOULD_RETURN_VALUE, ret->line_num,
				error_data::make_empty_load());
		}
	} else {
		type_check_expression(ret->value);
		if (is_convertible_to(ret->value->checked_type, m_function->return_type)) {
			ret->value->checked_type = m_function->return_type;
		} else {
			nyla::g_log->error(ERR_RETURN_TYPE_MISMATCH, ret->line_num,
				type_mismatch_data::make_type_mismatch(m_function->return_type, ret->value->checked_type));
		}
	}
}

void nyla::analysis::type_check_number(nyla::anumber* number) {
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


void nyla::analysis::type_check_binary_op(nyla::abinary_op* binary_op) {
	type_check_expression(binary_op->lhs);
	type_check_expression(binary_op->rhs);
	// TODO: this will need a number of modifications once modules
	// added
	nyla::type* lhs_checked_type = binary_op->lhs->checked_type;
	nyla::type* rhs_checked_type = binary_op->rhs->checked_type;

	if (lhs_checked_type == rhs_checked_type) {
		binary_op->checked_type = lhs_checked_type;
	}

	if (only_works_on_ints(binary_op->op)) {
		// TODO: convert to the larger integer size
		if (!lhs_checked_type->is_int()) {
			// TODO: produce error
		}
		if (!lhs_checked_type->is_int()) {
			// TODO: produce error
		}
	} else if (only_works_on_numbers(binary_op->op)) {
		if (!lhs_checked_type->is_number()) {
			// TODO: produce error
		}
		if (!rhs_checked_type->is_number()) {
			// TODO: produce error
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
		// At the moment it is very strict and always requires
		// the types be the same. Should probably allow upcasting
		if (lhs_checked_type != rhs_checked_type) {
			nyla::g_log->error(ERR_CANNOT_ASSIGN, binary_op->line_num,
				type_mismatch_data::make_type_mismatch(lhs_checked_type, rhs_checked_type));
			
		}
		binary_op->checked_type = lhs_checked_type;
	}

	// If it is a comparison operator it
	// gets auto converted into a bool type.
	if (is_comp_op(binary_op->op)) {
		binary_op->checked_type = nyla::type::get_bool();
	}
}

void nyla::analysis::type_check_variable_decl(nyla::avariable_decl* variable_decl) {
	m_sym_table.store_variable_type(variable_decl->variable);
	if (variable_decl->assignment != nullptr) {
		type_check_expression(variable_decl->assignment);
	}
}

void nyla::analysis::type_check_type_cast(nyla::atype_cast* type_cast) {
	type_check_expression(type_cast->value);
}

void nyla::analysis::type_check_variable(nyla::avariable* variable) {
	// Since the variable was declared previously we simple look up
	// it's type.
	variable->checked_type = m_sym_table.get_variable_type(variable);
}

void nyla::analysis::type_check_for_loop(nyla::afor_loop* for_loop) {
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		type_check_expression(var_decl);
	}
	type_check_expression(for_loop->cond);
	if (for_loop->cond->checked_type->tag != TYPE_BOOL) {
		// TODO: report error
	}
	for (nyla::aexpr* stmt : for_loop->scope->stmts) {
		type_check_expression(stmt);
	}
	type_check_expression(for_loop->post);
}

bool analysis::is_convertible_to(nyla::type* from, nyla::type* to) {
	if (from->tag == TYPE_ERROR || to->tag == TYPE_ERROR) {
		return true; // Assumed that the error for this has already been handled
	}
	return from == to; // This may be too strict.
}

bool nyla::analysis::only_works_on_ints(u32 op) {
	switch (op) {
	default:
		return false;
	}
}

bool nyla::analysis::only_works_on_numbers(u32 op) {
	switch (op) {
	case '+': case '-': case '*': case '/':
	case '<':
		return true;
	default:
		return false;
	}
}

bool nyla::analysis::is_comp_op(u32 op) {
	switch (op) {
	case '<':
		return true;
	default:
		return false;
	}
}

nyla::atype_cast* nyla::analysis::make_cast(nyla::aexpr* value, nyla::type* cast_to_type) {
	nyla::atype_cast* type_cast =
		nyla::make_node<nyla::atype_cast>(AST_TYPE_CAST, value->line_num);
	type_cast->value        = value;
	type_cast->checked_type = cast_to_type;
	return type_cast;
}
