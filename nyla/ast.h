#ifndef NYLA_AST_H
#define NYLA_AST_H

#include <vector>
#include <iostream>
#include <string>

#include "sym_table.h"
#include "types_ext.h"
#include "type.h"

namespace nyla {

	enum ast_tag {
		// High level
		AST_FILE_UNIT,
		AST_MODULE,
		AST_FUNCTION,
		AST_SCOPE,
		AST_IMPORT,

		// Statements

		AST_VARIABLE_DECL,
		AST_RETURN,
		AST_FOR_LOOP,
		AST_WHILE_LOOP,
		AST_IF,
		AST_BREAK,
		AST_CONTINUE,
		AST_THIS,
		AST_ANNOTATION,

		// Expressions
		
		AST_BINARY_OP,
		AST_DOT_OP,
		AST_UNARY_OP,
		AST_TYPE_CAST,
		AST_IDENT,
		AST_FUNCTION_CALL,
		AST_ARRAY,
		AST_STRING8,
		AST_STRING16,
		AST_STRING32,
		AST_ARRAY_ACCESS,
		AST_POST_INC,

		// Factors
		  // Integers
		AST_VALUE_BYTE,
		AST_VALUE_SHORT,
		AST_VALUE_INT,
		AST_VALUE_LONG,
		AST_VALUE_UBYTE,
		AST_VALUE_USHORT,
		AST_VALUE_UINT,
		AST_VALUE_ULONG,
		  // Characters
		AST_VALUE_CHAR8,
		AST_VALUE_CHAR16,
		AST_VALUE_CHAR32,
		  // Floats
		AST_VALUE_FLOAT,
		AST_VALUE_DOUBLE,
		  // Other
		AST_VALUE_BOOL,
		AST_VALUE_NULL,
		AST_NEW_OBJECT,
		AST_VAR_OBJECT,

		AST_ERROR
	};

	/* Node that all other
	 * nodes extend from.
	 */
	struct ast_node {
		virtual ~ast_node();

		ast_tag tag;
		u32     line_num;
		u32     spos, epos;

		bool comptime_compat = true;  // Tells whether or not the node is able to be
		                              // computed at compilation time or not. This does
		                              // not mean it is computed at compilation time simply
		                              // that it can be.
		bool literal_constant = true; // Tells wether or not the node is able to be computed
		                              // as a literal constant such as numbers, bools, other
		                              // nodes with comptime modifier. Essentially anything that
		                              // can be folded by llvm and directly assigned to memory.

		virtual void print(std::ostream& os, u32 depth = 0) const = 0;

		// Gets the string for the word_key in the word_table
		std::string word_to_string(u32 word_key) const;

		// Converts modifiers to a string
		std::string mods_as_string(u32 mods) const;

		// Create an indented string of spaces.
		// spaces = 4 * depth
		std::string indent(u32 depth) const;

	};

	struct amodule;
	struct avariable_decl;
	struct afunction;
	struct afunction_call;
	struct aexpr;
	struct aarray;
	struct aimport;
	struct aident;
	struct aannotation;

	/*
	 * Node that represents the contents
	 * of a file.
	 */
	struct afile_unit : public ast_node {
		virtual ~afile_unit() override;

		std::vector<nyla::amodule*> modules;

		std::unordered_map<std::string, aimport*> imports;
		std::unordered_map<u32, sym_module*>      loaded_modules;
		// alias name -> original name
		std::unordered_map<u32, u32>              module_aliases;

		// Find a module by either the module's name or its alias
		sym_module* find_module(u32 name_key);

		friend std::ostream& operator<<(std::ostream& os, const nyla::ast_node& node);

		virtual void print(std::ostream& os, u32 depth = 0) const override;

	};

	struct aimport : public ast_node {
		virtual ~aimport() override {}

		std::string                  path;
		// original name -> alias name
		std::unordered_map<u32, u32> module_aliases;
		

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct amodule : public ast_node {
		virtual ~amodule() override;
		u32 name_key; // Identifier name key
		              // into the symbol table.
		std::vector<nyla::afunction*>      constructors;

		std::vector<nyla::avariable_decl*> globals; // fields marked as static
		std::vector<nyla::avariable_decl*> fields;
		std::vector<nyla::afunction*>      functions;
		// The module symbol stored in the sym_table
		nyla::sym_module* sym_module = nullptr;
		nyla::sym_scope*  sym_scope  = nullptr;
		bool found_constructor = false;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct afunction : public ast_node {
		virtual ~afunction() override;

		nyla::type*                        return_type;
		u32                                name_key;
		std::vector<nyla::avariable_decl*> parameters;
		std::vector<nyla::aexpr*>          stmts;
		bool                               is_constructor = false;
		nyla::sym_scope*                   sym_scope  = nullptr;
		
		// Constructors use this to reference field declarations
		nyla::amodule* nmodule = nullptr;
		
		// The function symbol stored in the sym_table
		nyla::sym_function* sym_function = nullptr;
		bool is_main_function = false; // Tells if it is the entry point function to the program.

		bool is_external() const;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	/*----------------------------*\
	 * Statements and expressions *
	\*----------------------------*/

	// Statements extend from aexpr because it makes parsing
	// much easier even though they are technically just statements


	struct aexpr : public ast_node {
		virtual ~aexpr() override {}

		nyla::type* type = nullptr; // Type checked during analysis
		bool        global_initializer_expr = false;
		bool        reuses_lhs = false;

		std::string expr_header(u32 depth) const;
	};

	struct avariable_decl : public aexpr {
		virtual ~avariable_decl() override;

		u32           name_key;
		nyla::aexpr*  assignment   = nullptr;
		sym_variable* sym_variable = nullptr; // Variable in the symbol table
		nyla::aident* ident;
		bool          default_initialize = true;
		
		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct aident : public aexpr {
		virtual ~aident() override {}

		u32 ident_key;
		// Behaves as .length operator
		bool is_array_length = false;
		// Behaves as a static reference to a module name
		bool references_module = false;

		sym_variable* sym_variable = nullptr;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	// Creates a new object on the stack
	struct avar_object : public aexpr {
		virtual ~avar_object() override {}

		nyla::afunction_call* constructor_call;
		nyla::sym_module*     sym_module;
		bool assumed_default_constructor = false;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct areturn : public aexpr {
		virtual ~areturn() override;

		nyla::aexpr* value = nullptr;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct aarray : public aexpr {
		virtual ~aarray() override;

		  // How many elements. Might differ from elements.size()
		  // in the case of default initialization Ex. int[n] a;
		  //                                               ^
		u32                       dim_size;
		std::vector<nyla::aexpr*> elements;

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct aloop_expr : public aexpr {
		virtual ~aloop_expr() override;

		nyla::aexpr* cond = nullptr;
		nyla::sym_scope* sym_scope = nullptr;
		std::vector<nyla::aexpr*> body;
		// Expressions that occure every time the loop
		// is processed
		std::vector<nyla::aexpr*> post_exprs;

		virtual void print(std::ostream& os, u32 depth = 0) const = 0;
	};

	struct afor_loop : public aloop_expr {
		virtual ~afor_loop() override;

		std::vector<nyla::avariable_decl*> declarations;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct awhile_loop : public aloop_expr {
		virtual ~awhile_loop() override {}

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct acontrol : public aexpr {
		virtual ~acontrol() override {}

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct aif : public aexpr {
		virtual ~aif() override;

		nyla::aexpr*              cond;
		nyla::sym_scope*          sym_scope = nullptr;
		std::vector<nyla::aexpr*> body;
		nyla::aif*                else_if = nullptr;
		nyla::sym_scope*          else_sym_scope = nullptr;
		std::vector<nyla::aexpr*> else_body;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct aunary_op : public aexpr {
		virtual ~aunary_op() override;

		u32 op;
		nyla::aexpr* factor;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct abinary_op : public aexpr {
		virtual ~abinary_op() override;

		u32 op;
		nyla::aexpr* lhs = nullptr;
		nyla::aexpr* rhs = nullptr;

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	struct adot_op : public aexpr {
		virtual ~adot_op() override;

		// Must contain at least 2 factors
		std::vector<nyla::aexpr*> factor_list;

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct atype_cast : public aexpr {
		virtual ~atype_cast() override;

		nyla::aexpr* value = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct afunction_call : public aexpr {
		virtual ~afunction_call() override;

		u32                       name_key;
		std::vector<nyla::aexpr*> arguments;
		sym_function*             called_function = nullptr;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct anumber : public aexpr {
		virtual ~anumber() override {}

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
		virtual ~abool() override {}

		bool tof;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	// In cases where expressions cannot be parsed
	// correctly err_expr is generated instead
	struct err_expr : public aexpr {
		virtual ~err_expr() override {}

		virtual void print(std::ostream& os, u32 depth = 0) const override;
	};

	// TODO: Array accesses need the ability to access
	// arrays from function calls not just variables
	struct aarray_access : public aexpr {
		virtual ~aarray_access() override;

		nyla::aident*             ident;
		std::vector<nyla::aexpr*> indexes;
		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct astring : public aexpr {
		virtual ~astring() override {}

		// Not sure I should be trusting C++ with string efficiency

		// TODO: change names to str8,str16,str32 current names are confusing
		std::string    lit8;
		std::u16string lit16;
		std::u32string lit32;
		u32            dim_size; // Since strings are just arrays and the size
		                         // could be modified by default initialization

		virtual void print(std::ostream& os, u32 depth) const override;
	};

	struct aannotation : public ast_node {
		virtual ~aannotation() override {};

		u32 ident_key;

		virtual void print(std::ostream& os, u32 depth) const override;
	};
}

#endif
