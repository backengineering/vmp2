#include <devirt_t.hpp>
#include <vm_lifters.hpp>

namespace vm
{
    devirt_t::devirt_t( llvm::LLVMContext *llvm_ctx, llvm::Module *llvm_module, vmp2::v4::file_header *vmp2_file )
        : llvm_ctx( llvm_ctx ), llvm_module( llvm_module ), vmp2_file( vmp2_file )
    {
        ir_builder = std::make_shared< llvm::IRBuilder<> >( *llvm_ctx );
    }

    void devirt_t::push( std::uint8_t num_bytes, llvm::Value *val )
    {
        // sub rsp, num_bytes
        auto current_rtn = vmp_rtns.back();
        auto rsp_addr = ir_builder->CreateLoad( current_rtn->stack );
        rsp_addr->setAlignment( llvm::Align( 1 ) );

        auto sub_rsp_val =
            ir_builder->CreateGEP( ir_builder->getInt8Ty(), rsp_addr, ir_builder->getInt8( 0 - num_bytes ) );

        ir_builder->CreateStore( sub_rsp_val, current_rtn->stack )->setAlignment( llvm::Align( 1 ) );

        // mov [rsp], val
        auto resized_new_rsp_addr = ir_builder->CreateBitCast(
            sub_rsp_val, llvm::PointerType::get( llvm::IntegerType::get( *llvm_ctx, num_bytes * 8 ), 0ull ) );
        ir_builder->CreateStore( val, resized_new_rsp_addr )->setAlignment( llvm::Align( 1 ) );
    }

    llvm::Value *devirt_t::pop( std::uint8_t num_bytes )
    {
        // mov rax, [rsp]
        auto current_rtn = vmp_rtns.back();
        auto rsp_addr = ir_builder->CreateLoad( current_rtn->stack );
        rsp_addr->setAlignment( llvm::Align( 1 ) );

        auto new_rsp_addr =
            ir_builder->CreateGEP( ir_builder->getInt8Ty(), rsp_addr, ir_builder->getInt8( num_bytes ) );

        auto resized_new_rsp_addr = ir_builder->CreateBitCast(
            rsp_addr, llvm::PointerType::get( llvm::IntegerType::get( *llvm_ctx, num_bytes * 8 ), 0ull ) );

        auto pop_val = ir_builder->CreateLoad( resized_new_rsp_addr );
        pop_val->setAlignment( llvm::Align( 1 ) );

        ir_builder->CreateStore( new_rsp_addr, current_rtn->stack )->setAlignment( llvm::Align( 1 ) );
        ir_builder->CreateStore( llvm::UndefValue::get( ir_builder->getInt8Ty() ), rsp_addr )
            ->setAlignment( llvm::Align( 1 ) );
        return pop_val;
    }

    llvm::Value *devirt_t::load_value( std::uint8_t byte_size, llvm::GlobalValue *var )
    {
        if ( byte_size * 8 != var->getType()->getPrimitiveSizeInBits() )
        {
            auto cast_ptr = ir_builder->CreatePointerCast(
                var, llvm::PointerType::get( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), 0 ) );

            auto result = ir_builder->CreateLoad( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), cast_ptr );
            result->setAlignment( llvm::Align( 1 ) );
            return result;
        }

        auto result = ir_builder->CreateLoad( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), var );
        result->setAlignment( llvm::Align( 1 ) );
        return result;
    }

    llvm::Value *devirt_t::load_value( std::uint8_t byte_size, llvm::AllocaInst *var )
    {
        if ( byte_size * 8 != var->getType()->getPrimitiveSizeInBits() )
        {
            auto cast_ptr = ir_builder->CreatePointerCast(
                var, llvm::PointerType::get( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), 0 ) );

            auto result = ir_builder->CreateLoad( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), cast_ptr );
            result->setAlignment( llvm::Align( 1 ) );
            return result;
        }

        auto result = ir_builder->CreateLoad( llvm::IntegerType::get( *llvm_ctx, byte_size * 8 ), var );
        result->setAlignment( llvm::Align( 1 ) );
        return result;
    }

    bool devirt_t::compile( std::vector< std::uint8_t > &obj )
    {
        llvm::legacy::FunctionPassManager pass_mgr( llvm_module );
        pass_mgr.add( llvm::createPromoteMemoryToRegisterPass() );
        pass_mgr.add( llvm::createNewGVNPass() );
        pass_mgr.add( llvm::createReassociatePass() );
        pass_mgr.add( llvm::createDeadCodeEliminationPass() );
        pass_mgr.add( llvm::createInstructionCombiningPass() );

        for ( auto vmp_rtn : vmp_rtns )
        {
            pass_mgr.run( *vmp_rtn->llvm_fptr );
            std::printf( "> opt rtn_0x%p\n", vmp_rtn->rtn_begin );
        }

        llvm::TargetOptions opt;
        llvm::SmallVector< char, 128 > buff;
        llvm::raw_svector_ostream dest( buff );
        llvm::legacy::PassManager pass;
        std::string error;

        auto target_triple = llvm::sys::getDefaultTargetTriple();
        llvm_module->setTargetTriple( target_triple );

        auto target = llvm::TargetRegistry::lookupTarget( target_triple, error );
        auto reloc_model = llvm::Optional< llvm::Reloc::Model >();
        auto target_machine = target->createTargetMachine( target_triple, "generic", "", opt, reloc_model );
        llvm_module->setDataLayout( target_machine->createDataLayout() );

        target_machine->addPassesToEmitFile( pass, dest, nullptr, llvm::CGFT_ObjectFile );
        auto result = pass.run( *llvm_module );
        obj.insert( obj.begin(), buff.begin(), buff.end() );
        return result;
    }

    llvm::Function *devirt_t::lift( std::uintptr_t rtn_begin, std::vector< vm::instrs::code_block_t > code_blocks )
    {
        vmp_rtns.push_back(
            std::make_shared< vm::vmp_rtn_t >( rtn_begin, code_blocks, ir_builder, llvm_module, vmp2_file ) );

        auto &vmp_rtn = vmp_rtns.back();
        auto lifters = vm::lifters_t::get_instance();

        for ( auto idx = 0u; idx < vmp_rtn->vmp2_code_blocks.size(); ++idx )
        {
            ir_builder->SetInsertPoint( vmp_rtn->llvm_code_blocks[ idx ].second );
            if ( vmp_rtn->vmp2_code_blocks[ idx ].vinstrs.size() < 35 )
            {
                ir_builder->CreateRetVoid();
                continue;
            }

            for ( auto &vinstr : vmp_rtn->vmp2_code_blocks[ idx ].vinstrs )
            {
                if ( !lifters->lift( this, vmp_rtn->vmp2_code_blocks[ idx ], vinstr, ir_builder.get() ) )
                {
                    std::printf(
                        "> failed to devirtualize virtual instruction with opcode = %d, handler table rva = 0x%x\n",
                        vinstr.opcode, vinstr.trace_data.regs.r12 - vinstr.trace_data.regs.r13 );

                    return nullptr;
                }
            }
        }
        return vmp_rtn->llvm_fptr;
    }

    llvm::Value *devirt_t::sf( std::uint8_t byte_size, llvm::Value *val )
    {
        auto op_size = llvm::IntegerType::get( *llvm_ctx, byte_size * 8 );
        auto msb = ir_builder->CreateLShr( val, ( byte_size * 8 ) - 1 );
        return ir_builder->CreateZExt( msb, llvm::IntegerType::get( *llvm_ctx, 64 ) );
    }

    llvm::Value *devirt_t::zf( std::uint8_t byte_size, llvm::Value *val )
    {
        auto op_size = llvm::IntegerType::get( *llvm_ctx, byte_size * 8 );
        auto is_zero = ir_builder->CreateICmpEQ( val, llvm::ConstantInt::get( op_size, 0 ) );
        return ir_builder->CreateZExt( is_zero, llvm::IntegerType::get( *llvm_ctx, 64 ) );
    }

    llvm::Value *devirt_t::pf( std::uint8_t byte_size, llvm::Value *val )
    {
        auto operand_size = llvm::IntegerType::get( *llvm_ctx, byte_size * 8 );
        auto popcount_intrinsic = llvm::Intrinsic::getDeclaration( llvm_module, llvm::Intrinsic::ctpop,
                                                                   { llvm::IntegerType::get( *llvm_ctx, 64 ) } );

        auto lower_bits = ir_builder->CreateIntCast( val, llvm::IntegerType::get( *llvm_ctx, 8 ), false );
        auto extended_bits = ir_builder->CreateZExt( lower_bits, llvm::IntegerType::get( *llvm_ctx, 64 ) );
        return ir_builder->CreateCall( popcount_intrinsic, { extended_bits } );
    }

    llvm::Value *devirt_t::flags( llvm::Value *cf, llvm::Value *pf, llvm::Value *af, llvm::Value *zf, llvm::Value *sf,
                                  llvm::Value *of )
    {
        auto shifted_pf = ir_builder->CreateShl( pf, 2, "shifted_pf", true, true );
        auto shifted_af = ir_builder->CreateShl( llvm::ConstantInt::get( llvm::IntegerType::get( *llvm_ctx, 64 ), 0 ),
                                                 4, "shifted_af", true, true ); // treat af as zero

        auto shifted_zf = ir_builder->CreateShl( zf, 6, "shifted_zf", true, true );
        auto shifted_sf = ir_builder->CreateShl( sf, 7, "shifted_sf", true, true );
        auto shifted_of = ir_builder->CreateShl( of, 11, "shifted_of", true, true );
        auto or1 = ir_builder->CreateOr( cf, shifted_of );
        auto or2 = ir_builder->CreateOr( or1, shifted_zf );
        auto or3 = ir_builder->CreateOr( or2, shifted_sf );
        auto or4 = ir_builder->CreateOr( or3, shifted_af );
        auto or5 = ir_builder->CreateOr( or4, shifted_pf );
        return ir_builder->CreateOr( or5, llvm::ConstantInt::get( llvm::IntegerType::get( *llvm_ctx, 64 ), 514 ) );
    }

} // namespace vm