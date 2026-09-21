#include <stdio.h>
#include <string.h>
#include <windows.h>
#undef _Check_return_
#undef _In_reads_
#undef _In_reads_opt_
#undef _Inout_updates_
#undef _Out_writes_
#undef _Out_writes_opt_
#undef _Inout_updates_all_
#undef _Out_writes_bytes_all_
#undef _Out_writes_all_
#undef _Success_
#undef _Outptr_result_buffer_maybenull_
#include "onnxruntime_c_api.h"

typedef const OrtApiBase *(ORT_API_CALL *GetApiBaseFn)(void);

int main(void) {
    HMODULE module = LoadLibraryA("onnxruntime.dll");
    const OrtApiBase *base;
    if (!module) { fprintf(stderr, "onnxruntime.dll not found\n"); return 1; }
    GetApiBaseFn get_api_base;
    { FARPROC raw = GetProcAddress(module, "OrtGetApiBase");
      memcpy(&get_api_base, &raw, sizeof(get_api_base)); }
    if (!get_api_base) { FreeLibrary(module); return 2; }
    base = get_api_base();
    if (!base || !base->GetApi || !base->GetVersionString) { FreeLibrary(module); return 3; }
    printf("onnxruntime_c_api=%u version=%s\n", base->GetApi(ORT_API_VERSION) != NULL ? ORT_API_VERSION : 0, base->GetVersionString());
    FreeLibrary(module);
    return 0;
}
