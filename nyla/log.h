#pragma once

#include "types_ext.h"

namespace nyla {

	enum error_tag {
		ERR_NO_ERROR,
		ERR_UNKNOWN_CHAR,
		ERR_INT_TOO_LARGE,
		ERR_EXPECTED_TOKEN,
		ERR_CANNOT_RESOLVE_TYPE,
		ERR_EXPECTED_IDENTIFIER,
		ERR_EXPECTED_STMT,
		ERR_EXPECTED_FACTOR
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
		union {
			c8       character;
			c_string str_literal;
			u32      token_tag;
		};
	};

	struct expected_token_data : public error_data {
		virtual ~expected_token_data() {}
		static expected_token_data* make_expect_tk_tag(u32 expected_tag, u32 found_tag) {
			expected_token_data* data = new expected_token_data;
			data->expected_tag = expected_tag;
			data->found_tag    = found_tag;
			return data;
		}
		u32 expected_tag;
		u32 found_tag;
	};

	class log {
	public:

		void error(error_tag tag, u32 line_num, error_data* data);

		void set_should_print(bool tof) { m_should_print = tof; }

		error_tag last_err_and_reset();

		error_tag get_last_error_tag() { return m_last_error_tag; }

		u32 get_num_errors() { return m_num_errors; }

	private:
		bool      m_should_print   = true;
		error_tag m_last_error_tag = ERR_NO_ERROR;
		u32       m_num_errors     = 0;
	};

	extern nyla::log* g_log;
}