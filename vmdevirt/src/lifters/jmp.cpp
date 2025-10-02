#include <vm_lifters.hpp>

namespace vm
{
    lifters_t::lifter_callback_t lifters_t::jmp =
        [ & ]( vm::devirt_t *rtn, const vm::instrs::code_block_t &vm_code_block, const vm::instrs::virt_instr_t &vinstr,
               llvm::IRBuilder<> *ir_builder ) {
            if ( !vm_code_block.jcc.has_jcc )
            {
                ir_builder->CreateRetVoid();
                return;
            }

            auto &vmp_rtn = rtn->vmp_rtns.back();
            switch ( vm_code_block.jcc.type )
            {
            case vm::instrs::jcc_type::branching:
            {
                auto rva = rtn->pop( 8 );
                auto b1 = vm_code_block.jcc.block_addr[ 0 ] & std::numeric_limits< std::uint32_t >::max();

                auto _const_b1 = llvm::ConstantInt::get( ir_builder->getInt64Ty(), b1 );
                auto cmp = ir_builder->CreateCmp( llvm::CmpInst::ICMP_EQ, rva, _const_b1 );

                // find the first branch basic block...
                auto bb1 =
                    std::find_if( vmp_rtn->llvm_code_blocks.begin(), vmp_rtn->llvm_code_blocks.end(),
                                  [ & ]( const std::pair< std::uintptr_t, llvm::BasicBlock * > &block_data ) -> bool {
                                      return block_data.first == vm_code_block.jcc.block_addr[ 0 ];
                                  } );

                assert( bb1 != vmp_rtn->llvm_code_blocks.end(),
                        "[!] fatal error... unable to locate basic block for branching...\n" );

                // find the second branch basic block...
                auto bb2 =
                    std::find_if( vmp_rtn->llvm_code_blocks.begin(), vmp_rtn->llvm_code_blocks.end(),
                                  [ & ]( const std::pair< std::uintptr_t, llvm::BasicBlock * > &block_data ) -> bool {
                                      return block_data.first == vm_code_block.jcc.block_addr[ 1 ];
                                  } );

                assert( bb2 != vmp_rtn->llvm_code_blocks.end(),
                        "[!] fatal error... unable to locate basic block for branching...\n" );

                ir_builder->CreateCondBr( cmp, bb1->second, bb2->second );
                break;
            }
            case vm::instrs::jcc_type::absolute:
            {
                auto rva = rtn->pop( 8 );
                auto bb_data =
                    std::find_if( vmp_rtn->llvm_code_blocks.begin(), vmp_rtn->llvm_code_blocks.end(),
                                  [ & ]( const std::pair< std::uintptr_t, llvm::BasicBlock * > &block_data ) -> bool {
                                      return block_data.first == vm_code_block.jcc.block_addr[ 0 ];
                                  } );

                assert( bb_data != vmp_rtn->llvm_code_blocks.end(),
                        "[!] fatal error... unable to locate basic block...\n" );

                ir_builder->CreateBr( bb_data->second );
                break;
            }
            case vm::instrs::jcc_type::switch_case:
            {
                // TODO: add switch case support here...
                break;
            }
            default:
                break;
            }
        };
} // namespace vm