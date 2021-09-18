#include "code_gen.h"

// LLVM Target
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetOptions.h>

// Needed for writing .o files
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/LegacyPassManager.h>

void nyla::init_llvm_native_target() {
	llvm::InitializeNativeTarget();
	llvm::InitializeNativeTargetAsmParser();
	llvm::InitializeNativeTargetAsmPrinter();
}

llvm::TargetMachine* nyla::create_llvm_target_machine() {

	auto target_triple = llvm::sys::getDefaultTargetTriple();

	std::string target_error;
	auto target = llvm::TargetRegistry::lookupTarget(target_triple, target_error);

	if (!target) {
		llvm::errs() << target_error;
		return nullptr;
	}

	auto CPU = "generic";
	auto features = "";

	llvm::TargetOptions opt;
	auto RM = llvm::Optional<llvm::Reloc::Model>();
	llvm::TargetMachine* target_machine =
		target->createTargetMachine(target_triple, CPU, features, opt, RM);

	return target_machine;
}

bool nyla::write_obj_file(c_string fname, llvm::Module* llvm_module, llvm::TargetMachine* target_machine) {

	auto target_triple = llvm::sys::getDefaultTargetTriple();
	llvm_module->setTargetTriple(target_triple);

	llvm_module->setDataLayout(target_machine->createDataLayout());

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

	pass.run(*llvm_module);
	dest.flush();

	return true;
}
