#include "memory.hpp"
#include <Windows.h>
#include <stdio.h>
#include <stdint.h>

Memory::Memory() {}
Memory::~Memory() {}

std::vector<HookedFunction> Memory::hooked_functions;

bool Memory::detour_function(char* original_function, char* detour_function, const int& bytes_length) {
	DWORD old_protect;

	if (!VirtualProtect(reinterpret_cast<LPVOID>(original_function), bytes_length, PAGE_EXECUTE_READWRITE, &old_protect))
		return false;

	int32_t relative_offset = reinterpret_cast<int32_t>(detour_function) - reinterpret_cast<int32_t>(original_function) - 5;
	*(original_function) = 0xE9;
	*reinterpret_cast<int32_t*>(original_function + 1) = static_cast<int32_t>(relative_offset);

	for (int i = 0; i < bytes_length - 5; i++) {
		*(original_function + 6 + i) = 0x90;
	}

	if (!VirtualProtect(reinterpret_cast<LPVOID>(original_function), bytes_length, old_protect, &old_protect))
		return false;

	return true;
}


char* Memory::create_gateway(char* original_function, const int& bytes_length) {
	int gateway_size = bytes_length + sizeof(intptr_t) + 1;

	#ifdef _M_X64
	gateway_size++; // allocate 1 more byte for 0x48
	#endif

	char* gateway = reinterpret_cast<char*>(VirtualAlloc(nullptr, gateway_size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if (gateway == nullptr)
		return gateway;

	memcpy(gateway, original_function, bytes_length);

	char* gateway_end = gateway + bytes_length;

	#ifdef _M_X64
	*gateway_end = 0x48;
	gateway_end = gateway + bytes_length + 1;
	#endif

	// mov eax/rax, original_function+5
	*gateway_end = 0xB8;

	*reinterpret_cast<intptr_t*>(gateway_end + 1) = reinterpret_cast<intptr_t>(original_function + 5);

	// push eax/rax
	*(gateway_end + sizeof(intptr_t) + 1) = 0x50;

	// ret
	*(gateway_end + sizeof(intptr_t) + 2) = 0xC3;
	
	return gateway;
}


char* Memory::get_exported_function_address(const char* module_name, const char* function_name) {
	HMODULE module_handle = GetModuleHandleA(module_name);
	if (!module_handle)
		return 0;

	return reinterpret_cast<char*>(GetProcAddress(module_handle, function_name));
}

char* Memory::hook_function(char* original_function, char* detour_function, const int& bytes_length) {
	// check if there are sufficient bytes for a relative jmp (5 bytes)
	if (bytes_length < 5)
		return nullptr;

	char* gateway = Memory::create_gateway(original_function, bytes_length);
	if (gateway == nullptr) {
		printf("Could not create the gateway.\n");
		return nullptr;
	}

	if (!Memory::detour_function(original_function, detour_function, bytes_length)) {
		printf("Could not detour the function.\n");
		return nullptr;
	}

	HookedFunction hooked = { 0 };

	hooked.original_function = original_function;
	hooked.gateway = gateway;
	hooked.bytes_length = bytes_length;

	Memory::hooked_functions.push_back(hooked);
	return gateway;
}

bool Memory::unhook_function(const HookedFunction& hooked_function) {
	DWORD old_protect;

	if (!VirtualProtect(reinterpret_cast<LPVOID>(hooked_function.original_function), hooked_function.bytes_length, PAGE_EXECUTE_READWRITE, &old_protect))
		return false;

	memcpy(hooked_function.original_function, hooked_function.gateway, hooked_function.bytes_length);

	if (!VirtualProtect(reinterpret_cast<LPVOID>(hooked_function.original_function), hooked_function.bytes_length, old_protect, &old_protect))
		return false;

	if (!VirtualFree(reinterpret_cast<LPVOID>(hooked_function.gateway), 0, MEM_RELEASE))
		return false;

	return true;
}

bool Memory::unhook_functions() {
	for (HookedFunction& hooked_function : Memory::hooked_functions) {
		if (Memory::unhook_function(hooked_function))
			return false;
	}

	return true;
}