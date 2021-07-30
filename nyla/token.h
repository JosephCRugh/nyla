#pragma once

#include "name.h"

namespace nyla {

	enum token_tag {
		/* Ascii Characters reserved < 257 */
		TK_UNKNOWN = 257,

		TK_IDENTIFIER,

		// === Keywords === \\

		__TK_START_OF_KEYWORDS = TK_IDENTIFIER + 1,

		// Types

		TK_TYPE_BYTE,
		TK_TYPE_SHORT,
		TK_TYPE_INT,
		TK_TYPE_LONG,
		TK_TYPE_FLOAT,
		TK_TYPE_DOUBLE,
		TK_TYPE_BOOL,
		TK_TYPE_VOID,

		// Control Flow

		TK_FOR,
		TK_WHILE,
		TK_DO,
		TK_IF,
		TK_ELSE,
		TK_SWITCH,

		TK_RETURN,

		__TK_END_OF_KEYWORDS = TK_TYPE_VOID,

		// === Values === \\

		TK_VALUE_INT,
		TK_VALUE_FLOAT,
		TK_VALUE_DOUBLE,
		TK_VALUE_BOOL,

		// === Symbols === \\

		// === Other === \\

		TK_EOF
	};

	struct token {
		u32 tag;
		
		virtual ~token() {}

		token(u32 _tag) : tag(_tag) {}

		friend std::ostream& operator<<(std::ostream& os, const nyla::token* token) {
			token->print(os);
			return os;
		}
	
		bool operator==(const nyla::token& o) {
			return equals(o);
		}

		virtual void print(std::ostream& os) const = 0;

		virtual bool equals(const nyla::token& o) const = 0;

	};

	struct default_token : public token {

		virtual ~default_token() {}

		default_token(u32 _tag) : token(_tag) {}

		void print(std::ostream& os) const override {
			switch (tag) {
			case TK_UNKNOWN: os << "(Unknown)"; break;
			default:
				if (tag < TK_UNKNOWN) {
					os << (c8)tag;
				}
				
				break;
			}
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

		void print(std::ostream& os) const override {
			os << "(" << name << ")";
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

		void print(std::ostream& os) const override {
			switch (tag) {
			case TK_VALUE_INT:    os << "(" << value_int << ")"; break;
			case TK_VALUE_FLOAT:  os << "(" << value_float << ")"; break;
			case TK_VALUE_DOUBLE: os << "(" << value_double << ")"; break;
			default: break;
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
}