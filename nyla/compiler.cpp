#include "compiler.h"

#include "utils.h"
#include "source.h"
#include "lexer.h"
#include "parser.h"
#include "analysis.h"
#include "llvm_gen.h"
#include "code_gen.h"

#include <llvm/IR/Verifier.h>

#include <iomanip>

llvm::LLVMContext* nyla::llvm_context = nullptr;
llvm::TargetMachine* nyla::g_llvm_target_machine = nullptr;

nyla::compiler::compiler() {
	nyla::llvm_context = new llvm::LLVMContext;
	m_llvm_module      = new llvm::Module("Nyla Module", *nyla::llvm_context);
	nyla::setup_tokens();
}

void nyla::compiler::set_flags(u32 flags) {
	m_flags = flags;
	if (m_flags & COMPFLAGS_VERBOSE) {
		m_flags |= COMPFLAGS_VERBOSE;
		m_flags |= COMPFLAG_DISPLAY_AST;
		m_flags |= COMPFLAG_DISPLAY_STAGES;
		m_flags |= COMPFLAG_DISPLAY_SOURCE_PATHS;
		m_flags |= COMPFLAG_DISPLAY_LLVM_IR;
	}
}

void nyla::compiler::compile(const std::vector<std::string>& src_directories, const std::string& main_function_path) {
	assert(!main_function_path.empty());
	
	if (main_function_path.empty()) {
		m_found_compilation_errors = true;
		m_log.global_error(ERR_FILE_WITH_MAIN_FUNCTION_EMPTY);
		return;
	}

	m_main_function_file = main_function_path;

	init_llvm_native_target();
	if (!nyla::g_llvm_target_machine) {
		nyla::g_llvm_target_machine = nyla::create_llvm_target_machine();
	}

	if (!nyla::g_llvm_target_machine) {
		m_found_compilation_errors = true;
		return;
	}

	std::vector<std::string> resolved_src_directories;
	for (const std::string& src_directory : src_directories) {
		resolved_src_directories.push_back(
			nyla::replace(src_directory, "\\", "/"));
	}

	std::vector<file_location> source_files;
	for (const std::string& src_directory : resolved_src_directories) {
		
		std::vector<std::string> paths = nyla::split(src_directory, '/');

		collect_source_files(src_directory, "", source_files);
		
		for (const file_location& file_location : source_files) {
			if (m_flags & COMPFLAG_DISPLAY_SOURCE_PATHS) {
				std::cout << "-- Full path relative to compiler: " << file_location.system_path << '\n';
				std::cout << "-- Internal path: " << file_location.internal_path << '\n';
			}
		}
	}

	process_files(source_files);
	
	if (!m_main_function) {
		m_log.global_error(ERR_MAIN_FUNCTION_NOT_FOUND);
		return;
	}

	if (m_found_compilation_errors) {
		return;
	}

	nyla::llvm_generator generator(*this, m_llvm_module, nullptr, false);
	generator.gen_global_initializers(m_main_function, m_global_initializer_exprs);
	generator.gen_startup_function_calls(m_main_function, m_startup_functions);

	if (llvm::verifyModule(*m_llvm_module, &llvm::errs())) {
		return;
	}

	if (m_found_compilation_errors) {
		return;
	}

	if (m_flags & COMPFLAG_DISPLAY_LLVM_IR) {
		m_main_function->ll_function->print(llvm::outs());
		std::cout << '\n';
	}
	
	if (m_flags & COMPFLAG_DISPLAY_STAGES) {
		std::cout << "-- Finalizing compilation. Writing program.o\n";
	}

	std::string obj_name = m_executable_name + ".o";
	
	u64 compile_st = nyla::get_time_in_milliseconds();
	nyla::write_obj_file(obj_name.c_str(), m_llvm_module, nyla::g_llvm_target_machine);
	u64 compile_time = nyla::get_time_in_milliseconds() - compile_st;

	u64 link_st = nyla::get_time_in_milliseconds();
	std::string options = "";
	options += "-O0 ";

	std::string clang_command = "clang++ ";
	clang_command += options + " ";
	clang_command += obj_name;
	clang_command += " -o " + m_executable_name;

	std::cout << "-- Linking: " << m_executable_name << '\n';

	system(clang_command.c_str());
	u64 link_time = nyla::get_time_in_milliseconds() - link_st;

	if (m_flags & COMPFLAG_DISPLAY_TIMES) {
		std::cout << "-- Compilation times\n";
		std::cout << "---------------------------\n";
		std::cout << "Parse Time:   " << std::fixed << std::setprecision(3) << (m_total_parse_time_in_milliseconds / 1000.0F) << " seconds\n";
		std::cout << "LLVM IR time: " << std::fixed << std::setprecision(3) << (m_total_ir_gen_time_in_milliseconds / 1000.0F) << " seconds\n";
		std::cout << "Compile time: " << std::fixed << std::setprecision(3) << (compile_time / 1000.0F) << " seconds\n";
		std::cout << "Link time:    " << std::fixed << std::setprecision(3) << (link_time / 1000.0F) << " seconds\n";
		u64 total_time = m_total_parse_time_in_milliseconds + m_total_ir_gen_time_in_milliseconds + compile_time + link_time;
		std::cout << '\n';
		std::cout << "Total time:   " << (total_time / 1000.0F) << " seconds\n";
	}
}

nyla::sym_table* nyla::compiler::find_sym_table(const std::string& path) {
	auto it = m_sym_tables.find(path);
	if (it != m_sym_tables.end()) {
		return it->second;
	}
	return nullptr;
}

void nyla::compiler::set_main_function(sym_function* main_function) {
	if (m_main_function) {
		// TODO: probably want to tell location of where the multiple main
		// functions were found
		m_log.global_error(ERR_MULTIPLE_MAIN_FUNCTIONS_IN_PROGRAM);
		m_found_compilation_errors = true;
	}
	m_main_function = main_function;
}

u32 nyla::compiler::get_new_unique_module_id() {
	u32 unique_id = unique_module_id_count;
	++unique_module_id_count;
	return unique_id;
}

u32 nyla::compiler::get_num_global_const_array_count() {
	u32 count = m_num_global_const_array_count;
	++m_num_global_const_array_count;
	return count;
}

u32 nyla::compiler::get_num_global_variable_count() {
	u32 count = m_num_global_variable_count;
	++m_num_global_variable_count;
	return count;
}

u32 nyla::compiler::get_num_functions_count() {
	u32 count = m_num_functions_count;
	++m_num_functions_count;
	return count;
}

void nyla::compiler::add_global_initialize_expr(nyla::avariable_decl* expr) {
	expr->global_initializer_expr = true;
	m_global_initializer_exprs.push_back(expr);
}

void nyla::compiler::add_startup_function(llvm::Function* ll_startup_function) {
	m_startup_functions.push_back(ll_startup_function);
}

void nyla::compiler::set_executable_name(const std::string& executable_name) {
	m_executable_name = executable_name;
}

void nyla::compiler::collect_source_files(const std::string& directory,
	                                      const std::string& directory_rel_src,
	                                      std::vector<file_location>& source_files) {
	std::tuple<std::vector<search_file>, bool> files =
		nyla::get_directory_files(directory);
	if (!std::get<1>(files)) {
		m_log.global_error(ERR_FAILED_TO_READ_SOURCE_DIRECTORY,
			               error_payload::string({directory}));
		exit(-1);
		return;
	}
	for (const search_file& search_file : std::get<0>(files)) {
		if (search_file.is_directory) {
			collect_source_files(
				directory         + "/" + search_file.path, 
				directory_rel_src + (directory_rel_src.empty() ? "" : "/") + search_file.path,
				source_files);
		}
		if (nyla::string_ends_with(search_file.path, std::string(".nyla"))) {
			file_location file_location;
			file_location.system_path   = directory + "/" + search_file.path;
			file_location.internal_path = directory_rel_src
				                             + (directory_rel_src.empty() ? "" : "/")
				                             + search_file.path.substr(0, search_file.path.size() - 5);
			
			auto it = m_sym_tables.find(file_location.internal_path);
			if (it != m_sym_tables.end()) {
				m_log.global_error(
					ERR_CONFLICTING_INTERNAL_PATHS,
					error_payload::file_locations(err_file_locations{ &file_location, &file_location })
				);
				exit(-1);
			}

			sym_table* new_sym_table = new sym_table;
			new_sym_table->m_search_for_main_function =
				m_main_function_file == file_location.internal_path;

			new_sym_table->set_file_location(file_location);
			m_sym_tables[file_location.internal_path] = new_sym_table;
			source_files.push_back(file_location);
		}
	}
}

void nyla::compiler::process_files(std::vector<file_location>& source_files) {

	auto it = m_sym_tables.find(m_main_function_file);
	if (it == m_sym_tables.end()) {
		m_log.global_error(ERR_FILE_WITH_MAIN_FUNCTION_DOES_NOT_EXIST,
			               error_payload::string({ m_main_function_file }));
		m_found_compilation_errors = true;
		return;
	}

	// Parsing all files that depend on the file
	// with the main function and creating llvm ir
	sym_table* main_file_sym_table = it->second;
	file_location source_file = main_file_sym_table->get_file_location();
	process_file(source_file, main_file_sym_table);

	u32 save_flags = m_flags;
	// Making sure no object code is generated
	if (should_analyze()) {
		m_flags = COMPFLAG_ONLY_PARSE_AND_ANALYZE;
	}

	// Parsing the rest of the files but since they do
	// not depend on the main file there is no reason
	// to fully process them
	for (file_location& source_file : source_files) {
		sym_table* sym_table = m_sym_tables[source_file.internal_path];
		if (!sym_table->m_started_processing) {
			process_file(source_file, sym_table);
		}
	}

	m_flags = save_flags;
}

void nyla::compiler::process_file(const file_location& source_file, sym_table* our_sym_table) {
	
#define ERROR_RETURN() {                          \
m_found_compilation_errors = true;                \
our_sym_table->m_found_compilation_errors = true; \
	return;                                       \
 }

	if (our_sym_table->m_started_processing) return;
	our_sym_table->m_started_processing = true;

	std::cout << "-- Processing: " << source_file.system_path << '\n';

	// 1. Setup everything needed to parse, analyze and
	//    generate code
	u64 parse_st = nyla::get_time_in_milliseconds();
	c8* buffer;
	ulen buffer_len;
	if (!nyla::read_file(source_file.system_path, buffer, buffer_len)) {
		m_log.global_error(
			ERR_FAILED_TO_READ_FILE,
			error_payload::file_locations(err_file_locations{ &source_file })
		);
		exit(-1);
		return;
	}

	nyla::source source(buffer, buffer_len);
	nyla::log log(source);
	log.set_file_path(source_file.internal_path);
	nyla::lexer lexer(source, log);

	nyla::afile_unit* file_unit = new afile_unit;
	file_unit->tag = AST_FILE_UNIT;

	nyla::parser parser(*this, lexer, log, our_sym_table, file_unit);
	nyla::analysis analysis(*this, log, our_sym_table, file_unit);
	nyla::llvm_generator llvm_generator(*this, m_llvm_module, file_unit,
		m_flags & COMPFLAG_DISPLAY_LLVM_IR);

	our_sym_table->set_parser(&parser);
	our_sym_table->set_analysis(&analysis);
	our_sym_table->set_llvm_generator(&llvm_generator);

	// 2. Parsing our file
	parse_file(our_sym_table);
	if (log.has_errors()) ERROR_RETURN();

	our_sym_table->m_parse_imports_iterator   = file_unit->imports.begin();
	our_sym_table->m_resolve_imports_iterator = file_unit->imports.begin();
	our_sym_table->m_analyze_imports_iterator = file_unit->imports.begin();
	our_sym_table->m_gen_body_decl_iterator   = file_unit->imports.begin();
	our_sym_table->m_gen_type_decl_iterator   = file_unit->imports.begin();
	our_sym_table->m_imports_end              = file_unit->imports.end();

	// 3. Parsing all the dependencies found in
	//    the imports
	parse_dependencies(our_sym_table);
	if (log.has_errors()) ERROR_RETURN();

	// 4. Import resolution
	resolve_imports(our_sym_table);
	if (log.has_errors()) ERROR_RETURN();
	
	// 5. Import resolution for all dependencies
	if (!resolve_dependency_imports(our_sym_table)) {
		ERROR_RETURN();
	}

	// 6. Analyzing the file
	if (!should_analyze()) return;
	analyze_file(our_sym_table);
	if (log.has_errors()) ERROR_RETURN();
	
	// 7. Analyzing all the dependency files
	if (!analyze_dependencies(our_sym_table)) {
		ERROR_RETURN();
	}

	// Freeing the buffer since it was only
	// need to stay around for errors
	delete[] buffer;

	// 8. Generating module declarations
	if (!should_gen_obj_code()) return;
	gen_type_declarations(our_sym_table);

	// 9. Generating the module declarations for
	//    dependency files
	gen_dependency_type_declarations(our_sym_table);

	// 10. Generating the function declarations
	gen_body_declarations(our_sym_table);

	// 11. Generating the function declarations for dependencies
	gen_dependency_body_declarations(our_sym_table);

	// 12.
	// TODO: comptime function generation needs to happen here

	// 13. Generating the llvm function code

	if ((m_flags & COMPFLAG_DISPLAY_LLVM_IR) || (m_flags & COMPFLAG_DISPLAY_STAGES)) {
		std::cout << "-- LLVM IR: " << source_file.system_path << '\n';
	}
	u64 it_gen_st = nyla::get_time_in_milliseconds();
	llvm_generator.gen_file_unit();
	m_total_ir_gen_time_in_milliseconds += nyla::get_time_in_milliseconds() - it_gen_st;

	parse_st = nyla::get_time_in_milliseconds();
	// No longer need the AST so to free up memory deleting it
	delete file_unit;
	m_total_parse_time_in_milliseconds += nyla::get_time_in_milliseconds() - parse_st;

#undef ERROR_RETURN
}

void nyla::compiler::parse_file(sym_table* our_sym_table) {
	if (our_sym_table->m_started_parsing) return;
	u64 parse_st = nyla::get_time_in_milliseconds();
	our_sym_table->m_started_parsing = true;
	nyla::parser* parser = our_sym_table->get_parser();
	parser->parse_imports();
	parser->parse_file_unit();
	m_total_parse_time_in_milliseconds += nyla::get_time_in_milliseconds() - parse_st;
}

void nyla::compiler::parse_dependencies(sym_table* our_sym_table) {
	import_iterator& it = our_sym_table->m_parse_imports_iterator;
	while (it != our_sym_table->m_imports_end) {
		std::string internal_dep_path = it->first;
		sym_table* dep_sym_table = m_sym_tables[internal_dep_path];
		if (!dep_sym_table->m_started_processing) {
			nyla::file_location location = dep_sym_table->get_file_location();
			process_file(location, dep_sym_table);
		}
		if (it != our_sym_table->m_imports_end) {
			++it;
		}
	}
}

void nyla::compiler::resolve_imports(sym_table* our_sym_table) {
	if (our_sym_table->m_started_import_resolution) return;
	our_sym_table->m_started_import_resolution = true;

	if (m_flags & COMPFLAG_DISPLAY_STAGES) {
		std::cout << "-- Resolving imports: " << our_sym_table->get_file_location().system_path << '\n';
	}

	u64 parse_st = nyla::get_time_in_milliseconds();
	nyla::parser* parser = our_sym_table->get_parser();
	parser->resolve_imports();
	m_total_parse_time_in_milliseconds += nyla::get_time_in_milliseconds() - parse_st;
}

bool nyla::compiler::resolve_dependency_imports(sym_table* our_sym_table) {
	import_iterator& it = our_sym_table->m_resolve_imports_iterator;
	while (it != our_sym_table->m_imports_end) {
		std::string internal_dep_path = it->first;
		sym_table* dep_sym_table = m_sym_tables[internal_dep_path];
		ensure_state(dep_sym_table, FS_PARSED);
		if (dep_sym_table->m_found_compilation_errors) return false;
		resolve_imports(dep_sym_table);
		if (dep_sym_table->m_found_compilation_errors) return false;
		if (it != our_sym_table->m_imports_end) {
			++it;
		}
	}
	return true;
}

void nyla::compiler::analyze_file(sym_table* our_sym_table) {
	if (our_sym_table->m_started_analysis) return;
	our_sym_table->m_started_analysis = true;

	if (m_flags & COMPFLAG_DISPLAY_STAGES) {
		std::cout << "-- Analyzing: " << our_sym_table->get_file_location().system_path << '\n';
	}
	
	u64 parse_st = nyla::get_time_in_milliseconds();
	our_sym_table->get_analysis()->check_file_unit();
	m_total_parse_time_in_milliseconds += nyla::get_time_in_milliseconds() - parse_st;
	
}

bool nyla::compiler::analyze_dependencies(sym_table* our_sym_table) {
	import_iterator& it = our_sym_table->m_analyze_imports_iterator;
	while (it != our_sym_table->m_imports_end) {
		std::string internal_dep_path = it->first;
		sym_table* dep_sym_table = m_sym_tables[internal_dep_path];
		if (dep_sym_table->m_found_compilation_errors) return false;
		ensure_state(dep_sym_table, FS_IMPORT_RESOLVED);
		analyze_file(dep_sym_table);
		if (dep_sym_table->m_found_compilation_errors) return false;
		if (it != our_sym_table->m_imports_end) {
			++it;
		}
	}
	return true;
}

void nyla::compiler::gen_type_declarations(sym_table* our_sym_table) {
	if (our_sym_table->m_started_ir_module_declaration_gen) return;
	our_sym_table->m_started_ir_module_declaration_gen = true;

	if (m_flags & COMPFLAG_DISPLAY_STAGES) {
		std::cout << "-- Gen module decls: " << our_sym_table->get_file_location().system_path << '\n';
	}

	llvm_generator* llvm_generator = our_sym_table->get_llvm_generator();
	llvm_generator->gen_type_declarations();
}

void nyla::compiler::gen_dependency_type_declarations(sym_table* our_sym_table) {
	import_iterator& it = our_sym_table->m_gen_type_decl_iterator;
	while (it != our_sym_table->m_imports_end) {
		std::string internal_dep_path = it->first;
		sym_table* dep_sym_table = m_sym_tables[internal_dep_path];
		ensure_state(dep_sym_table, FS_ANALYZED);
		gen_type_declarations(dep_sym_table);
		if (it != our_sym_table->m_imports_end) {
			++it;
		}
	}
}

void nyla::compiler::gen_body_declarations(sym_table* our_sym_table) {
	if (our_sym_table->m_started_ir_func_declaration_gen) return;
	our_sym_table->m_started_ir_func_declaration_gen = true;

	if (m_flags & COMPFLAG_DISPLAY_STAGES) {
		std::cout << "-- Gen function decls: " << our_sym_table->get_file_location().system_path << '\n';
	}

	llvm_generator* llvm_generator = our_sym_table->get_llvm_generator();
	llvm_generator->gen_body_declarations();
}

void nyla::compiler::gen_dependency_body_declarations(sym_table* our_sym_table) {
	import_iterator& it = our_sym_table->m_gen_body_decl_iterator;
	while (it != our_sym_table->m_imports_end) {
		std::string internal_dep_path = it->first;
		sym_table* dep_sym_table = m_sym_tables[internal_dep_path];
		ensure_state(dep_sym_table, FS_TYPE_DECL_GEN);
		gen_body_declarations(dep_sym_table);
		if (it != our_sym_table->m_imports_end) {
			++it;
		}
	}
}

void nyla::compiler::ensure_state(sym_table* dep_sym_table, file_state state) {
	parse_dependencies(dep_sym_table);
	if (state < FS_IMPORT_RESOLVED) return;
	resolve_dependency_imports(dep_sym_table);
	if (state < FS_ANALYZED) return;
	analyze_dependencies(dep_sym_table);
	if (state < FS_TYPE_DECL_GEN) return;
	gen_dependency_type_declarations(dep_sym_table);
	if (state < FS_BODY_DECL_GEN) return;
	gen_dependency_body_declarations(dep_sym_table);
}

void nyla::compiler::completely_cleanup() {
	nyla::g_type_table->clear_table();
	nyla::g_word_table->clear_table();
	delete m_llvm_module;
	delete nyla::llvm_context;
}