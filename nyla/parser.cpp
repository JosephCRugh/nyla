#include "parser.h"

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
	while (m_current->tag != '}') {
		function->scope->expressions.push_back(parse_toplvl_expression());
	}
	match('}');
	if (!m_found_ret && function->return_type->tag == TYPE_VOID) {
		nyla::areturn* void_ret = nyla::make_node<nyla::areturn>(AST_RETURN);
		function->scope->expressions.push_back(void_ret);
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
		// TODO: produce error!
		std::cout << "Invalid type.. " << m_current->tag << std::endl;
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
		// TODO: produce error.
		std::cout << "Expected Identifier!" << std::endl;
	}
	return nyla::name();
}

nyla::avariable_decl* nyla::parser::parse_variable_decl() {
	nyla::type* type = parse_type();
	nyla::avariable* variable =
		nyla::make_node<nyla::avariable>(AST_VARIABLE);
	variable->type = type;
	variable->name = parse_identifier();
	nyla::avariable_decl* variable_delc =
		nyla::make_node<nyla::avariable_decl>(AST_VARIABLE_DECL);
	variable_delc->variable = variable;
	return variable_delc;
}

nyla::aexpr* parser::parse_toplvl_expression() {
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
	case TK_TYPE_BYTE:  case TK_TYPE_SHORT: 
	case TK_TYPE_INT:   case TK_TYPE_LONG:  
	case TK_TYPE_FLOAT: case TK_TYPE_DOUBLE:
	case TK_TYPE_BOOL:  case TK_TYPE_VOID:
	case TK_IDENTIFIER: {
		nyla::avariable_decl* variable_decl = parse_variable_decl();
		// int a  ;
		//        ^
		//        |-- no assignment
		if (m_current->tag != ';') {
			switch (m_current->tag) {
			case '=':
				break;
			default:
				// produce an error!
				break;
			}

			variable_decl->assignment = parse_expression(variable_decl->variable);
		}
		match_semis();
		return variable_decl;
	}
	}
	return nullptr;
}

std::unordered_set<u32> binary_ops_set = {
	'+', '-', '*', '/', '='
};
std::unordered_map<u32, u32> precedence = {
	{ '=', 0 },

	{ '+', 1 },
	{ '-', 1 },

	{ '*', 2 },
	{ '/', 2 },
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
	}
	return nullptr;
}

nyla::aexpr* parser::on_binary_op(u32 op, nyla::aexpr* lhs, nyla::aexpr* rhs) {
	// Letting folding occure via LLVM folding operation. Could handle it here
	// which may result in faster compilation.
	nyla::abinary_op* binary_op = nyla::make_node<nyla::abinary_op>(AST_BINARY_OP);
	binary_op->lhs = lhs;
	binary_op->rhs = rhs;
	binary_op->op = op;
	return binary_op;
}

nyla::aexpr* parser::parse_expression() {
	return parse_expression(parse_factor());
}

nyla::aexpr* parser::parse_expression(nyla::aexpr* lhs) {
	
	nyla::token* op = m_current;
	nyla::token* next_op;

	struct stack_unit {
		u32           op;  // operator kind
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
				if (more_operators && precedence[next_op->tag] > precedence[op->tag]) {
					lhs = rhs;
					op  = next_op;
					break;
				}

				op_stack.pop();
				lhs = unit.expr;
				// TODO: Might need to have the op = unit.op

				// Apply the binary operator!
				nyla::aexpr* res = on_binary_op(op->tag, lhs, rhs);
				lhs = res;
			}
			op = m_current;
		}
	}

	return lhs;
}

void parser::next_token() {
	m_current = m_lexer.next_token();
}

void nyla::parser::match(u32 token_tag) {
	if (m_current->tag == token_tag) {
		next_token();
	} else {
		// TODO: produce error
		std::cout << "Failed to match " << (c8)token_tag << std::endl;
	}
}

void parser::match_semis() {
	match(';');
	while (m_current->tag == ';') {
		next_token(); // Consuming any additional semis.
	}
}

