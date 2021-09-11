#ifndef NYLA_TOKENS_H
#define NYLA_TOKENS_H

#include "types_ext.h"
#include <string>
#include <unordered_map>

namespace nyla {

	enum token_tag {
		/* Ascii Characters reserved < 257 */

		TK_UNKNOWN = 257,

		// === Keywords === \\

		__TK_START_OF_KEYWORDS = TK_UNKNOWN + 1,

		TK_MODULE,

		// Types

		   // Integers
		TK_TYPE_BYTE,
		TK_TYPE_SHORT,
		TK_TYPE_INT,
		TK_TYPE_LONG,
		TK_TYPE_UBYTE,
		TK_TYPE_USHORT,
		TK_TYPE_UINT,
		TK_TYPE_ULONG,
		    // Characters  (Anything above 8 is unicode extension)
		TK_TYPE_CHAR8 ,
		TK_TYPE_CHAR16,
		TK_TYPE_CHAR32,
		   // Floats
		TK_TYPE_FLOAT,
		TK_TYPE_DOUBLE,
		   // Other
		TK_TYPE_BOOL,
		TK_TYPE_VOID,

		TK_TRUE,
		TK_FALSE,

		// Control Flow

		TK_FOR,
		TK_WHILE,
		TK_DO,
		TK_IF,
		TK_ELSE,
		TK_SWITCH,
		TK_CONTINUE,
		TK_BREAK,
		TK_RETURN,

		__TK_END_OF_KEYWORDS = TK_RETURN,

		// === Values === \\

		TK_VALUE_INT,
		TK_VALUE_UINT,
		TK_VALUE_LONG,
		TK_VALUE_ULONG,

		TK_VALUE_FLOAT,
		TK_VALUE_DOUBLE,
		TK_VALUE_STRING8,

		TK_VALUE_CHAR8,

		// === Symbols === \\

		__TK_START_OF_SYMBOLS = TK_VALUE_CHAR8 + 1,

		TK_MINUS_GT,    // ->
		TK_PLUS_PLUS,   // ++
		TK_MINUS_MINUS, // --
		TK_PLUS_EQ,     // +=
		TK_MINUS_EQ,    // -=
		TK_SLASH_EQ,    // /=
		TK_STAR_EQ,     // *=
		TK_MOD_EQ,      // %=
		TK_AMP_EQ,      // &=
		TK_BAR_EQ,      // |=
		TK_CRT_EQ,      // ^=
		TK_BAR_BAR,     // ||
		TK_AMP_AMP,     // &&
		TK_EXL_EQ,      // !=
		TK_EQ_EQ,	    // ==
		TK_LT_LT,       // <<
		TK_GT_GT,       // >>
		TK_LT_EQ,       // <=
		TK_GT_EQ,       // >=
		TK_QQQ,         // ???
		TK_LT_LT_EQ,    // <<=
		TK_GT_GT_EQ,    // >>=

		__TK_END_OF_SYMBOLS = TK_GT_GT_EQ,

		// === Other === \\

		TK_STATIC,
		TK_PRIVATE,
		TK_PROTECTED,
		TK_PUBLIC,
		TK_EXTERNAL,
		TK_CONST,
		TK_IMPORT,

		TK_IDENTIFIER,
		TK_CAST,
		TK_NULL,
		TK_NEW,
		TK_VAR,

		TK_EOF

	};

	// Maps between word's keys and the token_tag of
	// a reserved word.
	extern std::unordered_map<u32, nyla::token_tag> reserved_words;

	// Maps reseved token_tag numbers to the key indexes.
	extern std::unordered_map<u32, u32> reserved_tag_to_words;

	// Reserved word "__unidentified_ident" in cases
	// where an identifier cannot be found during parsing.
	extern u32 unidentified_ident;

	// The word "main" for identifying main functions for entry
	// points into the program.
	extern u32 main_ident;

	// The word "length" for identifying array lengths.
	extern u32 length_ident;

	// Initialize the reserved_words set
	void setup_tokens();

	std::string token_tag_to_string(u32 tag);

	struct token {
		u32 tag;
		u32 line_num;   // The line the token first appeard on
		u32 spos, epos; // Start and end position in the buffer
		                // End position is actually 1 character past end.
		                // If the range is 0-1 then it encompesses only character
		                // at index 0.
		
		std::string to_string() const;

		union {
			s32    value_int;
			u32    value_uint;
			s64    value_long;
			u64    value_ulong;
			c8     value_char8;
			float  value_float;
			double value_double;
			u32    word_key;    // Key into the word table.
		};
		std::string  value_string8;
		
	};

}

#endif