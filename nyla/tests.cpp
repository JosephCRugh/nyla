#include "tests.h"

#include "files_util.h"
#include "log.h"
#include "parser.h"
#include "test_suite.h"
#include "gen_llvm.h"
#include "analysis.h"
#include <assert.h>
#include <fstream>
#include <unordered_map>

void run_reader_tests() {
	test("reader tests");
	nyla::reader reader("ex%#");
	check_eq(reader.cur_char(), 'e');
	check_eq(reader.next_char(), 'x');
	check_eq(reader.next_char(), '%');
	check_eq(reader.next_char(), '#');
	check_eq(reader.next_char(), '\0');
}

void run_lexer_tests() {
	test("lexer_tests");
	{
		nyla::reader reader("var my_var5 gg");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("var")));
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("my_var5")));
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("gg")));	
	}
	{
		nyla::reader reader("void byte short int long float double");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("void"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("byte"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("short"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("int"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("long"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("float"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("double"));
	}
	{
		nyla::reader reader("if return else switch do for while");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("if"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("return"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("else"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("switch"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("do"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("for"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("while"));
	}
	{
		nyla::reader reader("+ - / * = ; () {} <> += -= /= *=");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), new nyla::default_token('+'));
		check_eq(lexer.next_token(), new nyla::default_token('-'));
		check_eq(lexer.next_token(), new nyla::default_token('/'));
		check_eq(lexer.next_token(), new nyla::default_token('*'));
		check_eq(lexer.next_token(), new nyla::default_token('='));
		check_eq(lexer.next_token(), new nyla::default_token(';'));
		check_eq(lexer.next_token(), new nyla::default_token('('));
		check_eq(lexer.next_token(), new nyla::default_token(')'));
		check_eq(lexer.next_token(), new nyla::default_token('{'));
		check_eq(lexer.next_token(), new nyla::default_token('}'));
		check_eq(lexer.next_token(), new nyla::default_token('<'));
		check_eq(lexer.next_token(), new nyla::default_token('>'));
		check_eq(lexer.next_token(), new nyla::default_token(nyla::TK_PLUS_EQ));
		check_eq(lexer.next_token(), new nyla::default_token(nyla::TK_MINUS_EQ));
		check_eq(lexer.next_token(), new nyla::default_token(nyla::TK_DIV_EQ));
		check_eq(lexer.next_token(), new nyla::default_token(nyla::TK_MUL_EQ));
	}
	{
		nyla::reader reader("1323   123.02   124.412E+3  123.2E-2");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), nyla::num_token::make_int(1323));
		check_eq(lexer.next_token(), nyla::num_token::make_double(123.02));
		check_eq(lexer.next_token(), nyla::num_token::make_double(124.412E+3));
		check_eq(lexer.next_token(), nyla::num_token::make_double(123.2E-2));
	}
	{
		nyla::reader reader("24346345142135325124342356235");
		nyla::log log(reader);
		log.set_should_print(false);
		nyla::lexer lexer(reader, log);
		lexer.next_token();
		check_eq(nyla::ERR_INT_TOO_LARGE, log.get_last_error_tag(), "Too large error");
	}
	{
		nyla::reader reader("213.0E+1242153253263412321146");
		nyla::log log(reader);
		log.set_should_print(false);
		nyla::lexer lexer(reader, log);
		lexer.next_token();
		check_eq(nyla::ERR_EXPONENT_TOO_LARGE, log.get_last_error_tag(), "Too large error");
	}
	{
		nyla::reader reader("\"Hello!\"");
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		check_eq(lexer.next_token(), new nyla::string_token(nyla::TK_STRING_VALUE, std::string("Hello!")));
	}
}

#define parser_setup(text)      \
nyla::reader reader(text);      \
nyla::log log(reader);          \
nyla::lexer lexer(reader, log); \
nyla::sym_table sym_table;      \
nyla::parser parser(lexer, sym_table, log);

void function_decl_test(c_string text, nyla::type_tag return_type,
	c_string fname,
	const std::vector<nyla::type_tag>& param_types,
	const std::vector<c_string>&       param_names
	) {
	parser_setup(text)
	nyla::afunction* function = parser.parse_function_decl(false);
	check_eq(function->return_type->tag, return_type, "Ret. Type Check");
	check_eq(function->name, nyla::name::make(fname));                             \
	assert(param_types.size() == param_names.size()                                \
		   && "The Number of names should match the number of types");            \
	check_eq(function->parameters.size(), param_types.size(), "Parameters Size");
	if (function->parameters.size() == param_types.size()) {
		for (u32 i = 0; i < function->parameters.size(); i++) {
			check_eq(function->parameters[i]->variable->name, nyla::name::make(param_names[i]), "Parameter Name Check");
			check_eq(function->parameters[i]->variable->checked_type->tag, param_types[i], "Parameter Type Check");
		}
	}
	// Cleaning up the tokens to free memory.
	for (nyla::token* token : parser.get_processed_tokens()) {
		delete token;
	}
	std::cout << "---------------------------" << std::endl;
}

void run_parser_tests() {

	test("Function Declaration Tests");
	function_decl_test("void func()", nyla::TYPE_VOID, "func", {}, {});
	function_decl_test("int gg_2(short aa)", nyla::TYPE_INT, "gg_2",
		{ nyla::TYPE_SHORT },
		{ "aa" });
	function_decl_test("float r___g(int y1, int x, byte t4)", 
		nyla::TYPE_FLOAT, "r___g",
		{ nyla::TYPE_INT, nyla::TYPE_INT, nyla::TYPE_BYTE },
		{ "y1", "x", "t4" });

}

std::unordered_map<std::string, int> program_err_codes = {
	{ "a.nyla", 55 + 88 },
	{ "simple_memory.nyla", 5+44 },
	{ "loop_sum.nyla", [](){
			int total = 0;
			for (int i = 0; i < 55; i++) {
				total += i;
			}
			return total;
		}() },
	{ "arithmetic.nyla", [](){
				int b = 22;
				int sum = 44 * 3 + 55 - 421 * b;
				int g = 4;
				sum /= g + 1;
				sum *= 3 - 1;
				sum = sum * sum;
				return sum;
			}()  },
	{ "func_call.nyla", 7+54 },
	{ "print.nyla", 0 },
	{ "arrays.nyla", 412 + 21 + 5 + 6 + 4 }
};

void run_llvm_gen_tests() {

	test("LLVM Code-Gen Tests");

	nyla::init_native_target();

	nyla::for_files(L"resources/*", [](const std::string& fpath) {
		// TODO make sure they are .nyla files
		
		std::cout << "Attempting to parse: " << fpath << std::endl;

		c8* buffer;
		ulen buffer_size;
		nyla::read_file("resources/" + fpath, buffer, buffer_size);
		nyla::reader reader(buffer, buffer_size);
		nyla::log log(reader);
		nyla::lexer lexer(reader, log);
		
		nyla::sym_table sym_table;
		nyla::parser parser(lexer, sym_table, log);
		nyla::analysis analysis(sym_table, log);

		nyla::working_llvm_module = new llvm::Module("My Module", *nyla::llvm_context);
		nyla::llvm_generator generator;
		nyla::afile_unit* file_unit = parser.parse_file_unit();
		std::cout << file_unit << std::endl;
		
		analysis.type_check_file_unit(file_unit);
		
		std::cout << file_unit << std::endl;
		if (log.get_num_errors() != 0) {
			return;
		}
		
		generator.gen_file_unit(file_unit, true);
		std::string out_path = fpath.substr(0, fpath.size()-5).append(".o");
		nyla::write_obj_file(out_path.c_str());

		auto it = program_err_codes.find(fpath);
		if (it != program_err_codes.end()) {
			std::string all_dll_import_paths = "";
			std::vector<std::string> dll_import_paths = sym_table.get_dll_import_paths();
			for (std::string import_path : dll_import_paths) {
				all_dll_import_paths += import_path + " ";
			}
			std::string options = "";
			options += "-O0 ";
			system((std::string("clang++ ") + options + " " + out_path + " -o program.exe ").c_str());
			int err_code = system("program.exe");
			check_eq(err_code, it->second, "Error code check");
			
		}

		// Cleaning up the tokens to free memory.
		for (nyla::token* token : parser.get_processed_tokens()) {
			delete token;
		}
		delete[] buffer;
		
	});

	delete nyla::working_llvm_module;
}

void print_fail_pass_rate() {
	std::cout << "Passed: " << passed_tests << std::endl;
	std::cout << "Failed: " << failed_tests << std::endl;
}
