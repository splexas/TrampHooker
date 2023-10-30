#ifndef MEMORY_HPP
#define MEMORY_HPP

#include <vector>

struct HookedFunction {
	int bytes_length;
	char* original_function;
	char* gateway;
};


class Memory {
private:
	// Replaces the `bytes_length` bytes of `original_function` with JMP instruction to `destination_function`.
	static bool detour_function(char* original_function, char* detour_function, const int& bytes_length);

	// Copies `bytes_length` bytes of `original_function` into the allocated gateway (trampoline).
	// ! Must be used before `detour_function`.
	static char* create_gateway(char* original_function, const int& bytes_length);

public:
	Memory();
	~Memory();

	static std::vector<HookedFunction> hooked_functions;

	static char* get_exported_function_address(const char* module_name, const char* function_name);

	// Returns the gateway (trampoline) function, aka. the original function.
	static char* hook_function(char* original_function, char* detour_function, const int& bytes_length = 5);

	// Copies `bytes_length` bytes from gateway to original_function, deletes the gateway after.
	static bool unhook_function(const HookedFunction& hooked_function);

	// Calls `unhook_function` for all the hooked functions.
	static bool unhook_functions();
};


#endif //MEMORY_HPP