#define NOMINMAX
#include <QFile>
#include <QTextStream>
#include <QtWidgets/QApplication>

#include "darkstyle/DarkStyle.h"
#include "darkstyle/framelesswindow/framelesswindow.h"
#include "qvm_handlers.h"
#include "qvm_inspector.h"
#include "qvm_virtual_instructions.h"
#include "qvm_virtual_routines.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setStyle(new DarkStyle);
  FramelessWindow FW;

  auto g_main_window = new qvm_inspector;
  qvm_virtual_instructions virt_instrs_panel(g_main_window);
  qvm_handlers virt_handler_panel(g_main_window);
  qvm_virtual_routines virt_routine_panel(g_main_window);

  FW.setContent(g_main_window);
  FW.setWindowIcon(QIcon("icon.ico"));
  FW.show();
  return app.exec();
}