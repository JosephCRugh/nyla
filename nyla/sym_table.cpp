#include "sym_table.h"

using namespace nyla;

nyla::ascope* sym_table::push_scope(nyla::token* start_scope_token) {
	nyla::ascope* scope = nyla::make_node<nyla::ascope>(AST_SCOPE, start_scope_token->line_num);
	if (!m_scopes.empty()) {
		scope->parent = m_scopes.top();
	}
	m_scopes.push(scope);
	return scope;
}

void sym_table::pop_scope() {
	m_scopes.pop();
}

void nyla::sym_table::store_alloca(nyla::avariable* variable, llvm::AllocaInst* var_alloca) {
	m_alloced_variables[variable->name] = var_alloca;
}

llvm::AllocaInst* nyla::sym_table::get_alloca(nyla::avariable* variable) {
	return m_alloced_variables[variable->name];
}

void nyla::sym_table::store_variable_type(nyla::avariable* variable) {
	m_variable_types[variable->name] = variable->checked_type;
}

nyla::type* nyla::sym_table::get_variable_type(nyla::avariable* variable) {
	return m_variable_types[variable->name];
}

