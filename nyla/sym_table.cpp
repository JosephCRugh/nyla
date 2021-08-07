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

void sym_table::store_function_decl(nyla::afunction* function) {
	m_declared_functions[function->name] = function;
}

nyla::afunction* sym_table::get_declared_function(nyla::name& name) {
	return m_declared_functions[name];
}

void sym_table::store_declared_variable(nyla::avariable* variable) {
	nyla::ascope* cur_scope = m_scopes.top();
	cur_scope->variables[variable->name] = variable;
}

nyla::avariable* sym_table::get_declared_variable(nyla::ascope* scope, nyla::avariable* variable) {
	nyla::ascope* scope_itr = scope;
	while (scope_itr != nullptr) {
		auto it = scope_itr->variables.find(variable->name);
		if (it != scope_itr->variables.end()) {
			return it->second;
		}
		scope_itr = scope_itr->parent;
	}
	return nullptr;
}

bool sym_table::has_been_declared(nyla::avariable* variable) {
	// Search through the scopes to see if it has been declared.
	assert(!m_scopes.empty() && "Cannot check declaration without a scope");
	nyla::ascope* scope = m_scopes.top();
	while (scope != nullptr) {
		if (scope->variables.find(variable->name) != scope->variables.end()) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}

void nyla::sym_table::import_dll(std::string& dll_path) {
	m_dll_import_paths.push_back(dll_path);
}

