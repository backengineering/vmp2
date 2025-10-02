#pragma once
#include <vmp_rtn_t.hpp>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Mangler.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"

#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/GVN.h"
#include "llvm/Transforms/Utils.h"

#include "X86TargetMachine.h"
#include "llvm/Pass.h"
#include "llvm/Passes/OptimizationLevel.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"

namespace llvm
{
    extern "C" void LLVMInitializeX86TargetInfo();
    extern "C" void LLVMInitializeX86Target();
    extern "C" void LLVMInitializeX86TargetMC();
    extern "C" void LLVMInitializeX86AsmParser();
    extern "C" void LLVMInitializeX86AsmPrinter();
} // namespace llvm

namespace vm
{
    class devirt_t
    {
        friend class lifters_t;

      public:
        explicit devirt_t( llvm::LLVMContext *llvm_ctx, llvm::Module *llvm_module, vmp2::v4::file_header * vmp2_file );
        llvm::Function *lift( std::uintptr_t rtn_begin, std::vector< vm::instrs::code_block_t > vmp2_code_blocks );
        bool compile( std::vector< std::uint8_t > &obj );

      private:
        llvm::LLVMContext *llvm_ctx;
        llvm::Module *llvm_module;
        vmp2::v4::file_header *vmp2_file;
        std::shared_ptr< llvm::IRBuilder<> > ir_builder;
        std::vector< std::shared_ptr< vm::vmp_rtn_t > > vmp_rtns;

        void push( std::uint8_t byte_size, llvm::Value *input_val );
        llvm::Value *pop( std::uint8_t byte_size );

        llvm::Value *load_value( std::uint8_t byte_size, llvm::GlobalValue *global );
        llvm::Value *load_value( std::uint8_t byte_size, llvm::AllocaInst *var );

        llvm::Value *sf( std::uint8_t byte_size, llvm::Value *val );
        llvm::Value *zf( std::uint8_t byte_size, llvm::Value *val );
        llvm::Value *pf( std::uint8_t byte_size, llvm::Value *val );
        llvm::Value *flags( llvm::Value *cf, llvm::Value *pf, llvm::Value *af, llvm::Value *zf, llvm::Value *sf,
                            llvm::Value *of );
    };
} // namespace vm