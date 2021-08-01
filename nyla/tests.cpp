#include "tests.h"

#include "files_util.h"

#include "parser.h"
#include "test_suite.h"
#include "gen_llvm.h"
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
		nyla::lexer lexer(reader);
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("var")));
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("my_var5")));
		check_eq(lexer.next_token(), new nyla::word_token(nyla::TK_IDENTIFIER, nyla::name::make("gg")));		
	}
	{
		nyla::reader reader("void byte short int long float double");
		nyla::lexer lexer(reader);
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
		nyla::lexer lexer(reader);
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("if"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("return"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("else"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("switch"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("do"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("for"));
		check_eq(lexer.next_token(), nyla::get_reserved_word_token("while"));
	}
}

#define parser_setup(text) \
nyla::reader reader(text); \
nyla::lexer lexer(reader); \
nyla::sym_table sym_table; \
nyla::parser parser(lexer, sym_table);

void function_decl_test(c_string text, nyla::type_tag return_type,
	c_string fname,
	const std::vector<nyla::type_tag>& param_types,
	const std::vector<c_string>&       param_names
	) {
	parser_setup(text)
	nyla::afunction* function = parser.parse_function_decl();
	check_eq(function->return_type->tag, return_type, "Ret. Type Check");
	check_eq(function->name, nyla::name::make(fname));                             \
	assert(param_types.size() == param_names.size()                                \
		   && "The Number of names should match the number of types");            \
	check_eq(function->parameters.size(), param_types.size(), "Parameters Size");
	if (function->parameters.size() == param_types.size()) {
		for (u32 i = 0; i < function->parameters.size(); i++) {
			check_eq(function->parameters[i]->variable->name, nyla::name::make(param_names[i]), "Parameter Name Check");
			check_eq(function->parameters[i]->variable->type->tag, param_types[i], "Parameter Type Check");
		}
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
	{ "simple_memory.nyla", 5+44 }
};

void run_llvm_gen_tests() {

	nyla::init_native_target();

	nyla::for_files(L"resources/*", [](const std::string& fpath) {
		// TODO make sure they are .nyla files

		c8* buffer;
		nyla::read_file("resources/" + fpath, buffer);
		parser_setup(buffer);
			nyla::working_llvm_module = new llvm::Module("My Module", *nyla::llvm_context);
		nyla::llvm_generator generator;
		llvm::Function* function = generator.gen_function(parser.parse_function());
		function->print(llvm::errs());
		std::string out_path = fpath.substr(0, fpath.size()-5).append(".o");
		nyla::write_obj_file(out_path.c_str());

		system((std::string("clang++ -O0 ") + out_path + " -o program.exe").c_str());
		int err_code = system("program.exe");
		
		auto it = program_err_codes.find(fpath);
		if (it != program_err_codes.end()) {
			check_eq(err_code, it->second, "Error code check");
		}

		delete[] buffer;

	});

	delete nyla::working_llvm_module;
}
