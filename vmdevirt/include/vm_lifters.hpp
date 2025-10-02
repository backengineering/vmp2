#pragma once
#include <devirt_t.hpp>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/IRPrintingPasses.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Pass.h"

namespace vm
{
    class lifters_t
    {
        // private default constructor...
        // used for singleton...
        lifters_t()
        {
        }

        using lifter_callback_t =
            std::function< void( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block,
                                 const vm::instrs::virt_instr_t &vinstr, llvm::IRBuilder<> *ir_builder ) >;

        static lifter_callback_t lconstq, lconstdwsxq, lconstwsxq, lconstbzxw, lconstbsxq, lconstwsxdw, lconstdw,
            lconstw, lconstbsxdw;

        static lifter_callback_t addq, adddw, addw, addb;
        static lifter_callback_t sregq, sregdw, sregw, sregb;
        static lifter_callback_t lregq, lregdw;
        static lifter_callback_t imulq, imuldw;
        static lifter_callback_t mulq, muldw;
        static lifter_callback_t pushvspq, pushvspdw;
        static lifter_callback_t popvsp;
        static lifter_callback_t writeq, writedw, writew, writeb;
        static lifter_callback_t readq, readdw, readw, readb;
        static lifter_callback_t nandq, nanddw, nandw, nandb;
        static lifter_callback_t shrq, shrdw, shrw, shrb;
        static lifter_callback_t shlq, shldw, shlb;
        static lifter_callback_t shldq, shlddw;
        static lifter_callback_t shrdq, shrddw;
        static lifter_callback_t jmp;
        static lifter_callback_t lflagsq;
        static lifter_callback_t vmexit;
        static lifter_callback_t readcr8;
        static lifter_callback_t readcr3;
        static lifter_callback_t writecr3;
        static lifter_callback_t rdtsc;
        static lifter_callback_t readgsq;
        static lifter_callback_t divq, divdw;
        static lifter_callback_t idivdw;

        std::map< vm::handler::mnemonic_t, lifter_callback_t * > lifters = { { vm::handler::LCONSTQ, &lconstq },
                                                                             { vm::handler::LCONSTDW, &lconstdw },
                                                                             { vm::handler::LCONSTW, &lconstw },
                                                                             { vm::handler::LCONSTDWSXQ, &lconstdwsxq },
                                                                             { vm::handler::LCONSTWSXQ, &lconstwsxq },
                                                                             { vm::handler::LCONSTBZXW, &lconstbzxw },
                                                                             { vm::handler::LCONSTBSXQ, &lconstbsxq },
                                                                             { vm::handler::LCONSTWSXDW, &lconstwsxdw },
                                                                             { vm::handler::LCONSTBSXDW, &lconstbsxdw },
                                                                             { vm::handler::DIVQ, &divq },
                                                                             { vm::handler::DIVDW, &divdw },
                                                                             { vm::handler::IDIVDW, &idivdw },
                                                                             { vm::handler::ADDQ, &addq },
                                                                             { vm::handler::ADDDW, &adddw },
                                                                             { vm::handler::ADDW, &addw },
                                                                             { vm::handler::ADDB, &addb },
                                                                             { vm::handler::SHRQ, &shrq },
                                                                             { vm::handler::SHRDW, &shrdw },
                                                                             { vm::handler::SHRW, &shrw },
                                                                             { vm::handler::SHRB, &shrb },
                                                                             { vm::handler::SHLQ, &shlq },
                                                                             { vm::handler::SHLDW, &shldw },
                                                                             { vm::handler::SHLB, &shlb },
                                                                             { vm::handler::SHLDQ, &shldq },
                                                                             { vm::handler::SHLDDW, &shlddw },
                                                                             { vm::handler::SHRDQ, &shrdq },
                                                                             { vm::handler::SHRDDW, &shrddw },
                                                                             { vm::handler::IMULQ, &imulq },
                                                                             { vm::handler::IMULDW, &imuldw },
                                                                             { vm::handler::MULQ, &mulq },
                                                                             { vm::handler::MULDW, &muldw },
                                                                             { vm::handler::PUSHVSPQ, &pushvspq },
                                                                             { vm::handler::PUSHVSPDW, &pushvspdw },
                                                                             { vm::handler::POPVSPQ, &popvsp },
                                                                             { vm::handler::SREGQ, &sregq },
                                                                             { vm::handler::SREGDW, &sregdw },
                                                                             { vm::handler::SREGW, &sregw },
                                                                             { vm::handler::SREGB, &sregb },
                                                                             { vm::handler::LREGQ, &lregq },
                                                                             { vm::handler::LREGDW, &lregdw },
                                                                             { vm::handler::READQ, &readq },
                                                                             { vm::handler::READDW, &readdw },
                                                                             { vm::handler::READW, &readw },
                                                                             { vm::handler::READB, &readb },
                                                                             { vm::handler::WRITEQ, &writeq },
                                                                             { vm::handler::WRITEDW, &writedw },
                                                                             { vm::handler::WRITEW, &writew },
                                                                             { vm::handler::WRITEB, &writeb },
                                                                             { vm::handler::NANDQ, &nandq },
                                                                             { vm::handler::NANDDW, &nanddw },
                                                                             { vm::handler::NANDW, &nandw },
                                                                             { vm::handler::NANDB, &nandb },
                                                                             { vm::handler::LFLAGSQ, &lflagsq },
                                                                             { vm::handler::READCR8, &readcr8 },
                                                                             { vm::handler::READCR3, &readcr3 },
                                                                             { vm::handler::WRITECR3, &writecr3 },
                                                                             { vm::handler::RDTSC, &rdtsc },
                                                                             { vm::handler::READGSQ, &readgsq },
                                                                             { vm::handler::JMP, &jmp },
                                                                             { vm::handler::VMEXIT, &vmexit } };

        static llvm::Value *and_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *result );
        static llvm::Value *add_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *lhs, llvm::Value *rhs );
        static llvm::Value *shr_flags( vm::devirt_t *rtn, std::uint8_t byte_size, llvm::Value *lhs, llvm::Value *rhs,
                                       llvm::Value *result );

      public:
        static lifters_t *get_instance( void )
        {
            static lifters_t obj;
            return &obj;
        }

        bool lift( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block,
                   const vm::instrs::virt_instr_t &vinstr, llvm::IRBuilder<> *ir_builder )
        {
            if ( vinstr.mnemonic_t == vm::handler::INVALID || lifters.find( vinstr.mnemonic_t ) == lifters.end() )
                return false;

            ( *( lifters[ vinstr.mnemonic_t ] ) )( rtn, vm_code_block, vinstr, ir_builder );
            return true;
        }
    };
} // namespace vm