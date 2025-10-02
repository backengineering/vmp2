#include "qvm_virtual_routines.h"

qvm_virtual_routines::qvm_virtual_routines( qvm_inspector *g_main_window )
    : g_main_window( g_main_window ), ui( &g_main_window->ui )
{
    connect( ui->virtual_machine_enters, &QTreeWidget::itemSelectionChanged, this, &qvm_virtual_routines::on_select );
}

void qvm_virtual_routines::update_vm_enter( vm::ctx_t *g_vm_ctx )
{
    char buffer[ 256 ];
    ZydisFormatter formatter;
    ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

    ui->virtual_machine_enter_instrs->clear();
    for ( auto [ instr, raw, addr ] : g_vm_ctx->vm_entry )
    {
        ZydisFormatterFormatInstruction( &formatter, &instr, buffer, sizeof( buffer ),
                                         ( addr - g_main_window->module_base ) + g_vm_ctx->image_base );

        auto newItem = new QTreeWidgetItem();
        newItem->setText( 0, QString::number( ( addr - g_main_window->module_base ) + g_vm_ctx->image_base, 16 ) );
        newItem->setText( 1, buffer );
        ui->virtual_machine_enter_instrs->addTopLevelItem( newItem );
    }
}

void qvm_virtual_routines::update_calc_jmp( vm::ctx_t *g_vm_ctx )
{
    char buffer[ 256 ];
    ZydisFormatter formatter;
    ZydisFormatterInit( &formatter, ZYDIS_FORMATTER_STYLE_INTEL );

    ui->virtual_machine_enter_calc_jmp->clear();
    for ( auto [ instr, raw, addr ] : g_vm_ctx->calc_jmp )
    {
        ZydisFormatterFormatInstruction( &formatter, &instr, buffer, sizeof( buffer ),
                                         ( addr - g_main_window->module_base ) + g_vm_ctx->image_base );

        auto newItem = new QTreeWidgetItem();
        newItem->setText( 0, QString::number( ( addr - g_main_window->module_base ) + g_vm_ctx->image_base, 16 ) );
        newItem->setText( 1, buffer );
        ui->virtual_machine_enter_calc_jmp->addTopLevelItem( newItem );
    }
}

void qvm_virtual_routines::update_vm_handlers( vm::ctx_t *g_vm_ctx )
{
    ui->virt_handlers_tree->clear();
    for ( auto idx = 0u; idx < g_vm_ctx->vm_handlers.size(); ++idx )
    {
        auto newItem = new QTreeWidgetItem;
        newItem->setData( 0, Qt::UserRole, idx );
        newItem->setText( 0, QString( "%1" ).arg( idx ) );
        newItem->setText( 1,
                          QString( "%1" ).arg( ( g_vm_ctx->vm_handlers[ idx ].address - g_main_window->module_base ) +
                                                   g_main_window->img_base,
                                               0, 16 ) );

        newItem->setText( 2, g_vm_ctx->vm_handlers[ idx ].profile ? g_vm_ctx->vm_handlers[ idx ].profile->name
                                                                  : "UNDEFINED" );

        newItem->setText( 3, QString( "%1" ).arg( g_vm_ctx->vm_handlers[ idx ].imm_size ) );

        if ( g_vm_ctx->vm_handlers[ idx ].profile && g_vm_ctx->vm_handlers[ idx ].imm_size )
            newItem->setText( 4,
                              g_vm_ctx->vm_handlers[ idx ].profile->extention == vm::handler::extention_t::sign_extend
                                  ? "SIGN EXTENDED"
                                  : "ZERO EXTENDED" );
        else
            newItem->setText( 4, "UNDEFINED" );

        ui->virt_handlers_tree->addTopLevelItem( newItem );
    }
    ui->virt_handlers_tree->topLevelItem( 0 )->setSelected( true );
}

void qvm_virtual_routines::on_select()
{
    if ( ui->virtual_machine_enters->selectedItems().empty() )
        return;

    auto item = ui->virtual_machine_enters->selectedItems()[ 0 ];

    if ( !item || !item->childCount() )
        return;

    auto entry_rva = item->data( 0, Qt::UserRole ).value< std::uint32_t >();

    if ( !entry_rva )
        return;

    if ( g_main_window->g_vm_ctx )
        delete g_main_window->g_vm_ctx;

    g_main_window->g_vm_ctx =
        new vm::ctx_t( g_main_window->module_base, g_main_window->img_base, g_main_window->img_size, entry_rva );

    if ( !g_main_window->g_vm_ctx->init() )
    {
        g_main_window->dbg_msg( "[!] failed to init vm::ctx_t...\n" );
        return;
    }

    update_vm_enter( g_main_window->g_vm_ctx );
    update_calc_jmp( g_main_window->g_vm_ctx );
    update_vm_handlers( g_main_window->g_vm_ctx );

    ui->virt_instrs->clear();
    g_main_window->code_block_addrs.clear();
    g_main_window->update_virtual_instructions( entry_rva );
}