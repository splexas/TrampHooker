# TrampHooker
A mechanism that trampoline hooks functions in x86/x64 systems.
# How does trampoline hooking work?
Let's say, the target function contains this assembly code:
```asm
1. 8B FF:     mov edi, edi
2. 55:        push ebp
3. 8B EC:     mov ebp, esp
4. 83 EC 10:  sub esp, 10
```

1. Create your own C++ detour function, make a `typedef` prototype and return it on the end of your function as you need to call the original function (gateway; trampoline).
```cpp
typedef void(*target_function)(int a, int b, int c);
target_function trampoline_function = nullptr;
void some_function(int a, int b, int c) {
    return trampoline_function(a, b, c);
}
```
2. Create a gateway (codecave), allocate atleast 5 bytes for the `jmp` and add `mov eax/rax, address`, `push eax/rax`, `ret` instructions' sizes in bytes too to make sure it jumps out the gateway and keeps the program flow going.
3. Copy the 5 bytes from the target function to the gateway, it basically redirects the code of the target function to the gateway, so you can call it anytime.
```asm
1. 8B FF:     mov edi, edi
2. 55:        push ebp
3. 8B EC:     mov ebp, esp
```
4. Fill out the additional bytes you've allocated earlier for the gateway. You can get those bytes from Cheat Engine by writing your own instructions. Make sure the address is `target_function_address + 5`, the next instruction after later detoured `jmp`.
```asm
1. 8B FF:                      mov edi, edi
2. 55:                         push ebp
3. 8B EC:                      mov ebp, esp
4. (48) B8 xxxxxxxx(xxxxxxxx): mov eax/rax, target_function_address+5
5. 50:                         push eax/rax
6. C3:                         ret
```
6. Cast the gateway address as the prototype you've defined earlier.
7. Detour (modify) the target function's first 5 bytes with `jmp` to your C++ function address. Relative `jmp` offset formula is: `src-dst-5`.
```asm
1. E9 xxxxxxxx: jmp your_function
4. 83 EC 10:    sub esp, 10
```
# Summary
![image](https://github.com/splexas/TrampHooker/assets/62573774/edaaaeee-5ffb-4134-b20f-488781fe5368)
