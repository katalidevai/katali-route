#ifndef KATALI_ROUTE_H
#define KATALI_ROUTE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const char *id;
    const char *backend;
    int context_length;
    int max_output_tokens;
    int available;
} KataliRouteModel;

typedef struct {
    const char *request_id;
    const char *text;
    const KataliRouteModel *models;
    size_t model_count;
    int routing_enabled;
    int has_tools;
    int output_json;
} KataliRouteRequest;

typedef enum {
    KATALI_ROUTE_GENERATE,
    KATALI_ROUTE_RETRY,
    KATALI_ROUTE_ESCALATE,
    KATALI_ROUTE_BYPASS
} KataliRouteAction;

typedef struct {
    const char *request_id;
    KataliRouteAction action;
    const char *model_id;
    double confidence;
    int validate_output;
    int retry_budget;
    char rationale[256];
} KataliRouteDecision;

typedef enum {
    KATALI_ROUTE_DECISION_CHOICE,
    KATALI_ROUTE_DECISION_BOOLEAN,
    KATALI_ROUTE_DECISION_SCORE,
    KATALI_ROUTE_DECISION_CONFIDENCE
} KataliRouteDecisionKind;

typedef struct {
    KataliRouteDecisionKind kind;
    const char *label;
    double value;
    const double *probabilities;
    size_t probability_count;
} KataliRouteTypedDecision;

typedef struct {
    int (*evaluate)(void *user, const KataliRouteRequest *request,
                    KataliRouteTypedDecision *result);
    void *user;
    const char *name;
} KataliRouteDecisionProvider;

typedef enum {
    KATALI_ROUTE_REASON_ROUTING_DISABLED,
    KATALI_ROUTE_REASON_NO_AVAILABLE_MODEL,
    KATALI_ROUTE_REASON_NO_CAPABLE_MODEL,
    KATALI_ROUTE_REASON_MODEL_SELECTED,
    KATALI_ROUTE_REASON_VALIDATION_REQUIRED,
    KATALI_ROUTE_REASON_LOW_CONFIDENCE,
    KATALI_ROUTE_REASON_VALIDATION_FAILED,
    KATALI_ROUTE_REASON_PROVIDER_ERROR
} KataliRouteReason;

typedef struct {
    double minimum_confidence;
    int max_retries;
    int validate_structured_output;
} KataliRoutePolicy;

typedef void (*KataliRouteTraceFn)(KataliRouteReason reason,
                                   const KataliRouteDecision *decision,
                                   void *user);
const char *katali_route_reason_name(KataliRouteReason reason);

typedef struct {
    const char *model_id;
    const char *backend;
    const char *endpoint;
    int context_length;
    int max_output_tokens;
    int available;
} KataliRouteModelInfo;

typedef struct {
    int (*generate)(void *user, const char *model_id, const char *prompt,
                    char *output, size_t output_cap);
    int (*validate)(void *user, const char *output);
    void *user;
} KataliRouteGgufClient;

typedef enum {
    KATALI_ROUTE_OK = 0,
    KATALI_ROUTE_ERR_ARGUMENT = -1,
    KATALI_ROUTE_ERR_BUFFER = -2,
    KATALI_ROUTE_ERR_MODEL = -3
} KataliRouteStatus;

KataliRouteStatus katali_route_trace_json(KataliRouteReason reason,
                                          const KataliRouteDecision *decision,
                                          char *output, size_t output_cap);

KataliRouteStatus katali_route_decide(const KataliRouteRequest *request,
                                      KataliRouteDecision *decision);
KataliRouteStatus katali_route_validate_request(const KataliRouteRequest *request);
KataliRouteStatus katali_route_decide_with_policy(
    const KataliRouteRequest *request,
    const KataliRoutePolicy *policy,
    KataliRouteTraceFn trace,
    void *trace_user,
    KataliRouteDecision *decision);
KataliRouteStatus katali_route_decide_with_provider(
    const KataliRouteRequest *request,
    const KataliRouteDecisionProvider *provider,
    KataliRouteDecision *decision,
    KataliRouteTypedDecision *typed);
KataliRouteStatus katali_route_decide_with_provider_policy(
    const KataliRouteRequest *request,
    const KataliRouteDecisionProvider *provider,
    const KataliRoutePolicy *policy,
    KataliRouteTraceFn trace,
    void *trace_user,
    KataliRouteDecision *decision,
    KataliRouteTypedDecision *typed);
const char *katali_route_action_name(KataliRouteAction action);
KataliRouteStatus katali_route_decision_json(const KataliRouteDecision *decision,
                                             char *output, size_t output_cap);
KataliRouteStatus katali_route_typed_decision_json(const KataliRouteTypedDecision *typed,
                                                   char *output, size_t output_cap);
KataliRouteStatus katali_route_model_json(const KataliRouteModelInfo *model,
                                          char *output, size_t output_cap);
KataliRouteStatus katali_route_execute(const KataliRouteRequest *request,
                                       const KataliRouteGgufClient *client,
                                       char *output, size_t output_cap,
                                       KataliRouteDecision *decision);
KataliRouteStatus katali_route_execute_decision(
    const KataliRouteRequest *request,
    const KataliRouteGgufClient *client,
    char *output, size_t output_cap,
    KataliRouteDecision *decision);

#ifdef __cplusplus
}
#endif
#endif
