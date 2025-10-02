#include <intrin.h>
#include <iostream>
#include <Windows.h>
#include "ia32.h"

int main()
{
	cpuid_eax_01 cpuid_info;
	__cpuid((int*)&cpuid_info, 1);

	if (cpuid_info.cpuid_feature_information_ecx.avx_support)
	{
		MessageBoxA(NULL, "CPU Supports AVX", "INFO", NULL);
		return 1;
	}

	MessageBoxA(NULL, "CPU Does Not Supports AVX", "INFO", NULL);
	return 0;
}