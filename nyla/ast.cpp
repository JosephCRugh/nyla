#include "ast.h"

#include "words.h"
#include "tokens.h"

nyla::ast_node::~ast_node() {

}

std::string nyla::ast_node::word_to_string(u32 word_key) const {
	return std::string(g_word_table->get_word(word_key).c_str());
}

std::string nyla::ast_node::indent(u32 depth) const {
	return std::string(depth * 4, ' ');
}

std::string nyla::ast_node::mods_as_string(u32 mods) const {
	std::string mods_str = "mods=[";
	for (u32 mod = nyla::modifier::MODS_START; mod <= nyla::modifier::MODS_END; mod *= 2) {
		if (mods & mod) {
			mods &= ~mod;
			mods_str += nyla::mod_to_string(mod) + (mods ? ", " : "");
		}
	}
	mods_str += "]";
	return mods_str;
}


std::string nyla::aexpr::expr_header(u32 depth) const {
	return indent(depth) + "(" + (type != nullptr ? "1" : "0") + ") ";
}


nyla::afile_unit::~afile_unit() {
	for (nyla::amodule* nmodule : modules) delete nmodule;
	for (auto& pair: imports) delete pair.second;
}

nyla::sym_module* nyla::afile_unit::find_module(u32 name_key) {
	auto it = loaded_modules.find(name_key);
	if (it != loaded_modules.end()) {
		return it->second;
	}
	return nullptr;
}

void nyla::afile_unit::print(std::ostream& os, u32 depth) const {
	os << "imports:\n";
	for (auto& pair : imports) {
		pair.second->print(os, depth);
		os << '\n';
	}
	os << "modules:\n";
	for (nyla::amodule* nmodule : modules) {
		nmodule->print(os, depth);
		os << '\n';
	}
}

void nyla::aimport::print(std::ostream& os, u32 depth) const {
	os << "path: \"" << path << "\"";
	if (!module_aliases.empty()) {
		os << '\n';
		os << "aliases:\n";
		for (auto& pair : module_aliases) {
			os << indent(1) << g_word_table->get_word(pair.first).c_str()
			   << " -> " << g_word_table->get_word(pair.second).c_str();
			os << '\n';
		}
	}
}

nyla::amodule::~amodule() {
	for (nyla::afunction* constructor : constructors)
		delete constructor;
	for (nyla::afunction* function : functions)
		delete function;

	// Not deleting fields since they are constantly reused
	// in the symbol table
}

void nyla::amodule::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "module=\"" << word_to_string(name_key) << "\"";
	os << " " << mods_as_string(sym_module->mods) << '\n';
	os << "constructors:\n";
	for (nyla::afunction* constructor : constructors) {
		constructor->print(os, depth + 1);
		os << '\n';
	}
	os << "fields:\n";
	for (nyla::avariable_decl* field : fields) {
		field->print(os, depth + 1);
		os << '\n';
	}
	os << "functions:\n";
	for (nyla::afunction* function : functions) {
		function->print(os, depth + 1);
		os << '\n';
	}
}

nyla::afunction::~afunction() {
	for (nyla::avariable_decl* param : parameters)
		delete param;
	for (nyla::aexpr* stmt : stmts) delete stmt;
}

bool nyla::afunction::is_external() const {
	return sym_function->mods & nyla::modifier::MOD_EXTERNAL;
}

void nyla::afunction::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "function=\"" << word_to_string(name_key) << "\"";
	os << " " << mods_as_string(sym_function->mods) << '\n';
	if (!is_external())
		os << indent(depth) << "stmts:\n";
	for (nyla::aexpr* stmt : stmts) {
		stmt->print(os, depth + 1);
		os << '\n';
	}
}

nyla::avariable_decl::~avariable_decl() {
	// If there is an assignment then it
	// it deleted by the '=' binary operator
	if (!assignment) delete ident;
}

void nyla::avariable_decl::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "var_decl=\"" << word_to_string(name_key) << "\"";
	os << " type='" << type->to_string() << "'";
	os << " " << mods_as_string(sym_variable->mods);
	if (sym_variable->is_field) {
		os << " field_index='" << sym_variable->field_index << "'";
	}
	if (assignment) {
		os << '\n';
		assignment->print(os, depth + 1);
	}
}

void nyla::areturn::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "return" << '\n';
	if (value)
		value->print(os, depth + 1);
}

nyla::areturn::~areturn() {
	delete value;
}

void nyla::aunary_op::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "unary_op: '" << nyla::token_tag_to_string(op) << "'\n";
	factor->print(os, depth + 1);
}

nyla::aunary_op::~aunary_op() {
	delete factor;
}

void nyla::abinary_op::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "bin_op: '" << nyla::token_tag_to_string(op) << "'";
	os << '\n';
	lhs->print(os, depth + 1);
	os << '\n';
	rhs->print(os, depth + 1);
}

nyla::abinary_op::~abinary_op() {
	if (!reuses_lhs) delete lhs;
	delete rhs;
}

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
	case AST_VALUE_CHAR8:
		os << expr_header(depth) << "char8: " << (c8)value_int; break;
	case AST_VALUE_CHAR16:
		os << expr_header(depth) << "char16: " << (char16_t)value_int; break;
	case AST_VALUE_CHAR32:
		os << expr_header(depth) << "char32: " << (char32_t)value_int; break;
	case AST_VALUE_NULL:
		os << expr_header(depth) << "null"; break;
	}
}

void nyla::abool::print(std::ostream& os, u32 depth) const {
}

void nyla::err_expr::print(std::ostream& os, u32 depth) const {
}

std::ostream& nyla::operator<<(std::ostream& os, const nyla::ast_node& node) {
	node.print(os);
	return os;
}

void nyla::aident::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "\"" << word_to_string(ident_key) << "\"";
}

void nyla::atype_cast::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "cast(" << type->to_string() << ")" << '\n';
	value->print(os, depth + 1);
}

nyla::atype_cast::~atype_cast() {
	delete value;
}

void nyla::astring::print(std::ostream& os, u32 depth) const {
	// TODO: replace escapes with proper output
	os << expr_header(depth) << "str8=\"" << lit8 << "\"";
}

void nyla::afor_loop::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "for_loop: " << '\n';
	os << indent(depth) << "declarations:" << '\n';
	for (nyla::avariable_decl* decl : declarations) {
		decl->print(os, depth + 1);
		os << '\n';
	}
	os << indent(depth) << "loop__condition:" << '\n';
	if (cond) {
		cond->print(os, depth + 1);
		os << '\n';
	}
	os << indent(depth) << "loop__body:" << '\n';
	for (nyla::aexpr* stmt : body) {
		stmt->print(os, depth + 1);
		os << '\n';
	}
}

nyla::afor_loop::~afor_loop() {
	for (nyla::avariable_decl* decl : declarations)
		delete decl;
}

void nyla::awhile_loop::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "while_loop: " << '\n';
	os << indent(depth) << "loop_condition:" << '\n';
	if (cond) {
		cond->print(os, depth + 1);
		os << '\n';
	}
	os << indent(depth) << "loop__body:" << '\n';
	for (nyla::aexpr* stmt : body) {
		stmt->print(os, depth + 1);
		os << '\n';
	}
}

nyla::aloop_expr::~aloop_expr() {
	delete cond;
	for (nyla::aexpr* stmt : body)
		delete stmt;
	for (nyla::aexpr* stmt : post_exprs)
		delete stmt;
}

void nyla::acontrol::print(std::ostream& os, u32 depth) const {
	switch (tag) {
	case AST_BREAK:    os << indent(depth) << "break"; break;
	case AST_CONTINUE: os << indent(depth) << "continue"; break;
	case AST_THIS:     os << indent(depth) << "this"; break;
	}
}

void nyla::aif::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "if:\n";
	os << indent(depth) << "cond:\n";
	cond->print(os, depth + 1);
	os << '\n';
	os << indent(depth) << "ifbody:\n";
	for (nyla::aexpr* stmt : body) {
		stmt->print(os, depth + 1);
		os << '\n';
	}
	if (else_if) {
		os << indent(depth) << "else_if:\n";
		else_if->print(os, depth + 1);
	}
}

nyla::aif::~aif() {
	delete cond;
	for (nyla::aexpr* stmt : body)
		delete stmt;
	if (else_if) delete else_if;
	for (nyla::aexpr* stmt : else_body)
		delete stmt;
}

void nyla::afunction_call::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "function call: " << word_to_string(name_key) << '\n';
	for (nyla::aexpr* parameter_value : arguments) {
		parameter_value->print(os, depth + 1);
		os << '\n';
	}
}

nyla::afunction_call::~afunction_call() {
	for (nyla::aexpr* argument : arguments)
		delete argument;
}

void nyla::adot_op::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "dot_op:\n";
	factor_list[0]->print(os, depth + 1);
	for (u32 i = 1; i < factor_list.size(); i++) {
		nyla::aexpr* rhs = factor_list[i];
		os << '\n';
		os << indent(depth + 1) << "dot";
		os << '\n';
		rhs->print(os, depth + 1);
	}
}

nyla::adot_op::~adot_op() {
	for (nyla::aexpr* factor : factor_list)
		delete factor;
}

void nyla::aarray_access::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "array_access: \n";
	for (nyla::aexpr* index : indexes) {
		index->print(os, depth + 1);
		os << '\n';
	}
}

void nyla::aarray::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "array:\n";
	for (nyla::aexpr* element : elements) {
		if (element) {
			element->print(os, depth + 1);
		} else {
			os << expr_header(depth + 1) << "default_value";
		}
		os << '\n';
	}
}

nyla::aarray::~aarray() {
	for (nyla::aexpr* element : elements) {
		delete element;
	}
}

void nyla::avar_object::print(std::ostream& os, u32 depth) const {
	os << expr_header(depth) << "var" << '\n';
	constructor_call->print(os, depth + 1);
}

void nyla::aannotation::print(std::ostream& os, u32 depth) const {
	os << indent(depth) << "annotation: " << word_to_string(ident_key);
}

nyla::aarray_access::~aarray_access() {
	delete ident;
	for (nyla::aexpr* index : indexes)
		delete index;
}
