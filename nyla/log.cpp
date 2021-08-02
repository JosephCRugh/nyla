#include "log.h"

#include <iostream>
#include "token.h"

using namespace nyla;

nyla::log* nyla::g_log = new nyla::log;

void log::error(error_tag tag, u32 line_num, error_data* data) {
	if (m_should_print) {
		std::cout << line_num << ": ";
		switch (tag) {
		case ERR_UNKNOWN_CHAR: {
			std::cout << "Unknown character: '"
				<< data->character
				<< "' (" << (+data->character & 0xFF) << ")";
			break;
		}
		case ERR_INT_TOO_LARGE: {
			std::cout << "Numberic literal '"
				<< data->str_literal
				<< "' is too large.";
			break;
		}
		case ERR_EXPECTED_TOKEN: {
			nyla::expected_token_data* expected_data =
				dynamic_cast<nyla::expected_token_data*>(data);
			std::cout << "Expected Token '"
				      << nyla::token_tag_to_string(expected_data->expected_tag) << "'"
				      << " but found '"
				      << nyla::token_tag_to_string(expected_data->found_tag) << "'";
			break;
		}
		case ERR_CANNOT_RESOLVE_TYPE: {
			std::cout << "Cannot resolve type: " << data->str_literal;
			break;
		}
		case ERR_EXPECTED_IDENTIFIER: {
			std::cout << "Expected identifier";
			break;
		}
		case ERR_EXPECTED_STMT: {
			std::cout << "Expected statement but found token '"
				      << nyla::token_tag_to_string(data->token_tag) << "'";
			break;
		}
		case ERR_EXPECTED_FACTOR: {
			std::cout << "Expected factor but found token '"
				<< nyla::token_tag_to_string(data->token_tag) << "'";
			break;
		}
		}
		std::cout << std::endl;
	}
	++m_num_errors;
	m_last_error_tag = tag;
	delete data;
}

error_tag log::last_err_and_reset() {
	error_tag tag = m_last_error_tag;
	m_last_error_tag = ERR_NO_ERROR;
	m_num_errors = 0;
	return tag;
}
