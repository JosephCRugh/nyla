#include "sym_table.h"

nyla::sym_module* nyla::sym_table::enter_module(u32 name_key) {
	m_modules[name_key] = new sym_module;
	return m_modules[name_key];
}

nyla::sym_module* nyla::sym_table::find_module(u32 name_key) {
	auto it = m_modules.find(name_key);
	if (it != m_modules.end()) {
		return it->second;
	}
	return nullptr;
}

s32 nyla::sym_table::has_function_been_declared(sym_module* sym_module, u32 name_key,
	                                             std::vector<nyla::type*> param_types) {
	std::vector<sym_function*>& functions = sym_module->functions[name_key];
	for (sym_function* function : functions) {
		if (function->name_key != name_key) continue;
		if (function->param_types.size() != param_types.size()) continue;
		bool match = true;
		for (u32 i = 0; i < param_types.size(); i++) {
			if (!function->param_types[i]->equals(param_types[i])) {
				match = false;
				break;
			}
		}
		if (!match) continue;
		return function->line_num;
	}
	return -1;
}

nyla::sym_function* nyla::sym_table::enter_function(sym_module* sym_module, u32 name_key) {
	std::vector<sym_function*>& functions = sym_module->functions[name_key];
	functions.push_back(new sym_function);
	return functions.back();
}

bool nyla::sym_table::has_variable_been_declared(u32 name_key, bool check_module_scope) {
	sym_scope* scope = m_scope;
	while (scope != nullptr) {
		if (!check_module_scope && scope->is_module_scope) return false;
		if (scope->variables.find(name_key) != scope->variables.end()) {
			return true;
		}
		scope = scope->parent;
	}
	return false;
}

nyla::sym_variable* nyla::sym_table::enter_variable(u32 name_key) {
	assert(m_scope && "Cannot enter a variable into an empty scope!");
	m_scope->variables[name_key] = new sym_variable;
	return m_scope->variables[name_key];
}

nyla::sym_variable* nyla::sym_table::find_variable(sym_scope* scope, u32 name_key) {
	sym_scope* scope_itr = scope;
	while (scope_itr != nullptr) {
		auto it = scope_itr->variables.find(name_key);
		if (it != scope_itr->variables.end()) {
			return it->second;
		}
		scope_itr = scope_itr->parent;
	}
	return nullptr;
}

std::vector<nyla::sym_module*> nyla::sym_table::get_modules() {
	std::vector<nyla::sym_module*> modules;
	for (auto& pair : m_modules) {
		modules.push_back(pair.second);
	}
	return modules;
}

nyla::sym_scope* nyla::sym_table::push_scope() {
	nyla::sym_scope* scope = new nyla::sym_scope;
	if (m_scope) {
		scope->parent = m_scope;
	}
	m_scope = scope;
	return m_scope;
}

void nyla::sym_table::pop_scope() {
	m_scope = m_scope->parent;
}

std::vector<nyla::sym_function*> nyla::sym_table::get_functions(sym_module* nmodule, u32 name_key) {
	auto it = nmodule->functions.find(name_key);
	if (it == nmodule->functions.end()) {
		return {};
	}
	return it->second;
}

std::vector<nyla::sym_function*> nyla::sym_table::get_constructors(sym_module* nmodule) {
	auto it = nmodule->constructors.find(nmodule->name_key);
	if (it == nmodule->constructors.end()) {
		return {};
	}
	return it->second;
}
