#include "katali_route.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static int json_string(char **cursor, size_t *left, const char *value);

const char *katali_route_action_name(KataliRouteAction action) {
    switch (action) {
    case KATALI_ROUTE_GENERATE: return "generate";
    case KATALI_ROUTE_RETRY: return "retry";
    case KATALI_ROUTE_ESCALATE: return "escalate";
    case KATALI_ROUTE_BYPASS: return "bypass";
    default: return "unknown";
    }
}

const char *katali_route_reason_name(KataliRouteReason reason) {
    static const char *names[] = {"routing_disabled", "no_available_model", "no_capable_model", "model_selected", "validation_required", "low_confidence", "validation_failed", "provider_error"};
    return reason >= KATALI_ROUTE_REASON_ROUTING_DISABLED && reason <= KATALI_ROUTE_REASON_PROVIDER_ERROR ? names[reason] : "unknown_reason";
}

KataliRouteStatus katali_route_trace_json(KataliRouteReason reason,
                                          const KataliRouteDecision *decision,
                                          char *output, size_t cap) {
    char *p = output; size_t left = cap; int n;
    if (!decision || !output || cap < 2 || !decision->request_id) return KATALI_ROUTE_ERR_ARGUMENT;
    n = snprintf(p, left, "{\"event\":\"route_decision\",\"reason\":\"%s\",\"request_id\":\"", katali_route_reason_name(reason));
    if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER;
    p += n;
    left -= (size_t)n;
    if (json_string(&p, &left, decision->request_id)) return KATALI_ROUTE_ERR_BUFFER;
    n = snprintf(p, left, "\",\"action\":\"%s\",\"confidence\":%.6f}", katali_route_action_name(decision->action), decision->confidence);
    return n < 0 || (size_t)n >= left ? KATALI_ROUTE_ERR_BUFFER : KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_decide(const KataliRouteRequest *request,
                                      KataliRouteDecision *decision) {
    KataliRoutePolicy defaults = {0.0, 1, 1};
    return katali_route_decide_with_policy(request, &defaults, NULL, NULL, decision);
}

KataliRouteStatus katali_route_validate_request(const KataliRouteRequest *request) {
    size_t i;
    if (!request || !request->request_id || !request->request_id[0] || !request->text) return KATALI_ROUTE_ERR_ARGUMENT;
    if (request->model_count && !request->models) return KATALI_ROUTE_ERR_ARGUMENT;
    for (i = 0; i < request->model_count; ++i) {
        const KataliRouteModel *model = &request->models[i];
        if (!model->id || !model->id[0] || !model->backend || !model->backend[0]) return KATALI_ROUTE_ERR_ARGUMENT;
        if (model->context_length < 0 || model->max_output_tokens < 0) return KATALI_ROUTE_ERR_ARGUMENT;
    }
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_decide_with_policy(
    const KataliRouteRequest *request,
    const KataliRoutePolicy *policy,
    KataliRouteTraceFn trace,
    void *trace_user,
    KataliRouteDecision *decision) {
    size_t i;
    const KataliRouteModel *best = NULL;
    KataliRoutePolicy defaults = {0.0, 1, 1};
    if (!policy) policy = &defaults;
    if (policy->minimum_confidence < 0.0 || policy->minimum_confidence > 1.0 || policy->max_retries < 0) return KATALI_ROUTE_ERR_ARGUMENT;
    if (!decision || katali_route_validate_request(request) != KATALI_ROUTE_OK) return KATALI_ROUTE_ERR_ARGUMENT;
    memset(decision, 0, sizeof(*decision));
    decision->request_id = request->request_id;
    if (!request->routing_enabled) {
        decision->action = KATALI_ROUTE_BYPASS;
        decision->confidence = 1.0;
        snprintf(decision->rationale, sizeof(decision->rationale), "%s", "routing_disabled");
        if (trace) trace(KATALI_ROUTE_REASON_ROUTING_DISABLED, decision, trace_user);
        return KATALI_ROUTE_OK;
    }
    for (i = 0; i < request->model_count; ++i) {
        const KataliRouteModel *m = &request->models[i];
        if (!m->available) continue;
        if ((int)strlen(request->text) > 12000 && m->context_length < (int)strlen(request->text)) continue;
        if (!best || m->context_length > best->context_length ||
            (m->context_length == best->context_length && m->max_output_tokens > best->max_output_tokens)) best = m;
    }
    if (!best) {
        decision->action = KATALI_ROUTE_ESCALATE;
        decision->confidence = 0.0;
        snprintf(decision->rationale, sizeof(decision->rationale), "%s", "no_capable_model");
        if (trace) trace(KATALI_ROUTE_REASON_NO_CAPABLE_MODEL, decision, trace_user);
        return KATALI_ROUTE_OK;
    }
    decision->action = KATALI_ROUTE_GENERATE;
    decision->model_id = best->id;
    decision->confidence = 0.85;
    decision->validate_output = policy->validate_structured_output && (request->has_tools || request->output_json);
    decision->retry_budget = decision->validate_output ? policy->max_retries : 0;
    if (decision->confidence < policy->minimum_confidence) {
        decision->action = KATALI_ROUTE_ESCALATE;
        snprintf(decision->rationale, sizeof(decision->rationale), "%s", "low_confidence");
        if (trace) trace(KATALI_ROUTE_REASON_LOW_CONFIDENCE, decision, trace_user);
        return KATALI_ROUTE_OK;
    }
    snprintf(decision->rationale, sizeof(decision->rationale), "%s", decision->validate_output ?
             "selected_model;validation_required" : "selected_model");
    if (trace) {
        trace(KATALI_ROUTE_REASON_MODEL_SELECTED, decision, trace_user);
        if (decision->validate_output) trace(KATALI_ROUTE_REASON_VALIDATION_REQUIRED, decision, trace_user);
    }
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_decide_with_provider(
    const KataliRouteRequest *request,
    const KataliRouteDecisionProvider *provider,
    KataliRouteDecision *decision,
    KataliRouteTypedDecision *typed) {
    int rc;
    if (!provider || !provider->evaluate || !typed) return KATALI_ROUTE_ERR_ARGUMENT;
    rc = katali_route_decide(request, decision);
    if (rc != KATALI_ROUTE_OK) return rc;
    memset(typed, 0, sizeof(*typed));
    if (provider->evaluate(provider->user, request, typed) != 0) return KATALI_ROUTE_ERR_MODEL;
    if (typed->kind == KATALI_ROUTE_DECISION_CONFIDENCE &&
        (typed->value < 0.0 || typed->value > 1.0)) return KATALI_ROUTE_ERR_ARGUMENT;
    if (typed->kind == KATALI_ROUTE_DECISION_BOOLEAN &&
        (typed->value < 0.0 || typed->value > 1.0)) return KATALI_ROUTE_ERR_ARGUMENT;
    if (typed->probability_count && !typed->probabilities) return KATALI_ROUTE_ERR_ARGUMENT;
    if (typed->probability_count > 64) return KATALI_ROUTE_ERR_ARGUMENT;
    if (typed->probability_count) {
        double total = 0.0;
        size_t i;
        for (i = 0; i < typed->probability_count; ++i) {
            if (!isfinite(typed->probabilities[i]) || typed->probabilities[i] < 0.0 || typed->probabilities[i] > 1.0) return KATALI_ROUTE_ERR_ARGUMENT;
            total += typed->probabilities[i];
        }
        if (fabs(total - 1.0) > 0.001) return KATALI_ROUTE_ERR_ARGUMENT;
    }
    if (typed->kind == KATALI_ROUTE_DECISION_CONFIDENCE || typed->kind == KATALI_ROUTE_DECISION_BOOLEAN ||
        (typed->kind == KATALI_ROUTE_DECISION_CHOICE && typed->value >= 0.0 && typed->value <= 1.0))
        decision->confidence = typed->value;
    if (typed->kind == KATALI_ROUTE_DECISION_CHOICE && typed->label) {
        size_t i; int matched = 0;
        for (i = 0; i < request->model_count; ++i) {
            if (request->models[i].available && strcmp(request->models[i].id, typed->label) == 0) {
                decision->model_id = request->models[i].id;
                matched = 1;
                break;
            }
        }
        if (!matched) return KATALI_ROUTE_ERR_MODEL;
    }
    if (provider->name && provider->name[0]) {
        snprintf(decision->rationale, sizeof(decision->rationale), "provider:%s", provider->name);
    }
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_decide_with_provider_policy(
    const KataliRouteRequest *request,
    const KataliRouteDecisionProvider *provider,
    const KataliRoutePolicy *policy,
    KataliRouteTraceFn trace,
    void *trace_user,
    KataliRouteDecision *decision,
    KataliRouteTypedDecision *typed) {
    KataliRouteStatus status;
    if (!policy || policy->minimum_confidence < 0.0 || policy->minimum_confidence > 1.0 || policy->max_retries < 0)
        return KATALI_ROUTE_ERR_ARGUMENT;
    status = katali_route_decide_with_provider(request, provider, decision, typed);
    if (status != KATALI_ROUTE_OK) return status;
    if (decision->confidence < policy->minimum_confidence) {
        decision->action = KATALI_ROUTE_ESCALATE;
        snprintf(decision->rationale, sizeof(decision->rationale), "low_confidence");
        if (trace) trace(KATALI_ROUTE_REASON_LOW_CONFIDENCE, decision, trace_user);
    }
    return KATALI_ROUTE_OK;
}

static int json_string(char **cursor, size_t *left, const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    while (*p) {
        const char *escaped = NULL;
        char hex[7];
        size_t need;
        switch (*p) {
        case '\\': escaped = "\\\\"; break;
        case '"': escaped = "\\\""; break;
        case '\n': escaped = "\\n"; break;
        case '\r': escaped = "\\r"; break;
        case '\t': escaped = "\\t"; break;
        default: break;
        }
        if ((unsigned char)*p < 0x20 && !escaped) {
            snprintf(hex, sizeof(hex), "\\u%04x", *p);
            escaped = hex;
        }
        need = escaped ? strlen(escaped) : 1;
        if (*left <= need) return -1;
        memcpy(*cursor, escaped ? escaped : (const char *)p, need);
        *cursor += need; *left -= need; ++p;
    }
    return 0;
}

KataliRouteStatus katali_route_decision_json(const KataliRouteDecision *d, char *output, size_t cap) {
    char *p = output; size_t left = cap; int n;
    if (!d || !output || cap < 2 || !d->request_id || !d->rationale) return KATALI_ROUTE_ERR_ARGUMENT;
    n = snprintf(p, left, "{\"request_id\":\""); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (json_string(&p, &left, d->request_id)) return KATALI_ROUTE_ERR_BUFFER;
    n = snprintf(p, left, "\",\"action\":\"%s\",\"model_id\":", katali_route_action_name(d->action)); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (!d->model_id) { n = snprintf(p, left, "null"); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n; }
    else { if (left < 2) return KATALI_ROUTE_ERR_BUFFER; *p++ = '"'; --left; if (json_string(&p, &left, d->model_id)) return KATALI_ROUTE_ERR_BUFFER; if (!left) return KATALI_ROUTE_ERR_BUFFER; *p++ = '"'; --left; }
    n = snprintf(p, left, ",\"confidence\":%.3f,\"validate_output\":%s,\"retry_budget\":%d,\"rationale\":\"", d->confidence, d->validate_output ? "true" : "false", d->retry_budget); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (json_string(&p, &left, d->rationale)) return KATALI_ROUTE_ERR_BUFFER;
    if (left < 3) return KATALI_ROUTE_ERR_BUFFER;
    memcpy(p, "\"}", 2);
    p[2] = '\0';
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_typed_decision_json(const KataliRouteTypedDecision *typed, char *output, size_t cap) {
    const char *kind;
    int n; size_t i, used = 0;
    if (!typed || !output || cap < 2 || typed->probability_count > 64 || (typed->probability_count && !typed->probabilities)) return KATALI_ROUTE_ERR_ARGUMENT;
    switch (typed->kind) {
    case KATALI_ROUTE_DECISION_CHOICE: kind = "choice"; break;
    case KATALI_ROUTE_DECISION_BOOLEAN: kind = "boolean"; break;
    case KATALI_ROUTE_DECISION_SCORE: kind = "score"; break;
    case KATALI_ROUTE_DECISION_CONFIDENCE: kind = "confidence"; break;
    default: return KATALI_ROUTE_ERR_ARGUMENT;
    }
    n = snprintf(output, cap, "{\"kind\":\"%s\",\"label\":%s,\"value\":%.6f,\"probabilities\":[", kind, typed->label ? "\"present\"" : "null", typed->value);
    if (n < 0 || (size_t)n >= cap) return KATALI_ROUTE_ERR_BUFFER;
    used = (size_t)n;
    for (i = 0; i < typed->probability_count; ++i) {
        n = snprintf(output + used, cap - used, "%s%.6f", i ? "," : "", typed->probabilities[i]);
        if (n < 0 || (size_t)n >= cap - used) return KATALI_ROUTE_ERR_BUFFER;
        used += (size_t)n;
    }
    n = snprintf(output + used, cap - used, "]}");
    return n < 0 || (size_t)n >= cap - used ? KATALI_ROUTE_ERR_BUFFER : KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_model_json(const KataliRouteModelInfo *m, char *output, size_t cap) {
    char *p = output; size_t left = cap; int n;
    if (!m || !output || cap < 2 || !m->model_id || !m->backend) return KATALI_ROUTE_ERR_ARGUMENT;
    n = snprintf(p, left, "{\"model_id\":\""); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (json_string(&p, &left, m->model_id)) return KATALI_ROUTE_ERR_BUFFER;
    n = snprintf(p, left, "\",\"backend\":\""); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (json_string(&p, &left, m->backend)) return KATALI_ROUTE_ERR_BUFFER;
    n = snprintf(p, left, "\",\"endpoint\":"); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n;
    if (m->endpoint) { if (left < 2) return KATALI_ROUTE_ERR_BUFFER; *p++ = '"'; --left; if (json_string(&p, &left, m->endpoint)) return KATALI_ROUTE_ERR_BUFFER; if (!left) return KATALI_ROUTE_ERR_BUFFER; *p++ = '"'; --left; }
    else { n = snprintf(p, left, "null"); if (n < 0 || (size_t)n >= left) return KATALI_ROUTE_ERR_BUFFER; p += n; left -= (size_t)n; }
    n = snprintf(p, left, ",\"context_length\":%d,\"max_output_tokens\":%d,\"available\":%s}", m->context_length, m->max_output_tokens, m->available ? "true" : "false");
    return n < 0 || (size_t)n >= left ? KATALI_ROUTE_ERR_BUFFER : KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_execute(const KataliRouteRequest *request,
                                       const KataliRouteGgufClient *client,
                                       char *output, size_t output_cap,
                                       KataliRouteDecision *decision) {
    int rc;
    int attempts = 0;
    if (!request || !client || !decision || !output || !client->generate) return KATALI_ROUTE_ERR_ARGUMENT;
    rc = katali_route_decide(request, decision);
    if (rc != KATALI_ROUTE_OK) return rc;
    if (decision->action != KATALI_ROUTE_BYPASS && decision->action != KATALI_ROUTE_GENERATE) return KATALI_ROUTE_OK;
    do {
        const char *model_id = decision->action == KATALI_ROUTE_BYPASS ? NULL : decision->model_id;
        rc = client->generate(client->user, model_id, request->text, output, output_cap);
        if (rc != 0) return KATALI_ROUTE_ERR_MODEL;
        if (!decision->validate_output || !client->validate || client->validate(client->user, output) == 0) return KATALI_ROUTE_OK;
        if (decision->retry_budget <= 0) {
            decision->action = KATALI_ROUTE_ESCALATE;
            return KATALI_ROUTE_OK;
        }
        decision->action = KATALI_ROUTE_RETRY;
        decision->retry_budget--;
        attempts++;
    } while (attempts <= 16);
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_execute_decision(
    const KataliRouteRequest *request,
    const KataliRouteGgufClient *client,
    char *output, size_t output_cap,
    KataliRouteDecision *decision) {
    int rc;
    if (!request || !client || !decision || !output || !client->generate) return KATALI_ROUTE_ERR_ARGUMENT;
    if (decision->action != KATALI_ROUTE_BYPASS && decision->action != KATALI_ROUTE_GENERATE) return KATALI_ROUTE_OK;
    rc = client->generate(client->user, decision->action == KATALI_ROUTE_BYPASS ? NULL : decision->model_id,
                          request->text, output, output_cap);
    if (rc != 0) return KATALI_ROUTE_ERR_MODEL;
    if (!decision->validate_output || !client->validate || client->validate(client->user, output) == 0) return KATALI_ROUTE_OK;
    if (decision->retry_budget <= 0) { decision->action = KATALI_ROUTE_ESCALATE; return KATALI_ROUTE_OK; }
    decision->action = KATALI_ROUTE_RETRY; decision->retry_budget--;
    rc = client->generate(client->user, decision->model_id, request->text, output, output_cap);
    if (rc != 0) return KATALI_ROUTE_ERR_MODEL;
    if (client->validate(client->user, output) != 0) decision->action = KATALI_ROUTE_ESCALATE;
    return KATALI_ROUTE_OK;
}
