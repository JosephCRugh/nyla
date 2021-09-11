#ifndef NYLA_COMPILER_H
#define NYLA_COMPILER_H

#include <string>
#include <unordered_map>
#include <llvm/IR/Module.h>

#include "sym_table.h"
#include "log.h"
#include "file_location.h"

namespace nyla {

	extern llvm::LLVMContext* llvm_context;

	struct avariable_decl;

	enum compiler_flags {
		// Only perform parsing on the files
		COMPFLAG_ONLY_PARSE             = 0x0000,
		// Only perform parsing and analysis on
		// files
		COMPFLAG_ONLY_PARSE_AND_ANALYZE = 0x0001,
		// Only performs parsing, analysis and
		// object compilation
		COMPFLAG_ONLY_GEN_OBJECT        = 0x0002,
		// Parses, analyzes, and generates
		// an executable for the project
		COMPFLAGS_FULL_COMPILATION      = 0x0003,
		// Enables all print flags to print detailed
		// information about the entire compilation process
		COMPFLAGS_VERBOSE               = 0x0004,
		// Enables displaying an abstract syntax tree
		// of the program
		COMPFLAG_DISPLAY_AST            = 0x0008,
		// Enables displaying at what stage the compiler
		// is at for each file
		COMPFLAG_DISPLAY_STAGES         = 0x0010,
		// Enables displaying the source files that
		// are being loaded for compilation
		COMPFLAG_DISPLAY_SOURCE_PATHS   = 0x0020,
		// Enables displaying Generated LLVM IR
		COMPFLAG_DISPLAY_LLVM_IR        = 0x0040,
		// Enables displaying how long it takes to
		// process different parts of the file
		COMPFLAG_DISPLAY_TIMES          = 0x0080,
	};

	/*
	 * Main class for compilation.
	 * 
	 * Handles compilation based on flags
	 * provided by the command line.
	 */
	class compiler {
	public:

		compiler(u32 flags);
		
		void compile(const std::vector<std::string>& src_directories);

		// Finds a sym_table if it exist based on the internal path
		sym_table* find_sym_table(const std::string& path);

		void set_found_compilation_errors() { m_found_compilation_errors = true; }
		bool get_found_compilation_errors() const { return m_found_compilation_errors; }

		void set_main_function(sym_function* main_function);

		u32 get_new_unique_module_id();

		u32 get_num_global_const_array_count();
		u32 get_num_global_variable_count();
		u32 get_num_functions_count();

		void add_global_initialize_expr(nyla::avariable_decl* expr);

		void set_main_function_file(const std::string& main_function_path);
		void set_executable_name(const std::string& executable_name);

	private:

		// Searches for .nyla files in the directory. Decends into sub-directories
		void collect_source_files(const std::string& directory, 
			                      const std::string& directory_rel_src,
			                      std::vector<file_location>& source_files);

		void process_files(std::vector<file_location>& source_files);
		void process_file(const file_location& source_file, sym_table* our_sym_table);

		void analyze_file(sym_table* sym_table);

		// Primary module all the IR is dumped into
		llvm::Module* m_llvm_module;

		// If true the compiler will not generate object code.
		bool m_found_compilation_errors = false;

		// For logging global errors
		nyla::log m_log;

		//  internal_path -> sym_table
		std::unordered_map<std::string, sym_table*> m_sym_tables;

		// Entry point function for the program
		sym_function* m_main_function = nullptr;

		struct array_hash {
			nyla::type* base_type;
			u32         depth;
			struct hash_gen {
				ulen operator()(const array_hash& arr_hash) const {
					return arr_hash.base_type->tag + arr_hash.depth;
				}
			};

			bool operator==(const array_hash& o) const {
				return base_type == o.base_type && depth == o.depth;
			}
		};

		// The compilation flags that tell how
		// to compile
		u32 m_flags;

		// A unique id that identifiers a module accross the entire program not
		// just based on it's name
		u32 unique_module_id_count = 0;

		u64 m_total_parse_time_in_milliseconds  = 0;
		u64 m_total_ir_gen_time_in_milliseconds = 0;

		u32 m_num_global_const_array_count = 0;
		u32 m_num_global_variable_count    = 0;
		u32 m_num_functions_count          = 0;

		// some global variables must be initialized by function
		// so the expressions are stored for computation at a later
		// time.
		std::vector<nyla::avariable_decl*> m_global_initializer_exprs;

		// If set then it is expected that the main
		// function will be found in the file
		std::string m_main_function_file;
		bool        m_main_function_file_set = false;

		std::string m_executable_name = "program.exe";

	};

}

#endif