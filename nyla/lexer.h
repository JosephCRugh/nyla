#ifndef NYLA_LEXER_H
#define NYLA_LEXER_H

#include "source.h"
#include "log.h"
#include "tokens.h"

namespace nyla {

	class lexer {
	public:

		lexer(nyla::source& source, nyla::log& log)
			: m_source(source), m_log(log) {}

		// Obtains the next token by
		// analyzing the current source.
		nyla::token next_token();

	private:

		// Consumes whitespace and comments
		// until the start of a new token.
		void consume_ignored();

		// Process encountering a new line
		void on_new_line();

		// Next token is an identifier or
		// keyword.
		nyla::token next_word();

		// Next token is a symbol.
		nyla::token next_symbol();

		// Next token is an integer
		// or float.
		nyla::token next_number();
		nyla::range read_unsigned_digits();
		nyla::token next_integer(const nyla::range& digits);
		nyla::range read_hexidecimal_digits();
		nyla::token next_hexidecimal();
		nyla::token finalize_integer(u64 int_value);
		nyla::token next_float(const range& whole_digits,
			                   const range& fraction_digits,
			                   const range& exponent_digits,
			                   c8 exponent_sign);


		// Calculate an integer value based on a range of digits
		// @returns the computed value and a boolean to indicate if
		//          there was overflow
		std::tuple<u64, bool> calculate_int_value(const nyla::range& digits);

		// Next token is a string
		nyla::token next_string();

		// Next token is a character
		nyla::token next_character();

		inline nyla::token make(u32 tag, u32 start_pos, u32 end_pos) {
			nyla::token token;
			token.tag      = tag;
			token.line_num = m_line_num;
			token.spos     = start_pos;
			token.epos     = end_pos;
			return token;
		}

		nyla::source& m_source;
		nyla::log&    m_log;
		u32           m_line_num  = 1;
		u32           m_start_pos = 0; // The buffer position that
							           // a token starts on
	};

}

#endif