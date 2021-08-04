#include "sym_table.h"

using namespace nyla;

nyla::ascope* sym_table::push_scope(nyla::token* start_scope_token) {
	nyla::ascope* scope = nyla::make_node<nyla::ascope>(AST_SCOPE, 
		start_scope_token, start_scope_token);
	if (!m_scopes.empty()) {
		scope->parent = m_scopes.top();
	}
	m_scopes.push(scope);
	return scope;
}

void sym_table::pop_scope() {
	m_scopes.pop();
}

void sym_table::store_alloca(nyla::avariable* variable, llvm::AllocaInst* var_alloca) {
	m_alloced_variables[variable->name] = var_alloca;
}

llvm::AllocaInst* sym_table::get_alloca(nyla::avariable* variable) {
	return m_alloced_variables[variable->name];
}

void sym_table::store_variable_type(nyla::avariable* variable) {
	m_variable_types[variable->name] = variable->checked_type;
}

nyla::type* sym_table::get_variable_type(nyla::avariable* variable) {
	return m_variable_types[variable->name];
}

void sym_table::store_function_decl(nyla::afunction* function) {
	m_declared_functions[function->name] = function;
}

nyla::afunction* sym_table::get_declared_function(nyla::name& name) {
	return m_declared_functions[name];
}

void sym_table::store_variable_decl(nyla::avariable* variable) {
	m_declared_variables.insert(variable->name);
}

bool sym_table::has_been_declared(nyla::avariable* variable) {
	return m_declared_variables.find(variable->name) != m_declared_variables.end();
}

