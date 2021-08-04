#pragma once

#include "token.h"
#include "type.h"
#include "name.h"
#include <vector>
#include <llvm/IR/Function.h>

namespace nyla {

	enum ast_tag {
		AST_FUNCTION,
		AST_VARIABLE,
		AST_VARIABLE_DECL,
		AST_SCOPE,
		AST_RETURN,
		AST_VALUE_INT,
		AST_VALUE_FLOAT,
		AST_VALUE_DOUBLE,
		AST_VALUE_BOOL,
		AST_BINARY_OP,
		AST_FOR_LOOP,
		AST_TYPE_CAST,
		AST_FILE_UNIT,
		AST_FUNCTION_CALL,
		AST_ERROR
	};

	struct ast_node {
		ast_tag tag;
		nyla::token* st = nullptr;
		nyla::token* et = nullptr;
		
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
		nyla::type* checked_type;
	};

	struct err_expr : public aexpr {
		virtual ~err_expr() {}
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "Error Expr";
		}
	};

	struct atype_cast : public aexpr {
		virtual ~atype_cast() {}
		nyla::aexpr* value        = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "cast(" << *checked_type << ")" << std::endl;
			value->print(os, depth+1);
		}
	};

	struct avariable : public aexpr {
		virtual ~avariable() {}
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

	struct afor_loop : public aexpr {
		virtual ~afor_loop() {}
		std::vector<nyla::avariable_decl*> declarations;
		nyla::aexpr*                       cond = nullptr;
		nyla::ascope*                      scope;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "for_loop" << std::endl;
			os << indent(depth) << "loop__declarations:" << std::endl;
			for (nyla::avariable_decl* decl : declarations) {
				decl->print(os, depth + 1);
				os << std::endl;
			}
			os << indent(depth) << "loop__condition:" << std::endl;
			if (cond) {
				cond->print(os, depth + 1);
				os << std::endl;
			}
			os << indent(depth) << "loop__body:" << std::endl;
			scope->print(os, depth + 1);
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
			switch (tag) {
			case AST_VALUE_INT:
				os << indent(depth) << "int: " << value_int; break;
			case AST_VALUE_FLOAT:
				os << indent(depth) << "float: " << value_float; break;
			case AST_VALUE_DOUBLE:
				os << indent(depth) << "double: " << value_double; break;
			}
			
		}
	};

	struct abool : aexpr {
		virtual ~abool() {}
		bool tof;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth);
			if (tof) os << "true"; else os << "false";
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
		llvm::Function*                    ll_function; // Needed for function calls
		virtual void print(std::ostream& os, u32 depth) const override {
			os << std::string(depth * 4, ' ');
			os << "function: " << name << std::endl;
			scope->print(os, depth + 1);
		}
	};

	struct afunction_call : public aexpr {
		virtual ~afunction_call() {}
		nyla::name                name;
		std::vector<nyla::aexpr*> parameter_values;
		nyla::afunction*          called_function = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override {
			os << indent(depth) << "function call: " << name << std::endl;
			for (nyla::aexpr* parameter_value : parameter_values) {
				parameter_value->print(os, depth + 1);
				os << std::endl;
			}
		}
	};

	struct afile_unit : public ast_node {
		virtual ~afile_unit() {}
		std::vector<nyla::afunction*> functions;
		virtual void print(std::ostream& os, u32 depth) const override {
			for (nyla::afunction* function : functions) {
				function->print(os, depth);
				os << std::endl;
			}
		}
	};

	template<typename node>
	node* make_node(ast_tag tag, nyla::token* st, nyla::token* et) {
		node* n = new node;
		n->tag = tag;
		n->st = st;
		n->et = et;
		return n;
	}
}