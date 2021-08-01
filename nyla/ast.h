#pragma once

#include "type.h"
#include "name.h"
#include <vector>

namespace nyla {

	enum ast_tag {
		AST_FUNCTION,
		AST_VARIABLE,
		AST_VARIABLE_DECL,
		AST_SCOPE,
		AST_RETURN,
		AST_VALUE_INT,
		AST_BINARY_OP
	};

	struct ast_node {
		ast_tag tag;
		
		virtual ~ast_node() {}
	};
	
	struct aexpr : public ast_node {
		virtual ~aexpr() {}
	};

	struct avariable : public aexpr {
		virtual ~avariable() {}
		nyla::type* type;
		nyla::name  name;
	};

	struct avariable_decl : public aexpr {
		virtual ~avariable_decl() {}
		nyla::avariable* variable;
		nyla::aexpr*     assignment = nullptr;
	};

	struct ascope : public ast_node {
		virtual ~ascope() {}
		std::vector<nyla::aexpr*> expressions;
		nyla::ascope*             parent;
	};

	struct areturn : public aexpr {
		virtual ~areturn() {}
		nyla::aexpr* value = nullptr;
	};

	struct anumber : public aexpr {
		virtual ~anumber() {}
		union {
			s32    value_int;
			float  value_float;
			double value_double;
		};
	};

	struct abinary_op : public aexpr {
		virtual ~abinary_op() {}
		u32 op;
		nyla::aexpr* lhs;
		nyla::aexpr* rhs;
	};

	struct afunction : public ast_node {
		virtual ~afunction() {}
		nyla::type*                        return_type;
		nyla::name                         name;
		std::vector<nyla::avariable_decl*> parameters;
		nyla::ascope*                      scope;
	};

	template<typename node>
	node* make_node(ast_tag tag) {
		node* n = new node;
		n->tag = tag;
		return n;
	}
}