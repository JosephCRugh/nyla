#pragma once

#include "token.h"
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
		AST_BINARY_OP,
		AST_FOR_LOOP
	};

	struct ast_node {
		ast_tag tag;
		
		virtual ~ast_node() {}

		friend std::ostream& operator<<(std::ostream& os, const nyla::ast_node* node) {
			node->print(os);
			return os;
		}

		virtual void print(std::ostream& os, u32 depth = 0) const = 0;

		std::string indent(u32 depth) const { return std::string(depth * 4, ' '); }
	};

	struct aexpr : public ast_node {
		virtual ~aexpr() {}
		virtual void print(std::ostream& os, u32 depth = 0) const = 0;
	};

	struct avariable : public aexpr {
		virtual ~avariable() {}
		nyla::type* type;
		nyla::name  name;

		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "\"" << name << "\"";
		}
	};

	struct avariable_decl : public aexpr {
		virtual ~avariable_decl() {}
		nyla::avariable* variable;
		nyla::aexpr* assignment = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "var_delc: " << variable << std::endl;
			if (assignment) {
				assignment->print(os, depth + 1);
			} 
		}
	};

	struct areturn : public aexpr {
		virtual ~areturn() {}
		nyla::aexpr* value = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "return" << std::endl;
			if (value) {
				value->print(os, depth+1);
			}
		}
	};

	struct afor_loop : public aexpr {
		virtual ~afor_loop() {}
		std::vector<nyla::avariable_decl*> declarations;
		nyla::aexpr*                       cond = nullptr;
		nyla::aexpr*                       post = nullptr;
		std::vector<nyla::aexpr*>          body;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "for_loop" << std::endl;
			os << indent(depth) << "loop__declarations:" << std::endl;
			for (nyla::avariable_decl* decl : declarations) {
				decl->print(os, depth+1);
				os << std::endl;
			}
			os << indent(depth) << "loop__condition:" << std::endl;
			if (cond) {
				cond->print(os, depth + 1);
				os << std::endl;
			}
			os << indent(depth) <<  "loop__body:" << std::endl;
			for (nyla::aexpr* stmt : body) {
				stmt->print(os, depth + 1);
				os << std::endl;
			}
			os << indent(depth) << "loop__post_body:" << std::endl;
			if (post) {
				post->print(os, depth + 1);
			}
		}
	};

	struct ascope : public ast_node {
		virtual ~ascope() {}
		std::vector<nyla::aexpr*> stmts;
		nyla::ascope*             parent;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "scope" << std::endl;
			for (nyla::aexpr* stmt : stmts) {
				stmt->print(os, depth + 1);
				os << std::endl;
			}
		}
	};

	struct anumber : public aexpr {
		virtual ~anumber() {}
		union {
			s32    value_int;
			float  value_float;
			double value_double;
		};
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "number (" << value_int << ")";
		}
	};

	struct abinary_op : public aexpr {
		virtual ~abinary_op() {}
		u32 op;
		nyla::aexpr* lhs;
		nyla::aexpr* rhs;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "bin_op: " << nyla::token_tag_to_string(op) << std::endl;
			lhs->print(os, depth + 1);
			os << std::endl;
			rhs->print(os, depth + 1);
		}
	};

	struct afunction : public ast_node {
		virtual ~afunction() {}
		nyla::type*                        return_type;
		nyla::name                         name;
		std::vector<nyla::avariable_decl*> parameters;
		nyla::ascope*                      scope;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << std::string(depth * 4, ' ');
			os << "function: " << name << std::endl;
			scope->print(os, depth + 1);
		}
	};

	template<typename node>
	node* make_node(ast_tag tag) {
		node* n = new node;
		n->tag = tag;
		return n;
	}
}