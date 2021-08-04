#include "token.h"

std::string nyla::token_tag_to_string(u32 tag) {
	switch (tag) {
	case TK_PLUS_EQ:    return "+=";
	case TK_MINUS_EQ:   return "-=";
	case TK_DIV_EQ:     return "/=";
	case TK_MUL_EQ:     return "*=";
	case TK_IDENTIFIER: return "identifier";
	case TK_VALUE_INT:  return "int value";
	case TK_VALUE_FLOAT:  return "int float";
	case TK_VALUE_DOUBLE:  return "int double";
	default:
		if (tag < TK_UNKNOWN)
			return std::string(1, tag);
		if (tag >= __TK_START_OF_KEYWORDS && tag <= __TK_END_OF_KEYWORDS) {
			return std::string(reversed_reserved_words[tag].c_str());
		}
	}
	return "";
}

std::unordered_map<nyla::name, nyla::token_tag,
	nyla::name::hash_gen> nyla::reserved_words;
std::unordered_map<u32, nyla::name>
	nyla::reversed_reserved_words;

nyla::name nyla::bool_true_word;
nyla::name nyla::bool_false_word;

void nyla::setup_tokens() {
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
	for (auto it = reserved_words.begin(); it != reserved_words.end(); it++) {
		reversed_reserved_words[it->second] = it->first;
	}
	nyla::bool_true_word = nyla::name::make("true");
	nyla::bool_false_word = nyla::name::make("false");;
}
