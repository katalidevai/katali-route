#ifndef KATALI_ROUTE_LAYA_HOST_H
#define KATALI_ROUTE_LAYA_HOST_H

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
#include "katali_route_laya_ort.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    HMODULE dll;
    const OrtApi *api;
    OrtEnv *env;
    OrtSessionOptions *options;
    OrtSession *session;
    OrtMemoryInfo *memory;
    char *tokenizer_json;
    KataliRouteLayaTokenizer tokenizer;
    int64_t input_ids[512];
    int64_t attention_mask[512];
    int64_t marker_pos[64];
    unsigned char marker_mask[64];
    const char *labels[64];
    KataliRouteTokenizerOutput output;
    KataliRouteLayaOrtTokenizerProvider provider;
    char error[256];
} KataliRouteLayaHost;

KataliRouteStatus katali_route_laya_host_init(
    KataliRouteLayaHost *host, const char *tokenizer_json_path,
    const char *onnx_path, const char *runtime_dll_path);
void katali_route_laya_host_dispose(KataliRouteLayaHost *host);
KataliRouteDecisionProvider katali_route_laya_host_provider(KataliRouteLayaHost *host);
const char *katali_route_laya_host_error(const KataliRouteLayaHost *host);

#ifdef __cplusplus
}
#endif
#endif
