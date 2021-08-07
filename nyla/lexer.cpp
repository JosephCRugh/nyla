#include "lexer.h"

#include <unordered_map>
#include <assert.h>

using namespace nyla;

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

nyla::word_token* nyla::get_reserved_word_token(c_string name) {
	nyla::name n = nyla::name::make(name);
	return new nyla::word_token(reserved_words[n], n);
}

lexer::lexer(nyla::reader& reader, nyla::log& log)
	: m_reader(reader), m_log(log) {
}

void nyla::lexer::consume_ignored() {
	bool continue_eating = false;
	do {
		// Eating newlines
		while (eol_set[m_reader.cur_char()]) {
			switch (m_reader.cur_char()) {
			case '\n':
				m_reader.next_char(); // consuming '\n'
				on_new_line();
				break;
			case '\r':
				m_reader.next_char(); // consuming '\r'
				if (m_reader.cur_char() == '\n')
					m_reader.next_char(); // consuming '\n' 
				on_new_line();
				break;
			default:
				assert(!"Unreachable!");
				break;
			}
		}
		// Eating whitespace
		c8 ch = m_reader.cur_char();
		while (whitespace_set[ch])
			ch = m_reader.next_char();
		// Eating single line comments
		if (ch == '`' && m_reader.peek_char() == '`') {
			m_reader.next_char(); // Consuming `
			m_reader.next_char(); // Consuming `
			ch = m_reader.cur_char();
			while (!eof_eol_set[ch])
				ch = m_reader.next_char();
		}
		continue_eating = whitespace_set[ch]                       ||
			              ch == '`' && m_reader.peek_char() == '`' ||
			              eol_set[m_reader.cur_char()];
	} while (continue_eating);
}

void lexer::on_new_line() {
	++m_line_num;
}

nyla::token* lexer::next_token() {
	consume_ignored();
	m_start_pos = m_reader.position();
	switch (m_reader.cur_char()) {
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
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return next_number();
	case '+': case '-': case '*': case '/': case ';':
	case '{': case '}': case ',': case '=': case '!':
	case '<': case '>':
	case '(': case ')':
	case '[': case ']':
	case '.':
		return next_symbol();
	case '"':
		return next_string();
	case '\0': case EOF:
		return make_token<nyla::default_token>(TK_EOF, m_start_pos, m_start_pos);
	default: {
		nyla::token* err_token = make_token<nyla::default_token>(TK_UNKNOWN, m_start_pos);
		err_token->epos = m_start_pos + 1;
		produce_error(ERR_UNKNOWN_CHAR,
			error_data::make_char_load(m_reader.cur_char()),  err_token);
		m_reader.next_char(); // Reading the unknown character.
		return err_token;
	}
	}
}

void nyla::lexer::print_tokens() {
	nyla::token* token;
	while ((token = next_token())->tag != TK_EOF) {
		std::cout << token << std::endl;
		delete token;
	}
}

nyla::token* lexer::next_word() {
	nyla::name name;
	c8 ch = m_reader.cur_char();
	while (identifier_set[ch]) {
		name.append(ch);
		ch = m_reader.next_char();
	}

	
	if (name.size() > 0xFF) {
		// TODO: report error about the name being too long
	}

	if (name == nyla::bool_true_word) {
		nyla::bool_token* bool_token = make_token<nyla::bool_token>(TK_VALUE_BOOL, m_start_pos);
		bool_token->epos = m_reader.position();
		bool_token->tof = true;
		return bool_token;
	}
	if (name == nyla::bool_false_word) {
		nyla::bool_token* bool_token = make_token<nyla::bool_token>(TK_VALUE_BOOL, m_start_pos);
		bool_token->epos = m_reader.position();
		bool_token->tof = false;
		return bool_token;
	}
	nyla::word_token* word_token = make_token<nyla::word_token>(TK_IDENTIFIER, m_start_pos);
	word_token->epos = m_reader.position();

	auto it = reserved_words.find(name);
	if (it != reserved_words.end()) {
		word_token->tag  = it->second;
		word_token->name = *g_name_table->find(name);
		return word_token;
	}

	word_token->name = g_name_table->register_name(name);
	return word_token;
}

nyla::token* lexer::next_symbol() {
	c8 ch = m_reader.cur_char();
	switch (ch) {
	case '+': {
		ch = m_reader.next_char(); // consuming +
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return make_token<nyla::default_token>(TK_PLUS_EQ, m_start_pos, m_start_pos + 2);
		}
		return make_token<nyla::default_token>('+', m_start_pos, m_start_pos + 1);
	}
	case '-': {
		ch = m_reader.next_char(); // consuming -
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return make_token<nyla::default_token>(TK_MINUS_EQ, m_start_pos, m_start_pos + 2);
		}
		return make_token<nyla::default_token>('-', m_start_pos, m_start_pos + 1);
	}
	case '/': {
		ch = m_reader.next_char(); // consuming /
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return make_token<nyla::default_token>(TK_DIV_EQ, m_start_pos, m_start_pos + 2);
		}
		return make_token<nyla::default_token>('/', m_start_pos, m_start_pos + 1);
	}
	case '*': {
		ch = m_reader.next_char(); // consuming *
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return make_token<nyla::default_token>(TK_MUL_EQ, m_start_pos, m_start_pos + 2);
		}
		return make_token<nyla::default_token>('*', m_start_pos, m_start_pos + 1);
	}
	default:
		nyla::token* symbol_token = make_token<nyla::default_token>(ch, m_start_pos, m_start_pos + 1);
		m_reader.next_char(); // Consuming the symbol.
		return symbol_token;
	}
}

std::tuple<u32, u32> lexer::read_unsigned_digits() {

	u32 start = m_reader.position();
	c8 ch = m_reader.cur_char();
	while (digits_set[ch]) {
		ch = m_reader.next_char();
	}
	u32 end = m_reader.position();
	return std::tuple<u32, u32>{ start, end };
}

void lexer::produce_error(error_tag tag, error_data* data, nyla::token* token) {
	m_log.error(tag, data, token, token);
}

nyla::num_token* lexer::next_integer(const std::tuple<u32, u32>& digits_before_dot) {
	std::tuple<u64, bool> int_value = calculate_int_value(digits_before_dot);

	nyla::num_token* num_token = nyla::num_token::make_int(std::get<0>(int_value));
	num_token->line_num = m_line_num;
	num_token->spos     = m_start_pos;
	num_token->epos     = m_reader.position();

	// Modifies the type. Ex. u converts to unsigned
	c8 unit_type = m_reader.cur_char();
	// TODO: modifiers

	if (std::get<1>(int_value)) {
		produce_error(ERR_INT_TOO_LARGE,
			error_data::make_str_literal_load(m_reader.from_range(digits_before_dot).c_str()),
			num_token);
	}

	return num_token;
}

std::tuple<u64, bool> lexer::calculate_int_value(const std::tuple<u32, u32>& digits) {
	u64 int_value = 0;
	u64 prev_value = -1;
	bool too_large = false;
	for (ulen index = std::get<0>(digits); index < std::get<1>(digits); index++) {
		prev_value = int_value;
		int_value *= 10;
		int_value += (u64)((u64)m_reader[index] - '0');
		if (int_value < prev_value) {
			too_large = true;
			break;
		} else if (int_value == 0xFF'FF'FF'FF'FF'FF'FF'FF) {
			// Possible overflow unless the numeric value fits exactly.
			// largest possible value:  18,446,744,073,709,551,615
			// previous digits must be: 18,446,744,073,709,551,61
			// followed by 5
			if (!(prev_value == 1844674407370955161 &&
				m_reader[index] == '5'
				)) {
				too_large = true;
				break;
			}
		}
	}

	return std::tuple<u64, bool>{ int_value, too_large };
}

nyla::num_token* lexer::next_float(const std::tuple<u32, u32>& digits_before_dot) {
	m_reader.next_char(); // Consuming .
	std::tuple<u32, u32> fraction_digits = read_unsigned_digits();
	std::tuple<u32, u32> exponent_digits = { 0, 0 };
	c8 exponent_sign = '+';

	// Exponent symbol     4202.412E+34
	if (m_reader.cur_char() == 'E') {
		c8 possible_sign = m_reader.next_char();
		if (possible_sign == '+' || possible_sign == '-') {
			exponent_sign = possible_sign;
			m_reader.next_char(); // Consuming + or -
		}

		exponent_digits = read_unsigned_digits();
	}

	std::tuple<u64, bool> unsigned_exponent_calc = calculate_int_value(exponent_digits);
	bool exponent_too_large = std::get<1>(unsigned_exponent_calc);
	s64 exponent_value = 0;
	if (std::get<0>(unsigned_exponent_calc) > 0x7FFFFFFFFFFFFFFF) {
		exponent_too_large = true;
		exponent_value = 0x7FFFFFFFFFFFFFFF;
	} else {
		exponent_value = std::get<0>(unsigned_exponent_calc);
	}

	if (exponent_sign == '-') {
		exponent_value = -exponent_value;
	}

	u32 fraction_length = std::get<1>(fraction_digits) - std::get<0>(fraction_digits);
	u32 whole_length = std::get<1>(digits_before_dot) - std::get<0>(digits_before_dot);

	// Since the whole digits and fraction digits
	// will be processed as if they are after the
	// decimal the value will have to be shifted back
	// the amount of digits that were shifted past the decimal.
	exponent_value -= fraction_length;

	u32 index = 0;
	double value = 0.0;
	while (index < whole_length) {
		u32 i = (u32)m_reader[index + std::get<0>(digits_before_dot)];
		value = 10.0 * value + (u32)(i - '0');
		++index;
	}
	index = 0;
	while (index < fraction_length) {
		u32 i = (u32)m_reader[index + std::get<0>(fraction_digits)];
		value = 10.0 * value + (u32)(i - '0');
		++index;
	}

	if (exponent_value != 0) {
		value *= pow(10.0, exponent_value);
	}

	nyla::num_token* float_token;
	if (m_reader.cur_char() == 'f') {
		float_token = nyla::num_token::make_float((float)value);
		m_reader.next_char();
	} else if (m_reader.cur_char() == 'd') {
		float_token = nyla::num_token::make_double(value);
		m_reader.next_char();
	} else {
		float_token = nyla::num_token::make_double(value);
	}
	float_token->line_num = m_line_num;
	float_token->spos = m_start_pos;
	float_token->epos = m_reader.position();

	if (exponent_too_large) {
		produce_error(ERR_EXPONENT_TOO_LARGE,
			error_data::make_str_literal_load(m_reader.from_range(exponent_digits).c_str()),
				float_token);
	}

	return float_token;
}

nyla::num_token* lexer::next_number() {
	std::tuple<u32, u32> digits_before_dot = read_unsigned_digits();

	nyla::num_token* num_token = nullptr;
	if (m_reader.cur_char() == '.') {
		num_token = next_float(digits_before_dot);
	} else {
		num_token = next_integer(digits_before_dot);
	}
	return num_token;
}

nyla::string_token* nyla::lexer::next_string() {
	c8 ch = m_reader.next_char(); // Consuming "
	std::string lit = "";
	while (ch != '"' && ch != '\0') {
		switch (ch) {
		case '\\':
			ch = m_reader.next_char();
			if (ch == 'n') {
				lit += '\n';
			}
			break;
		default:
			lit += ch;
			break;
		}
		ch = m_reader.next_char();
	}

	if (ch == '"') {
		m_reader.next_char(); // Consuming "
	} else {
		// TODO: report error.
	}

	nyla::string_token* string_token =
		make_token<nyla::string_token>(TK_STRING_VALUE, m_start_pos, m_reader.position());
	string_token->lit = lit;
	return string_token;
}
