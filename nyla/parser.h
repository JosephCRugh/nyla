#pragma once

#include "lexer.h"
#include "ast.h"
#include "sym_table.h"
#include "log.h"
#include <queue>

static constexpr u32 MAX_MEM_DEPTH = 8;

namespace nyla {

	class parser {
	public:
		parser(nyla::lexer& lexer, nyla::sym_table& sym_table, nyla::log& log);

		nyla::afile_unit* parse_file_unit();

		/// <summary>
		/// 
		/// Parses the declaration of a function but not
		/// the definition of the function.
		/// 
		/// function_decl = type identifier '(' variable_decl* ')'
		/// </summary>
		nyla::afunction* parse_function_decl(bool make_new_scope = true);

		/// <summary>
		/// function = function_decl '{' function_stmt* '}'
		/// </summary>
		nyla::afunction* parse_function();

		nyla::afunction* parse_external_function();

		nyla::type* parse_type();

		nyla::name parse_identifier();

		/// <summary>
		/// Parses a type followed by a variable name.
		/// 
		/// variable_decl = type identifier
		/// </summary>
		/// <param name="type">The type of the variable read beforehand</param>
		nyla::avariable_decl* parse_variable_decl(nyla::type* type, bool has_scope = true);

		/// <summary>
		/// Parses a type followed by a variable name.
		/// </summary>
		nyla::avariable_decl* parse_variable_decl(bool has_scope = true);

		/// <summary>
		/// Parses a list of variable declarations and possible
		/// assignments seperated by commas.
		/// 
		/// variable_assign_list = type decl_part ';'
		///                      | type decl_part (',' decl_part)+ ';'
		/// decl_part = identifier ('=' expr)?
		/// </summary>
		std::vector<avariable_decl*> parse_variable_assign_list();

		nyla::aexpr* parse_function_stmt();

		/// <summary>
		/// Parses statements such as return, if, ect...
		/// </summary>
		nyla::aexpr* parse_function_stmt_rest();

		/// <summary>
		/// for_loop = for assign_list? ';' expression? ';' expression? '{' function_stmt* '}'
		/// </summary>
		nyla::aexpr* parse_for_loop();

		nyla::aexpr* parse_unary();

		nyla::aexpr* parse_factor();

		nyla::aexpr* on_binary_op(nyla::token* op_token,
				                  nyla::aexpr* lhs, nyla::aexpr* rhs);

		nyla::aexpr* parse_expression();

		nyla::aexpr* parse_expression(nyla::aexpr* lhs);

		nyla::afunction_call* parse_function_call(nyla::name& name, nyla::token* start_token);

		nyla::aarray* parse_array();

		nyla::abinary_op* parse_dot_op(nyla::aexpr* lhs);

		nyla::aarray_access* parse_array_access(nyla::name& name, nyla::token* start_token);

		std::vector<nyla::token*> get_processed_tokens() { return m_processed_tokens; }

		void parse_dll_import();

	private:

		/// <summary>
		/// Looks ahead n tokens and saves the tokens
		/// skipped into m_saved_tokens.
		/// </summary>
		/// <param name="n">The lookahead number to peek</param>
		/// <returns>The token to peek at.</returns>
		nyla::token* peek_token(u32 n);

		void next_token();

		/// <summary>
		/// Checks a match between the current token and
		/// the token_tag.
		/// </summary>
		/// <param name="token_tag">The tag to compare against</param>
		/// <param name="consume">If the current token should be consumed
		/// given a successful match</param>
		bool match(u32 token_tag, bool consume = true);
		
		/// <summary>
		/// semis = ';'+
		/// </summary>
		void match_semis();

		void skip_recovery();

		template<typename node>
		node* make_node(ast_tag tag, nyla::token* st, nyla::token* et) {
			return nyla::make_node<node>(tag, st, et);
		}

		nyla::lexer&     m_lexer;
		nyla::sym_table& m_sym_table;
		nyla::log&       m_log;

		   /* Tokens saved during peeking */
		std::queue<nyla::token*> m_saved_tokens;

		  /* Storing tokens so they can be cleaned up */
		std::vector<nyla::token*> m_processed_tokens;

		/// <summary>
		/// Looks back at the previously traversed
		/// tokens and returns the nth token.
		/// </summary>
		/// <param name="n">the number of tokens to look back at.</param>
		nyla::token* look_back(s32 n);

		void produce_error(error_tag tag, error_data* data,
			               nyla::token* start_token, nyla::token* end_token);

		u64              m_max_array_depths[MAX_MEM_DEPTH] = {0};
		u32              m_array_depth_ptr  = 0;
		u32              m_array_depth      = 0;
		bool             m_array_too_deep   = false;

		bool             m_recovery_accept_brackets = false;
		bool             m_error_recovery   = false;
		nyla::token*     m_current          = nullptr;
		bool             m_found_ret        = false;

	};

}