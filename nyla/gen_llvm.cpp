#include "gen_llvm.h"

#include "type.h"

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

void llvm_generator::gen_file_unit(nyla::afile_unit* file_unit, bool print_functions) {

	// Seperating generation of the function declaration
	// out so that the function exist for use later.
	std::vector<llvm::Function*> ll_functions;
	for (nyla::afunction* function : file_unit->functions) {
		gen_function(function);
	}

	for (nyla::afunction* function : file_unit->functions) {
		llvm::Function* ll_function = function->ll_function;
		if (!function->is_external) {
			m_bb = &ll_function->getEntryBlock();
			nyla::llvm_builder->SetInsertPoint(m_bb);
		}
		m_ll_function = ll_function;

		// scope may be nullptr if all that was parsed was
		// a function declaration.
		if (function->scope != nullptr) {
			for (nyla::aexpr* stmt : function->scope->stmts) {
				gen_expression(stmt);
			}
		}

		if (print_functions) {
			ll_function->print(llvm::errs());
		}
	}
}

llvm::Function* llvm_generator::gen_function(nyla::afunction* function) {
	llvm::Type* ll_return_type = gen_type(function->return_type);

	std::vector<llvm::Type*> ll_parameter_types;
	
	for (nyla::avariable_decl* param : function->parameters) {
		if (param->variable->checked_type->is_arr()) {


			// Need to create additional arguments for
			// the array lengths
			nyla::type* arr_ptr_type =
				nyla::type::get_ptr(
					param->variable
					->checked_type
					->get_array_base_type());
			
			ll_parameter_types.push_back(gen_type(arr_ptr_type));

			// Impicit non-visible arguments that define the size of the array's dimensions
			for (u32 i = 0; i < param->variable->checked_type->arr_depth; i++) {
				ll_parameter_types.push_back(llvm::Type::getInt64Ty(*nyla::llvm_context));
			}

		} else {
			ll_parameter_types.push_back(gen_type(param->variable->checked_type));
		}
	}

	bool is_var_args = false;
	llvm::FunctionType* ll_function_type = llvm::FunctionType::get(ll_return_type, ll_parameter_types, is_var_args);

	llvm::Function* ll_function = llvm::Function::Create(
		ll_function_type,
		llvm::Function::ExternalLinkage, // publically visible
		function->ident->name.c_str(),
		*nyla::working_llvm_module
	);

	if (function->is_external) {
		ll_function->setDLLStorageClass(llvm::GlobalValue::DLLImportStorageClass);
		// TODO: the user should be allowed to set the calling convention in code.
		ll_function->setCallingConv(llvm::CallingConv::X86_StdCall); // TODO Windows only!
	}

	// Setting the names of the variables.
	u64 param_index = 0;
	u32 real_argument_index = 0;
	for (param_index = 0; param_index < function->parameters.size(); param_index++) {
		nyla::avariable* variable = function->parameters[param_index]->variable;
		if (variable->checked_type->is_arr()) {
			// Name of the array.
			ll_function->getArg(real_argument_index++)
				       ->setName(function->parameters[param_index]->variable->ident->name.c_str());
			// Implicit names for array dimension sizes
			for (u32 i = 0; i < variable->checked_type->arr_depth; i++) {
				llvm::Twine arr_twine =
					llvm::Twine(function->parameters[param_index]->variable->ident->name.c_str());

				ll_function->getArg(real_argument_index++)
					       ->setName(arr_twine.concat("_dim" + std::to_string(i)));
			}
		} else {
			ll_function->getArg(real_argument_index++)
				       ->setName(function->parameters[param_index]->variable->ident->name.c_str());
		}
	}

	if (!function->is_external) {
		// The Main block for the function.
		llvm::BasicBlock* ll_basic_block = llvm::BasicBlock::Create(*nyla::llvm_context, "function block", ll_function);
		nyla::llvm_builder->SetInsertPoint(ll_basic_block);
		m_bb = ll_basic_block;

		// Allocating memory for the parameters
		param_index = 0;
		real_argument_index = 0;
		for (param_index = 0; param_index < function->parameters.size(); param_index++) {

			nyla::avariable* variable = function->parameters[param_index]->variable;
			if (variable->checked_type->is_arr()) {
				
				// Arrays have to be handled differently since
				// the sizes of the array's dimensions are treated
				// as a seperate arguments
				
				u64 num_dims = variable->checked_type->arr_depth;
				
				if (num_dims > 1) {
					variable->ll_arr_mem_offsets.reserve(num_dims - 1);
				}
				llvm::Argument* arr_argument = ll_function->getArg(real_argument_index++);
				
				// The size of the array is the sum of multiples
				// of it's dimensions
				llvm::Value* array_size = ll_function->getArg(real_argument_index++);
				variable->ll_arr_sizes.push_back(array_size);
				for (u32 i = 1; i < variable->checked_type->arr_depth; i++) {
					llvm::Value* dimension_size = ll_function->getArg(real_argument_index++);
					variable->ll_arr_sizes.push_back(dimension_size);
					variable->ll_arr_mem_offsets.push_back(array_size);
					array_size = nyla::llvm_builder->CreateMul(array_size, dimension_size);
				}

				llvm::AllocaInst* var_alloca =
					nyla::llvm_builder->CreateAlloca(arr_argument->getType(),
						nullptr, variable->ident->name.c_str());

				// Storing the array's pointer
				nyla::llvm_builder->CreateStore(arr_argument, var_alloca);
				variable->ll_alloca = var_alloca;

			} else {
				llvm::AllocaInst* var_alloca = gen_alloca(function->parameters[param_index++]->variable);
				nyla::llvm_builder->CreateStore(ll_function->getArg(real_argument_index), var_alloca); // Storing the incoming value
				++real_argument_index;
			}			
		}
	}

	function->ll_function = ll_function;
	return ll_function;
}

llvm::Value* nyla::llvm_generator::gen_function_call(nyla::afunction_call* function_call) {
	llvm::Function* ll_callee = function_call->called_function->ll_function;
	std::vector<llvm::Value*> ll_parameter_values;

 	for (nyla::aexpr* parameter_value : function_call->parameter_values) {
		if (parameter_value->checked_type->is_arr()) {
			nyla::avariable* declared_variable =
				dynamic_cast<nyla::aidentifier*>(parameter_value)->variable;
			ll_parameter_values.push_back(gen_expression(parameter_value));
			// Passing the array dimension sizes as parameters
			for (u32 i = 0; i < declared_variable->ll_arr_sizes.size(); i++) {
				ll_parameter_values.push_back(declared_variable->ll_arr_sizes[i]);
			}
		} else {
			ll_parameter_values.push_back(gen_expression(parameter_value));
		}
	}
	return nyla::llvm_builder->CreateCall(ll_callee, ll_parameter_values);
}

llvm::Type* llvm_generator::gen_type(nyla::type* type) {
	switch (type->tag) {
	case TYPE_BYTE:
	case TYPE_UBYTE:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_SHORT:
	case TYPE_USHORT:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_INT:
	case TYPE_UINT:
		return llvm::Type::getInt32Ty(*nyla::llvm_context);
	case TYPE_LONG:
	case TYPE_ULONG:
		return llvm::Type::getInt64Ty(*nyla::llvm_context);
	case TYPE_FLOAT:
		return llvm::Type::getFloatTy(*nyla::llvm_context);
	case TYPE_DOUBLE:
		return llvm::Type::getDoubleTy(*nyla::llvm_context);
	case TYPE_BOOL:
		return llvm::Type::getInt8Ty(*nyla::llvm_context);
	case TYPE_VOID:
		return llvm::Type::getVoidTy(*nyla::llvm_context);
	case TYPE_CHAR16:
		return llvm::Type::getInt16Ty(*nyla::llvm_context);
	case TYPE_PTR: {
		// Pointers TODO: pointers to pointers
		switch (type->elem_type->tag) {
		case TYPE_BYTE:
		case TYPE_UBYTE:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context);
		case TYPE_SHORT:
		case TYPE_USHORT:
			return llvm::Type::getInt16PtrTy(*nyla::llvm_context);
		case TYPE_INT:
		case TYPE_UINT:
			return llvm::Type::getInt32PtrTy(*nyla::llvm_context);
		case TYPE_LONG:
		case TYPE_ULONG:
			return llvm::Type::getInt64PtrTy(*nyla::llvm_context);
		case TYPE_FLOAT:
			return llvm::Type::getFloatPtrTy(*nyla::llvm_context);
		case TYPE_DOUBLE:
			return llvm::Type::getDoublePtrTy(*nyla::llvm_context);
		case TYPE_BOOL:
			return llvm::Type::getInt8PtrTy(*nyla::llvm_context);
		case TYPE_CHAR16:
			return llvm::Type::getInt16PtrTy(*nyla::llvm_context);
		}
	}
	case TYPE_ARR: {
		assert(!"Should call get_element_base_type()");
		return nullptr;
	}
	default: {
		assert(!"Missing gen_type implementation");
		return nullptr;
	}
	}
}

llvm::Value* llvm_generator::gen_expression(nyla::aexpr* expr) {
	switch (expr->tag) {
	case AST_RETURN:
		return gen_return(dynamic_cast<nyla::areturn*>(expr));
	case AST_VARIABLE_DECL:
		return gen_variable_decl(dynamic_cast<nyla::avariable_decl*>(expr));
	case AST_VALUE_BYTE:
	case AST_VALUE_SHORT:
	case AST_VALUE_INT:
	case AST_VALUE_LONG:
	case AST_VALUE_UBYTE:
	case AST_VALUE_USHORT:
	case AST_VALUE_UINT:
	case AST_VALUE_ULONG:
	case AST_VALUE_FLOAT:
	case AST_VALUE_DOUBLE:
		return gen_number(dynamic_cast<nyla::anumber*>(expr));
	case AST_BINARY_OP:
		return gen_binary_op(dynamic_cast<nyla::abinary_op*>(expr));
	case AST_UNARY_OP:
		return gen_unary_op(dynamic_cast<nyla::aunary_op*>(expr));
	case AST_IDENTIFIER:
		return gen_identifier(dynamic_cast<nyla::aidentifier*>(expr));
	case AST_VARIABLE:
		return gen_variable(dynamic_cast<nyla::avariable*>(expr));
	case AST_FOR_LOOP:
		return gen_for_loop(dynamic_cast<nyla::afor_loop*>(expr));
	case AST_TYPE_CAST:
		return gen_type_cast(dynamic_cast<nyla::atype_cast*>(expr));
	case AST_FUNCTION_CALL:
		return gen_function_call(dynamic_cast<nyla::afunction_call*>(expr));
	case AST_ARRAY_ACCESS:
		return gen_array_access(dynamic_cast<nyla::aarray_access*>(expr));
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_return(nyla::areturn* ret) {
	if (ret->value == nullptr) {
		// Void return.
		return nyla::llvm_builder->CreateRet(
			llvm::UndefValue::get(llvm::Type::getVoidTy(*nyla::llvm_context)));
	}
	return nyla::llvm_builder->CreateRet(gen_expression(ret->value));
}

llvm::Value* llvm_generator::gen_number(nyla::anumber* number) {
	switch (number->tag) {
	case AST_VALUE_BYTE:
	case AST_VALUE_UBYTE:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt8Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_SHORT:
	case AST_VALUE_USHORT:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt16Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_INT:
	case AST_VALUE_UINT:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt32Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_LONG:
	case AST_VALUE_ULONG:
		return llvm::ConstantInt::get(
			llvm::IntegerType::getInt64Ty(*nyla::llvm_context), number->value_int, number->checked_type->is_signed());
	case AST_VALUE_FLOAT:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_float));
	case AST_VALUE_DOUBLE:
		return llvm::ConstantFP::get(*nyla::llvm_context, llvm::APFloat(number->value_double));
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_binary_op(nyla::abinary_op* binary_op) {
	switch (binary_op->op) {
	case '=': {
		nyla::aidentifier* identifier = dynamic_cast<nyla::aidentifier*>(binary_op->lhs);
		llvm::Value* ll_variable = identifier->variable->ll_alloca;
		if (binary_op->rhs->tag == AST_ARRAY) {
			return gen_array(dynamic_cast<nyla::aarray*>(binary_op->rhs),
				nyla::llvm_builder->CreateLoad(ll_variable));
		} else {
			llvm::Value* ll_value = gen_expression(binary_op->rhs);
			nyla::llvm_builder->CreateStore(ll_value, ll_variable);
			return ll_value; // In case of multiple assignments they
							 // may assign the value assigned lower on the tree.
							 // Ex.     a = b = c + 5;    c + 5 Value is returned
							 //                           which is then used to assign
							 //                           a and b.
		}
	}
	case TK_ARRAY_LENGTH: {
		nyla::avariable* declared_variable = 
			dynamic_cast<nyla::aidentifier*>(binary_op->lhs)->variable;
		nyla::anumber* dimension_index = dynamic_cast<nyla::anumber*>(binary_op->rhs);
		return declared_variable->ll_arr_sizes[declared_variable->ll_arr_sizes.size() - dimension_index->value_int - 1];
	}
	case '+': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateAdd(ll_lhs, ll_rhs, "addt");
		}
		return nyla::llvm_builder->CreateFAdd(ll_lhs, ll_rhs, "addft");
	}
	case '-': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateSub(ll_lhs, ll_rhs, "subt");
		}
		return nyla::llvm_builder->CreateFSub(ll_lhs, ll_rhs, "subft");
	}
	case '*': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateMul(ll_lhs, ll_rhs, "mult");
		}
		return nyla::llvm_builder->CreateFMul(ll_lhs, ll_rhs, "mulft");
	}
	case '/': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->checked_type->is_int()) {
			if (binary_op->checked_type->is_signed()) {
				return nyla::llvm_builder->CreateSDiv(ll_lhs, ll_rhs, "divt");
			} else {
				return nyla::llvm_builder->CreateUDiv(ll_lhs, ll_rhs, "divt");
			}
		}
		return nyla::llvm_builder->CreateFDiv(ll_lhs, ll_rhs, "divft");
	}
	case '<': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->lhs->checked_type->is_int()) {
			return nyla::llvm_builder->CreateICmpULT(ll_lhs, ll_rhs, "cmplt");
		}
		return nyla::llvm_builder->CreateFCmpULT(ll_lhs, ll_rhs, "cmplt");
	}
	case '>': {
		llvm::Value* ll_lhs = gen_expression(binary_op->lhs);
		llvm::Value* ll_rhs = gen_expression(binary_op->rhs);
		if (binary_op->lhs->checked_type->is_int()) {
			return nyla::llvm_builder->CreateICmpUGT(ll_lhs, ll_rhs, "cmpgt");
		}
		return nyla::llvm_builder->CreateFCmpUGT(ll_lhs, ll_rhs, "cmpgt");
	}
	default:
		assert(!"An operator has not been handled!");
		break;
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_unary_op(nyla::aunary_op* unary_op) {
	switch (unary_op->op) {
	case '-':
		llvm::Value * ll_factor = gen_expression(unary_op->factor);
		if (unary_op->checked_type->is_int()) {
			return nyla::llvm_builder->CreateNeg(ll_factor, "negt");
		}
		return nyla::llvm_builder->CreateFNeg(ll_factor, "fnegt");
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_variable_decl(nyla::avariable_decl* variable_decl) {
	llvm::Value* var_alloca = gen_alloca(variable_decl->variable);
	if (variable_decl->assignment != nullptr) {
		gen_expression(variable_decl->assignment);
	}
	return var_alloca;
}

llvm::Value* llvm_generator::gen_for_loop(nyla::afor_loop* for_loop) {
	
	// Generating declarations before entering the loop
	for (nyla::avariable_decl* var_decl : for_loop->declarations) {
		gen_variable_decl(var_decl);
	}

	// Top of loop with condition
	llvm::BasicBlock* ll_cond_bb = llvm::BasicBlock::Create(*llvm_context, "loopcond", m_ll_function);

	// Jumping directly into the loop condition
	nyla::llvm_builder->CreateBr(ll_cond_bb);
	// Telling llvm we want to put code into the loop condition block
	nyla::llvm_builder->SetInsertPoint(ll_cond_bb);

	llvm::BasicBlock* ll_loop_body_bb = llvm::BasicBlock::Create(*llvm_context, "loopbody", m_ll_function);

	// Generating the condition and telling it to jump to the body or the finish point
	llvm::Value* ll_cond = gen_expression(for_loop->cond);
	llvm::BasicBlock* ll_finish_bb = llvm::BasicBlock::Create(*llvm_context, "finishloop", m_ll_function);
	nyla::llvm_builder->CreateCondBr(ll_cond, ll_loop_body_bb, ll_finish_bb);
	
	// Telling llvm we want to put code into the loop body
	nyla::llvm_builder->SetInsertPoint(ll_loop_body_bb);

	// Generating the body of the loop
	for (nyla::aexpr* stmt : for_loop->scope->stmts) {
		gen_expression(stmt);
	}
	
	// Unconditional branch back to the condition
	nyla::llvm_builder->CreateBr(ll_cond_bb);

	// All new code will be inserted into the block after
	// the loop
	nyla::llvm_builder->SetInsertPoint(ll_finish_bb);
	
	return nullptr;
}

llvm::Value* llvm_generator::gen_identifier(nyla::aidentifier* identifier) {
	return gen_variable(identifier->variable);
}

llvm::Value* llvm_generator::gen_variable(nyla::avariable* variable) {
	return nyla::llvm_builder->CreateLoad(
		variable->ll_alloca,
		variable->ident->name.c_str());
}

llvm::Value* llvm_generator::gen_type_cast(nyla::atype_cast* type_cast) {
	nyla::type*  val_type     = type_cast->value->checked_type;
	nyla::type*  cast_to_type = type_cast->checked_type;
	llvm::Value* ll_val       = gen_expression(type_cast->value);
	llvm::Type*  ll_cast_type = gen_type(type_cast->checked_type);
	
	if (val_type->is_int() && cast_to_type->is_int()) {
		if (cast_to_type->get_mem_size() < val_type->get_mem_size()) {
			// Signed and unsigned downcasting use trunc
			return nyla::llvm_builder->CreateTrunc(ll_val, ll_cast_type, "trunctt");
		} else {
			if (cast_to_type->is_signed()) {
				// Signed upcasting
				return nyla::llvm_builder->CreateSExt(ll_val, ll_cast_type, "supcastt");
			} else {
				// Unsigned upcasting
				return nyla::llvm_builder->CreateZExt(ll_val, ll_cast_type, "uuocastt");
			}
		}
	} else if (cast_to_type->is_float() && val_type->is_int()) {
		// Int to floating point
		if (val_type->is_signed()) {
			return nyla::llvm_builder->CreateSIToFP(ll_val, ll_cast_type, "sitofpt");
		} else {
			return nyla::llvm_builder->CreateUIToFP(ll_val, ll_cast_type, "uitofpt");
		}
	} else if (cast_to_type->is_int() && val_type->is_float()) {
		// Floating point to Int
		if (cast_to_type->is_signed()) {
			return nyla::llvm_builder->CreateFPToSI(ll_val, ll_cast_type, "fptosit");
		} else {
			return nyla::llvm_builder->CreateFPToUI(ll_val, ll_cast_type, "fptouit");
		}
	} else if (cast_to_type->is_float() && val_type->is_float()) {
		// Float to Float
		if (cast_to_type->get_mem_size() > val_type->get_mem_size()) {
			// Upcasting float
			return nyla::llvm_builder->CreateFPExt(ll_val, ll_cast_type, "fpupcastt");
		} else {
			// Downcasting float
			return nyla::llvm_builder->CreateFPTrunc(ll_val, ll_cast_type, "fptrunctt");
		}
	} else if (cast_to_type->is_int() && val_type->is_ptr()) {
		return nyla::llvm_builder->CreatePtrToInt(ll_val, ll_cast_type, "itoptrt");
	} else if (cast_to_type->is_ptr() && val_type->is_int()) {
		return nyla::llvm_builder->CreateIntToPtr(ll_val, ll_cast_type, "ptrtoit");
	} else if (cast_to_type->is_ptr() && val_type->is_arr()) {
		return ll_val;  // Arrays are already stored internalls as pointers
	}
	assert(!"Unhandled cast case");
	return nullptr;
}

llvm::Value* llvm_generator::gen_array(nyla::aarray* arr, llvm::Value* ptr_to_arr) {
	// TODO: might want to use memcpy or something to move the values into the array instead
	for (u64 index = 0; index < arr->elements.size(); index++) {
		llvm::Value* gep = nyla::llvm_builder->CreateGEP(ptr_to_arr, llvm::ConstantInt::get(
			llvm::IntegerType::getInt64Ty(*nyla::llvm_context), index, false));
		nyla::llvm_builder->CreateStore(gen_expression(arr->elements[index]), gep);
	}
	return nullptr;
}

llvm::Value* llvm_generator::gen_array_access(nyla::aarray_access* array_access) {
	// Keep a running total of the total
	// index into the array
	
	nyla::avariable* decl_variable = array_access->ident->variable;

	std::vector<llvm::Value*> ll_mem_offsets = decl_variable->ll_arr_mem_offsets;
	
	// Basically we keep a running total and if we hit
	// the base element index then we load
	// else we just return a GEP with how far into the
	// array we are

	llvm::Value* total_index_offset = nullptr;
	if (ll_mem_offsets.size() > 0) {
		total_index_offset = nyla::llvm_builder->CreateMul(gen_expression(array_access->index),
			ll_mem_offsets[ll_mem_offsets.size() - 1]);
	}

	if (array_access->next) {
		array_access = array_access->next;
	}

	u32 index_count = 1;
	while (array_access->next) {
		// total_index_offset += ll_mem_arr_offset * index
		llvm::Value* ll_arr_index = gen_expression(array_access->index);
		llvm::Value* ll_index_offset =
			nyla::llvm_builder->CreateMul(ll_arr_index,
				ll_mem_offsets[ll_mem_offsets.size() - index_count - 1]);

		total_index_offset = nyla::llvm_builder->CreateAdd(total_index_offset, ll_index_offset);
		array_access = dynamic_cast<nyla::aarray_access*>(array_access->next);
		++index_count;
	}

	llvm::Value* arr_ptr = gen_variable(decl_variable);

	if (index_count >= decl_variable->checked_type->arr_depth - 1) {
		if (total_index_offset) {
			total_index_offset = nyla::llvm_builder->CreateAdd(total_index_offset, gen_expression(array_access->index));
		} else {
			total_index_offset = gen_expression(array_access->index);
		}
		
		llvm::Value* gep = nyla::llvm_builder->CreateGEP(arr_ptr, total_index_offset);
		return nyla::llvm_builder->CreateLoad(gep);
	} else {
		return nyla::llvm_builder->CreateGEP(arr_ptr, total_index_offset);
	}
	return nullptr;
}

llvm::AllocaInst* llvm_generator::gen_alloca(nyla::avariable* variable) {
	llvm::IRBuilder<> tmp_builder(m_bb, m_bb->begin());
	llvm::AllocaInst* var_alloca = nullptr;
	
	if (variable->checked_type->is_arr()) {
		
		if (!variable->arr_alloc_reference) {

			llvm::Value* array_size = nullptr;

			assert(variable->checked_type->arr_depth != 0);
			// The size of the array is the sum of multiples
			// of it's dimension sizes
			if (variable->checked_type->arr_depth > 1)
				variable->ll_arr_mem_offsets.reserve(variable->checked_type->arr_depth - 1);
			array_size = calc_array_size(variable->checked_type, variable);
			

			// Allocating space for the array since it does
			// not rely on an already allocated variable
			
			llvm::AllocaInst* arr_alloca = tmp_builder.CreateAlloca(
				gen_type(variable->checked_type->get_array_base_type()),
				array_size, "arr_alloca");
			llvm::Value* arr_ptr = nyla::llvm_builder->CreateGEP(
				arr_alloca,
				llvm::ConstantInt::get(
					llvm::IntegerType::getInt64Ty(*nyla::llvm_context), 0, false));

			// Wanting to store the reference as a pointer
			// to the first element
			var_alloca = tmp_builder.CreateAlloca(arr_ptr->getType(), nullptr, variable->ident->name.c_str());
			nyla::llvm_builder->CreateStore(arr_ptr, var_alloca);
		} else {
			
			var_alloca = tmp_builder.CreateAlloca(
				gen_type(
					nyla::type::get_ptr(
					variable->checked_type->get_array_base_type())),
				nullptr, variable->ident->name.c_str());
		
			calc_array_size(variable->arr_alloc_reference, variable);
		}
	} else {
		var_alloca = tmp_builder.CreateAlloca(gen_type(variable->checked_type),
			nullptr, variable->ident->name.c_str());
	}
	variable->ll_alloca = var_alloca;
	return var_alloca;
}

llvm::Value* llvm_generator::calc_array_size(nyla::type* arr_type, nyla::avariable* variable) {
	if (arr_type->elem_type->tag == TYPE_ARR) {
		llvm::Value* prev_array_size = calc_array_size(arr_type->elem_type, variable);
		llvm::Value* dim_size = gen_expression(arr_type->dim_size);
		llvm::Value* ll_array_size = nyla::llvm_builder->CreateMul(prev_array_size, dim_size);
		variable->ll_arr_sizes.push_back(dim_size);
		if (variable->ll_arr_mem_offsets.size() != variable->ll_arr_mem_offsets.capacity()) {
			variable->ll_arr_mem_offsets.push_back(ll_array_size);
		}
		return ll_array_size;
	}
	llvm::Value* ll_array_size = gen_expression(arr_type->dim_size);
	variable->ll_arr_mem_offsets.push_back(ll_array_size);
	variable->ll_arr_sizes.push_back(ll_array_size);
	return ll_array_size;
}

void llvm_generator::calc_array_size(nyla::avariable* arr_alloc_reference, nyla::avariable* variable) {
	std::vector<llvm::Value*> ll_mem_offsets = arr_alloc_reference->ll_arr_mem_offsets;
	std::vector<llvm::Value*> ll_sizes = arr_alloc_reference->ll_arr_sizes;
	for (u32 i = 0; i < variable->checked_type->arr_depth; i++) {
		if (i != 0) {
			variable->ll_arr_mem_offsets.push_back(
				ll_mem_offsets[ll_mem_offsets.size() - i - 1]);
		}
		variable->ll_arr_sizes.push_back(ll_sizes[ll_sizes.size() - i - 1]);
	}
}
