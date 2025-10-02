#include "qvm_handlers.h"

qvm_handlers::qvm_handlers( qvm_inspector *g_main_window )
    : g_main_window( g_main_window ), ui( &g_main_window->ui )
{
    connect( ui->virt_handlers_tree, &QTreeWidget::itemSelectionChanged, this, &qvm_handlers::on_select );
}

void qvm_handlers::update_transforms( vm::handler::handler_t &vm_handler )
{
    char buffer[ 256 ];
    ZydisFormatter formatter;
    ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

    ui->virt_handler_transforms_tree->clear();
    const auto &vm_handler_transforms = vm_handler.transforms;

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
        ui->virt_handler_transforms_tree->addTopLevelItem( new_transform_entry );
    }
}

void qvm_handlers::update_instrs( vm::handler::handler_t &vm_handler )
{
    char buffer[ 256 ];
    ZydisFormatter formatter;
    ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

    ui->virt_handler_instrs_tree->clear();
    const auto &vm_handler_instrs = vm_handler.instrs;

    // display vm handler instructions...
    for ( const auto &instr : vm_handler_instrs )
    {
        auto new_instr = new QTreeWidgetItem();
        new_instr->setText( 0, QString::number( ( instr.addr - g_main_window->module_base ) + g_main_window->img_base, 16 ) );

        ZydisFormatterFormatInstruction( &formatter, &instr.instr, buffer, sizeof( buffer ),
                                         ( instr.addr - g_main_window->module_base ) + g_main_window->img_base );

        new_instr->setText( 1, buffer );
        ui->virt_handler_instrs_tree->addTopLevelItem( new_instr );
    }
}

void qvm_handlers::on_select()
{
    if ( ui->virt_handlers_tree->selectedItems().empty() )
        return;

    auto item = ui->virt_handlers_tree->selectedItems()[ 0 ];

    if ( !item )
        return;

    if ( !g_main_window->g_vm_ctx )
        return;

    const auto handler_idx = item->data( 0, Qt::UserRole ).value< std::uint8_t >();
    update_instrs( g_main_window->g_vm_ctx->vm_handlers[ handler_idx ] );
    update_transforms( g_main_window->g_vm_ctx->vm_handlers[ handler_idx ] );
}