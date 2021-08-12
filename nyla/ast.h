#pragma once

#include "token.h"
#include "name.h"
#include <vector>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

namespace nyla {

	enum ast_tag {
		AST_FUNCTION,
		AST_IDENTIFIER,
		AST_VARIABLE,
		AST_VARIABLE_DECL,
		AST_SCOPE,
		AST_RETURN,
		
		AST_VALUE_BYTE,
		AST_VALUE_SHORT,
		AST_VALUE_INT,
		AST_VALUE_LONG,

		AST_VALUE_UBYTE,
		AST_VALUE_USHORT,
		AST_VALUE_UINT,
		AST_VALUE_ULONG,

		AST_VALUE_CHAR16,

		AST_VALUE_NULL,
		
		AST_VALUE_FLOAT,
		AST_VALUE_DOUBLE,
		AST_VALUE_BOOL,
		AST_VALUE_STRING,
		AST_BINARY_OP,
		AST_UNARY_OP,
		AST_FOR_LOOP,
		AST_TYPE_CAST,
		AST_FILE_UNIT,
		AST_FUNCTION_CALL,
		AST_ARRAY_ACCESS,
		AST_ARRAY,
		AST_ERROR
	};

	struct ast_node;
	struct type;

	struct ast_node {
		ast_tag tag;
		nyla::token* st = nullptr;
		nyla::token* et = nullptr;
		
		virtual ~ast_node() {}

		friend std::ostream& operator<<(std::ostream& os, const nyla::ast_node* node);

		virtual void print(std::ostream& os, u32 depth = 0) const = 0;

		std::string indent(u32 depth) const;

	};

	struct aexpr : public ast_node {
		virtual ~aexpr() {}
		bool is_constexpr = true;
		virtual void print(std::ostream& os, u32 depth = 0) const = 0;
		
		std::string expr_header(u32 depth) const;

		nyla::type* checked_type = nullptr;
	};

	struct err_expr : public aexpr {
		virtual ~err_expr() {}
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct atype_cast : public aexpr {
		virtual ~atype_cast() {}
		nyla::aexpr* value        = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct avariable; 
	struct aidentifier : public aexpr {
		virtual ~aidentifier() {}
		nyla::name       name;
		nyla::avariable* variable = nullptr; // Determined when type checking
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct avariable : public aexpr {
		virtual ~avariable() {}
		nyla::aidentifier* ident;
		llvm::AllocaInst*  ll_alloca;

			// Only applies if the type of the variable is an array
		std::vector<llvm::Value*> ll_arr_mem_offsets;
		std::vector<llvm::Value*> ll_arr_sizes;

		nyla::avariable* arr_alloc_reference = nullptr;

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct avariable_decl : public aexpr {
		virtual ~avariable_decl() {}
		nyla::avariable* variable;
		nyla::aexpr* assignment = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct areturn : public aexpr {
		virtual ~areturn() {}
		nyla::aexpr* value = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct ascope : public ast_node {
		virtual ~ascope() {}
		std::vector<nyla::aexpr*> stmts;
		nyla::ascope*             parent = nullptr;
		// Variables declared by name in the scope
		std::unordered_map<nyla::name,
			nyla::avariable*, nyla::name::hash_gen> variables;

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct afor_loop : public aexpr {
		virtual ~afor_loop() {}
		std::vector<nyla::avariable_decl*> declarations;
		nyla::aexpr*                       cond = nullptr;
		nyla::ascope*                      scope;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct anumber : public aexpr {
		virtual ~anumber() {}
		union {
			s32    value_int;
			s64    value_long;
			u32    value_uint;
			u64    value_ulong;
			float  value_float;
			double value_double;
		};
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct abool : public aexpr {
		virtual ~abool() {}
		bool tof;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct astring : public aexpr {
		virtual ~astring() {}
		std::string lit;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct anull : public aexpr {
		virtual ~anull() {}
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct aarray : public aexpr {
		virtual ~aarray() {}
		std::vector<u64>          depths;
		std::vector<nyla::aexpr*> elements;
		virtual void print(std::ostream& os, u32 depth) const override;

	};

	struct aunary_op : public aexpr {
		virtual ~aunary_op() {}
		u32 op;
		nyla::aexpr* factor;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct abinary_op : public aexpr {
		virtual ~abinary_op() {}
		u32 op;
		nyla::aexpr* lhs;
		nyla::aexpr* rhs;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct afunction : public ast_node {
		virtual ~afunction() {}
		nyla::type*                        return_type;
		nyla::aidentifier*                 ident;
		std::vector<nyla::avariable_decl*> parameters;
		nyla::ascope*                      scope = nullptr;
		llvm::Function*                    ll_function; // Needed for function calls

		bool is_external = false; // Indicates if it is an external to a C
		                          // style library such as in a DLL.
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct aarray_access : public aexpr {
		virtual ~aarray_access() {}
		nyla::aarray_access* next = nullptr;
		nyla::aidentifier*   ident;
		nyla::aexpr*         index;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct afunction_call : public aexpr {
		virtual ~afunction_call() {}
		nyla::aidentifier*        ident;
		std::vector<nyla::aexpr*> parameter_values;
		nyla::afunction*          called_function = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct afile_unit : public ast_node {
		virtual ~afile_unit() {}
		std::vector<nyla::afunction*> functions;
		virtual void print(std::ostream& os, u32 depth) const override;
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