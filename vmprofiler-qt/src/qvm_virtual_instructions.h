#pragma once
#include "qvm_inspector.h"

class qvm_virtual_instructions : public QObject
{
    Q_OBJECT
  public:
    explicit qvm_virtual_instructions( qvm_inspector *g_main_window );

  private:
    Ui::QVMProfilerClass *ui;
    qvm_inspector *g_main_window;

    void update_native_registers( vm::instrs::virt_instr_t *virt_instr );
    void update_virtual_registers( vm::instrs::virt_instr_t *virt_instr );
    void update_virtual_stack( vm::instrs::virt_instr_t *virt_instr );
    void update_vmhandler_info( vm::instrs::virt_instr_t *virt_instr );

  private slots:
    void on_select();
};