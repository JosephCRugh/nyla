#include "parser.h"

#include "log.h"
#include <unordered_map>

using namespace nyla;

parser::parser(nyla::lexer& lexer, nyla::sym_table& sym_table)
	: m_lexer(lexer), m_sym_table(sym_table)
{
	// Read first token to get things started.
	next_token();
}

nyla::afunction* parser::parse_function_decl() {
	// TODO: parse function modifiers (public, private access, ect..)
	nyla::type* return_type = parse_type();

	nyla::afunction* function = nyla::make_node<nyla::afunction>(AST_FUNCTION);
	function->return_type = return_type;
	function->name        = parse_identifier();
	
	match('(');
	if (m_current->tag != ')') {
		// Assuming parameters.

		bool more_parameters = false;
		do {
			nyla::avariable_decl* parameter = parse_variable_decl();
			function->parameters.push_back(parameter);

			more_parameters = m_current->tag == ',';
			if (more_parameters) {
				next_token(); // consuming ,
			}
		} while (more_parameters);
	}
	match(')');

	return function;
}

nyla::afunction* nyla::parser::parse_function() {
	nyla::afunction* function = parse_function_decl();
	function->scope = m_sym_table.push_scope();
	match('{');
	m_found_ret = false;
	while (m_current->tag != '}' && m_current->tag != '\0') {
		function->scope->stmts.push_back(parse_function_stmt());
	}
	match('}');
	if (!m_found_ret && function->return_type->tag == TYPE_VOID) {
		nyla::areturn* void_ret = nyla::make_node<nyla::areturn>(AST_RETURN);
		function->scope->stmts.push_back(void_ret);
	}
	m_sym_table.pop_scope();
	return function;
}

nyla::type* parser::parse_type() {
	type_tag elem_tag = type_tag::ERROR;
	switch (m_current->tag) {
	case TK_TYPE_BYTE:   elem_tag = TYPE_BYTE;   next_token(); break;
	case TK_TYPE_SHORT:  elem_tag = TYPE_SHORT;  next_token(); break;
	case TK_TYPE_INT:    elem_tag = TYPE_INT;    next_token(); break;
	case TK_TYPE_LONG:   elem_tag = TYPE_LONG;   next_token(); break;
	case TK_TYPE_FLOAT:  elem_tag = TYPE_FLOAT;  next_token(); break;
	case TK_TYPE_DOUBLE: elem_tag = TYPE_DOUBLE; next_token(); break;
	case TK_TYPE_BOOL:   elem_tag = TYPE_BOOL;   next_token(); break;
	case TK_TYPE_VOID:   elem_tag = TYPE_VOID;   next_token(); break;
	default:
		nyla::g_log->error(ERR_CANNOT_RESOLVE_TYPE, m_lexer.get_line_num(),
			error_data::make_str_literal_load(m_current->to_string().c_str()));
		break;
	}
	return new type(elem_tag);
}

nyla::name parser::parse_identifier() {
	if (m_current->tag == TK_IDENTIFIER) {
		nyla::name name = dynamic_cast<nyla::word_token*>(m_current)->name;
		next_token(); // Consuming identifier
		return name;
	} else {
		nyla::g_log->error(ERR_EXPECTED_IDENTIFIER, m_lexer.get_line_num(),
			error_data::make_empty_load());
	}
	return nyla::name();
}

nyla::avariable_decl* nyla::parser::parse_variable_decl() {
	return parse_variable_decl(parse_type());
}

nyla::avariable_decl* nyla::parser::parse_variable_decl(nyla::type* type) {
	nyla::avariable* variable =
		nyla::make_node<nyla::avariable>(AST_VARIABLE);
	variable->type = type;
	variable->name = parse_identifier();
	nyla::avariable_decl* variable_delc =
		nyla::make_node<nyla::avariable_decl>(AST_VARIABLE_DECL);
	variable_delc->variable = variable;
	return variable_delc;
}

std::vector<avariable_decl*> nyla::parser::parse_variable_assign_list() {
	std::vector<avariable_decl*> decl_list;
	nyla::type* fst_type = parse_type();
	bool more_decls = false;
	do {
		nyla::avariable_decl* var_decl = parse_variable_decl(fst_type);
		if (m_current->tag != ';' && m_current->tag != ',')
			var_decl->assignment = parse_expression(var_decl->variable);
		decl_list.push_back(var_decl);
		more_decls = m_current->tag == ',';
		if (more_decls) {
			next_token(); // consuming ,
		}
	} while (more_decls);
	return decl_list;
}

nyla::aexpr* parser::parse_function_stmt() {
	switch (m_current->tag) {
	case TK_RETURN: {
		next_token(); // Consuming return token.
		nyla::areturn* ret = nyla::make_node<nyla::areturn>(AST_RETURN);
		m_found_ret = true;
		if (m_current->tag == ';') {
			match_semis();
			// Must be a void return.
			return ret;
		}
		ret->value = parse_expression();
		match_semis();
		return ret;
	}
	case TK_FOR: {
		return parse_for_loop();
	}
	case TK_TYPE_BYTE:  case TK_TYPE_SHORT: 
	case TK_TYPE_INT:   case TK_TYPE_LONG:  
	case TK_TYPE_FLOAT: case TK_TYPE_DOUBLE:
	case TK_TYPE_BOOL:  case TK_TYPE_VOID: {
		nyla::avariable_decl* variable_decl = parse_variable_decl();
		// int a  ;
		//        ^
		//        |-- no assignment
		if (m_current->tag != ';') {
			match('=', false);
			variable_decl->assignment = parse_expression(variable_decl->variable);
		}
		match_semis();
		return variable_decl;
	}
	case TK_IDENTIFIER: {
		// If followed by another identifier the first
		// identifier is assumed to be a type. Otherwise
		// it is assumed to be part of an expression
		nyla::token* next_token = peek_token(1);
		if (next_token->tag == TK_IDENTIFIER) {
			// TODO: Handle variable declaration!
		} else {
			nyla::aexpr* expr = parse_expression();
			match_semis();
			return expr;
		}
		break;
	}
	case ';': {
		match_semis();
		return parse_function_stmt();
	}
	default: {
		nyla::g_log->error(ERR_EXPECTED_STMT, m_lexer.get_line_num(),
			error_data::make_token_tag_load(m_current->tag));
		next_token(); // Consuming the token that doesn't belong
		return nullptr;
	}
	}
}

nyla::aexpr* nyla::parser::parse_for_loop() {
	next_token(); // Consuming for token.
	nyla::afor_loop* loop = nyla::make_node<nyla::afor_loop>(AST_FOR_LOOP);
	if (m_current->tag != ';') {
		loop->declarations = parse_variable_assign_list();
	}
	match(';');
	loop->cond = parse_expression();
	match(';');
	loop->post = parse_expression();
	match('{');
	while (m_current->tag != '}' && m_current->tag != '\0') {
		loop->body.push_back(parse_function_stmt());
	}
	match('}');
	return loop;
}

std::unordered_set<u32> binary_ops_set = {
	'+', '-', '*', '/', '=', '<', '>',
	TK_PLUS_EQ, TK_MINUS_EQ, TK_DIV_EQ, TK_MUL_EQ,
};
std::unordered_map<u32, u32> precedence = {
	{ '=', 0 },
	{ TK_PLUS_EQ, 0 },
	{ TK_MINUS_EQ, 0 },
	{ TK_DIV_EQ, 0 },
	{ TK_MUL_EQ, 0 },

	{ '<', 1 },
	{ '>', 1 },

	{ '+', 2 },
	{ '-', 2 },

	{ '*', 3 },
	{ '/', 3 },
};

nyla::aexpr* nyla::parser::parse_factor() {
	switch (m_current->tag) {
	case TK_VALUE_INT: {
		nyla::anumber* number = nyla::make_node<nyla::anumber>(AST_VALUE_INT);
		number->value_int = dynamic_cast<nyla::num_token*>(m_current)->value_int;
		next_token(); // Consuming the number token.
		return number;
	}
	case TK_IDENTIFIER: {
		nyla::name name = parse_identifier();
		switch (m_current->tag) { 
		case  '(':
			// TODO: function call
			break;
		case '.':
			// TODO: dot reference operator
			break;
		default:
			break;
		}
		nyla::avariable* variable = nyla::make_node<nyla::avariable>(AST_VARIABLE);
		variable->name = name;
		// The type is determined during symantic analysis.
		return variable;
	}
	default: {
		nyla::g_log->error(ERR_EXPECTED_FACTOR, m_lexer.get_line_num(),
			error_data::make_token_tag_load(m_current->tag));
		next_token(); // consuming the unknown factor
		return nullptr;
	}
	}
}

nyla::aexpr* parser::on_binary_op(u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs) {
	// Subdivides an equal + operator into seperate nodes.
	static auto equal_and_op_apply = [](u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs) -> nyla::abinary_op* {
		nyla::abinary_op* eq_op = nyla::make_node<nyla::abinary_op>(AST_BINARY_OP);
		nyla::abinary_op* op_op = nyla::make_node<nyla::abinary_op>(AST_BINARY_OP);
		eq_op->op  = '=';
		op_op->op  = op;
		op_op->lhs = lhs;
		op_op->rhs = rhs;
		eq_op->lhs = lhs;
		eq_op->rhs = op_op;
		return eq_op;
	};
	switch (op) {
	case TK_PLUS_EQ:  return equal_and_op_apply('+', lhs, rhs);
	case TK_MINUS_EQ: return equal_and_op_apply('-', lhs, rhs);
	case TK_DIV_EQ:   return equal_and_op_apply('/', lhs, rhs);
	case TK_MUL_EQ:   return equal_and_op_apply('*', lhs, rhs);
	default: {
		// Letting folding occure via LLVM folding operation. Could handle it here
		// which may result in faster compilation.
		nyla::abinary_op* binary_op = nyla::make_node<nyla::abinary_op>(AST_BINARY_OP);
		binary_op->lhs = lhs;
		binary_op->rhs = rhs;
		binary_op->op = op;
		return binary_op;
	}
	}
}

nyla::aexpr* parser::parse_expression() {
	return parse_expression(parse_factor());
}

nyla::aexpr* parser::parse_expression(nyla::aexpr* lhs) {
	
	nyla::token* op = m_current;
	nyla::token* next_op;

	struct stack_unit {
		u32          op;  // operator kind
		nyla::aexpr* expr;
	};
	std::stack<stack_unit> op_stack;

	while (binary_ops_set.find(op->tag) != binary_ops_set.end()) {
		next_token(); // Consuming the operator.
	
		nyla::aexpr* rhs = parse_factor();
		
		next_op = m_current;
		bool more_operators = binary_ops_set.find(next_op->tag) != binary_ops_set.end();
		if (more_operators && precedence[next_op->tag] > precedence[op->tag]) {
			// Delaying the operation until later since the next operator has a
			// higher precedence.
			stack_unit unit = { op->tag, lhs };
			op_stack.push(unit);
			lhs = rhs;
			op  = next_op;

		} else {
			// Apply the binary operator!
			nyla::aexpr* res = on_binary_op(op->tag, lhs, rhs);
			lhs = res;

			while (!op_stack.empty()) {
				rhs = lhs;
				stack_unit unit = op_stack.top();
				// Still possible to have the right side have higher precedence.
				if (more_operators && precedence[next_op->tag] > precedence[unit.op]) {
					lhs = rhs;
					op  = next_op;
					break;
				}

				op_stack.pop();
				lhs = unit.expr;
				
				// Apply the binary operator!
				nyla::aexpr* res = on_binary_op(unit.op, lhs, rhs);
				lhs = res;
			}
			op = m_current;
		}
	}

	return lhs;
}

nyla::token* parser::peek_token(u32 n) {
	if (n == 0) {
		assert(!"There is no reason to peek a single token");
	}
	for (u32 i = 0; i < n; i++) {
		m_saved_tokens.push(m_lexer.next_token());
	}
	return m_saved_tokens.back();
}

void parser::next_token() {
	if (!m_saved_tokens.empty()) {
		m_current = m_saved_tokens.front();
		m_saved_tokens.pop();
	} else {
		m_current = m_lexer.next_token();
	}
}

void nyla::parser::match(u32 token_tag, bool consume) {
	if (m_current->tag == token_tag) {
		if (consume) {
			next_token();
		}
	} else {
		nyla::g_log->error(ERR_EXPECTED_TOKEN, m_lexer.get_line_num(),
			expected_token_data::make_expect_tk_tag(token_tag, m_current->tag));
	}
}

void parser::match_semis() {
	match(';');
	while (m_current->tag == ';') {
		next_token(); // Consuming any additional semis.
	}
}

