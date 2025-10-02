#include "vmhook.hpp"

namespace vm
{
	namespace handler
	{
		table_t::table_t(u64* table_addr, edit_entry_t edit_entry)
			: 
			table_addr(table_addr),
			edit_entry(edit_entry)
		{}

		u64 table_t::get_entry(u8 idx) const
		{
			return table_addr[idx];
		}

		entry_t table_t::get_meta_data(u8 idx) const
		{
			return handlers[idx];
		}

		void table_t::set_entry(u8 idx, u64 entry)
		{
			edit_entry(table_addr + idx, entry);
		}

		void table_t::set_meta_data(u8 idx, const entry_t& entry)
		{
			handlers[idx] = entry;
		}

		void table_t::set_callback(u8 idx, entry_callback_t callback)
		{
			handlers[idx].callback = callback;
		}
	}

	hook_t::hook_t(
		u64 module_base,
		u64 image_base,
		decrypt_handler_t decrypt_handler, 
		encrypt_handler_t encrypt_handler,
		vm::handler::table_t* vm_handler_table
	)
		: 
		decrypt_handler(decrypt_handler),
		encrypt_handler(encrypt_handler),
		handler_table(vm_handler_table),
		module_base(module_base),
		image_base(image_base)
	{
		for (auto idx = 0u; idx < 256; ++idx)
		{
			vm::handler::entry_t entry = 
				vm_handler_table->get_meta_data(idx);

			entry.encrypted = vm_handler_table->get_entry(idx);
			entry.decrypted = decrypt(entry.encrypted);
			entry.virt = (entry.decrypted - image_base) + module_base;
			vm_handler_table->set_meta_data(idx, entry);
		}
		
		vm::g_vmctx = this;
		vtrap_encrypted = encrypt(
			(reinterpret_cast<std::uintptr_t>(
				&__vtrap) - module_base) + image_base);
	}

	u64 hook_t::encrypt(u64 val) const
	{
		return encrypt_handler(val);
	}

	u64 hook_t::decrypt(u64 val) const
	{
		return decrypt_handler(val);
	}

	void hook_t::set_trap(u64 val) const
	{
		for (auto idx = 0u; idx < 256; ++idx)
			handler_table->set_entry(idx, val);
	}

	void hook_t::start() const
	{
		for (auto idx = 0u; idx < 256; ++idx)
			handler_table->set_entry(idx, vtrap_encrypted);
	}

	void hook_t::stop() const
	{
		for (auto idx = 0u; idx < 256; ++idx)
		{
			const auto handler_entry = 
				handler_table->get_meta_data(idx).encrypted;

			handler_table->set_entry(idx, handler_entry);
		}
	}
}

void vtrap_wrapper(vm::registers* regs, u8 handler_idx)
{
	regs->vm_handler = vm::g_vmctx->
		handler_table->get_meta_data(handler_idx).virt;

	const auto callback = vm::g_vmctx->
		handler_table->get_meta_data(handler_idx).callback;

	// per-virtual instruction callbacks...
	if (callback) callback(regs, handler_idx);
}