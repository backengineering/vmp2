#pragma once
#include <cstdint>
#include <xmmintrin.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
using u128 = __m128;
extern "C" void __vtrap(void);

namespace vm
{
	typedef struct _registers
	{
		u128 xmm0;
		u128 xmm1;
		u128 xmm2;
		u128 xmm3;
		u128 xmm4;
		u128 xmm5;
		u128 xmm6;
		u128 xmm7;
		u128 xmm8;
		u128 xmm9;
		u128 xmm10;
		u128 xmm11;
		u128 xmm12;
		u128 xmm13;
		u128 xmm14;
		u128 xmm15;

		u64 gap0;

		u64 r15;
		u64 r14;
		u64 r13;
		u64 r12;
		u64 r11;
		u64 r10;
		u64 r9;
		u64 r8;
		u64 rbp;
		u64 rdi;
		u64 rsi;
		u64 rdx;
		u64 rcx;
		u64 rbx;
		u64 rax;
		u64 rflags;
		u64 vm_handler;
	} registers, * pregisters;

	using decrypt_handler_t = u64(*)(u64);
	using encrypt_handler_t = u64(*)(u64);

	namespace handler
	{
		// these lambdas handle page protections...
		using edit_entry_t = void (*)(u64*, u64);
		using entry_callback_t = void (*)(vm::registers* regs, u8 handler_idx);

		struct entry_t
		{
			u64 virt;
			u64 encrypted;
			u64 decrypted;
			entry_callback_t callback;
		};

		class table_t
		{
		public:
			explicit table_t(u64* table_addr, edit_entry_t edit_entry);
			u64 get_entry(u8 idx) const;
			entry_t get_meta_data(u8 idx) const;

			void set_entry(u8 idx, u64 entry);
			void set_meta_data(u8 idx, const entry_t& entry);
			void set_callback(u8 idx, entry_callback_t callback);
		private:
			u64* table_addr;
			edit_entry_t edit_entry;
			entry_t handlers[256];
		};
	}

	class hook_t
	{
	public:
		explicit hook_t(
			u64 module_base,
			u64 image_base,
			decrypt_handler_t decrypt_handler,
			encrypt_handler_t encrypt_handler,
			vm::handler::table_t* vm_handler_table
		);

		u64 encrypt(u64 val) const;
		u64 decrypt(u64 val) const;
		void set_trap(u64 val) const;

		void start() const;
		void stop() const;

		vm::handler::table_t* handler_table;
	private:
		const u64 module_base, image_base;
		u64 vtrap_encrypted;

		const decrypt_handler_t decrypt_handler;
		const encrypt_handler_t encrypt_handler;
	};

	inline vm::hook_t* g_vmctx = nullptr;
}

extern "C" void vtrap_wrapper(vm::registers * regs, u8 handler_idx);