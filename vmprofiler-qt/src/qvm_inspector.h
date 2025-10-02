#pragma once
#include <qpushbutton.h>
#include <qtreewidget.h>

#include <QtWidgets/QtWidgets>
#include <filesystem>
#include <fstream>
#include <vmprofiler.hpp>

#include "ui_qvminspector.h"

/**
 * The 64-bit RFLAGS register contains a group of status flags, a control flag,
 * and a group of system flags in 64-bit mode. The upper 32 bits of RFLAGS
 * register is reserved. The lower 32 bits of RFLAGS is the same as EFLAGS.
 *
 * @see EFLAGS
 * @see Vol1[3.4.3.4(RFLAGS Register in 64-Bit Mode)] (reference)
 */
typedef union {
  struct {
    /**
     * @brief Carry flag
     *
     * [Bit 0] See the description in EFLAGS.
     */
    uint64_t carry_flag : 1;
#define RFLAGS_CARRY_FLAG_BIT 0
#define RFLAGS_CARRY_FLAG_FLAG 0x01
#define RFLAGS_CARRY_FLAG_MASK 0x01
#define RFLAGS_CARRY_FLAG(_) (((_) >> 0) & 0x01)

    /**
     * [Bit 1] Reserved - always 1
     */
    uint64_t read_as_1 : 1;
#define RFLAGS_READ_AS_1_BIT 1
#define RFLAGS_READ_AS_1_FLAG 0x02
#define RFLAGS_READ_AS_1_MASK 0x01
#define RFLAGS_READ_AS_1(_) (((_) >> 1) & 0x01)

    /**
     * @brief Parity flag
     *
     * [Bit 2] See the description in EFLAGS.
     */
    uint64_t parity_flag : 1;
#define RFLAGS_PARITY_FLAG_BIT 2
#define RFLAGS_PARITY_FLAG_FLAG 0x04
#define RFLAGS_PARITY_FLAG_MASK 0x01
#define RFLAGS_PARITY_FLAG(_) (((_) >> 2) & 0x01)
    uint64_t reserved1 : 1;

    /**
     * @brief Auxiliary Carry flag
     *
     * [Bit 4] See the description in EFLAGS.
     */
    uint64_t auxiliary_carry_flag : 1;
#define RFLAGS_AUXILIARY_CARRY_FLAG_BIT 4
#define RFLAGS_AUXILIARY_CARRY_FLAG_FLAG 0x10
#define RFLAGS_AUXILIARY_CARRY_FLAG_MASK 0x01
#define RFLAGS_AUXILIARY_CARRY_FLAG(_) (((_) >> 4) & 0x01)
    uint64_t reserved2 : 1;

    /**
     * @brief Zero flag
     *
     * [Bit 6] See the description in EFLAGS.
     */
    uint64_t zero_flag : 1;
#define RFLAGS_ZERO_FLAG_BIT 6
#define RFLAGS_ZERO_FLAG_FLAG 0x40
#define RFLAGS_ZERO_FLAG_MASK 0x01
#define RFLAGS_ZERO_FLAG(_) (((_) >> 6) & 0x01)

    /**
     * @brief Sign flag
     *
     * [Bit 7] See the description in EFLAGS.
     */
    uint64_t sign_flag : 1;
#define RFLAGS_SIGN_FLAG_BIT 7
#define RFLAGS_SIGN_FLAG_FLAG 0x80
#define RFLAGS_SIGN_FLAG_MASK 0x01
#define RFLAGS_SIGN_FLAG(_) (((_) >> 7) & 0x01)

    /**
     * @brief Trap flag
     *
     * [Bit 8] See the description in EFLAGS.
     */
    uint64_t trap_flag : 1;
#define RFLAGS_TRAP_FLAG_BIT 8
#define RFLAGS_TRAP_FLAG_FLAG 0x100
#define RFLAGS_TRAP_FLAG_MASK 0x01
#define RFLAGS_TRAP_FLAG(_) (((_) >> 8) & 0x01)

    /**
     * @brief Interrupt enable flag
     *
     * [Bit 9] See the description in EFLAGS.
     */
    uint64_t interrupt_enable_flag : 1;
#define RFLAGS_INTERRUPT_ENABLE_FLAG_BIT 9
#define RFLAGS_INTERRUPT_ENABLE_FLAG_FLAG 0x200
#define RFLAGS_INTERRUPT_ENABLE_FLAG_MASK 0x01
#define RFLAGS_INTERRUPT_ENABLE_FLAG(_) (((_) >> 9) & 0x01)

    /**
     * @brief Direction flag
     *
     * [Bit 10] See the description in EFLAGS.
     */
    uint64_t direction_flag : 1;
#define RFLAGS_DIRECTION_FLAG_BIT 10
#define RFLAGS_DIRECTION_FLAG_FLAG 0x400
#define RFLAGS_DIRECTION_FLAG_MASK 0x01
#define RFLAGS_DIRECTION_FLAG(_) (((_) >> 10) & 0x01)

    /**
     * @brief Overflow flag
     *
     * [Bit 11] See the description in EFLAGS.
     */
    uint64_t overflow_flag : 1;
#define RFLAGS_OVERFLOW_FLAG_BIT 11
#define RFLAGS_OVERFLOW_FLAG_FLAG 0x800
#define RFLAGS_OVERFLOW_FLAG_MASK 0x01
#define RFLAGS_OVERFLOW_FLAG(_) (((_) >> 11) & 0x01)

    /**
     * @brief I/O privilege level field
     *
     * [Bits 13:12] See the description in EFLAGS.
     */
    uint64_t io_privilege_level : 2;
#define RFLAGS_IO_PRIVILEGE_LEVEL_BIT 12
#define RFLAGS_IO_PRIVILEGE_LEVEL_FLAG 0x3000
#define RFLAGS_IO_PRIVILEGE_LEVEL_MASK 0x03
#define RFLAGS_IO_PRIVILEGE_LEVEL(_) (((_) >> 12) & 0x03)

    /**
     * @brief Nested task flag
     *
     * [Bit 14] See the description in EFLAGS.
     */
    uint64_t nested_task_flag : 1;
#define RFLAGS_NESTED_TASK_FLAG_BIT 14
#define RFLAGS_NESTED_TASK_FLAG_FLAG 0x4000
#define RFLAGS_NESTED_TASK_FLAG_MASK 0x01
#define RFLAGS_NESTED_TASK_FLAG(_) (((_) >> 14) & 0x01)
    uint64_t reserved3 : 1;

    /**
     * @brief Resume flag
     *
     * [Bit 16] See the description in EFLAGS.
     */
    uint64_t resume_flag : 1;
#define RFLAGS_RESUME_FLAG_BIT 16
#define RFLAGS_RESUME_FLAG_FLAG 0x10000
#define RFLAGS_RESUME_FLAG_MASK 0x01
#define RFLAGS_RESUME_FLAG(_) (((_) >> 16) & 0x01)

    /**
     * @brief Virtual-8086 mode flag
     *
     * [Bit 17] See the description in EFLAGS.
     */
    uint64_t virtual_8086_mode_flag : 1;
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_BIT 17
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_FLAG 0x20000
#define RFLAGS_VIRTUAL_8086_MODE_FLAG_MASK 0x01
#define RFLAGS_VIRTUAL_8086_MODE_FLAG(_) (((_) >> 17) & 0x01)

    /**
     * @brief Alignment check (or access control) flag
     *
     * [Bit 18] See the description in EFLAGS.
     *
     * @see Vol3A[4.6(ACCESS RIGHTS)]
     */
    uint64_t alignment_check_flag : 1;
#define RFLAGS_ALIGNMENT_CHECK_FLAG_BIT 18
#define RFLAGS_ALIGNMENT_CHECK_FLAG_FLAG 0x40000
#define RFLAGS_ALIGNMENT_CHECK_FLAG_MASK 0x01
#define RFLAGS_ALIGNMENT_CHECK_FLAG(_) (((_) >> 18) & 0x01)

    /**
     * @brief Virtual interrupt flag
     *
     * [Bit 19] See the description in EFLAGS.
     */
    uint64_t virtual_interrupt_flag : 1;
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_BIT 19
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_FLAG 0x80000
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG_MASK 0x01
#define RFLAGS_VIRTUAL_INTERRUPT_FLAG(_) (((_) >> 19) & 0x01)

    /**
     * @brief Virtual interrupt pending flag
     *
     * [Bit 20] See the description in EFLAGS.
     */
    uint64_t virtual_interrupt_pending_flag : 1;
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_BIT 20
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_FLAG 0x100000
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG_MASK 0x01
#define RFLAGS_VIRTUAL_INTERRUPT_PENDING_FLAG(_) (((_) >> 20) & 0x01)

    /**
     * @brief Identification flag
     *
     * [Bit 21] See the description in EFLAGS.
     */
    uint64_t identification_flag : 1;
#define RFLAGS_IDENTIFICATION_FLAG_BIT 21
#define RFLAGS_IDENTIFICATION_FLAG_FLAG 0x200000
#define RFLAGS_IDENTIFICATION_FLAG_MASK 0x01
#define RFLAGS_IDENTIFICATION_FLAG(_) (((_) >> 21) & 0x01)
    uint64_t reserved4 : 42;
  };

  uint64_t flags;
} rflags;

#define ABS_TO_IMG(addr, mod_base, img_base) (addr - mod_base) + img_base
Q_DECLARE_METATYPE(vm::instrs::virt_instr_t)

struct rtn_data_t {
  std::uint32_t rtn_rva;
  std::vector<vm::instrs::code_block_t> rtn_blks;
};

class qvm_inspector : public QMainWindow {
  friend class qvm_virtual_instructions;
  friend class qvm_handlers;
  friend class qvm_virtual_routines;
  Q_OBJECT
 public:
  qvm_inspector(QWidget *parent = Q_NULLPTR);

 private slots:
  void on_open();
  void on_close();

 private:
  void dbg_print(QString DbgOutput);
  void dbg_msg(QString DbgOutput);
  void update_ui();
  bool serialize_vmp2(std::vector<rtn_data_t> &virt_rtns);
  void update_virtual_instructions(std::uintptr_t rtn_addr,
                                   std::uintptr_t blk_addr = 0ull,
                                   QTreeWidgetItem *parent = nullptr);
  bool init_data();

  Ui::QVMProfilerClass ui;
  std::uint64_t img_base, module_base, img_size;
  vm::ctx_t *g_vm_ctx;

  vmp2::v4::file_header *file_header;
  std::vector<rtn_data_t> virt_rtns;
  std::vector<std::uintptr_t> code_block_addrs;
};