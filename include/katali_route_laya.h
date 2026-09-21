#ifndef KATALI_ROUTE_LAYA_H
#define KATALI_ROUTE_LAYA_H

#include "katali_route.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    KataliRouteDecisionKind kind;
    const char *label;
    double value;
    const double *probabilities;
    size_t probability_count;
} KataliRouteLayaAnswer;

typedef int (*KataliRouteLayaEvaluateFn)(void *user,
                                         const KataliRouteRequest *request,
                                         KataliRouteLayaAnswer *answer);

typedef struct {
    KataliRouteLayaEvaluateFn evaluate;
    void *user;
    const char *model_revision;
} KataliRouteLayaProvider;

KataliRouteDecisionProvider katali_route_laya_provider(
    const KataliRouteLayaProvider *provider);

#ifdef __cplusplus
}
#endif
#endif
