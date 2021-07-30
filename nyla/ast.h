#pragma once

#include "type.h"
#include "name.h"
#include <vector>

namespace nyla {

	enum ast_tag {
		AST_FUNCTION,
		AST_VARIABLE,
		AST_VARIABLE_DECL,
	};

	struct ast_node {
		ast_tag tag;
		
		virtual ~ast_node() {}
	};
	
	struct aexpr : public ast_node {

	};

	struct avariable : public aexpr {
		nyla::type* type;
		nyla::name  name;
	};

	struct avariable_decl : public aexpr {
		nyla::avariable* variable;
		nyla::aexpr*     assignment = nullptr;
	};

	struct afunction : public ast_node {
		virtual ~afunction() {}
		nyla::type*                         return_type;
		nyla::name                          name;
		std::vector<nyla::avariable_decl*> parameters;
	};

	template<typename node>
	node* make_node(ast_tag tag) {
		node* n = new node;
		n->tag = tag;
		return n;
	}
}