#pragma once

#include "lexer.h"
#include "ast.h"

namespace nyla {

	class parser {
	public:
		parser(nyla::lexer& lexer);

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

	private:

		void next_token();

		void match(u32 token_tag);
		
		nyla::lexer  m_lexer;

		nyla::token* m_current;
	};

}