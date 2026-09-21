#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "katali_route_gguf_sdk.h"

int main(void) {
    KataliRouteModel models[] = {
        {"qwen3-0.6b", "katali-gguf", 2048, 32, 1},
        {"qwen3-1.7b", "katali-gguf", 2048, 32, 1}
    };
    const char *paths[] = {
        "C:\\models\\qwen3-0.6b-gguf\\Qwen3-0.6B-Q4_K_M.gguf",
        "C:\\models\\qwen3-1.7b-gguf\\Qwen3-1.7B.Q4_K_M.gguf"
    };
    KataliRouteGgufPool pool; KataliRouteGgufClient client; KataliRouteRequest request;
    KataliRouteDecision decision; char output[4096];
    assert(katali_route_gguf_pool_open(&pool, models, paths, 2, 0, 2048) == KATALI_ROUTE_OK);
    client = katali_route_gguf_pool_client(&pool);
    request = (KataliRouteRequest){"pool", "Answer with one number: 2+2", models, 2, 1, 0, 0};
    models[0].available = 0;
    assert(katali_route_execute(&request, &client, output, sizeof(output), &decision) == KATALI_ROUTE_OK);
    printf("gguf_pool_model=%s output=%s\n", decision.model_id, output);
    models[0].available = 1; models[1].available = 0;
    assert(katali_route_execute(&request, &client, output, sizeof(output), &decision) == KATALI_ROUTE_OK);
    printf("gguf_pool_escalated_model=%s output=%s\n", decision.model_id, output);
    decision.model_id = models[0].id; decision.confidence = 0.25;
    assert(katali_route_gguf_pool_apply_escalation(&pool, 0.80, &decision) == KATALI_ROUTE_OK);
    assert(!strcmp(decision.model_id, "qwen3-1.7b") && decision.action == KATALI_ROUTE_ESCALATE);
    pool.entries[1].available = 0; decision.model_id = models[0].id;
    assert(katali_route_gguf_pool_apply_escalation(&pool, 0.80, &decision) == KATALI_ROUTE_OK);
    assert(!strcmp(decision.model_id, "qwen3-0.6b"));
    katali_route_gguf_pool_close(&pool); return 0;
}
