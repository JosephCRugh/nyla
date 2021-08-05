#pragma once

#include "types_ext.h"
#include "type.h"
#include "token.h"
#include "reader.h"

namespace nyla {

	enum error_tag {
		ERR_NO_ERROR,
		// Lexer errors
		ERR_UNKNOWN_CHAR,
		ERR_INT_TOO_LARGE,
		ERR_EXPONENT_TOO_LARGE,

		// Parser before analysis errors
		ERR_EXPECTED_TOKEN,
		ERR_CANNOT_RESOLVE_TYPE,
		ERR_EXPECTED_IDENTIFIER,
		ERR_EXPECTED_STMT,
		ERR_EXPECTED_FACTOR,
		ERR_VARIABLE_REDECLARATION,
		
		// Analysis errors
		ERR_SHOULD_RETURN_VALUE,
		ERR_RETURN_TYPE_MISMATCH,
		ERR_CANNOT_ASSIGN,
		ERR_OP_CANNOT_APPLY_TO,
		ERR_EXPECTED_BOOL_COND,
		ERR_FUNCTION_NOT_FOUND,
		ERR_UNDECLARED_VARIABLE,
		ERR_USE_BEFORE_DECLARED_VARIABLE
	};

	struct error_data {
		virtual ~error_data() {}
		static error_data* make_empty_load() {
			return new error_data;
		}
		static error_data* make_char_load(c8 character) {
			error_data* data = new error_data;
			data->character = character;
			return data;
		}
		static error_data* make_str_literal_load(c_string str_literal) {
			error_data* data = new error_data;
			data->str_literal = str_literal;
			return data;
		}
		static error_data* make_token_tag_load(u32 token_tag) {
			error_data* data = new error_data;
			data->token_tag = token_tag;
			return data;
		}
		static error_data* make_token_load(nyla::token* token) {
			error_data* data = new error_data;
			data->token = token;
			return data;
		}
		union {
			c8           character;
			c_string     str_literal;
			u32          token_tag;
			nyla::token* token;
		};
	};

	struct expected_token_data : public error_data {
		virtual ~expected_token_data() {}
		static expected_token_data* make_expect_tk_tag(u32 expected_tag, nyla::token* found_token) {
			expected_token_data* data = new expected_token_data;
			data->expected_tag = expected_tag;
			data->found_token = found_token;
			return data;
		}
		u32          expected_tag;
		nyla::token* found_token;
	};

	struct type_mismatch_data : public error_data {
		virtual ~type_mismatch_data() {}
		static type_mismatch_data* make_type_mismatch(nyla::type* expected_type, nyla::type* found_type) {
			type_mismatch_data* data = new type_mismatch_data;
			data->expected_type = expected_type;
			data->found_type    = found_type;
			return data;
		}
		nyla::type* expected_type;
		nyla::type* found_type;
	};

	struct op_applies_to_data : public error_data {
		virtual ~op_applies_to_data() {}
		static op_applies_to_data* make_applies_to(u32 op, nyla::token* err_token) {
			op_applies_to_data* data = new op_applies_to_data;
			data->op = op;
			data->err_token = err_token;
			return data;
		}
		u32 op;
		nyla::token* err_token;
	};

	class log {
	public:

		log(nyla::reader& reader);

		void error(error_tag tag, error_data* data,
			       nyla::token* start_token, nyla::token* end_token);

		void set_should_print(bool tof) { m_should_print = tof; }

		error_tag get_last_error_tag() { return m_last_error_tag; }

		u32 get_num_errors() { return m_num_errors; }

	private:
		nyla::reader m_reader;
		bool         m_should_print   = true;
		error_tag    m_last_error_tag = ERR_NO_ERROR;
		u32          m_num_errors     = 0;
	};

}