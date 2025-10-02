#pragma once
#include "qvm_inspector.h"

class qvm_virtual_routines : public QObject
{
    Q_OBJECT
  public:
    explicit qvm_virtual_routines( qvm_inspector *g_main_window );

  private:
    Ui::QVMProfilerClass *ui;
    qvm_inspector *g_main_window;

    void update_vm_enter( vm::ctx_t *g_vm_ctx );
    void update_calc_jmp( vm::ctx_t *g_vm_ctx );
    void update_vm_handlers( vm::ctx_t *g_vm_ctx );
  private slots:
    void on_select();
};