#include <iostream>
#include "test.hpp"

int main()
{
    const auto hello = vm::call< vm::calls::get_hello, vm::u64 >();
    const auto world = vm::call< vm::calls::get_world, vm::u64 >();
    std::printf( "> %s %s\n", ( char * )&hello, (char*)&world );
}