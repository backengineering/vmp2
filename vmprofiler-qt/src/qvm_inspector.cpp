#include "qvm_inspector.h"

qvm_inspector::qvm_inspector(QWidget *parent)
    : QMainWindow(parent), file_header(nullptr), g_vm_ctx(nullptr) {
  ui.setupUi(this);
  ui.virt_instrs->setColumnWidth(0, 125);
  ui.virt_instrs->setColumnWidth(1, 150);
  ui.virt_instrs->setColumnWidth(2, 190);
  ui.virt_instrs->setColumnWidth(3, 200);
  ui.virtual_machine_enters->setColumnWidth(0, 180);

  connect(ui.action_open, &QAction::triggered, this, &qvm_inspector::on_open);
  connect(ui.action_close, &QAction::triggered, this, &qvm_inspector::on_close);
}

void qvm_inspector::on_close() { exit(0); }

void qvm_inspector::on_open() {
  if (file_header) free(file_header);

  file_header = nullptr;
  img_base = 0u, module_base = 0u;

  auto file_path = QFileDialog::getOpenFileName(
      this, tr("open vmp2 file"),
      std::filesystem::current_path().string().c_str(),
      tr("vmp2 file (*.vmp2)"));

  if (file_path.isEmpty()) {
    dbg_msg("invalid vmp2 file... no file selected...");
    return;
  }

  if (!std::filesystem::exists(file_path.toStdString())) {
    dbg_msg("vmp2 file does not exist...");
    return;
  }

  const auto file_size = std::filesystem::file_size(file_path.toStdString());

  if (!file_size) {
    dbg_msg("invalid vmp2 file size...");
    return;
  }

  QFile open_file(file_path);
  file_header = reinterpret_cast<vmp2::v4::file_header *>(malloc(file_size));

  if (!open_file.open(QIODevice::ReadOnly)) {
    dbg_msg("failed to open vmp2 file...");
    return;
  }

  memcpy(file_header, open_file.readAll().data(), file_size);

  if (!init_data()) {
    dbg_msg("failed to init vmp2 file data...");
    return;
  }
}

void qvm_inspector::dbg_print(QString dbg_output) {
  ui.dbg_output_window->appendPlainText(dbg_output);
}

void qvm_inspector::dbg_msg(QString dbg_output) {
  QMessageBox msg_box;
  msg_box.setText(dbg_output);
  msg_box.exec();
  dbg_print(dbg_output);
}

bool qvm_inspector::init_data() {
  if (file_header->magic != VMP_MAGIC) {
    dbg_msg("invalid magic bytes for vmp2 file...");

    return false;
  }

  dbg_print("valid magic bytes for vmp2 file...");

  if (file_header->version != vmp2::version_t::v4) {
    dbg_msg(
        "invalid vmp2 file version... "
        "this vminspector is compiled for version 4...\n");

    return false;
  }

  img_base = file_header->image_base;
  img_size = file_header->module_size;
  module_base = reinterpret_cast<std::uintptr_t>(file_header) +
                file_header->module_offset;

  if (!serialize_vmp2(virt_rtns)) {
    dbg_msg("failed to serialize vmp2 file format...\n");
    return false;
  }

  update_ui();
  return true;
}

void qvm_inspector::update_ui() {
  ui.virtual_machine_enters->clear();
  for (auto &[rtn_rva, rtn_blks] : virt_rtns) {
    auto new_item = new QTreeWidgetItem();
    new_item->setText(
        0, QString("rtn_%1").arg(rtn_rva + file_header->image_base, 0, 16));
    new_item->setText(
        1, QString("%1").arg(rtn_rva + file_header->image_base, 0, 16));
    new_item->setText(2, QString("%1").arg(rtn_blks.size()));
    new_item->setData(0, Qt::UserRole, QVariant(rtn_rva));

    std::for_each(
        rtn_blks.begin(), rtn_blks.end(),
        [&](vm::instrs::code_block_t &code_blk) {
          auto new_child = new QTreeWidgetItem();
          new_child->setText(0,
                             QString("blk_%1").arg(code_blk.vip_begin, 0, 16));
          new_child->setText(1, QString("%1").arg(code_blk.vip_begin, 0, 16));
          new_child->setText(2, QString("%1").arg(code_blk.vinstrs.size()));
          new_item->addChild(new_child);
        });

    ui.virtual_machine_enters->addTopLevelItem(new_item);
  }
}

bool qvm_inspector::serialize_vmp2(std::vector<rtn_data_t> &virt_rtns) {
  if (file_header->version != vmp2::version_t::v4) {
    std::printf("[!] invalid vmp2 file version... this build uses v3...\n");
    return false;
  }

  auto first_rtn = reinterpret_cast<vmp2::v4::rtn_t *>(
      reinterpret_cast<std::uintptr_t>(file_header) + file_header->rtn_offset);

  for (auto [rtn_block, rtn_idx] = std::pair{first_rtn, 0ull};
       rtn_idx < file_header->rtn_count;
       ++rtn_idx, rtn_block = reinterpret_cast<vmp2::v4::rtn_t *>(
                      reinterpret_cast<std::uintptr_t>(rtn_block) +
                      rtn_block->size)) {
    virt_rtns.push_back({rtn_block->vm_enter_offset, {}});
    for (auto [code_block, block_idx] =
             std::pair{&rtn_block->code_blocks[0], 0ull};
         block_idx < rtn_block->code_block_count;
         ++block_idx, code_block = reinterpret_cast<vmp2::v4::code_block_t *>(
                          reinterpret_cast<std::uintptr_t>(code_block) +
                          code_block->next_block_offset)) {
      auto block_vinstrs = reinterpret_cast<vm::instrs::virt_instr_t *>(
          reinterpret_cast<std::uintptr_t>(code_block) +
          sizeof(vmp2::v4::code_block_t) + (code_block->num_block_addrs * 8));

      vm::instrs::code_block_t _code_block{code_block->vip_begin};
      _code_block.jcc.has_jcc = code_block->has_jcc;
      _code_block.jcc.type = code_block->jcc_type;

      for (auto idx = 0u; idx < code_block->num_block_addrs; ++idx)
        _code_block.jcc.block_addr.push_back(code_block->branch_addr[idx]);

      for (auto idx = 0u; idx < code_block->vinstr_count; ++idx)
        _code_block.vinstrs.push_back(block_vinstrs[idx]);

      virt_rtns.back().rtn_blks.push_back(_code_block);
    }
  }

  return true;
}

void qvm_inspector::update_virtual_instructions(std::uintptr_t rtn_addr,
                                                std::uintptr_t blk_addr,
                                                QTreeWidgetItem *parent) {
  auto _rtn = std::find_if(
      virt_rtns.begin(), virt_rtns.end(),
      [&](rtn_data_t &rtn) -> bool { return rtn.rtn_rva == rtn_addr; });

  if (_rtn == virt_rtns.end()) return;

  auto code_blk =
      blk_addr ? std::find_if(
                     _rtn->rtn_blks.begin(), _rtn->rtn_blks.end(),
                     [&](const vm::instrs::code_block_t &code_block) -> bool {
                       return code_block.vip_begin == blk_addr;
                     })
               : _rtn->rtn_blks.begin();

  if (code_blk == _rtn->rtn_blks.end()) return;

  if (std::find(code_block_addrs.begin(), code_block_addrs.end(),
                code_blk->vip_begin) != code_block_addrs.end())
    return;

  code_block_addrs.push_back(code_blk->vip_begin);

  for (auto &vinstr : code_blk->vinstrs) {
    const auto profile = vm::handler::get_profile(vinstr.mnemonic_t);
    auto virt_instr_entry = new QTreeWidgetItem();

    // virtual instruction image base'ed rva... (column 1)...
    virt_instr_entry->setText(
        0, QString("0x%1").arg(QString::number(
               (vinstr.trace_data.vip - file_header->module_base) +
                   file_header->image_base,
               16)));

    // virtual instruction operand bytes... (column 2)...
    QString operand_bytes;
    operand_bytes.append(QString("%1").arg(vinstr.opcode, 0, 16));

    // if virt instruction has an imm... grab its bytes...
    if (vinstr.operand.has_imm) {
      operand_bytes.append(" - ");
      for (auto _idx = 0u; _idx < vinstr.operand.imm.imm_size / 8; ++_idx)
        operand_bytes.append(QString("%1 ").arg(
            reinterpret_cast<const std::uint8_t *>(&vinstr.operand.imm.u)[_idx],
            0, 16));
    }

    virt_instr_entry->setText(1, operand_bytes);

    // virtual instruction string, includes imm... (colume 3)...
    QString decoded_instr(QString("%1").arg(
        profile ? profile->name
                : QString("UNK(%1)").arg(vinstr.opcode, 0, 16)));

    if (vinstr.operand
            .has_imm)  // if there is a second operand (imm) put its value...
      decoded_instr.append(QString(" %1").arg(vinstr.operand.imm.u, 0, 16));

    virt_instr_entry->setText(2, decoded_instr);

    // add comments to the virtual instruction... (colume 4)...
    if (vinstr.mnemonic_t == vm::handler::LREGQ ||
        vinstr.mnemonic_t == vm::handler::SREGQ)
      virt_instr_entry->setText(
          3, QString("; vreg%1")
                 .arg(vinstr.operand.imm.u ? (vinstr.operand.imm.u / 8) : 0u));

    if (vinstr.mnemonic_t == vm::handler::JMP) {
      switch (code_blk->jcc.type) {
        case vm::instrs::jcc_type::branching: {
          virt_instr_entry->setText(
              3, QString("; { 0x%1, 0x%2 }")
                     .arg(code_blk->jcc.block_addr[0], 0, 16)
                     .arg(code_blk->jcc.block_addr[1], 0, 16));

          auto entry_rva =
              g_vm_ctx->vm_handlers[vinstr.opcode].address - module_base;
          auto branch_entry1 = new QTreeWidgetItem(),
               branch_entry2 = new QTreeWidgetItem();
          const auto block1_addr = code_blk->jcc.block_addr[0];
          const auto block2_addr = code_blk->jcc.block_addr[1];

          branch_entry1->setText(0, QString("0x%1").arg(block1_addr, 0, 16));
          branch_entry1->setText(3,
                                 QString("; blk_0x%1").arg(block1_addr, 0, 16));

          branch_entry2->setText(0, QString("0x%1").arg(block2_addr, 0, 16));
          branch_entry2->setText(3,
                                 QString("; blk_0x%1").arg(block2_addr, 0, 16));

          if (g_vm_ctx) delete g_vm_ctx;

          if (!(g_vm_ctx =
                    new vm::ctx_t(module_base, img_base, img_size, entry_rva))
                   ->init()) {
            dbg_print(QString("> failed to init vm::ctx_t for jmp at = %1...")
                          .arg(QString::number(code_blk->vip_begin, 16)));
            return;
          }

          update_virtual_instructions(rtn_addr, code_blk->jcc.block_addr[0],
                                      branch_entry1);

          if (g_vm_ctx) delete g_vm_ctx;

          if (!(g_vm_ctx =
                    new vm::ctx_t(module_base, img_base, img_size, entry_rva))
                   ->init()) {
            dbg_print(QString("> failed to init vm::ctx_t for jmp at = %1...")
                          .arg(QString::number(code_blk->vip_begin, 16)));
            return;
          }

          update_virtual_instructions(rtn_addr, code_blk->jcc.block_addr[1],
                                      branch_entry2);
          virt_instr_entry->addChildren({branch_entry1, branch_entry2});
          break;
        }
        case vm::instrs::jcc_type::absolute: {
          auto entry_rva =
              g_vm_ctx->vm_handlers[vinstr.opcode].address - module_base;
          virt_instr_entry->setText(
              3, QString("; { 0x%1 }").arg(code_blk->jcc.block_addr[0], 0, 16));

          if (g_vm_ctx) delete g_vm_ctx;

          if (!(g_vm_ctx =
                    new vm::ctx_t(module_base, img_base, img_size, entry_rva))
                   ->init()) {
            dbg_print(QString("> failed to init vm::ctx_t for jmp at = %1...")
                          .arg(QString::number(code_blk->vip_begin, 16)));
            return;
          }

          auto branch_entry1 = new QTreeWidgetItem();
          branch_entry1->setText(
              0, QString("0x%1").arg(code_blk->jcc.block_addr[0], 0, 16));
          branch_entry1->setText(
              3, QString("; blk_0x%1").arg(code_blk->jcc.block_addr[0], 0, 16));
          update_virtual_instructions(rtn_addr, code_blk->jcc.block_addr[0],
                                      branch_entry1);
          virt_instr_entry->addChild(branch_entry1);
          break;
        }
        case vm::instrs::jcc_type::switch_case: {
          auto entry_rva =
              g_vm_ctx->vm_handlers[vinstr.opcode].address - module_base;

          if (g_vm_ctx) delete g_vm_ctx;

          if (!(g_vm_ctx =
                    new vm::ctx_t(module_base, img_base, img_size, entry_rva))
                   ->init()) {
            dbg_print(QString("> failed to init vm::ctx_t for jmp at = %1...")
                          .arg(QString::number(code_blk->vip_begin, 16)));
            return;
          }

          for (auto branch_addr : code_blk->jcc.block_addr) {
            virt_instr_entry->setText(3, QString("; switch case"));

            auto branch_entry = new QTreeWidgetItem();
            branch_entry->setText(0, QString("0x%1").arg(branch_addr, 0, 16));

            update_virtual_instructions(rtn_addr, branch_addr, branch_entry);
            virt_instr_entry->addChild(branch_entry);
          }
          break;
        }
        default:
          break;
      }
    }

    QVariant var;
    var.setValue(vinstr);
    virt_instr_entry->setData(3, Qt::UserRole, var);
    parent ? parent->addChild(virt_instr_entry)
           : ui.virt_instrs->addTopLevelItem(virt_instr_entry);
  }
}
