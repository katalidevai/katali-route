#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include "katali_route_gguf_sdk.h"

int main(int argc, char **argv) {
    KataliRouteGgufSdk adapter; KataliRouteGgufClient client; KataliRouteDecision decision; katali_sdk_stats stats;
    const char *model_id = argc > 1 ? argv[1] : "qwen3-8b";
    const char *model_path = argc > 2 ? argv[2] : "C:\\models\\qwen3-8b-gguf\\Qwen3-8B-Q4_K_M.gguf";
    KataliRouteModel models[] = {{NULL, "katali-gguf", 2048, 32, 1}};
    KataliRouteRequest request = {"gguf-e2e", "Answer in one short sentence: what is 2 plus 2?", models, 1, 1, 0, 0};
    char output[4096]; clock_t start; double total_ms;
    models[0].id = model_id;
    if (katali_route_gguf_sdk_open(&adapter, model_id, model_path, 0, 2048) != KATALI_ROUTE_OK) {
        fprintf(stderr, "gguf_open_error=%s\n", katali_route_gguf_sdk_error()); return 1;
    }
    client = katali_route_gguf_sdk_client(&adapter);
    start = clock();
    if (katali_route_execute(&request, &client, output, sizeof(output), &decision) != KATALI_ROUTE_OK) {
        fprintf(stderr, "gguf_generate_error=%s\n", katali_route_gguf_sdk_error()); katali_route_gguf_sdk_close(&adapter); return 1;
    }
    total_ms = (double)(clock() - start) * 1000.0 / CLOCKS_PER_SEC;
    memset(&stats, 0, sizeof(stats)); stats.struct_size = sizeof(stats); katali_sdk_session_stats(adapter.session, &stats);
    printf("gguf_e2e_model=%s action=%s generation_ms=%.3f sdk_total_ms=%.3f prompt_tokens=%d generated_tokens=%d decode_tps=%.3f output=%s\n", decision.model_id, katali_route_action_name(decision.action), total_ms, stats.last_total_s * 1000.0, stats.last_prompt_tokens, stats.last_generated_tokens, stats.last_decode_tps, output);
    katali_route_gguf_sdk_close(&adapter); return 0;
}
