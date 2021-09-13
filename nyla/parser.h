#ifndef NYLA_PARSER_H
#define NYLA_PARSER_H

#include "lexer.h"
#include "ast.h"
#include "compiler.h"
#include "sym_table.h"
#include "type.h"

#include <queue>

namespace nyla {

	class parser {
	public:

		parser(nyla::compiler& compiler, nyla::lexer& lexer,
			   nyla::log& log, nyla::sym_table* sym_table,
			   nyla::afile_unit* file_unit
			);
	
		// imports := import*
		void parse_imports();

		void resolve_imports();

		// file_unit := module*
		void parse_file_unit();

	private:

		// import := import (ident '.')* ident
		bool parse_import();

		// module := module ident '{' (field|function)* '}'
		nyla::amodule* parse_module();

		// modifiers := static | private | protected | public
		//                     | external | const 
		u32 parse_modifiers();

		// There is some extra info 
		// 
		// type := (byte | int | short | ...) '*'* ('[' expr? ']')*
		nyla::type_info parse_type();

		// function := ident '(' (var_decl ',')* var_decl?  ')' '{' stmts '}'
		nyla::afunction* parse_function(bool is_constructor, nyla::aannotation* annotation, u32 mods, const nyla::type_info& return_type_info);

		// variable_decl := modifiers type ident
		nyla::avariable_decl* parse_variable_decl(u32 mods,
			                                      const nyla::type_info& type,
			                                      nyla::aident* ident,
			                                      bool check_module_scope);
		nyla::avariable_decl* parse_variable_decl(u32 mods,
			                                      nyla::type* type,
			                                      const std::vector<nyla::aexpr*>& dim_sizes,
			                                      nyla::aident* ident,
			                                      bool check_module_scope);
		nyla::avariable_decl* parse_variable_decl(bool check_module_scope);

		// assignment_list := ('=' expr)? (',' ident ('=' expr)?)*
		void parse_assignment_list(nyla::avariable_decl* first_decl,
			                       std::vector<nyla::avariable_decl*>& declarations,
			                       bool check_module_scope);
		void parse_assignment_list(std::vector<nyla::avariable_decl*>& declarations,
			                       bool check_module_scope);
		void parse_assignment(nyla::avariable_decl* variable_decl);

		// stmts := stmt*
		void parse_stmts(std::vector<nyla::aexpr*>& stmts);

		// stmt := 
		void parse_stmt(std::vector<nyla::aexpr*>& stmts);

		// annotation := '@' ident
		nyla::aannotation* parse_annotation();

		// return := return expr? semis
		nyla::aexpr* parse_return();

		// for_loop := for variable_decl? ';' expression? ';' expression? ('{' stmts '}' | stmt)
		nyla::aexpr* parse_for_loop();

		// while_loop := ('{' stmts '}' | stmt)
		nyla::aexpr* parse_while_loop();

		// if expr ('{' stmts '}' | stmt) (else if expr ('{' stmts '}' | stmt))? else ('{' stmts '}' | stmt)
		nyla::aif* parse_if();

		// expression := unary op unary
		nyla::aexpr* parse_expression();
		nyla::aexpr* parse_expression(nyla::aexpr* lhs);
		nyla::aexpr* on_binary_op(const nyla::token& op_token,
			                      nyla::aexpr* lhs, nyla::aexpr* rhs);

		// unary := op factor
		nyla::aexpr* parse_unary();

		// factor := 
		nyla::aexpr* parse_factor();

		// ident '(' (expr ',')* expr? ')'
		nyla::afunction_call* parse_function_call(u32 name_key, const nyla::token& start_token);

		// dot_op := (factor '.')* factor
		nyla::aexpr* parse_dot_op(nyla::aexpr* factor);

		// array_access := ident ('[' expr ']')+
		nyla::aarray_access* parse_array_access(u32 name_key, const nyla::token& start_token);

		// array := '{' ((factor ',')+ factor)* '}'
		nyla::aexpr* parse_array();

		// scope := ('{' stmts '}' | stmt)
		void parse_scope(sym_scope*& sym_scope, std::vector<nyla::aexpr*>& stmts);

		// semis := ';'+
		void parse_semis();

		/*
		 * Utilities
		 */

		// Retreives the next token from either
		// the lexer or the m_saved_tokens and
		// stores it in m_current.
		void next_token();

		// Looks ahead n tokens and saves the tokens 
		// skipped into m_saved_tokens.
		// @param n The lookahead number to peek
		// @return  The token to peek at
		nyla::token peek_token(u32 n);

		// returns the word_key of the
		// word_table of the identifier
		// token.
		u32 parse_identifier();

		// Tries to recover parsing by finding
		// the next statement and skipping over
		// tokens that would be related to expressions
		void skip_recovery();

		// Checks a match between the current token and 
		// the token_tag.
		// @param token_tag The tag to compare against
		// @param consume If it should consume the token
		//                when it finds a match
		bool match(u32 token_tag, bool consume = true);

		template<typename node>
		node* make(ast_tag tag, const nyla::token& st_et) {
			return make<node>(tag, st_et, st_et);
		}

		template<typename node>
		node* make(ast_tag tag, const nyla::token& st, const nyla::token& et) {
			return make<node>(tag, st.line_num, st.spos, et.epos);
		}

		template<typename node>
		node* make(ast_tag tag, const nyla::ast_node* sn, const nyla::ast_node* en) {
			return make<node>(tag, sn->line_num, sn->spos, en->epos);
		}

		// Creates a new ast_node
		template<typename node>
		node* make(ast_tag tag, u32 line_num, u32 spos, u32 epos) {
			node* n = new node;
			n->tag      = tag;
			n->line_num = line_num;
			n->spos     = spos;
			n->epos     = epos;
			return n;
		}

		nyla::compiler&  m_compiler;
		nyla::lexer&     m_lexer;
		nyla::log&       m_log;
		nyla::sym_table* m_sym_table;

		   // Tokens saved during peeking
		std::queue<nyla::token> m_saved_tokens;
		   // Last token processed
		nyla::token m_prev_token;
		   // Currrent token being examined
		nyla::token m_current;
		   // Current file unit
		nyla::afile_unit* m_file_unit = nullptr;
		   // Current module
		nyla::amodule* m_module = nullptr;
		   // Current function
		nyla::afunction* m_function = nullptr;

		// Modules may be forward declared with types but
		// need to be stored to make sure the declaration is
		// found later
		struct forward_declared_type {
			nyla::type* type = nullptr;
			// Debugging info of all the locations
			// the type was found
			struct debug {
				u32 line_num;
				u32 spos, epos;
			};
			std::vector<debug> debug_locations;
		};

		nyla::type_table                               m_fd_type_table;
		std::unordered_map<u32, forward_declared_type> m_forward_declared_types;

		  // Variables used when parsing arrays
		u32  m_array_depth_ptr  = 0;
		u32  m_array_depth      = 0;
		bool m_array_too_deep   = false;

		// True when parsing fields of a module
		bool m_field_mode = false;

	};

}

#endif
