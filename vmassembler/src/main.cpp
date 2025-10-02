#include <Windows.h>
#include <argparse.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <transform.hpp>
#include <xtils.hpp>

#include <parser.tab.h>
#include <compiler.hpp>
#include <gen_code.hpp>
#include <parser.hpp>
#include <vmprofiler.hpp>

extern FILE* yyin;
extern "C" int yywrap() {
  return 1;
}

void yyerror(char* msg) {
  std::printf("[!] parsing failure: %s\n", msg);
}

int __cdecl main(int argc, const char* argv[]) {
  argparse::ArgumentParser argp("vmassembler", "virtual instruction assembler");

  argp.add_argument()
      .names({"--input", "--in"})
      .description("path to a vasm file to be assembled...")
      .required(true);

  argp.add_argument()
      .names({"--vmpbin", "--bin"})
      .description("path to protected binary...")
      .required(true);
  argp.add_argument()
      .names({"--vmentry", "--entry"})
      .description("rva to vm entry...")
      .required(true);
  argp.add_argument()
      .names({"--out", "--output"})
      .description("output file name and path...")
      .required(true);

  argp.enable_help();
  auto err = argp.parse(argc, argv);

  if (err) {
    std::cout << err << std::endl;
    return -1;
  }

  if (argp.exists("help")) {
    argp.print_help();
    return 0;
  }

  //
  // set yyin to the vasm file...
  //

  if ((yyin = fopen(argp.get<std::string>("input").c_str(), "r")) == nullptr) {
    std::printf("[!] failed to open vasm file...\n");
    return -1;
  }

  //
  // parse vasm file for all of the instructions...
  //

  yyparse();
  std::printf("[+] finished parsing vasm file...\n");

  //
  // init vm variables...
  //

  const auto module_base = reinterpret_cast<std::uintptr_t>(
      LoadLibraryExA(argp.get<std::string>("vmpbin").c_str(), NULL,
                     DONT_RESOLVE_DLL_REFERENCES));

  const auto vm_entry_rva =
      std::strtoull(argp.get<std::string>("vmentry").c_str(), nullptr, 16);
  const auto image_base = xtils::um_t::get_instance()->image_base(
      argp.get<std::string>("vmpbin").c_str());
  const auto image_size = NT_HEADER(module_base)->OptionalHeader.SizeOfImage;
  vm::ctx_t vmctx(module_base, image_base, image_size, vm_entry_rva);

  if (!vmctx.init()) {
    std::printf(
        "> failed to init vmctx... make sure all arguments are valid\n"
        "and that the binary you are providing is unpacked and protected\n"
        "by VMProtect 2...\n");
    return -1;
  }

  std::printf("> flattened and deobfuscated vm entry...\n");
  vm::util::print(vmctx.vm_entry);
  std::printf("> extracted calc jmp from vm entry...\n");
  vm::util::print(vmctx.calc_jmp);

  vm::compiler_t compiler(&vmctx);

  //
  // encode virtual instructions...
  //

  auto virt_labels = compiler.encode();
  std::printf("[+] finished encoding... encoded instructions below...\n");

  for (auto& label : *virt_labels) {
    for (const auto& vinstr : label.vinstrs) {
      if (vinstr.imm_size)
        std::printf("> 0x%x - 0x%x\n", vinstr.vm_handler, vinstr.operand);
      else
        std::printf("> 0x%x\n", vinstr.vm_handler);
    }
  }

  //
  // encrypt virtual instructions...
  //

  auto compiled_labels = compiler.encrypt();
  std::printf("[+] finished encrypting... encrypted labels below...\n");

  for (const auto& label : compiled_labels) {
    std::printf("> %s must be allocated at = 0x%p, encrypted rva = 0x%p\n",
                label.label_name.c_str(), label.alloc_rva, label.enc_alloc_rva);

    std::printf("> ");
    {
      for (auto byte : label.vinstrs)
        std::printf("0x%x ", byte);

      std::printf("\n");
    }
  }

  const auto cpp_result =
      gen::code(compiled_labels, argp.get<std::string>("vmpbin"), vmctx);
  std::ofstream output(argp.get<std::string>("out"));
  output.write(cpp_result.data(), cpp_result.size());
  output.close();

  std::printf("> generated header file...\n");
}