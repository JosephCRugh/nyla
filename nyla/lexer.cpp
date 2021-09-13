#include "lexer.h"

#include "words.h"

#include <assert.h>

/*
 * No hash lookups simply character boolean
 * sets to ensure speed.
 */

constexpr bool identifier_set[256] = {
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 16
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 32
	0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0, // 48
	// Digits
	1,1,1,1, 1,1,1,1, 1,1,0,0, 0,0,0,0, // 64
	// Uppercase letters
	0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, // 88
	// Last 1 is for underscores
	1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,1, // 96
	// Lowercase letters
	0,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1, // 112
	1,1,1,1, 1,1,1,1, 1,1,1,0, 0,0,0,0,
};

constexpr bool whitespace_set[256] = {
		0,0,0,0,0, 0,0,0,1,1,  // 0-9
		0,1,1,0,0, 0,0,0,0,0,  // 10-19
		0,0,0,0,0, 0,0,0,0,0,  // 20-29
		0,0,1,0,0, 0,0,0,0,0,  // 30-39
};

constexpr bool digits_set[256] = {
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,1,1,
		1,1,1,1,1, 1,1,1,0,0,
};

constexpr bool hexidecimal_set[256] = {
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		0,0,0,0,0, 0,0,0,0,0,
		// 0..9
		0,0,0,0,0, 0,0,0,1,1,
		1,1,1,1,1, 1,1,1,0,0, // << 59
		// A-Z   64-90
		0,0,0,0,1, 1,1,1,1,1, // << 69
		1,1,1,1,1, 1,1,1,1,1, // << 79
		1,1,1,1,1, 1,1,1,1,1, // << 89
		// a-z   97-122
		1,0,0,0,0, 0,0,1,1,1, // << 99
		1,1,1,1,1, 1,1,1,1,1, // << 109
		1,1,1,1,1, 1,1,1,1,1, // << 119
		1,1,1,
};

constexpr bool eof_eol_set[256] = {
	// '\0'                       '\n'
		1,   0,0,0, 0,0,0,0, 0,0,  1,  0,
		//    '\r'
			0, 1
};

constexpr bool eol_set[256] = {
	0,0,0,0, 0,0,0,0, 0,0,1,0,
	0,1
};

// '"' '\r' '\n' '\0'
constexpr bool string_end_set[256] = {
	// '\0'                       '\n'
		1,   0,0,0, 0,0,0,0, 0,0,  1,  0,  // 0-11
	//    '\r'
		0, 1,0,0,0, 0,0,0,0, 0,0,  0,  0,   // 12-24
	//                     '"'
		0, 0,0,0,0, 0,0,0,0,1
};

void nyla::lexer::consume_ignored() {
	bool continue_eating = false;
	do {
		// Eating newlines
		while (eol_set[m_source.cur_char()]) {
			switch (m_source.cur_char()) {
			case '\n':
				m_source.next_char(); // consuming '\n'
				on_new_line();
				break;
			case '\r':
				m_source.next_char(); // consuming '\r'
				if (m_source.cur_char() == '\n')
					m_source.next_char(); // consuming '\n' 
				on_new_line();
				break;
			default:
				assert(!"Unreachable!");
				break;
			}
		}
		// Eating whitespace
		c8 ch = m_source.cur_char();
		while (whitespace_set[ch])
			ch = m_source.next_char();
		// Eating single line comments
		if (ch == '/' && m_source.peek_char() == '/') {
			m_source.next_char(); // Consuming /
			m_source.next_char(); // Consuming /
			ch = m_source.cur_char();
			while (!eof_eol_set[ch])
				ch = m_source.next_char();
		}

		// Eating multi-line comments
		if (ch == '/' && m_source.peek_char() == '*') {
			m_source.next_char(); // Eating first  (
			m_source.next_char(); // Eating second `
			ch = m_source.cur_char();
			while (!(ch == '*' && m_source.peek_char() == '/')) {
				if (ch == '\0') {
					m_log.err(ERR_UNCLOSED_COMMENT,
						      m_line_num,
						      m_source.position(),
						      m_source.position());
					return;
				}
				ch = m_source.next_char();
			}
			m_source.next_char(); // Eating first  `
			m_source.next_char(); // Eating second )
		}
		ch = m_source.cur_char();
		continue_eating = whitespace_set[ch] ||
			ch == '`' && m_source.peek_char() == '`' ||
			eol_set[m_source.cur_char()];
	} while (continue_eating);
}

void nyla::lexer::on_new_line() {
	++m_line_num;
}

nyla::token nyla::lexer::next_token() {
	consume_ignored();
	m_start_pos = m_source.position();
	switch (m_source.cur_char()) {
	case 'a': case 'b': case 'c': case 'd': case 'e':
	case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o':
	case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y':
	case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E':
	case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O':
	case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y':
	case 'Z':
		return next_word();
	case '+': case '-': case '*': case '/':
	case '%': case '&': case '|': case '^':
	case '~': case '!': case '@': case '#':
	case '=': case '.': case ';': case ',':
	case '(': case ')': case '[': case ']':
	case '<': case '>': case '{': case '}':
	case '?':
		return next_symbol();
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return next_number();
	case '"':
		return next_string();
	case '\'':
		return next_character();
	case '\0':
		return make(TK_EOF, m_start_pos, m_start_pos);
	default: {
		nyla::token unknown_token = make(TK_UNKNOWN, m_start_pos, m_start_pos);
		m_log.err(ERR_UNKNOWN_CHARACTER, unknown_token);
		m_source.next_char(); // Eating the unknown character.
		return unknown_token;
	}
	}
	return nyla::token();
}

nyla::token nyla::lexer::next_word() {
	nyla::word word;
	c8 ch = m_source.cur_char();
	while (identifier_set[ch]) {
		word.append(ch);
		ch = m_source.next_char();
	}

	nyla::token word_token = make(TK_IDENTIFIER, m_start_pos, m_source.position());
	word_token.word_key = g_word_table->get_key(word);

	if (word.size() > 0xFF) {
		m_log.err(ERR_IDENTIFIER_TOO_LONG, word_token);
	}

	auto it = reserved_words.find(word_token.word_key);
	if (it != reserved_words.end()) {
		word_token.tag = it->second;
	}

	return word_token;
}

nyla::token nyla::lexer::next_symbol() {
	#define DOUBLE_SYMBOLS(fst, snd, tk_tag) \
case fst: {                              \
	ch = m_source.next_char();           \
	if (ch == snd) {                     \
			m_source.next_char();        \
			return make(tk_tag,          \
				        m_start_pos,     \
				        m_start_pos + 2);\
	}                                    \
	return make(fst, m_start_pos,        \
	                 m_start_pos + 1);   \
}
	c8 ch = m_source.cur_char();
	switch (ch) {
	DOUBLE_SYMBOLS('*', '=', TK_STAR_EQ)
	DOUBLE_SYMBOLS('/', '=', TK_SLASH_EQ)
	DOUBLE_SYMBOLS('%', '=', TK_MOD_EQ)
	DOUBLE_SYMBOLS('^', '=', TK_CRT_EQ)
	DOUBLE_SYMBOLS('!', '=', TK_EXL_EQ)
	DOUBLE_SYMBOLS('=', '=', TK_EQ_EQ)
	case '+': {
		ch = m_source.next_char(); // Eating +
		if (ch == '=') {
			m_source.next_char(); // Eating =
			return make(TK_PLUS_EQ, m_start_pos, m_start_pos + 2);
		} else if (ch == '+') {
			m_source.next_char(); // Eating +
			return make(TK_PLUS_PLUS, m_start_pos, m_start_pos + 2);
		}
		return make('+', m_start_pos, m_start_pos + 1);
	}
	case '?': {
		ch = m_source.next_char(); // Eating ?
		if (ch == '?') {
			if (m_source.peek_char() == '?') {
				m_source.next_char(); // Eating ?
				m_source.next_char(); // Eating ?
				return make(TK_QQQ, m_start_pos, m_start_pos + 3);
			}
		}
		return make('?', m_start_pos, m_start_pos + 1);
	}
	case '-': {
		ch = m_source.next_char(); // Eating -
		if (ch == '=') {
			m_source.next_char(); // Eating =
			return make(TK_MINUS_EQ, m_start_pos, m_start_pos + 2);
		} else if (ch == '-') {
			m_source.next_char(); // Eating -
			return make(TK_MINUS_MINUS, m_start_pos, m_start_pos + 2);
		} else if (ch == '>') {
			m_source.next_char(); // Eating >
			return make(TK_MINUS_GT, m_start_pos, m_start_pos + 2);
		}
		return make('-', m_start_pos, m_start_pos + 1);
	}
	case '|': {
		ch = m_source.next_char(); // Eating |
		switch (ch) {
		case '=': {
			m_source.next_char(); // Eating =
			return make(TK_BAR_EQ, m_start_pos, m_start_pos + 2);
		}
		case '|': {
			m_source.next_char(); // Eating |
			return make(TK_BAR_BAR, m_start_pos, m_start_pos + 2);
		}
		}
		return make('|', m_start_pos, m_start_pos + 1);
	}
	case '&': {
		ch = m_source.next_char(); // Eating &
		switch (ch) {
		case '=': {
			m_source.next_char(); // Eating =
			return make(TK_AMP_EQ, m_start_pos, m_start_pos + 2);
		}
		case '&': {
			m_source.next_char(); // Eating &
			return make(TK_AMP_AMP, m_start_pos, m_start_pos + 2);
		}
		}
		return make('&', m_start_pos, m_start_pos + 1);
	}
	case '>': {
		ch = m_source.next_char(); // Eating >
		if (ch == '>') {
			ch = m_source.next_char(); // Eating >
			if (ch == '=') {
				m_source.next_char(); // Eating =
				return make(nyla::TK_GT_GT_EQ, m_start_pos, m_start_pos + 3);
			}
			return make(nyla::TK_GT_GT, m_start_pos, m_start_pos + 2);
		} else if (ch == '=') { // >=
			m_source.next_char(); // Eating =
			return make(nyla::TK_GT_EQ, m_start_pos, m_start_pos + 2);
		}
		return make('>', m_start_pos, m_start_pos + 1);
	}
	case '<': {
		ch = m_source.next_char(); // Eating <
		if (ch == '<') {
			ch = m_source.next_char(); // Eating <
			if (ch == '=') {
				m_source.next_char(); // Eating =
				return make(nyla::TK_LT_LT_EQ, m_start_pos, m_start_pos + 3);
			}
			return make(nyla::TK_LT_LT, m_start_pos, m_start_pos + 2);
		} else if (ch == '=') { // <=
			m_source.next_char(); // Eating =
			return make(nyla::TK_LT_EQ, m_start_pos, m_start_pos + 2);
		}
		return make('<', m_start_pos, m_start_pos + 1);
	}
	default: {
		nyla::token symbol_token = make(ch, m_start_pos,
			                                 m_start_pos + 1);
		m_source.next_char(); // Eating the symbol
		return symbol_token;
	}
	}
#undef DOUBLE_SYMBOLS
}

nyla::token nyla::lexer::next_number() {
	// Checking for hexidecimal
	if (m_source.cur_char() == '0' && m_source.peek_char() == 'x') {
		m_source.next_char(); // Eating '0'
		m_source.next_char(); // Eating 'x'
		return next_hexidecimal();
	}

	range digits_before_dot = read_unsigned_digits();
	range fraction_digits = { 0, 0 };
	range exponent_digits = { 0, 0 };
	c8 exponent_sign = '+';
	bool is_integer = true;

	if (m_source.cur_char() == '.') {
		m_source.next_char(); // Eating .
		fraction_digits = read_unsigned_digits();
		is_integer = false;
	}

	if (m_source.cur_char() == 'E') {
		is_integer = false;

		c8 possible_sign = m_source.next_char();
		if (possible_sign == '+' || possible_sign == '-') {
			exponent_sign = possible_sign;
			m_source.next_char(); // Consuming + or -
		}

		exponent_digits = read_unsigned_digits();
	}

	// checking for floating point units
	if (m_source.cur_char() == 'f' || m_source.cur_char() == 'd') {
		is_integer = false;
	}

	if (is_integer) {
		return next_integer(digits_before_dot);
	} else {
		return next_float(digits_before_dot, fraction_digits,
			              exponent_digits, exponent_sign);;
	}
}

nyla::range nyla::lexer::read_unsigned_digits() {
	u32 start = m_source.position();
	c8 ch = m_source.cur_char();
	while (digits_set[ch]) {
		ch = m_source.next_char();
	}
	u32 end = m_source.position();
	return range{ start, end };
}

nyla::token nyla::lexer::next_integer(const nyla::range& digits) {
	std::tuple<u64, bool> int_calc_result = calculate_int_value(digits);
	u64 int_value = std::get<0>(int_calc_result);
	bool overflow = std::get<1>(int_calc_result);

	nyla::token int_token;

	if (overflow) {
		int_token = make(TK_VALUE_ULONG, m_start_pos, m_source.position());
		int_token.value_ulong = int_value;
		m_log.err(ERR_INT_TOO_LARGE, int_token);
		return int_token;
	}

	return finalize_integer(int_value);
}

nyla::range nyla::lexer::read_hexidecimal_digits() {
	// TODO: probably should abstract the ability to read
	// a range of values from a set into a common function
	// since the same logic will apply to base-10, hex, octal
	// and binary
	u32 start = m_source.position();
	c8 ch = m_source.cur_char();
	while (hexidecimal_set[ch]) {
		ch = m_source.next_char();
	}
	u32 end = m_source.position();
	return range{ start, end };
}

// Mapping hex characters to numbers
std::unordered_map<c8, u64> hex_to_decimal_mapping =
{
	{ '0', 0  }, { '1', 1  }, { '2', 2  }, { '3', 3  }, { '4', 4  },
	{ '5', 5  }, { '6', 6  }, { '7', 7  }, { '8', 8  }, { '9', 9  },
	{ 'a', 10 }, { 'b', 11 }, { 'c', 12 }, { 'd', 13 }, { 'e', 14 },
	{ 'f', 15 },
	{ 'A', 10 }, { 'B', 11 }, { 'C', 12 }, { 'D', 13 }, { 'E', 14 },
	{ 'F', 15 },
};

nyla::token nyla::lexer::next_hexidecimal() {
	nyla::range digits = read_hexidecimal_digits();


	if (digits.length() == 0) {
		// TODO: produce an error!
	}

	nyla::token int_token;

	u64 int_value = 0;
	u64 prev_value = -1;
	bool overflow = false;
	for (ulen index = digits.start; index < digits.end; index++) {
		prev_value = int_value;
		int_value <<= 4;
		int_value += hex_to_decimal_mapping[m_source[index]];
		if (int_value < prev_value) {
			// TODO: error production?
		}
	}

	return finalize_integer(int_value);
}

nyla::token nyla::lexer::finalize_integer(u64 int_value) {

	nyla::token int_token;

	// Maximum signed 32-bit integer
	if (int_value <= std::numeric_limits<s32>::max()) {
		int_token = make(TK_VALUE_INT, m_start_pos, m_source.position());
		int_token.value_int = int_value;
	}
	// Maximum unsigned 32-bit integer
	else if (int_value <= std::numeric_limits<u32>::max()) {
		int_token = make(TK_VALUE_UINT, m_start_pos, m_source.position());
		int_token.value_uint = int_value;
	}
	// Maximum signed 64-bit integer
	else if (int_value <= std::numeric_limits<s64>::max()) {
		int_token = make(TK_VALUE_LONG, m_start_pos, m_source.position());
		int_token.value_long = int_value;
	}
	// Only catagory left is an unsigned 64 bit integer
	else {
		int_token = make(TK_VALUE_ULONG, m_start_pos, m_source.position());
		int_token.value_ulong = int_value;
	}

	return int_token;
}

nyla::token nyla::lexer::next_float(const range& whole_digits,
	                                const range& fraction_digits,
	                                const range& exponent_digits,
	                                c8 exponent_sign) {
	std::tuple<u64, bool> exp_value_result = calculate_int_value(exponent_digits);
	u64 unsigned_exp_value = std::get<0>(exp_value_result);
	bool exponent_too_large = std::get<1>(exp_value_result);
	if (unsigned_exp_value > std::numeric_limits<s64>::max()) {
		exponent_too_large = true;
	}

	s64 exponent_value = unsigned_exp_value;
	if (exponent_sign == '-') {
		exponent_value = -exponent_value;
	}

	// Since the whole digits and fraction digits
	// will be processed as if they are after the
	// decimal the value will have to be shifted back
	// the amount of digits that were shifted past the decimal.
	exponent_value -= fraction_digits.length();

	u32 index = 0;
	double value = 0.0;
	while (index < whole_digits.length()) {
		u32 i = (u32)m_source[index + whole_digits.start];
		value = 10.0 * value + (u32)(i - '0');
		++index;
	}
	index = 0;
	while (index < fraction_digits.length()) {
		u32 i = (u32)m_source[index + fraction_digits.start];
		value = 10.0 * value + (u32)(i - '0');
		++index;
	}

	if (exponent_value != 0) {
		value *= pow(10.0, exponent_value);
	}

	nyla::token float_token;

	// Error in reading since the value is too large
	if (exponent_too_large || isinf(value)) {
		bool is_float32bits = false;
		if (m_source.cur_char() == 'f') {
			m_source.next_char(); // Eating f
			float_token = make(TK_VALUE_FLOAT, m_start_pos, m_source.position());
			float_token.value_float = std::numeric_limits<float>::max();
			is_float32bits = true;
		} else if (m_source.cur_char() == 'd') {
			m_source.next_char(); // Eating d
		}
		if (!is_float32bits) {
			float_token = make(TK_VALUE_DOUBLE, m_start_pos, m_source.position());
			float_token.value_double = std::numeric_limits<double>::max();
		}

		m_log.err(ERR_FLOAT_TOO_LARGE, float_token);
		
		return float_token;
	}

	bool is_float32bits = false;
	if (m_source.cur_char() == 'f') {
		is_float32bits = true;
		m_source.next_char(); // Eating f
	} else if (m_source.cur_char() == 'd') {
		m_source.next_char(); // Eating d
	}

	if (!is_float32bits) {
		float_token = make(TK_VALUE_DOUBLE, m_start_pos, m_source.position());
		float_token.value_double = value;
	} else {
		float_token = make(TK_VALUE_FLOAT, m_start_pos, m_source.position());
		float_token.value_float = value;
	}

	return float_token;
}

std::tuple<u64, bool> nyla::lexer::calculate_int_value(const nyla::range& digits) {
	u64 int_value = 0;
	u64 prev_value = -1;
	bool overflow = false;
	for (ulen index = digits.start; index < digits.end; index++) {
		prev_value = int_value;
		int_value *= 10;
		int_value += (u64)((u64)m_source[index] - '0');
		if (int_value < prev_value) {
			overflow = true;
			break;
		} else if (int_value == 0xFF'FF'FF'FF'FF'FF'FF'FF) {
			// Possible overflow unless the numeric value fits exactly.
			// largest possible value:  18,446,744,073,709,551,615
			// previous digits must be: 18,446,744,073,709,551,61
			// followed by 5 and it must be the last digit.
			if (!(prev_value == 1844674407370955161 &&
				m_source[index] == '5' && index == digits.end - 1
				)) {
				overflow = true;
				break;
			}
		}
	}

	return std::tuple<u64, bool>{ int_value, overflow };
}

nyla::token nyla::lexer::next_string() {
	c8 ch = m_source.next_char(); // Eating "
	std::string str = "";
	while (!string_end_set[ch]) {
		switch (ch) {
		case '\\': {
			ch = m_source.next_char(); // Eating '\'
			switch (ch) {
				// TODO: Is this all the escape sequences?
			case 'n':
				str += '\n';
				ch = m_source.next_char();
				break;
			case 'r':
				str += '\r';
				ch = m_source.next_char();
				break;
			case 't':
				str += '\t';
				ch = m_source.next_char();
				break;
			case '\\':
				str += '\\';
				ch = m_source.next_char();
				break;
			case '"':
				str += '"';
				ch = m_source.next_char();
				break;
			default: {
				// No escape sequence found
				m_log.err(ERR_INVALID_ESCAPE_SEQUENCE,
					      m_line_num, m_source.position() - 1, m_source.position () + 1);
				ch = m_source.next_char(); // Eating the bad '\'
			}
			}
			break;
		}
		default:
			str += ch;
			ch = m_source.next_char();
			break;
		}
	}

	nyla::token str_token = make(TK_VALUE_STRING8, m_start_pos, m_source.position());
	str_token.value_string8 = str;

	if (ch == '"') {
		m_source.next_char();
		str_token.epos = m_source.position();
	} else {
		m_log.err(ERR_MISSING_CLOSING_QUOTE, str_token);
	}

	return str_token;
}

nyla::token nyla::lexer::next_character() {
	c8 ch = m_source.next_char(); // Eating '
	nyla::token character_token = make(TK_VALUE_CHAR8, m_start_pos, m_start_pos);
	if (ch == '\\') {
		// TODO: escape sequences
		ch = m_source.next_char();
		switch (ch) {
		case 'n':
			character_token.value_char8 = '\n';
			ch = m_source.next_char(); // Consuming 'n'
			break;
		default: break;// TODO: produce error
		}

	} else {
		character_token.value_char8 = ch;
		ch = m_source.next_char(); // Eating character inside ''
	}

	if (ch == '\'') {
		m_source.next_char(); // Eating closing '
		character_token.epos = m_source.position();
	} else {
		character_token.epos = m_source.position();
		m_log.err(ERR_MISSING_CLOSING_CHAR_QUOTE, character_token);
	}

	return character_token;
}
