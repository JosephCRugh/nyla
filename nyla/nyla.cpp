/*
 * Compiler being writen with LLVM.
 * 
 * Lack of comments currently reflect that
 * functions are likely to change.
 * 
 * Contributors: Joseph C Rugh 
 */

#include "tests.h"

#include "lexer.h"
#include "token.h"

#include "gen_llvm.h"

#include "gen_llvm.h"

int main() {

	nyla::setup_lexer();

	run_reader_tests();

	run_lexer_tests();
	
	run_parser_tests();
	
	nyla::init_llvm();

	run_llvm_gen_tests();

	nyla::clean_llvm();

	return 0;
}
