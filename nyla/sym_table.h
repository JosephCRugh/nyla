#ifndef NYLA_SYM_TABLE_H
#define NYLA_SYM_TABLE_H

#include <unordered_map>

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>

#include "types_ext.h"
#include "type.h"
#include "modifiers.h"
#include "file_location.h"

namespace nyla {

	struct sym_function;
	struct sym_variable;
	struct sym_scope;
	struct parser;
	struct analysis;
	struct llvm_generator;
	struct avariable_decl;
	struct aannotation;

	struct sym_module {
		u32         name_key;
		u32         mods;
		std::string internal_path; // Internal path of the source file.
		                           // Ex. "project/A.nyla"
		std::unordered_map<u32, std::vector<sym_function*>> functions;
		std::unordered_map<u32, std::vector<sym_function*>> constructors;
		std::vector<nyla::avariable_decl*>                  fields;


		llvm::StructType* ll_struct_type = nullptr;
		// TODO change name to "unique_module_key"
		u32 unique_module_id;

		sym_scope* scope = nullptr;

		bool no_constructors_found = true;
	};

	struct sym_scope {
		sym_scope* parent = nullptr;
		std::unordered_map<u32, sym_variable*> variables;
		bool                                   is_module_scope;
		bool                                   found_return = false;
	};

	struct sym_function {
		u32                      mods = 0;
		nyla::type*              return_type;
		u32                      name_key;
		std::vector<nyla::type*> param_types;
		llvm::Function*          ll_function = nullptr;
		sym_module*              sym_module  = nullptr;
		u32                      line_num; // Line number in source code where it was declared
		aannotation*             annotation = nullptr;
		bool                     call_at_startup = false; // True if the function has @StartUp annotation

		bool is_member_function() {
			return !(mods & MOD_STATIC) &&
				   !(mods & MOD_EXTERNAL);
		}
	};

	struct sym_variable {
		u32               name_key;
		u32               mods;
		u32               field_index; // If the variable is part of a module
									   // it has a module field index
		bool is_field = false; // Field of module
		bool is_global = false; // variable marked with static
		u32 position_declared_at; // Position in character buffer that the variable
		                          // was declared at. Used to make sure assignments occure
		                          // after declaration and not before.
		
		sym_module* sym_module = nullptr;

		nyla::type* type;
		
		std::vector<nyla::aexpr*> arr_dim_sizes;
		std::vector<u32>          computed_arr_dim_sizes;

		llvm::Value* ll_alloc;
	};

	class sym_table {
	public:

		// Flags indicating the state while processing
		// a file
		bool m_started_processing         = false;
		bool m_started_import_resolution  = false;
		bool m_started_analysis           = false;
		bool m_started_ir_declaration_gen = false;

		bool m_found_compilation_errors  = false;

		// If set to false the main function will be ignored
		bool m_search_for_main_function;
		
	public:

		// Creates a new module entry in the symbol table
		sym_module* enter_module(u32 name_key);

		// Finds a module within this symbol table or returns
		// nullptr
		sym_module* find_module(u32 name_key);

		// Checks if a function has already been created by the given name and
		// parameter types. Returns -1 if it has not been declared otherwise
		// it returns the line number it was first declared on
		s32 has_function_been_declared(sym_module* sym_module, u32 name_key,
			                            std::vector<nyla::type*> param_types);

		// Same logic as has_function_been_declared except for constructors
		s32 has_constructor_been_declared(sym_module* sym_module, u32 name_key,
			                              std::vector<nyla::type*> param_types);

		// Creates a new function entry in the symbol table based on the
		// function name.
		sym_function* enter_function(sym_module* sym_module, u32 name_key);

		// Creates a new constructor entry in the symbol table based on the
		// constructor name.
		sym_function* enter_constructor(sym_module* sym_module, u32 name_key);

		// Has a variable been declared already within the current
		// scope.
		bool has_variable_been_declared(u32 name_key, bool check_module_scope);

		// Creates a new variable entry in the current scope
		sym_variable* enter_variable(u32 name_key);

		// Finds a variable within the scope or parent scopes by it's name key
		sym_variable* find_variable(sym_scope* scope, u32 name_key);

		// Gets all the modules declared in the file
		std::vector<sym_module*> get_modules();

		// Get a functions in the module based on it's name.
		std::vector<sym_function*> get_functions(sym_module* nmodule, u32 name_key);

		// Get a constructors in the module based on the module name
		std::vector<sym_function*> get_constructors(sym_module* nmodule);

		sym_scope* push_scope();
		void pop_scope();

		void set_file_location(file_location file_location) { m_file_location = file_location; }
		file_location get_file_location() { return m_file_location; }

		void set_parser(nyla::parser* parser) { m_parser = parser; }
		nyla::parser* get_parser() { return m_parser; };

		void set_analysis(nyla::analysis* analysis) { m_analysis = analysis; }
		nyla::analysis* get_analysis() { return m_analysis; }

		void set_llvm_generator(nyla::llvm_generator* llvm_generator) { m_llvm_generator = llvm_generator; }
		nyla::llvm_generator* get_llvm_generator() { return m_llvm_generator; }

	private:
		// Maps between a module's name_key and the symbol
		// for the module
		std::unordered_map<u32, sym_module*> m_modules;

		s32 function_search(const std::vector<sym_function*>& functions, u32 name_key,
			                std::vector<nyla::type*> param_types);

		   // Current scope
		sym_scope* m_scope = nullptr;

		file_location m_file_location;

		nyla::parser*         m_parser;
		nyla::analysis*       m_analysis;
		nyla::llvm_generator* m_llvm_generator;

	};
}

#endif