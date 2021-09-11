#ifndef NYLA_LOG_H
#define NYLA_LOG_H

#include "source.h"
#include "tokens.h"
#include "type.h"

namespace nyla {

	enum error_tag {
		// Global Errors
		ERR_FAILED_TO_READ_SOURCE_DIRECTORY,
		ERR_CONFLICTING_INTERNAL_PATHS,
		ERR_FAILED_TO_READ_FILE,
		ERR_MULTIPLE_MAIN_FUNCTIONS_IN_PROGRAM,
		ERR_MAIN_FUNCTION_NOT_FOUND,

		// Lexer Errors
		ERR_UNKNOWN_CHARACTER,
		ERR_IDENTIFIER_TOO_LONG,
		ERR_INT_TOO_LARGE,
		ERR_FLOAT_TOO_LARGE,
		ERR_MISSING_CLOSING_QUOTE,
		ERR_MISSING_CLOSING_CHAR_QUOTE,
		ERR_INVALID_ESCAPE_SEQUENCE,
		ERR_UNCLOSED_COMMENT,

		// Parser Errors
		ERR_EXPECTED_IDENTIFIER,
		ERR_EXPECTED_TOKEN,
		ERR_MODULE_REDECLARATION,
		ERR_EXPECTED_MODULE,
		ERR_EXPECTED_IMPORT,
		ERR_EXPECTED_MODULE_STMT,
		ERR_VARIABLE_REDECLARATION,
		ERR_VARIABLE_HAS_VOID_TYPE,
		ERR_FUNCTION_REDECLARATION,
		ERR_EXPECTED_FACTOR,
		ERR_EXPECTED_STMT,
		ERR_TOO_MANY_PTR_SUBSCRIPTS,
		ERR_TOO_MANY_ARR_SUBSCRIPTS,
		ERR_ARRAY_TOO_DEEP,
		ERR_DUPLICATE_MODULE_ALIAS,
		ERR_DUPLICATE_IMPORT,
		ERR_COULD_NOT_FIND_MODULE_TYPE,
		ERR_ILLEGAL_MODIFIERS,
		ERR_EXPECTED_VALID_TYPE,
		ERR_CAN_ONLY_HAVE_ONE_ACCESS_MOD,
		ERR_MODULE_FOR_ALIAS_NOT_FOUND,
		
		// Analysis Errors
		ERR_CANNOT_FIND_IMPORT,
		ERR_CONFLICTING_IMPORTS,
		ERR_MODULE_ALIAS_NAME_ALREADY_USED,
		ERR_COULD_NOT_FIND_MODULE_FOR_TYPE,
		ERR_CANNOT_ASSIGN,
		ERR_UNDECLARED_VARIABLE,
		ERR_USE_OF_VARIABLE_BEFORE_DECLARATION,
		ERR_OP_CANNOT_APPLY_TO,
		ERR_EXPECTED_BOOL_COND,
		ERR_COULD_NOT_FIND_CONSTRUCTOR,
		ERR_COULD_NOT_FIND_FUNCTION,
		ERR_ARR_TOO_MANY_INIT_VALUES,
		ERR_ELEMENT_OF_ARRAY_NOT_COMPATIBLE_WITH_ARRAY,
		ERR_FUNCTION_EXPECTS_RETURN,
		ERR_STMTS_AFTER_RETURN,
		ERR_DOT_OP_EXPECTS_VARIABLE,
		ERR_TYPE_DOES_NOT_HAVE_FIELD,
		ERR_ARRAY_ACCESS_ON_INVALID_TYPE,
		ERR_ARRAY_ACCESS_EXPECTS_INT,
		ERR_TOO_MANY_ARRAY_ACCESS_INDEXES,
		ERR_RETURN_VALUE_NOT_COMPATIBLE_WITH_RETURN_TYPE,
		ERR_FUNCTION_EXPECTS_RETURN_VALUE,
		ERR_CALLED_NON_STATIC_FUNC_FROM_STATIC,
		ERR_CIRCULAR_FIELDS,

	};

	struct file_location;
	struct aident;

	struct err_file_locations {
		const nyla::file_location* loc1;
		const nyla::file_location* loc2;
	};

	struct err_expected_token {
		u32                expected_tag;
		const nyla::token& found_token;
	};

	struct err_conflicting_import {
		u32         module_name_key;
		std::string internal_path1;
		std::string internal_path2;
	};

	struct err_types {
		nyla::type* t1;
		nyla::type* t2;
	};

	struct err_op_cannot_apply {
		u32         op;
		nyla::type* type;
	};

	struct err_idents {
		nyla::aident* ident1;
		nyla::aident* ident2;
	};

	struct err_words {
		u32 word_key1;
		u32 word_key2;
	};

	struct err_func_decl {
		u32 word_key;
		u32 line_num;
	};

	struct err_string {
		std::string str;
	};

	struct afunction_call;
	struct ast_node;

	struct error_payload {

		static error_payload none() {
			return {};
		}

		static error_payload word(u32 word_key) {
			error_payload payload;
			payload.d_word_key = word_key;
			return payload;
		}

		static error_payload file_locations(err_file_locations&& d_file_locations) {
			error_payload payload;
			payload.d_file_locations = &d_file_locations;
			return payload;
		}

		static error_payload expected_token(err_expected_token&& expected_token) {
			error_payload payload;
			payload.d_expected_token = &expected_token;
			return payload;
		}

		static error_payload conflicting_import(err_conflicting_import&& conflicting_import) {
			error_payload payload;
			payload.d_conflicting_import = &conflicting_import;
			return payload;
		}

		static error_payload types(err_types&& types) {
			error_payload payload;
			payload.d_types = &types;
			return payload;
		}

		static error_payload op_cannot_apply(err_op_cannot_apply&& op_cannot_apply) {
			error_payload payload;
			payload.d_op_cannot_apply = &op_cannot_apply;
			return payload;
		}

		static error_payload function_call(afunction_call* function_call) {
			error_payload payload;
			payload.d_function_call = function_call;
			return payload;
		}

		static error_payload idents(err_idents&& err_idents) {
			error_payload payload;
			payload.d_idents = &err_idents;
			return payload;
		}

		static error_payload words(err_words&& words) {
			error_payload payload;
			payload.d_words = &words;
			return payload;
		}

		static error_payload func_decl(err_func_decl&& func_decl) {
			error_payload payload;
			payload.d_func_decl = &func_decl;
			return payload;
		}

		static error_payload string(err_string&& string) {
			error_payload payload;
			payload.d_string = &string;
			return payload;
		}

		union {
			u32                     d_word_key;
			err_file_locations*     d_file_locations;
			err_expected_token*     d_expected_token;
			err_conflicting_import* d_conflicting_import;
			err_types*              d_types;
			err_op_cannot_apply*    d_op_cannot_apply;
			afunction_call*         d_function_call;
			err_idents*             d_idents;
			err_words*              d_words;
			err_func_decl*          d_func_decl;
			err_string*             d_string;
		};
	};

	class log {
	public:

		log() {}

		log(nyla::source& source)
			: m_source(&source) {}

		void global_error(error_tag tag);

		void global_error(error_tag tag, const error_payload& payload);

		bool has_errors() { return m_num_errors != 0; }

		/*
		 * Send an error message that formats
		 * an error message showing the user
		 * the area the error started and ended.
		 */
		void err(error_tag tag, const error_payload& payload,
			     u32 line_num,
			     u32 spos,
			     u32 epos);

		void err(error_tag tag,
			     u32 line_num,
			     u32 spos,
			     u32 epos);

		void err(error_tag tag, const error_payload& payload,
			     const nyla::token& st,
			     const nyla::token& et);

		void err(error_tag tag,
			     const nyla::token& st,
			     const nyla::token& et);

		void err(error_tag tag, const nyla::token& st_et);

		void err(error_tag tag, const error_payload& payload,
			     const nyla::token& st_et);

		void err(error_tag tag, const error_payload& payload,
			     const nyla::ast_node* node);

		void err(error_tag tag, const nyla::ast_node* node);

		void set_file_path(const std::string& file_path) { m_file_path = file_path; }

	private:

		std::string word_as_string(u32 word_key);

		// Spaces of: "path:number error: "
		std::string header_spaces(u32 line_num);

		std::string   m_file_path;
		nyla::source* m_source     = nullptr;
		u32           m_num_errors = 0;
	};

}

#endif