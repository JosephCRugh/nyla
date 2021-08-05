#pragma once

#include "reader.h"
#include "token.h"
#include "log.h"

namespace nyla {

	nyla::word_token* get_reserved_word_token(c_string name);

	class lexer {
	public:
		
		lexer(nyla::reader& reader, nyla::log& log);

		/// <summary>
		/// Obtains the next token by reading
		/// from the reader.
		/// </summary>
		nyla::token* next_token();

		void print_tokens();

		u32 get_line_num() { return m_line_num; }

	private:

		template<typename token_type>
		token_type* make_token(u32 tag, u32 start_pos, u32 end_pos) {
			token_type* token = new token_type(tag);
			token->line_num = m_line_num;
			token->spos = start_pos;
			token->epos = end_pos;
			return token;
		}

		template<typename token_type>
		token_type* make_token(u32 tag, u32 start_pos) {
			return make_token<token_type>(tag, start_pos, start_pos);
		}

		void consume_ignored();

		void on_new_line();

		nyla::token* next_word();

		nyla::token* next_symbol();

		nyla::num_token* next_number();

		nyla::string_token* next_string();

		nyla::num_token* next_float(const std::tuple<u32, u32>& digits_before_dot);

		nyla::num_token* next_integer(const std::tuple<u32, u32>& digits_before_dot);

		std::tuple<u64, bool> calculate_int_value(const std::tuple<u32, u32>& digits);

		std::tuple<u32, u32> read_unsigned_digits();

		void produce_error(error_tag tag, error_data* data, nyla::token* token);

		nyla::reader& m_reader;
		nyla::log&    m_log;
		u32           m_line_num = 1;
		u32           m_start_pos;
	};
}