#pragma once

#include "name.h"
#include <string>
#include <unordered_map>

namespace nyla {

	enum token_tag {
		/* Ascii Characters reserved < 257 */
		TK_UNKNOWN = 257,

		TK_IDENTIFIER,

		// === Keywords === \\

		__TK_START_OF_KEYWORDS = TK_IDENTIFIER + 1,

		TK_DLLIMPORT,
		TK_EXTERNAL,

		// Types

		TK_TYPE_BYTE,
		TK_TYPE_SHORT,
		TK_TYPE_INT,
		TK_TYPE_LONG,
		TK_TYPE_FLOAT,
		TK_TYPE_DOUBLE,
		TK_TYPE_BOOL,
		TK_TYPE_VOID,
		TK_TYPE_CHAR16,

		// Control Flow

		TK_FOR,
		TK_WHILE,
		TK_DO,
		TK_IF,
		TK_ELSE,
		TK_SWITCH,

		TK_RETURN,

		__TK_END_OF_KEYWORDS = TK_RETURN,

		// === Values === \\

		TK_VALUE_INT,
		TK_VALUE_FLOAT,
		TK_VALUE_DOUBLE,
		TK_VALUE_BOOL,
		TK_STRING_VALUE,

		// === Symbols === \\

		TK_PLUS_EQ,
		TK_MINUS_EQ,
		TK_DIV_EQ,
		TK_MUL_EQ,

		// === Other === \\

		TK_EOF
	};

	std::string token_tag_to_string(u32 tag);

	extern std::unordered_map<nyla::name, nyla::token_tag,
		nyla::name::hash_gen> reserved_words;
	extern std::unordered_map<u32, nyla::name>
		reversed_reserved_words;

	extern nyla::name bool_true_word;
	extern nyla::name bool_false_word;

	void setup_tokens();

	struct token {
		u32 tag;
		u32 line_num;
		u32 spos, epos;   // Start and end position in the buffer
		
		virtual ~token() {}

		token(u32 _tag) : tag(_tag) {}

		friend std::ostream& operator<<(std::ostream& os, const nyla::token* token) {
			os << "(" << token->to_string() << ")";
			return os;
		}
	
		bool operator==(const nyla::token& o) {
			return equals(o);
		}

		virtual std::string to_string() const = 0;

		virtual bool equals(const nyla::token& o) const = 0;

	};

	struct default_token : public token {

		virtual ~default_token() {}

		default_token(u32 _tag) : token(_tag) {}

		virtual std::string to_string() const override {
			return nyla::token_tag_to_string(tag);
		}

		bool equals(const nyla::token& o) const override {
			return tag == o.tag;
		}
	};

	struct word_token : public token {

		nyla::name name;

		virtual ~word_token() {}
		
		word_token(u32 _tag) : token(_tag) {}
		word_token(u32 _tag, nyla::name _name) : token(_tag), name(_name) {}

		virtual std::string to_string() const override {
			return name.c_str();
		}

		bool equals(const nyla::token& o) const override {
			if (tag != o.tag) return false;
			const word_token* wt = dynamic_cast<const word_token*>(&o);
			return wt->name == name;
		}
	};

	struct num_token : public token {
		union {
			s32    value_int;
			float  value_float;
			double value_double;
		};

		virtual ~num_token() {}

		num_token(u32 _tag) : token(_tag) {}

		static num_token* make_int(s32 _value_int) {
			num_token* tok = new num_token(TK_VALUE_INT);
			tok->value_int = _value_int;
			return tok;
		}
		static num_token* make_float(float _value_float) {
			num_token* tok = new num_token(TK_VALUE_FLOAT);
			tok->value_float = _value_float;
			return tok;
		}
		static num_token* make_double(double _value_double) {
			num_token* tok = new num_token(TK_VALUE_DOUBLE);
			tok->value_double = _value_double;
			return tok;
		}

		virtual std::string to_string() const override {
			switch (tag) {
			case TK_VALUE_INT:    return std::to_string(value_int);
			case TK_VALUE_FLOAT:  return std::to_string(value_float);
			case TK_VALUE_DOUBLE: return std::to_string(value_double);
			default: return "";
			}
		}

		bool equals(const nyla::token& o) const override {
			if (tag != o.tag) return false;
			const num_token* nt = dynamic_cast<const num_token*>(&o);
			switch (tag) {
			case TK_VALUE_INT:    return value_int == nt->value_int;
			case TK_VALUE_FLOAT:  return value_float == nt->value_float;
			case TK_VALUE_DOUBLE: return value_double == nt->value_double;
			default: return false;
			}
			return false;
		}
	};

	struct bool_token : public token {
		bool tof;

		virtual ~bool_token() {}

		bool_token(u32 _tag) : token(_tag) {}

		virtual std::string to_string() const override {
			if (tof) return "true";
			return "false";
		}

		bool equals(const nyla::token& o) const override {
			if (tag != o.tag) return false;
			const bool_token* bt = dynamic_cast<const bool_token*>(&o);
			return tof == bt->tof;
		}
	};

	struct string_token : public token {
		std::string lit;

		string_token(u32 _tag) : token(_tag) {}
		string_token(u32 _tag, std::string& _lit)
			: token(_tag), lit(_lit) {}

		virtual ~string_token() {}

		virtual std::string to_string() const override {
			return lit;
		}

		bool equals(const nyla::token& o) const override {
			if (tag != o.tag) return false;
			const string_token* st = dynamic_cast<const string_token*>(&o);
			return st->lit == lit;
		}
	};
}