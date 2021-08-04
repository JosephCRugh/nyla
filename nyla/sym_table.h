#pragma once

#include <unordered_map>
#include <stack>
#include "ast.h"
#include "token.h"
#include <llvm/IR/Instructions.h>

namespace nyla {
	class sym_table {
	public:

		nyla::ascope* push_scope(nyla::token* start_scope_token);

		void pop_scope();

		void store_alloca(nyla::avariable* variable, llvm::AllocaInst* var_alloca);

		llvm::AllocaInst* get_alloca(nyla::avariable* variable);

		void store_variable_type(nyla::avariable* variable);

		nyla::type* get_variable_type(nyla::avariable* variable);

		void store_function_decl(nyla::afunction* function);

		nyla::afunction* get_declared_function(nyla::name& name);

		void store_variable_decl(nyla::avariable* variable);

		bool has_been_declared(nyla::avariable* variable);

	private:
		// TODO: replace with symantic analysis scope mapping.
		//   The way is should work in the end is that given a variable
		//   instead of it's name the correct AllocaInst can be found
		std::unordered_map<nyla::name, llvm::AllocaInst*,
			               nyla::name::hash_gen> m_alloced_variables;
		// Same logic applies as it does above.
		std::unordered_map<nyla::name, nyla::type*,
			               nyla::name::hash_gen> m_variable_types;
		// Same logic applies as it does above (and will probably be grouped together).
		std::unordered_set<nyla::name, nyla::name::hash_gen> m_declared_variables;

		std::unordered_map<nyla::name, nyla::afunction*,
			nyla::name::hash_gen> m_declared_functions;

		std::stack<nyla::ascope*> m_scopes;
	};
}