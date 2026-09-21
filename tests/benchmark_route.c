#include "katali_route.h"
#include <stdio.h>
#include <time.h>

static int fixture_provider(void *user, const KataliRouteRequest *request, KataliRouteTypedDecision *out) {
    (void)user; (void)request; out->kind = KATALI_ROUTE_DECISION_CONFIDENCE; out->value = 0.9; return 0;
}

static double seconds(void) { return (double)clock() / (double)CLOCKS_PER_SEC; }

int main(void) {
    enum { ITERATIONS = 100000, BATCH = 64 };
    KataliRouteModel model = {"bench", "katali-gguf", 32768, 512, 1};
    KataliRouteRequest request = {"bench", "benchmark request", &model, 1, 1, 0, 0};
    KataliRouteDecision decision;
    KataliRouteDecisionProvider provider = {fixture_provider, NULL, "fixture"};
    KataliRouteTypedDecision typed;
    double start, direct_ms, provider_ms, batch_ms;
    int i;
    start = seconds();
    for (i = 0; i < ITERATIONS; ++i) katali_route_decide(&request, &decision);
    direct_ms = (seconds() - start) * 1000.0 / ITERATIONS;
    start = seconds();
    for (i = 0; i < ITERATIONS; ++i) katali_route_decide_with_provider(&request, &provider, &decision, &typed);
    provider_ms = (seconds() - start) * 1000.0 / ITERATIONS;
    start = seconds();
    for (i = 0; i < BATCH; ++i) katali_route_decide(&request, &decision);
    batch_ms = (seconds() - start) * 1000.0;
    printf("route_single_ms=%.6f provider_single_ms=%.6f route_batch_%d_ms=%.6f\n", direct_ms, provider_ms, BATCH, batch_ms);
    return 0;
}
