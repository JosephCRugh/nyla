#include "lexer.h"
#include "log.h"

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

std::unordered_map<nyla::name, nyla::token_tag,
	nyla::name::hash_gen> reserved_words;

void nyla::setup_lexer() {
	reserved_words = {
		{ nyla::name::make("byte")  , nyla::TK_TYPE_BYTE   },
		{ nyla::name::make("short") , nyla::TK_TYPE_SHORT  },
		{ nyla::name::make("int")   , nyla::TK_TYPE_INT    },
		{ nyla::name::make("long")  , nyla::TK_TYPE_LONG   },
		{ nyla::name::make("float") , nyla::TK_TYPE_FLOAT  },
		{ nyla::name::make("double"), nyla::TK_TYPE_DOUBLE },
		{ nyla::name::make("bool")  , nyla::TK_TYPE_BOOL   },
		{ nyla::name::make("void")  , nyla::TK_TYPE_VOID   },
		{ nyla::name::make("for")   , nyla::TK_FOR         },
		{ nyla::name::make("while") , nyla::TK_WHILE       },
		{ nyla::name::make("do")    , nyla::TK_DO          },
		{ nyla::name::make("if")    , nyla::TK_IF          },
		{ nyla::name::make("else")  , nyla::TK_ELSE        },
		{ nyla::name::make("switch"), nyla::TK_SWITCH      },
		{ nyla::name::make("return"), nyla::TK_RETURN      },
	};
}

nyla::word_token* nyla::get_reserved_word_token(c_string name) {
	nyla::name n = nyla::name::make(name);
	return new nyla::word_token(reserved_words[n], n);
}

lexer::lexer(nyla::reader& reader) : m_reader(reader) {
	
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
	assert(!whitespace_set[m_reader.cur_char()] && "Character in whitespace_set not consumed.");
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
		return next_symbol();
	case '\0': case EOF:
		return new nyla::default_token(TK_EOF);
	default: {
		g_log->error(ERR_UNKNOWN_CHAR, m_line_num, error_data::make_char_load(m_reader.cur_char()));
		m_reader.next_char(); // Reading the unknown character.
		return new nyla::default_token(TK_UNKNOWN);
	}
	}
}

nyla::word_token* lexer::next_word() {
	nyla::word_token* word_token = new nyla::word_token(TK_IDENTIFIER);
	nyla::name name;
	c8 ch = m_reader.cur_char();
	while (identifier_set[ch]) {
		name.append(ch);
		ch = m_reader.next_char();
	}
	if (name.size() > 0xFF) {
		// TODO: report error about the name being too long
	}

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
			return new nyla::default_token(TK_PLUS_EQ);
		}
		return new nyla::default_token('+');
	}
	case '-': {
		ch = m_reader.next_char(); // consuming -
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return new nyla::default_token(TK_MINUS_EQ);
		}
		return new nyla::default_token('-');
	}
	case '/': {
		ch = m_reader.next_char(); // consuming /
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return new nyla::default_token(TK_DIV_EQ);
		}
		return new nyla::default_token('/');
	}
	case '*': {
		ch = m_reader.next_char(); // consuming *
		if (ch == '=') {
			m_reader.next_char(); // consuming =
			return new nyla::default_token(TK_MUL_EQ);
		}
		return new nyla::default_token('*');
	}
	default:
		nyla::token* symbol_token = new nyla::default_token(ch);
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

nyla::num_token* lexer::next_integer(const std::tuple<u32, u32>& digits_before_dot) {
	u64 int_value = calculate_int_value(digits_before_dot);

	// Modifies the type. Ex. u converts to unsigned
	c8 unit_type = m_reader.cur_char();
	// TODO: modifiers

	return nyla::num_token::make_int(int_value);
}

u64 lexer::calculate_int_value(const std::tuple<u32, u32>& digits)
{
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
	
	if (too_large) {
		g_log->error(ERR_INT_TOO_LARGE, m_line_num,
			     error_data::make_str_literal_load(m_reader.from_range(digits).c_str()));
	}

	return int_value;
}

nyla::num_token* lexer::next_number() {
	std::tuple<u32, u32> digits_before_dot = read_unsigned_digits();
	return next_integer(digits_before_dot);
}
