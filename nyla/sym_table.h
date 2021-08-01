#pragma once

#include <unordered_map>
#include <stack>
#include "ast.h"
#include <llvm/IR/Instructions.h>

namespace nyla {
	class sym_table {
	public:

		nyla::ascope* push_scope();

		void pop_scope();

		void store_alloca(nyla::avariable* variable, llvm::AllocaInst* var_alloca);

		llvm::AllocaInst* get_alloca(nyla::avariable* variable);

	private:
		// TODO: replace with symantic analysis scope mapping.
		//   The way is should work in the end is that given a variable
		//   instead of it's name the correct AllocaInst can be found
		std::unordered_map<nyla::name, llvm::AllocaInst*,
			               nyla::name::hash_gen> m_alloced_variables;

		std::stack<nyla::ascope*> m_scopes;
	};
}