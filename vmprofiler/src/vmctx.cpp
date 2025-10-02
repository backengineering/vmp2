#include <vmprofiler.hpp>

namespace vm {
ctx_t::ctx_t(std::uintptr_t module_base, std::uintptr_t image_base,
             std::uintptr_t image_size, std::uintptr_t vm_entry_rva)
    : module_base(module_base),
      image_base(image_base),
      image_size(image_size),
      vm_entry_rva(vm_entry_rva) {}

bool ctx_t::init() {
  vm::util::init();
  if (!vm::util::flatten(vm_entry, vm_entry_rva + module_base)) return false;

  vm::util::deobfuscate(vm_entry);
  if (!vm::calc_jmp::get(vm_entry, calc_jmp)) return false;

  if (auto vm_handler_table = vm::handler::table::get(vm_entry);
      !vm::handler::get_all(module_base, image_base, vm_entry, vm_handler_table,
                            vm_handlers))
    return false;

  if (auto advancement = vm::calc_jmp::get_advancement(calc_jmp);
      advancement.has_value())
    exec_type = advancement.value();
  else
    return false;

  return true;
}
}  // namespace vm