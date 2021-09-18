#include "tokens.h"

#include "words.h"

std::unordered_map<u32, nyla::token_tag> nyla::reserved_words;
std::unordered_map<u32, u32> nyla::reserved_tag_to_words;
u32 nyla::unidentified_ident;
u32 nyla::main_ident;
u32 nyla::length_ident;
u32 nyla::startup_ident;
u32 nyla::memcpy_ident;

void nyla::setup_tokens() {
	reserved_words = {
		// Types
		{ nyla::word::make("byte")     , nyla::TK_TYPE_BYTE   },
		{ nyla::word::make("short")    , nyla::TK_TYPE_SHORT  },
		{ nyla::word::make("int")      , nyla::TK_TYPE_INT    },
		{ nyla::word::make("long")     , nyla::TK_TYPE_LONG   },
		{ nyla::word::make("ubyte")    , nyla::TK_TYPE_UBYTE  },
		{ nyla::word::make("ushort")   , nyla::TK_TYPE_USHORT },
		{ nyla::word::make("uint")     , nyla::TK_TYPE_UINT   },
		{ nyla::word::make("ulong")    , nyla::TK_TYPE_ULONG  },
		{ nyla::word::make("float")    , nyla::TK_TYPE_FLOAT  },
		{ nyla::word::make("double")   , nyla::TK_TYPE_DOUBLE },
		{ nyla::word::make("bool")     , nyla::TK_TYPE_BOOL   },
		{ nyla::word::make("void")     , nyla::TK_TYPE_VOID   },
		{ nyla::word::make("char")     , nyla::TK_TYPE_CHAR8  },
		{ nyla::word::make("char16")   , nyla::TK_TYPE_CHAR16 },
		{ nyla::word::make("char32")   , nyla::TK_TYPE_CHAR32 },
		// True/False
		{ nyla::word::make("true")     , nyla::TK_TRUE        },
		{ nyla::word::make("false")    , nyla::TK_FALSE       },
		// Control Flow
		{ nyla::word::make("for")      , nyla::TK_FOR         },
		{ nyla::word::make("while")    , nyla::TK_WHILE       },
		{ nyla::word::make("do")       , nyla::TK_DO          },
		{ nyla::word::make("if")       , nyla::TK_IF          },
		{ nyla::word::make("else")     , nyla::TK_ELSE        },
		{ nyla::word::make("switch")   , nyla::TK_SWITCH      },
		{ nyla::word::make("return")   , nyla::TK_RETURN      },
		{ nyla::word::make("continue") , nyla::TK_CONTINUE    },
		{ nyla::word::make("break")    , nyla::TK_BREAK       },
		// Other
		{ nyla::word::make("module")   , nyla::TK_MODULE      },
		{ nyla::word::make("static")   , nyla::TK_STATIC      },
		{ nyla::word::make("private")  , nyla::TK_PRIVATE     },
		{ nyla::word::make("protected"), nyla::TK_PROTECTED   },
		{ nyla::word::make("external") , nyla::TK_EXTERNAL    },
		{ nyla::word::make("const")    , nyla::TK_CONST       },
		{ nyla::word::make("comptime") , nyla::TK_COMPTIME    },
		{ nyla::word::make("cast")     , nyla::TK_CAST        },
		{ nyla::word::make("null")     , nyla::TK_NULL        },
		{ nyla::word::make("new")      , nyla::TK_NEW         },
		{ nyla::word::make("var")      , nyla::TK_VAR         },
		{ nyla::word::make("import")   , nyla::TK_IMPORT      },
		{ nyla::word::make("this")     , nyla::TK_THIS        },
	};

	unidentified_ident  = nyla::word::make("__unidentified_ident");
	main_ident          = nyla::word::make("main");
	length_ident        = nyla::word::make("length");
	nyla::startup_ident = nyla::word::make("StartUp");
	nyla::memcpy_ident  = nyla::word::make("memcpy");

	for (auto it = reserved_words.begin(); it != reserved_words.end(); it++) {
		reserved_tag_to_words[it->second] = it->first;
	}
}

std::string nyla::token_tag_to_string(u32 tag) {
	switch (tag) {
	case TK_PLUS_EQ:      return "+=";
	case TK_MINUS_EQ:     return "-=";
	case TK_SLASH_EQ:     return "/=";
	case TK_STAR_EQ:      return "*=";
	case TK_MOD_EQ:       return "%=";
	case TK_AMP_EQ:       return "&=";
	case TK_BAR_EQ:       return "|=";
	case TK_CRT_EQ:       return "^=";
	case TK_BAR_BAR:      return "||";
	case TK_AMP_AMP:      return "&&";
	case TK_EXL_EQ:       return "!=";
	case TK_EQ_EQ:        return "==";
	case TK_LT_LT:        return "<<";
	case TK_GT_GT:        return ">>";
	case TK_LT_LT_EQ:     return "<<=";
	case TK_GT_GT_EQ:     return ">>=";
	case TK_UNKNOWN:      return "unknown";
	case TK_IDENTIFIER:   return "identifier";
	case TK_VALUE_INT:    return "value:int";
	case TK_VALUE_UINT:   return "value:uint";
	case TK_VALUE_LONG:   return "value:long";
	case TK_VALUE_ULONG:  return "value:ulong";
	case TK_VALUE_FLOAT:  return "value:float";
	case TK_VALUE_DOUBLE: return "value:double";
	case TK_VALUE_STRING8: return "value:string";
	case TK_VALUE_CHAR8:  return "value:char8";
	case TK_EOF:          return "eof";
	default:
		if (tag < TK_UNKNOWN)
			return std::string(1, tag);
		if (tag >= __TK_START_OF_KEYWORDS && tag <= __TK_END_OF_KEYWORDS) {
			return std::string(g_word_table->get_word(reserved_tag_to_words[tag]).c_str());
		}
	}
	return "";
}

std::string nyla::token::to_string() const {
	switch (tag) {
	case TK_UNKNOWN:      return "unknown";
	case TK_EOF:          return "eof";
	case TK_IDENTIFIER: {
		// TODO: special cases for escapes
		return g_word_table->get_word(word_key).c_str();
	}
	case TK_VALUE_INT:    return std::to_string(value_int);
	case TK_VALUE_UINT:   return std::to_string(value_uint);
	case TK_VALUE_LONG:   return std::to_string(value_long);
	case TK_VALUE_ULONG:  return std::to_string(value_ulong);
	case TK_VALUE_FLOAT:  return std::to_string(value_float);
	case TK_VALUE_DOUBLE: return std::to_string(value_double);
	case TK_VALUE_CHAR8: {
		// TODO: special cases for escapes
		return "(char)" + value_char8;
	}
	case TK_VALUE_STRING8: {
		std::string out_string;
		for (u32 i = 0; i < value_string8.size(); i++) {
			c8 ch = value_string8[i];
			if (ch == '\\') {
				ch = value_string8[i];
				out_string += "\\" + ch;
			} else {
				out_string += ch;
			}
		}
		return std::string("\"") + out_string + "\"";
	}
	default: {
		if (tag < TK_UNKNOWN)
			return std::string(1, tag);
		if (tag >= __TK_START_OF_KEYWORDS && tag <= __TK_END_OF_KEYWORDS) {
			return std::string(g_word_table->get_word(word_key).c_str());
		}
		if (tag >= __TK_START_OF_SYMBOLS && tag <= __TK_END_OF_SYMBOLS) {
			return token_tag_to_string(tag);
		}
		return std::string(1, tag);
	}
	}
}