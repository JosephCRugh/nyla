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

int main() {

	nyla::setup_lexer();

	run_reader_tests();

	run_lexer_tests();
	
	run_parser_tests();
	
	return 0;
}
