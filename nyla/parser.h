#pragma once

#include "lexer.h"
#include "ast.h"
#include "sym_table.h"

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
		///  
		/// </summary>
		nyla::afunction* parse_function_decl();

		/// <summary>
		/// 
		/// function = function_decl { expression* }
		/// 
		/// </summary>
		nyla::afunction* parse_function();

		nyla::type* parse_type();

		nyla::name parse_identifier();

		/// <summary>
		/// 
		/// Parses a type followed by a variable name.
		/// 
		/// variable_decl = type identifier
		/// 
		/// </summary>
		nyla::avariable_decl* parse_variable_decl();

		nyla::aexpr* parse_toplvl_expression();

		nyla::aexpr* parse_factor();

		nyla::aexpr* on_binary_op(u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs);

		nyla::aexpr* parse_expression();

		nyla::aexpr* parse_expression(nyla::aexpr* lhs);

	private:

		void next_token();

		void match(u32 token_tag);
		
		void match_semis();

		nyla::lexer     m_lexer;
		nyla::sym_table m_sym_table;

		nyla::token* m_current;
		bool         m_found_ret = false;

	};

}