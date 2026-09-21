#include <assert.h>
#include <stdio.h>
#include "katali_route_config.h"
#include "katali_route_gguf_sdk.h"

int main(void) {
    KataliRouteConfig config; KataliRouteModel models[4]; KataliRouteGgufPool pool;
    KataliRouteGgufClient client; KataliRouteRequest request; KataliRouteDecision decision; char output[4096];
    assert(katali_route_config_load("config\\route.conf.example", &config) == KATALI_ROUTE_OK);
    config.pool_load_mode = KATALI_ROUTE_CONFIG_MODE_LAZY;
    katali_route_config_models(&config, models);
    assert(katali_route_gguf_pool_open_config(&pool, &config) == KATALI_ROUTE_OK);
    assert(pool.count == 4 && !pool.entries[0].loaded && !pool.entries[3].loaded);
    request = (KataliRouteRequest){"lazy", "Answer with one number: 2+2", models, 4, 1, 0, 0};
    client = katali_route_gguf_pool_client(&pool);
    assert(katali_route_decide(&request, &decision) == KATALI_ROUTE_OK);
    assert(katali_route_execute_decision(&request, &client, output, sizeof(output), &decision) == KATALI_ROUTE_OK);
    assert(pool.entries[0].loaded && pool.entries[0].load_ms >= 0.0);
    assert(!pool.entries[1].loaded && !pool.entries[2].loaded && !pool.entries[3].loaded);
    printf("gguf_lazy_loaded=%s load_ms=%.3f output=%s\n", pool.entries[0].model_id, pool.entries[0].load_ms, output);
    katali_route_gguf_pool_close(&pool); return 0;
}
