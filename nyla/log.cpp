#include "log.h"

#include "console_colors.h"
#include <iostream>

using namespace nyla;

nyla::log::log(nyla::reader& reader)
	: m_reader(reader) {
}

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

void log::error(error_tag tag, error_data* data,
	            nyla::token* start_token, nyla::token* end_token) {
	if (m_should_print) {
		std::cerr << start_token->line_num << ": ";
		set_console_color(test_color_red);
		std::cerr << "error: ";
		set_console_color(test_color_default);
		switch (tag) {
		case ERR_UNKNOWN_CHAR: {
			std::cerr << "Unknown character: '"
				      << data->character
				      << "' (" << (+data->character & 0xFF) << ")";
			break;
		}
		case ERR_INT_TOO_LARGE: {
			std::cerr << "Numberic literal '"
				      << data->str_literal
				      << "' is too large.";
			break;
		}
		case ERR_EXPONENT_TOO_LARGE: {
			std::cerr << "Float Exponent: '"
				      << data->str_literal
				      << "' is too large.";
			break;
		}
		case ERR_EXPECTED_TOKEN: {
			nyla::expected_token_data* expected_data =
				dynamic_cast<nyla::expected_token_data*>(data);
			std::cerr << "Expected Token '"
				      << nyla::token_tag_to_string(expected_data->expected_tag) << "'"
				      << " but found '"
				      << expected_data->found_token->to_string() << "'";
			break;
		}
		case ERR_CANNOT_RESOLVE_TYPE: {
			std::cerr << "Cannot resolve type: " << data->str_literal;
			break;
		}
		case ERR_EXPECTED_IDENTIFIER: {
			std::cerr << "Expected identifier";
			break;
		}
		case ERR_EXPECTED_STMT: {
			std::cerr << "Expected statement but found token '"
				      << data->token->to_string() << "'";
			break;
		}
		case ERR_EXPECTED_FACTOR: {
			std::cerr << "Expected factor but found token '"
				      << nyla::token_tag_to_string(data->token_tag) << "'";
			break;
		}
		case ERR_SHOULD_RETURN_VALUE:
			std::cerr << "Should return a value in return statement";
			break;
		case ERR_RETURN_TYPE_MISMATCH: {
			nyla::type_mismatch_data* mismatch_data =
				dynamic_cast<nyla::type_mismatch_data*>(data);
			std::cerr << "Return type mismatch. Found type "
				      << *mismatch_data->found_type
				      << " but expected type "
				      << *mismatch_data->expected_type;
			break;
		}
		case ERR_CANNOT_ASSIGN: {
			nyla::type_mismatch_data* mismatch_data =
				dynamic_cast<nyla::type_mismatch_data*>(data);
			std::cerr << "Cannot assign value of type '"
				      << *mismatch_data->found_type
				      << "' to variable of type '"
				      << *mismatch_data->expected_type
				      << "'";
			break;
		}
		case ERR_DIMENSIONS_OF_ARRAYS_MISMATCH: {
			nyla::type_mismatch_data* mismatch_data =
				dynamic_cast<nyla::type_mismatch_data*>(data);
			std::cerr << "Array dimension of '"
				      << mismatch_data->found_type->array_depths.size()
				      << "' is not compatible with the expected dimension '"
				      << mismatch_data->expected_type->array_depths.size() << "'";
			break;
		}
		case ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY: {
			nyla::type_mismatch_data* mismatch_data =
				dynamic_cast<nyla::type_mismatch_data*>(data);
			std::cerr << "Element of type '"
				      << *mismatch_data->found_type
				      << "' is not compatible with array element type '"
				      << *mismatch_data->expected_type << "'";
			break;
		}
		case ERR_ARRAY_SIZE_SHOULD_BE_IMPLICIT: {
			std::cerr << "The size of the array should be implict when initializing";
			break;
		}
		case ERR_OP_CANNOT_APPLY_TO: {
			nyla::op_applies_to_data* applies_to_data =
				dynamic_cast<nyla::op_applies_to_data*>(data);
			std::cerr << "Operator '"
				      << nyla::token_tag_to_string(applies_to_data->op)
				      << "' cannot be applied to '"
				      << applies_to_data->err_token->to_string() << "'";
			break;
		}
		case ERR_DOT_OP_ON_ARRAY_EXPECTS_LENGTH: {
			std::cerr << "Operator '.' applied to array expected .length";
			break;
		}
		case ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_SINGLE_DIM: {
			// TODO: oddly worded
			std::cerr << "Accessing the dimension of an array length expects a single ['index'] subscript";
			break;
		}
		case ERR_ARRAY_LENGTH_OPERATOR_EXPECTS_INT_DIM: {
			// TODO: oddly worded
			std::cerr << "Expected integer when accessing length dimension of array";
			break;
		}
		case ERR_ARRAY_LENGTH_OPERATOR_INVALID_DIM_INDEX: {
			std::cerr << "Invalid dimension index of array.length['index'] for array of type '"
				      << *data->type << "'";
			break;
		}
		case ERR_ARRAY_LENGTH_NO_DIM_INDEX_FOR_MULTIDIM_ARRAY: {
			std::cerr << "For a multi-dimensional array the .length operator expects a dimension. Snytax: .length['index']";
			break;
		}
		case ERR_EXPECTED_BOOL_COND: {
			std::cerr << "Expected boolean condition";
			break;
		}
		case ERR_VARIABLE_REDECLARATION: {
			std::cerr << "Attempting to redeclare variable: '"
				      << data->str_literal << "'";
			break;
		}
		case ERR_FUNCTION_NOT_FOUND: {
			std::cerr << "Function not found '"
				      << data->str_literal << "'";
			break;
		}
		case ERR_UNDECLARED_VARIABLE: {
			std::cerr << "Could not find declaration for variable '"
				      << data->str_literal << "'";
			break;
		}
		case ERR_USE_BEFORE_DECLARED_VARIABLE: {
			std::cerr << "Attempted to use variable '"
				      << data->str_literal
				      << "' before it was declared";
			break;
		}
		case ERR_ARRAY_SIZE_EXPECTS_INT: {
			std::cerr << "Size of array for type '"
				      << *data->type
				      << "' is expected to be an integer value";
			break;
		}
		case ERR_TOO_MANY_ARRAY_SUBSCRIPTS: {
			std::cerr << "Too many array subscripts. Maximum is 8";
			break;
		}
		case ERR_TOO_MANY_PTR_SUBSCRIPTS: {
			std::cerr << "Too many pointer subscripts. Maximum is 8";
			break;
		}
		case ERR_ARRAY_TOO_DEEP: {
			std::cerr << "Array too deep. Can only go down 8 levels.";
			break;
		}
		}
		std::cerr << std::endl;

		u32 spos = start_token->spos;
		u32 epos = end_token->epos;
		std::string between   = m_reader.from_range({spos, epos});
		std::string backwards = m_reader.from_window_till_nl(spos-1, -40);
		std::string forwards  = m_reader.from_window_till_nl(epos, +40);
		between   = replace_tabs_with_spaces(between);
		backwards = replace_tabs_with_spaces(backwards);
		forwards  = replace_tabs_with_spaces(forwards);
		std::cerr << backwards << between << forwards << std::endl;

		set_console_color(test_color_red);
		std::string spaces = std::string(backwards.size(), ' ');
		std::cerr << spaces << std::string(between.size(), '~') << std::endl;
		std::cerr << spaces << "^" << std::endl;
		set_console_color(test_color_default);

	}
	++m_num_errors;
	m_last_error_tag = tag;
	delete data;
}
