#pragma once

#include <unordered_map>
#include <stack>
#include <vector>
#include "ast.h"
#include "token.h"
#include <llvm/IR/Instructions.h>

namespace nyla {
	class sym_table {
	public:

		nyla::ascope* push_scope(nyla::token* start_scope_token);

		void pop_scope();

		void store_function_decl(nyla::afunction* function);

		nyla::afunction* get_declared_function(nyla::name& name);

		void store_declared_variable(nyla::avariable* variable);

		nyla::avariable* get_declared_variable(nyla::ascope* scope, nyla::avariable* variable);

		bool has_been_declared(nyla::avariable* variable);

		void import_dll(std::string& dll_path);

		std::vector<std::string> get_dll_import_paths() const { return m_dll_import_paths; }

	private:

		std::unordered_map<nyla::name, nyla::afunction*,
			nyla::name::hash_gen> m_declared_functions;

		std::stack<nyla::ascope*> m_scopes;

		std::vector<std::string> m_dll_import_paths;
	};
}