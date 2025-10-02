#pragma once
#include "qvm_inspector.h"

class qvm_handlers : public QObject
{
    Q_OBJECT
  public:
    explicit qvm_handlers( qvm_inspector *g_main_window );

  private:
    Ui::QVMProfilerClass *ui;
    qvm_inspector *g_main_window;
    void update_transforms( vm::handler::handler_t &vm_handler );
    void update_instrs( vm::handler::handler_t &vm_handler );

  private slots:
    void on_select();
};