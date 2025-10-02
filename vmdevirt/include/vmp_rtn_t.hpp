#pragma once
#include <vmprofiler.hpp>

#include "llvm/IR/InlineAsm.h"
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

namespace vm
{
    class vmp_rtn_t
    {
        llvm::Function *llvm_fptr;
        llvm::AllocaInst *flags, *stack;
        llvm::Module *llvm_module;
        vmp2::v4::file_header *vmp2_file;

        std::uintptr_t rtn_begin;
        std::shared_ptr< llvm::IRBuilder<> > ir_builder;
        std::vector< llvm::AllocaInst * > virtual_registers;
        std::vector< std::pair< std::uintptr_t, llvm::BasicBlock * > > llvm_code_blocks;
        std::vector< vm::instrs::code_block_t > vmp2_code_blocks;

        void create_virtual_registers( void );
        void create_routine( void );

        friend class devirt_t;
        friend class lifters_t;

      public:
        explicit vmp_rtn_t( std::uintptr_t rtn_begin, std::vector< vm::instrs::code_block_t > vmp2_code_blocks,
                            std::shared_ptr< llvm::IRBuilder<> > &ir_builder, llvm::Module *llvm_module,
                            vmp2::v4::file_header *vmp2_file );
    };
} // namespace vm