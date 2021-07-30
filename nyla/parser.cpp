#include "parser.h"

using namespace nyla;

parser::parser(nyla::lexer& lexer)
	: m_lexer(lexer)
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
	match('{');
	// TODO: parse the body expressions
	match('}');
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


