#include "gen_llvm.h"

using namespace nyla;

llvm::LLVMContext* nyla::llvm_context;
llvm::IRBuilder<>* nyla::llvm_builder;
llvm::Module*      nyla::working_llvm_module;

void nyla::init_llvm() {
	nyla::llvm_context = new llvm::LLVMContext;
	nyla::llvm_builder = new llvm::IRBuilder<>(*llvm_context);
}

void nyla::init_native_target() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();
}

void nyla::clean_llvm() {
	delete nyla::llvm_context;
	delete nyla::llvm_builder;
}

bool nyla::write_obj_file(c_string fname) {

	auto target_triple = llvm::sys::getDefaultTargetTriple();
	nyla::working_llvm_module->setTargetTriple(target_triple);

	std::string target_error;
	auto target = llvm::TargetRegistry::lookupTarget(target_triple, target_error);

	if (!target) {
		llvm::errs() << target_error;
		return false;
	}

	auto CPU = "generic";
	auto features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	auto target_machine =
		target->createTargetMachine(target_triple, CPU, features, opt, RM);

	nyla::working_llvm_module->setDataLayout(target_machine->createDataLayout());

	std::error_code err;
	llvm::raw_fd_ostream dest(fname, err, llvm::sys::fs::OF_None);

	if (err) {
		llvm::errs() << "Could not open file: " << err.message();
		return false;
	}

	llvm::legacy::PassManager pass;
	if (target_machine->addPassesToEmitFile(pass, dest, nullptr, llvm::CGFT_ObjectFile)) {
		llvm::errs() << "TheTargetMachine can't emit a file of this type";
		return false;
	}

	pass.run(*nyla::working_llvm_module);
	dest.flush();

	llvm::outs() << "Wrote " << fname << "\n";

	return true;
}

llvm::Function* llvm_generator::gen_function(nyla::afunction* function) {
	llvm::Type* ll_return_type = gen_type(function->return_type);

	std::vector<llvm::Type*> ll_parameter_types;
	for (nyla::avariable_decl* param : function->parameters) {
		ll_parameter_types.push_back(gen_type(param->variable->type));
	}

	bool is_var_args = false;
	llvm::FunctionType* ll_function_type = llvm::FunctionType::get(ll_return_type, ll_parameter_types, is_var_args);

	llvm::Function* ll_function = llvm::Function::Create(
		ll_function_type,
		llvm::Function::ExternalLinkage, // publically visible
		function->name.c_str(),
		*nyla::working_llvm_module
	);

	// Setting the names of the variables.
	u64 param_index = 0;
	for (auto& llvm_param : ll_function->args())
		llvm_param.setName(function->parameters[param_index++]->variable->name.c_str());

	// The Main block for the function.
	llvm::BasicBlock* ll_basic_block = llvm::BasicBlock::Create(*nyla::llvm_context, "function block", ll_function);
	nyla::llvm_builder->SetInsertPoint(ll_basic_block);

	m_bb = ll_basic_block;
	// scope may be nullptr if all that was parsed was
	// a function declaration.
	if (function->scope != nullptr) {
		for (nyla::aexpr* expr : function->scope->expressions) {
			gen_expression(expr);
		}
	}

	return ll_function;
}

llvm::Type* llvm_generator::gen_type(nyla::type* type) {
	switch (type->tag) {
	case TYPE_BYTE:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_SHORT:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_INT:
		return llvm::Type::getInt32Ty(*nyla::llvm_context);
	case TYPE_LONG:
		return llvm::Type::getInt64Ty(*nyla::llvm_context);
	case TYPE_FLOAT:
		return llvm::Type::getFloatTy(*nyla::llvm_context);
	case TYPE_DOUBLE:
		return llvm::Type::getDoubleTy(*nyla::llvm_context);
	case TYPE_BOOL:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_VOID:
		return llvm::Type::getVoidTy(*nyla::llvm_context);
	}
	return nullptr;
}

llvm::Value* nyla::llvm_generator::gen_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_RETURN:
		return gen_return(dynamic_cast<nyla::areturn*>(expr));
	case AST_VALUE_INT:
		return gen_number(dynamic_cast<nyla::anumber*>(expr));
	case AST_BINARY_OP:
		return gen_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
	case AST_VARIABLE:
		return gen_variable(dynamic_cast<nyla::avariable*>(expr));
	case AST_VARIABLE_DECL:
		return gen_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		// Void return.
		nyla::llvm_builder->CreateRet(
			llvm::UndefValue::get(llvm::Type::getVoidTy(*nyla::llvm_context)));
	}
	return nyla::llvm_builder->CreateRet(gen_expression(ret->value));
}

llvm::Value* nyla::llvm_generator::gen_number(nyla::anumber* number) {
	switch (number->tag) {
	case AST_VALUE_INT:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt32Ty(*nyla::llvm_context), number->value_int, true);
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_binary_op(nyla::abinary_op* binary_op) {
	switch (binary_op->op) {
	case '=': {
		nyla::avariable* variable = dynamic_cast<nyla::avariable*>(binary_op->lhs);
		llvm::Value* ll_variable  = m_sym_table.get_alloca(variable);
		llvm::Value* ll_value     = gen_expression(binary_op->rhs);
		nyla::llvm_builder->CreateStore(ll_value, ll_variable);
		return ll_value; // In case of multiple assignments they
						 // may assign the value assigned lower on the tree.
						 // Ex.     a = b = c + 5;    c + 5 Value is returned
						 //                           which is then used to assign
						 //                           a and b.
	}
	case '+': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		return nyla::llvm_builder->CreateAdd(ll_lhs, ll_rhs);
	}
	}
	return nullptr;
}

llvm::Value* nyla::llvm_generator::gen_variable_decl(nyla::avariable_decl* variable_decl) {
	llvm::IRBuilder<> tmp_builder(m_bb, m_bb->begin());
	llvm::AllocaInst* var_alloca;
	var_alloca = tmp_builder.CreateAlloca(
		gen_type(variable_decl->variable->type),
		nullptr,                                    // Array size
		variable_decl->variable->name.c_str());
	m_sym_table.store_alloca(variable_decl->variable, var_alloca);
	if (variable_decl->assignment != nullptr) {
		gen_expression(variable_decl->assignment);
	}
	return var_alloca;
}

llvm::Value* nyla::llvm_generator::gen_variable(nyla::avariable* variable) {
	// Assumed that the variable is already loaded and in scope!
	return nyla::llvm_builder->CreateLoad(
		m_sym_table.get_alloca(variable),
		variable->name.c_str());;
}
