#include "log.h"

#include "words.h"
#include "utils.h"
#include "compiler.h"
#include "ast.h"

#include <iostream>

std::string replace_tabs_with_spaces(std::string& s) {
	std::string no_tabs;
	for (c8& c : s) {
		if (c != '\t') {
			no_tabs += c;
		} else {
			no_tabs += "    ";
		}
	}
	return no_tabs;
}

void nyla::log::global_error(error_tag error_tag) {
	global_error(error_tag, error_payload::none());
}

void nyla::log::global_error(error_tag tag, const error_payload& payload) {
	set_console_color(console_color_red);
	std::cerr << "error: ";
	set_console_color(console_color_default);
	switch (tag) {
	case ERR_FAILED_TO_READ_SOURCE_DIRECTORY: {
		std::cerr << "Failed to read source directory: \"" << payload.d_string->str << "\"\n"
		          << "Make sure you have valid system permissions";
		break;
	}
	case ERR_CONFLICTING_INTERNAL_PATHS: {
		std::cerr << "Conflicting files. Files share identical internal paths.\n";
		std::cerr << "    Internal Path: " << payload.d_file_locations->loc1->internal_path << '\n';
		std::cerr << "    System Path1:  " << payload.d_file_locations->loc1->system_path   << '.' << '\n';
		std::cerr << "    System Path2:  " << payload.d_file_locations->loc1->system_path;
		break;
	}
	case ERR_FAILED_TO_READ_FILE: {
		std::cerr << "Failed to read file: " << payload.d_file_locations->loc1->system_path;
		break;
	}
	case ERR_MULTIPLE_MAIN_FUNCTIONS_IN_PROGRAM: {
		std::cerr << "Multiple main function entry points found in the program";
		break;
	}
	case ERR_MAIN_FUNCTION_NOT_FOUND: {
		std::cerr << "Could not find entry point for program";
		break;
	}
	}
	std::cerr << ".\n";
}

void nyla::log::err(error_tag tag, const error_payload& payload,
	                u32 line_num,
	                u32 spos,
	                u32 epos) {
	if (!m_file_path.empty()) {
		std::cerr << m_file_path << ":";
	}
	std::cerr << line_num << ": ";
	set_console_color(console_color_red);
	std::cerr << "error: ";
	set_console_color(console_color_default);
	switch (tag) {
	case ERR_UNKNOWN_CHARACTER: {
		std::cerr << "Unknown character encountered in the source";
		break;
	}
	case ERR_INT_TOO_LARGE: {
		std::cerr << "Integer literal is too large";
		break;
	}
	case ERR_FLOAT_TOO_LARGE: {
		std::cerr << "Float literal is too large";
		break;
	}
	case ERR_IDENTIFIER_TOO_LONG: {
		std::cerr << "Identifier is too long";
		break;
	}
	case ERR_MISSING_CLOSING_QUOTE: {
		std::cerr << "Missing closing quote on String";
		break;
	}
	case ERR_MISSING_CLOSING_CHAR_QUOTE: {
		std::cerr << "Missing close quote on character";
		break;
	}
	case ERR_INVALID_ESCAPE_SEQUENCE: {
		std::cerr << "Invalid escape sequence";
		break;
	}
	case ERR_UNCLOSED_COMMENT: {
		std::cerr << "Unclosed comment";
		break;
	}
	case ERR_EXPECTED_IDENTIFIER: {
		std::cerr << "Expected identifier after this token";
		break;
	}
	case ERR_EXPECTED_TOKEN: {
		err_expected_token* expected_token = payload.d_expected_token;
		std::cerr << "Expected Token '"
			      << nyla::token_tag_to_string(expected_token->expected_tag) << "'"
			      << " but found '"
			      << expected_token->found_token.to_string() << "'";
		break;
	}
	case ERR_CANNOT_FIND_IMPORT: {
		std::cerr << "Could not find import";
		break;
	}
	case ERR_EXPECTED_MODULE: {
		std::cerr << "Expected to find a module";
		break;
	}
	case ERR_MODULE_REDECLARATION: {
		std::cerr << "Redeclaration of module";
		break;
	}
	case ERR_EXPECTED_IMPORT: {
		std::cerr << "Expected to find imports";
		break;
	}
	case ERR_EXPECTED_MODULE_STMT: {
		std::cerr << "Unexpected token in module";
		break;
	}
	case ERR_VARIABLE_REDECLARATION: {
		std::cerr << "Attempting to redeclare variable: '"
			      << word_as_string(payload.d_word_key) << "'";
		break;
	}
	case ERR_FUNCTION_REDECLARATION: {
		std::cerr << "Attempting to redeclare function '"
			      << word_as_string(payload.d_func_decl->word_key)
			      << "'. First declared at line: " << payload.d_func_decl->line_num;
		break;
	}
	case ERR_VARIABLE_HAS_VOID_TYPE: {
		std::cerr << "Variable '"
			      << word_as_string(payload.d_word_key) << "' has type void";
		break;
	}
	case ERR_EXPECTED_FACTOR: {
		std::cerr << "Expected factor";
		break;
	}
	case ERR_EXPECTED_STMT: {
		std::cerr << "Expected to find a statement";
		break;
	}
	case ERR_TOO_MANY_PTR_SUBSCRIPTS: {
		std::cerr << "Too many pointer '*' subscripts";
		break;
	}
	case ERR_TOO_MANY_ARR_SUBSCRIPTS: {
		std::cerr << "Too many array '[]' subscripts";
		break;
	}
	case ERR_ARRAY_TOO_DEEP: {
		std::cerr << "Too many array depths";
		break;
	}
	case ERR_DUPLICATE_MODULE_ALIAS: {
		std::cerr << "Already aliased that module";
		break;
	}
	case ERR_DUPLICATE_IMPORT: {
		std::cerr << "Duplicate import";
		break;
	}
	case ERR_CONFLICTING_IMPORTS: {
		err_conflicting_import* conflicting_import = payload.d_conflicting_import;
		std::cerr << "Conflicting module imports. Module '"
			<< word_as_string(conflicting_import->module_name_key) << "' exist in both \""
			<< conflicting_import->internal_path1 << "\" and in \""
			<< conflicting_import->internal_path2 << "\"\n"
			<< header_spaces(line_num)
			<< "Use syntax: import <path> (<module name> -> <alias>); to provide an"
			<< " alias name for one of the modules";
		break;
	}
	case ERR_MODULE_ALIAS_NAME_ALREADY_USED: {
		err_conflicting_import* conflicting_import = payload.d_conflicting_import;
		std::cerr << "Module import alias name '"
			      << word_as_string(conflicting_import->module_name_key) << "' already used"
			      << " with import path \"" << conflicting_import->internal_path1 << "\"";
		break;
	}
	case ERR_COULD_NOT_FIND_MODULE_FOR_TYPE: {
		std::cerr << "'" << word_as_string(payload.d_word_key)
			      << "' could not be resolved to a type";
		break;
	}
	case ERR_CANNOT_ASSIGN: {
		std::cerr << "Cannot assign value of type '"
			      << payload.d_types->t1->to_string()
			      << "' to variable of type '"
			      << payload.d_types->t2->to_string()
			      << "'";
		break;
	}
	case ERR_RETURN_VALUE_NOT_COMPATIBLE_WITH_RETURN_TYPE: {
		std::cerr << "Return value of type '"
			      << payload.d_types->t1->to_string()
			      << "' not compatible with return type '"
			      << payload.d_types->t2->to_string()
			      << "'";
		break;
	}
	case ERR_UNDECLARED_VARIABLE: {
		std::cerr << "Could not find declaration for variable '"
			      << word_as_string(payload.d_word_key) << "'";
		break;
	}
	case ERR_USE_OF_VARIABLE_BEFORE_DECLARATION: {
		std::cerr << "Using variable '"
			      << word_as_string(payload.d_word_key)
			      << "' before it was declared";
		break;
	}
	case ERR_OP_CANNOT_APPLY_TO: {
		err_op_cannot_apply* op_cannot_apply = payload.d_op_cannot_apply;
		std::cerr << "Operator '" << nyla::token_tag_to_string(op_cannot_apply->op)
			      << "' cannot apply to type '"
			      << op_cannot_apply->type->to_string() << "'";
		break;
	}
	case ERR_EXPECTED_BOOL_COND: {
		std::cerr << "Expected boolean condition";
		break;
	}
	case ERR_COULD_NOT_FIND_FUNCTION: {
		nyla::afunction_call* function_call = payload.d_function_call;
		std::cerr << "Could not find overloaded function match for function type: "
			      << word_as_string(function_call->name_key) << "(";
		for (u32 i = 0; i < function_call->arguments.size(); i++) {
			std::cerr << function_call->arguments[i]->type->to_string();
			if (i + 1 != function_call->arguments.size()) std::cerr << ", ";
		}
		std::cerr << ")";
		break;
	}
	case ERR_COULD_NOT_FIND_CONSTRUCTOR: {
		nyla::afunction_call* function_call = payload.d_function_call;
		std::cerr << "Could not find overloaded constructor match constructor type: "
			      << word_as_string(function_call->name_key) << "(";
		for (u32 i = 0; i < function_call->arguments.size(); i++) {
			std::cerr << function_call->arguments[i]->type->to_string();
			if (i + 1 != function_call->arguments.size()) std::cerr << ", ";
		}
		std::cerr << ")";
		break;
	}
	case ERR_ARR_TOO_MANY_INIT_VALUES: {
		std::cerr << "Too many initializer values for array";
		break;
	}
	case ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY: {
		std::cerr << "Element of type '" << payload.d_types->t2->to_string()
			      << "' is not compatible with the array's element type '"
			      << payload.d_types->t1->to_string() << "'";
		break;
	}
	case ERR_COULD_NOT_FIND_MODULE_TYPE: {
		std::cerr << "Could not find type";
		break;
	}
	case ERR_FUNCTION_EXPECTS_RETURN: {
		std::cerr << "Function expects return";
		break;
	}
	case ERR_STMTS_AFTER_RETURN: {
		std::cerr << "Unreachable code";
		break;
	}
	case ERR_ILLEGAL_MODIFIERS: {
		std::cerr << "Illegal modifiers for variable";
		break;
	}
	case ERR_DOT_OP_EXPECTS_VARIABLE: {
		std::cerr << "The '.' operator expects a variable";
		break;
	}
	case ERR_TYPE_DOES_NOT_HAVE_FIELD: {
		std::cerr << "Operator '.' cannot apply to the type for variable '"
			      << word_as_string(payload.d_word_key) << "'";
		break;
	}
	case ERR_ARRAY_ACCESS_EXPECTS_INT: {
		std::cerr << "Array access expected to be an integer";
		break;
	}
	case ERR_TOO_MANY_ARRAY_ACCESS_INDEXES: {
		std::cerr << "Too many indexes into the array";
		break;
	}
	case ERR_EXPECTED_VALID_TYPE: {
		std::cerr << "Expected valid type";
		break;
	}
	case ERR_FUNCTION_EXPECTS_RETURN_VALUE: {
		std::cerr << "Function expects a return value";
		break;
	}
	case ERR_CALLED_NON_STATIC_FUNC_FROM_STATIC: {
		std::cerr << "Cannot make a call to a non-static function from a static context";
		break;
	}
	case ERR_CAN_ONLY_HAVE_ONE_ACCESS_MOD: {
		std::cerr << "Cannot have more than one access modifier";
		break;
	}
	case ERR_MODULE_FOR_ALIAS_NOT_FOUND: {
		std::cerr << "The module '"
			      << word_as_string(payload.d_word_key)
			      << "' does not exist so it could not be aliased";
		break;
	}
	case ERR_ARRAY_ACCESS_ON_INVALID_TYPE: {
		std::cerr << "Array access on invalid type";
		break;
	}
	case ERR_CIRCULAR_FIELDS: {
		std::cerr << "Circular fields found. A module has a field which references itself";
		break;
	}
	}

	std::cerr << '\n';

	std::string between   = m_source->from_range({ spos, epos });
	std::string backwards = m_source->from_window_till_nl(spos - 1, -40);
	std::string forwards  = m_source->from_window_till_nl(epos, +40);

	between   = replace_tabs_with_spaces(between);
	backwards = replace_tabs_with_spaces(backwards);
	forwards  = replace_tabs_with_spaces(forwards);
	std::cerr << backwards << between << forwards << std::endl;
	
	set_console_color(console_color_red);
	std::string spaces = std::string(backwards.size(), ' ');
	std::cerr << spaces << std::string(between.size(), '~') << std::endl;
	std::cerr << spaces << "^" << std::endl;
	set_console_color(console_color_default);
	++m_num_errors;
}

void nyla::log::err(error_tag tag,
	u32 line_num,
	u32 spos,
	u32 epos) {
	err(tag, error_payload::none(), line_num, spos, epos);
}

void nyla::log::err(error_tag tag,
	                const nyla::token& st,
	                const nyla::token& et) {
	err(tag, error_payload::none(), st, et);
}

void nyla::log::err(error_tag tag, const nyla::token& st_et) {
	err(tag, st_et, st_et);
}

void nyla::log::err(error_tag tag, const error_payload& payload,
	                const nyla::token& st_et) {
	err(tag, payload, st_et, st_et);
}

void nyla::log::err(error_tag tag, const error_payload& payload,
	                const nyla::ast_node* node) {
	err(tag, payload, node->line_num, node->spos, node->epos);
}

void nyla::log::err(error_tag tag, const nyla::ast_node* node) {
	err(tag, error_payload::none(), node->line_num, node->spos, node->epos);
}

void nyla::log::err(error_tag tag, const error_payload& payload,
	                const nyla::token& st,
	                const nyla::token& et) {
	err(tag, payload, st.line_num, st.spos, et.epos);
}

std::string nyla::log::word_as_string(u32 word_key) {
	return g_word_table->get_word(word_key).c_str();
}

std::string nyla::log::header_spaces(u32 line_num) {
	u32 num_spaces = m_file_path.size();
	if (!m_file_path.empty()) {
		++num_spaces; // For :
	}
	num_spaces += std::to_string(line_num).size();

	return std::string(num_spaces + 9, ' ');
}
