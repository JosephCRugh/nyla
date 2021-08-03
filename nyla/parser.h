#pragma once

#include "lexer.h"
#include "ast.h"
#include "sym_table.h"
#include <queue>

namespace nyla {

	class parser {
	public:
		parser(nyla::lexer& lexer, nyla::sym_table& sym_table);

		/// <summary>
		/// 
		/// Parses the declaration of a function but not
		/// the definition of the function.
		/// 
		/// function_decl = type identifier '(' variable_decl* ')'
		/// </summary>
		nyla::afunction* parse_function_decl();

		/// <summary>
		/// function = function_decl '{' function_stmt* '}'
		/// </summary>
		nyla::afunction* parse_function();

		nyla::type* parse_type();

		nyla::name parse_identifier();

		nyla::ascope* parse_scope(bool pop_scope = true);

		/// <summary>
		/// Parses a type followed by a variable name.
		/// 
		/// variable_decl = type identifier
		/// </summary>
		/// <param name="type">The type of the variable read beforehand</param>
		nyla::avariable_decl* parse_variable_decl(nyla::type* type);

		/// <summary>
		/// Parses a type followed by a variable name.
		/// </summary>
		nyla::avariable_decl* parse_variable_decl();

		/// <summary>
		/// Parses a list of variable declarations and possible
		/// assignments seperated by commas.
		/// 
		/// variable_assign_list = type decl_part ';'
		///                      | type decl_part (',' decl_part)+ ';'
		/// decl_part = identifier ('=' expr)?
		/// </summary>
		std::vector<avariable_decl*> parse_variable_assign_list();

		/// <summary>
		/// Parses statements such as return, if, ect...
		/// </summary>
		nyla::aexpr* parse_function_stmt();

		/// <summary>
		/// for_loop = for assign_list? ';' expression? ';' expression? '{' function_stmt* '}'
		/// </summary>
		nyla::aexpr* parse_for_loop();

		nyla::aexpr* parse_factor();

		nyla::aexpr* on_binary_op(nyla::token* op_token,
				                  nyla::aexpr* lhs, nyla::aexpr* rhs);

		nyla::aexpr* parse_expression();

		nyla::aexpr* parse_expression(nyla::aexpr* lhs);

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
		void match(u32 token_tag, bool consume = true);
		
		/// <summary>
		/// semis = ';'+
		/// </summary>
		void match_semis();

		template<typename node>
		node* make_node(ast_tag tag, nyla::token* token) {
			return nyla::make_node<node>(tag, token->line_num);
		}

		nyla::lexer     m_lexer;
		nyla::sym_table m_sym_table;

		   /* Tokens saved during peeking */
		std::queue<nyla::token*> m_saved_tokens;

		nyla::token* m_current   = nullptr;
		bool         m_found_ret = false;

	};

}