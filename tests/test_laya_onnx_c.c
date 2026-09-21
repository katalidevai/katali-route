#include <stdio.h>
#include <string.h>
#include <time.h>
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
#include "katali_route_laya_ort.h"

typedef const OrtApiBase *(ORT_API_CALL *GetApiBaseFn)(void);
static int fail(const OrtApi *api, OrtStatus *status) { if (status) { fprintf(stderr, "ORT: %s\n", api->GetErrorMessage(status)); api->ReleaseStatus(status); } return 1; }

typedef struct {
    int64_t *ids; int64_t *mask; size_t sequence_length;
    int64_t *marker_pos; unsigned char *marker_mask; size_t option_count;
    int64_t *qtype; const char **labels;
} FixtureTokens;

static int fixture_tokenize(void *user, const KataliRouteRequest *request, KataliRouteLayaTokens *tokens) {
    FixtureTokens *fixture = (FixtureTokens *)user;
    KataliRouteTokenizerOutput output;
    (void)request;
    output.input_ids = fixture->ids; output.attention_mask = fixture->mask;
    output.marker_pos = fixture->marker_pos; output.marker_mask = fixture->marker_mask;
    output.labels = fixture->labels; output.sequence_capacity = 8;
    output.option_capacity = 2; output.sequence_length = fixture->sequence_length;
    output.option_count = fixture->option_count; output.question_type = *fixture->qtype;
    return katali_route_laya_tokens_from_tokenizer(&output, tokens) == KATALI_ROUTE_OK ? 0 : -1;
}

int main(void) {
    HMODULE module = LoadLibraryA("onnxruntime.dll"); FARPROC raw; GetApiBaseFn get_api_base;
    const OrtApiBase *base; const OrtApi *api; OrtStatus *status = NULL;
    OrtEnv *env = NULL; OrtSessionOptions *options = NULL; OrtSession *session = NULL; OrtMemoryInfo *memory = NULL;
    OrtValue *values[5] = {0}; OrtValue *outputs[2] = {0}; void *logits = NULL; void *act_probs = NULL;
    const char *names[] = {"input_ids", "attention_mask", "marker_pos", "marker_mask", "qtype"};
    const char *out_names[] = {"logits", "act_probs"};
    int64_t ids[8] = {101, 7590, 2023, 6251, 102, 0, 0, 0}; int64_t mask[8] = {1,1,1,1,1,0,0,0};
    int64_t marker_pos[2] = {2,4}; unsigned char marker_mask[2] = {1,1}; int64_t qtype[1] = {0};
    int64_t shapes[5][2] = {{1,8},{1,8},{1,2},{1,2},{1,0}};
    void *data[] = {ids, mask, marker_pos, marker_mask, qtype}; ONNXTensorElementDataType types[] = {
        ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64};
    size_t bytes[] = {sizeof(ids), sizeof(mask), sizeof(marker_pos), sizeof(marker_mask), sizeof(qtype)};
    size_t i; int run; double start;
    if (!module) return 1;
    raw = GetProcAddress(module, "OrtGetApiBase");
    memcpy(&get_api_base, &raw, sizeof(get_api_base));
    if (!get_api_base) return 2;
    base = get_api_base(); api = base->GetApi(ORT_API_VERSION); if (!api) return 3;
    if ((status = api->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "katali-route-laya-smoke", &env))) return fail(api, status);
    if ((status = api->CreateSessionOptions(&options))) return fail(api, status);
    if ((status = api->CreateSession(env, L"work\\laya-onnx\\laya.onnx", options, &session))) return fail(api, status);
    if ((status = api->CreateCpuMemoryInfo(OrtDeviceAllocator, OrtMemTypeDefault, &memory))) return fail(api, status);
    { KataliRouteLayaOrt runtime = {api, session, memory, 0.0}; KataliRouteTypedDecision typed; const char *labels[] = {"small", "large"};
      if (katali_route_laya_ort_choice(&runtime, ids, mask, 8, marker_pos, marker_mask, 2, 0, labels, &typed) != KATALI_ROUTE_OK) return 4;
      printf("katali_laya_choice=%s confidence=%.6f\n", typed.label, typed.value);
      { KataliRouteModel models[] = {{"small", "katali-gguf", 4096, 128, 1}, {"large", "katali-gguf", 32768, 512, 1}};
        KataliRouteRequest request = {"laya-c", "fixture", models, 2, 1, 0, 0}; KataliRouteDecision decision;
        FixtureTokens fixture = {ids, mask, 8, marker_pos, marker_mask, 2, qtype, labels};
        KataliRouteLayaOrtProvider ort_provider = {{api, session, memory, 0.0}, fixture_tokenize, &fixture};
        KataliRouteDecisionProvider provider = katali_route_laya_ort_provider(&ort_provider);
        if (katali_route_decide_with_provider(&request, &provider, &decision, &typed) != KATALI_ROUTE_OK || strcmp(decision.model_id, "large") != 0) return 5;
        printf("katali_route_model=%s\n", decision.model_id); }
    }
    for (i = 0; i < 5; ++i) {
        size_t rank = i == 4 ? 1 : 2;
        if ((status = api->CreateTensorWithDataAsOrtValue(memory, data[i], bytes[i], shapes[i], rank, types[i], &values[i]))) return fail(api, status);
    }
    start = (double)clock();
    for (run = 0; run < 10; ++run) {
        outputs[0] = outputs[1] = NULL;
        if ((status = api->Run(session, NULL, names, (const OrtValue *const *)values, 5, out_names, 2, outputs))) return fail(api, status);
        if ((status = api->GetTensorMutableData(outputs[0], &logits))) return fail(api, status);
        if ((status = api->GetTensorMutableData(outputs[1], &act_probs))) return fail(api, status);
        if (run == 9) printf("laya_onnx_c_logits_shape=1x2 act_probs=[%.6f,%.6f] warm_cpu_ms=%.3f\n", ((float *)act_probs)[0], ((float *)act_probs)[1], ((double)clock() - start) * 1000.0 / CLOCKS_PER_SEC / 10.0);
        api->ReleaseValue(outputs[0]); api->ReleaseValue(outputs[1]);
    }
    for (i = 0; i < 5; ++i) api->ReleaseValue(values[i]);
    api->ReleaseMemoryInfo(memory);
    api->ReleaseSession(session);
    api->ReleaseSessionOptions(options);
    api->ReleaseEnv(env);
    FreeLibrary(module);
    return 0;
}
