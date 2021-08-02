#pragma once

#include "reader.h"
#include "token.h"

namespace nyla {

	void setup_lexer();

	nyla::word_token* get_reserved_word_token(c_string name);

	class lexer {
	public:
		
		lexer(nyla::reader& reader);

		/// <summary>
		/// Obtains the next token by reading
		/// from the reader.
		/// </summary>
		nyla::token* next_token();

		u32 get_line_num() { return m_line_num; }

	private:

		void consume_ignored();

		void on_new_line();

		nyla::word_token* next_word();

		nyla::token* next_symbol();

		nyla::num_token* next_number();

		nyla::num_token* next_integer(const std::tuple<u32, u32>& digits_before_dot);

		u64 calculate_int_value(const std::tuple<u32, u32>& digits);

		std::tuple<u32, u32> read_unsigned_digits();

		nyla::reader m_reader;
		u32          m_line_num = 1;
	};
}