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

static int fail(const OrtApi *api, OrtStatus *status) {
    if (status) { fprintf(stderr, "ORT: %s\n", api->GetErrorMessage(status)); api->ReleaseStatus(status); }
    return 1;
}

int main(void) {
    HMODULE module = LoadLibraryA("onnxruntime.dll");
    GetApiBaseFn get_api_base; FARPROC raw; const OrtApiBase *base; const OrtApi *api;
    OrtEnv *env = NULL; OrtSessionOptions *options = NULL; OrtSession *session = NULL; OrtStatus *status = NULL;
    OrtMemoryInfo *memory = NULL; OrtValue *input = NULL; OrtValue *output = NULL; void *data = NULL;
    const char *input_names[] = {"x"}; const char *output_names[] = {"y"};
    float input_data[3 * 4 * 5] = {0.0f}; int64_t shape[] = {3, 4, 5};
    if (!module) return 1;
    raw = GetProcAddress(module, "OrtGetApiBase"); memcpy(&get_api_base, &raw, sizeof(get_api_base));
    if (!get_api_base) return 2;
    base = get_api_base(); api = base->GetApi(ORT_API_VERSION); if (!api) return 3;
    if ((status = api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "katali-route-smoke", &env))) return fail(api, status);
    if ((status = api->CreateSessionOptions(&options))) return fail(api, status);
    if ((status = api->CreateSession(env, L"build\\sigmoid.onnx", options, &session))) return fail(api, status);
    if ((status = api->CreateCpuMemoryInfo(OrtDeviceAllocator, OrtMemTypeDefault, &memory))) return fail(api, status);
    if ((status = api->CreateTensorWithDataAsOrtValue(memory, input_data, sizeof(input_data), shape, 3, ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input))) return fail(api, status);
    if ((status = api->Run(session, NULL, input_names, (const OrtValue *const *)&input, 1, output_names, 1, &output))) return fail(api, status);
    if ((status = api->GetTensorMutableData(output, &data))) return fail(api, status);
    printf("onnxruntime_c_inference=%.6f\n", *(float *)data);
    api->ReleaseValue(output); api->ReleaseValue(input); api->ReleaseMemoryInfo(memory);
    api->ReleaseSession(session); api->ReleaseSessionOptions(options); api->ReleaseEnv(env); FreeLibrary(module);
    return 0;
}
