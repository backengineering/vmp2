#include "qvm_virtual_instructions.h"

qvm_virtual_instructions::qvm_virtual_instructions( qvm_inspector *g_main_window )
    : g_main_window( g_main_window ), ui( &g_main_window->ui )
{
    connect( ui->virt_instrs, &QTreeWidget::itemSelectionChanged, this, &qvm_virtual_instructions::on_select );
}

void qvm_virtual_instructions::on_select()
{
    if ( ui->virt_instrs->selectedItems().empty() )
        return;

    auto item = ui->virt_instrs->selectedItems()[ 0 ];

    if ( !item )
        return;

    auto virt_instr = item->data( 3, Qt::UserRole ).value< vm::instrs::virt_instr_t >();
    update_native_registers( &virt_instr );
    update_virtual_registers( &virt_instr );
    update_virtual_stack( &virt_instr );
    update_vmhandler_info( &virt_instr );
}

void qvm_virtual_instructions::update_native_registers( vm::instrs::virt_instr_t *virt_instr )
{
    const auto &trace_data = virt_instr->trace_data;

    // set native register values....
    for ( auto idx = 0u; idx < 15; ++idx )
        ui->native_regs->topLevelItem( idx )->setText( 1, QString::number( trace_data.regs.raw[ idx ], 16 ) );

    // set RFLAGs and its bits...
    ui->native_regs->topLevelItem( 16 )->setText( 1, QString::number( trace_data.regs.rflags, 16 ) );

    rflags flags;
    flags.flags = trace_data.regs.rflags;

    // could probably use a for loop here and shift bits around maybe...
    ui->native_regs->topLevelItem( 16 )->child( 0 )->setText( 1, QString::number( flags.zero_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 1 )->setText( 1, QString::number( flags.parity_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 2 )->setText( 1, QString::number( flags.auxiliary_carry_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 3 )->setText( 1, QString::number( flags.overflow_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 4 )->setText( 1, QString::number( flags.sign_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 5 )->setText( 1, QString::number( flags.direction_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 6 )->setText( 1, QString::number( flags.carry_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 7 )->setText( 1, QString::number( flags.trap_flag ) );
    ui->native_regs->topLevelItem( 16 )->child( 8 )->setText( 1, QString::number( flags.interrupt_enable_flag ) );
}

void qvm_virtual_instructions::update_virtual_registers( vm::instrs::virt_instr_t *virt_instr )
{
    const auto &trace_data = virt_instr->trace_data;

    // set VIP in virtual registers window...
    ui->virt_regs->topLevelItem( 0 )->setText(
        1, QString::number( ( trace_data.vip - g_main_window->file_header->module_base ) +
                                g_main_window->file_header->image_base,
                            16 ) );

    // set VSP in virtual registers window...
    ui->virt_regs->topLevelItem( 1 )->setText( 1, QString::number( trace_data.regs.rbp, 16 ) );

    // set decrypt key in virtual registers window...
    ui->virt_regs->topLevelItem( 2 )->setText( 1, QString::number( trace_data.regs.rdx, 16 ) );

    // set the virtual register values....
    for ( auto idx = 4u; idx < 28; ++idx )
        ui->virt_regs->topLevelItem( idx )->setText( 1, QString::number( trace_data.vregs.qword[ idx - 4 ], 16 ) );
}

void qvm_virtual_instructions::update_virtual_stack( vm::instrs::virt_instr_t *virt_instr )
{
    ui->virt_stack->clear();
    const auto &trace_data = virt_instr->trace_data;

    for ( auto idx = 0u; idx < sizeof( trace_data.vsp ) / 8; ++idx )
    {
        auto new_stack_entry = new QTreeWidgetItem();
        new_stack_entry->setText( 0, QString::number( trace_data.regs.rbp + ( idx * 8 ), 16 ) );
        new_stack_entry->setText( 1, QString::number( trace_data.vsp.qword[ idx ], 16 ) );
        ui->virt_stack->addTopLevelItem( new_stack_entry );
    }
}

void qvm_virtual_instructions::update_vmhandler_info( vm::instrs::virt_instr_t *virt_instr )
{
    char buffer[ 256 ];
    ZydisFormatter formatter;
    ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

    ui->vm_handler_instrs->clear();
    const auto &vm_handler_instrs = g_main_window->g_vm_ctx->vm_handlers[ virt_instr->opcode ].instrs;

    // display vm handler instructions...
    for ( const auto &instr : vm_handler_instrs )
    {
        auto new_instr = new QTreeWidgetItem();
        new_instr->setText(
            0, QString::number( ( instr.addr - g_main_window->module_base ) + g_main_window->img_base, 16 ) );

        ZydisFormatterFormatInstruction( &formatter, &instr.instr, buffer, sizeof( buffer ),
                                         ( instr.addr - g_main_window->module_base ) + g_main_window->img_base );

        new_instr->setText( 1, buffer );
        ui->vm_handler_instrs->addTopLevelItem( new_instr );
    }

    // display vm handler transformations...
    ui->vm_handler_transforms->clear();
    const auto &vm_handler_transforms = g_main_window->g_vm_ctx->vm_handlers[ virt_instr->opcode ].transforms;

    for ( auto [ transform_type, transform_instr ] : vm_handler_transforms )
    {
        if ( transform_type == vm::transform::type::generic0 && transform_instr.mnemonic == ZYDIS_MNEMONIC_INVALID )
            continue;

        auto new_transform_entry = new QTreeWidgetItem();

        switch ( transform_type )
        {
        case vm::transform::type::rolling_key:
        {
            new_transform_entry->setText( 0, "Key Transform" );
            break;
        }
        case vm::transform::type::generic0:
        case vm::transform::type::generic1:
        case vm::transform::type::generic2:
        case vm::transform::type::generic3:
        {
            new_transform_entry->setText( 0, "Generic" );
            break;
        }
        case vm::transform::type::update_key:
        {
            new_transform_entry->setText( 0, "Update Key" );
            break;
        }
        default:
            throw std::invalid_argument( "invalid transformation type..." );
        }

        ZydisFormatterFormatInstruction( &formatter, &transform_instr, buffer, sizeof( buffer ), NULL );

        new_transform_entry->setText( 1, buffer );
        ui->vm_handler_transforms->addTopLevelItem( new_transform_entry );
    }
}