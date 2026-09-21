#include "katali_route_laya.h"
#include <string.h>

static int laya_evaluate(void *user, const KataliRouteRequest *request,
                         KataliRouteTypedDecision *result) {
    KataliRouteLayaProvider *provider = (KataliRouteLayaProvider *)user;
    KataliRouteLayaAnswer answer;
    if (!provider || !provider->evaluate || !result) return -1;
    memset(&answer, 0, sizeof(answer));
    if (provider->evaluate(provider->user, request, &answer) != 0) return -1;
    result->kind = answer.kind;
    result->label = answer.label;
    result->value = answer.value;
    result->probabilities = answer.probabilities;
    result->probability_count = answer.probability_count;
    return 0;
}

KataliRouteDecisionProvider katali_route_laya_provider(
    const KataliRouteLayaProvider *provider) {
    KataliRouteDecisionProvider result = {laya_evaluate, (void *)provider, "laya"};
    return result;
}
