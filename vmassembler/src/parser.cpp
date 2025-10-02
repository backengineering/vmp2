#include <parser.hpp>

parse_t::parse_t() {}

auto parse_t::get_instance() -> parse_t* {
  static parse_t obj;
  return &obj;
}

void parse_t::add_label(std::string label_name) {
  // remove ":" from the end of the label name...
  label_name.erase(label_name.end() - 1);
  virt_labels.push_back({label_name});
}

void parse_t::add_vinstr(std::string vinstr_name) {
  virt_labels.back().vinstrs.push_back({vinstr_name, false, 0u});
}

void parse_t::add_vinstr(std::string vinstr_name, std::uintptr_t imm_val) {
  virt_labels.back().vinstrs.push_back({vinstr_name, true, imm_val});
}

bool parse_t::for_each(callback_t callback) {
  for (auto& entry : virt_labels)
    if (!callback(&entry))
      return false;

  return true;
}