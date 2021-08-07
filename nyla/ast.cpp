#include "ast.h"

#include "type.h"

using namespace nyla;

// ast_node

std::ostream& nyla::operator<<(std::ostream& os, const nyla::ast_node* node) {
	node->print(os);
	return os;
}

std::string nyla::ast_node::indent(u32 depth) const {return std::string(depth * 4, ' '); }

// err_expr

void nyla::err_expr::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "Error Expr";
}

// atype_cast

void nyla::atype_cast::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "cast(" << *checked_type << ")" << std::endl;
	value->print(os, depth + 1);
}

// avariable

void nyla::avariable::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "\"" << name << "\"";
}

// avariable_decl

void nyla::avariable_decl::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "var_delc: " << variable << std::endl;
	if (assignment) {
		assignment->print(os, depth + 1);
	}
}

// areturn

void nyla::areturn::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "return" << std::endl;
	if (value) {
		value->print(os, depth + 1);
	}
}

// ascope

void nyla::ascope::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "scope" << std::endl;
	for (nyla::aexpr* stmt : stmts) {
		stmt->print(os, depth + 1);
		os << std::endl;
	}
}

// afor_loop

void nyla::afor_loop::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "for_loop" << std::endl;
	os << expr_header(depth) << "loop__declarations:" << std::endl;
	for (nyla::avariable_decl* decl : declarations) {
		decl->print(os, depth + 1);
		os << std::endl;
	}
	os << expr_header(depth) << "loop__condition:" << std::endl;
	if (cond) {
		cond->print(os, depth + 1);
		os << std::endl;
	}
	os << expr_header(depth) << "loop__body:" << std::endl;
	scope->print(os, depth + 1);
}

// anumber

void nyla::anumber::print(std::ostream& os, u32 depth) const {
	switch (tag) {
	case AST_VALUE_BYTE:
		os << expr_header(depth) << "byte: " << value_int; break;
	case AST_VALUE_SHORT:
		os << expr_header(depth) << "short: " << value_int; break;
	case AST_VALUE_INT:
		os << expr_header(depth) << "int: " << value_int; break;
	case AST_VALUE_LONG:
		os << expr_header(depth) << "long: " << value_long; break;
	case AST_VALUE_UBYTE:
		os << expr_header(depth) << "ubyte: " << value_uint; break;
	case AST_VALUE_USHORT:
		os << expr_header(depth) << "ushort: " << value_uint; break;
	case AST_VALUE_UINT:
		os << expr_header(depth) << "uint: " << value_uint; break;
	case AST_VALUE_ULONG:
		os << expr_header(depth) << "ulong: " << value_ulong; break;
	case AST_VALUE_FLOAT:
		os << expr_header(depth) << "float: " << value_float; break;
	case AST_VALUE_DOUBLE:
		os << expr_header(depth) << "double: " << value_double; break;
	}

}

// abool

void nyla::abool::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth);
	if (tof) os << "true"; else os << "false";
}

// aunary_op

void nyla::aunary_op::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "unary_op: " << nyla::token_tag_to_string(op) << std::endl;
	factor->print(os, depth + 1);
}

// abinary_op

void nyla::abinary_op::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "bin_op: " << nyla::token_tag_to_string(op) << std::endl;
	lhs->print(os, depth + 1);
	os << std::endl;
	rhs->print(os, depth + 1);
}

// afunction

void nyla::afunction::print(std::ostream& os, u32 depth) const {
	os << std::string(depth * 4, ' ');
	os << "function: " << name << std::endl;
	if (scope != nullptr) {
		scope->print(os, depth + 1);
	}
}

// afunction_call

void nyla::afunction_call::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "function call: " << name << std::endl;
	for (nyla::aexpr* parameter_value : parameter_values) {
		parameter_value->print(os, depth + 1);
		os << std::endl;
	}
}

// afile_unit

void nyla::afile_unit::print(std::ostream& os, u32 depth) const {
	for (nyla::afunction* function : functions) {
		function->print(os, depth);
		os << std::endl;
	}
}

// astring

void nyla::astring::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << lit;
}

void nyla::aarray::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "array:";
	os << " @depths: ";
	for (u32 i = 0; i < depths.size(); i++) {
		os << "[" << depths[i] << "]";
	}
	os << std::endl;
	for (nyla::aexpr* element : elements) {
		element->print(os, depth + 1);
		os << std::endl;
	}
}

// aarray_access

void aarray_access::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "array_access: ";
	variable->print(os, 0);
	os << std::endl;
	for (nyla::aexpr* index : indexes) {
		index->print(os, depth + 1);
		os << std::endl;
	}
}

// aaexpr

std::string nyla::aexpr::expr_header(u32 depth) const {
	return indent(depth) + "(" + (checked_type != nullptr ? "1" : "0") + ") ";
}

