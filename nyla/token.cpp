#include "token.h"

std::string nyla::token_tag_to_string(u32 tag) {
	switch (tag) {
	case TK_PLUS_EQ:    return "+=";
	case TK_MINUS_EQ:   return "-=";
	case TK_DIV_EQ:     return "/=";
	case TK_MUL_EQ:     return "*=";
	case TK_IDENTIFIER: return "identifier";
	default:
		if (tag < TK_UNKNOWN)
			return std::string(1, tag);
	}
	return "";
}