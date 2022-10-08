#include <stdio.h>
#include <unwind.h>

extern "C" void arm_function_3(int*p);
extern "C" void thumb_function_1(int*p);
extern "C" void thumb_function_2(int*p);

extern "C" _Unwind_Reason_Code trace_function(_Unwind_Context *context, void *arg)
{
    int i = 0;
    printf("0x%x\n", _Unwind_GetIP(context));
    fflush(stdout);
    return _URC_NO_REASON;
}

void thumb_function_1(int*p)
{
    int a = 0;
    arm_function_3(&a);
}

void thumb_function_2(int*p)
{
    int a = 0;
    printf("unwinding...\n");
    _Unwind_Backtrace(trace_function, (void*)"backtrace!");
}
