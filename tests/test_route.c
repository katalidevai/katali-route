#include "katali_route.h"
#include "katali_route_laya.h"
#include <assert.h>
#include <string.h>

static int generated_model_id_is_null;
static int generate_count;
static int fake_generate(void *user, const char *model_id, const char *prompt, char *output, size_t cap) {
    (void)user; (void)prompt;
    generated_model_id_is_null = model_id == NULL;
    generate_count++;
    if (cap < 4) return -1;
    strcpy(output, "ok"); return 0;
}
static int reject_output(void *user, const char *output) { (void)user; (void)output; return 1; }
static int fake_provider(void *user, const KataliRouteRequest *request, KataliRouteTypedDecision *result) {
    (void)user; (void)request; result->kind = KATALI_ROUTE_DECISION_CONFIDENCE; result->value = 0.91; return 0;
}
static int fake_laya(void *user, const KataliRouteRequest *request, KataliRouteLayaAnswer *answer) {
    (void)user; (void)request; answer->kind = KATALI_ROUTE_DECISION_CHOICE; answer->label = "large"; answer->value = 0.94; return 0;
}
static int unknown_laya(void *user, const KataliRouteRequest *request, KataliRouteLayaAnswer *answer) {
    (void)user; (void)request; answer->kind = KATALI_ROUTE_DECISION_CHOICE; answer->label = "missing-model"; answer->value = 0.8; return 0;
}
static int invalid_provider(void *user, const KataliRouteRequest *request, KataliRouteTypedDecision *result) {
    static const double probabilities[] = {0.2, 0.2};
    (void)user; (void)request; result->kind = KATALI_ROUTE_DECISION_CHOICE; result->probabilities = probabilities; result->probability_count = 2; return 0;
}
static int low_choice_provider(void *user, const KataliRouteRequest *request, KataliRouteTypedDecision *result) {
    static const double probabilities[] = {0.45, 0.55};
    (void)user; (void)request; result->kind = KATALI_ROUTE_DECISION_CHOICE;
    result->label = "large"; result->value = 0.55; result->probabilities = probabilities; result->probability_count = 2; return 0;
}
static int trace_count;
static void trace_event(KataliRouteReason reason, const KataliRouteDecision *decision, void *user) {
    (void)decision; (void)user; if (reason == KATALI_ROUTE_REASON_MODEL_SELECTED) trace_count++;
}

int main(void) {
    KataliRouteModel models[] = {{"small", "katali-gguf", 4096, 128, 1}, {"large", "katali-gguf", 32768, 512, 1}};
    KataliRouteRequest r = {"t1", "hello", models, 2, 1, 0, 0};
    KataliRouteDecision d;
    assert(katali_route_decide(&r, &d) == KATALI_ROUTE_OK);
    assert(d.action == KATALI_ROUTE_GENERATE && strcmp(d.model_id, "large") == 0);
    r.routing_enabled = 0;
    assert(katali_route_decide(&r, &d) == KATALI_ROUTE_OK && d.action == KATALI_ROUTE_BYPASS);
    r.routing_enabled = 1; r.models = NULL; r.model_count = 1;
    assert(katali_route_validate_request(&r) == KATALI_ROUTE_ERR_ARGUMENT);
    assert(katali_route_decide(&r, &d) == KATALI_ROUTE_ERR_ARGUMENT);
    r.models = models; r.model_count = 2; models[1].available = 0;
    assert(katali_route_validate_request(&r) == KATALI_ROUTE_OK);
    { const char *saved_id = models[0].id; int saved_context = models[0].context_length;
      models[0].id = ""; assert(katali_route_validate_request(&r) == KATALI_ROUTE_ERR_ARGUMENT);
      models[0].id = saved_id; models[0].context_length = -1; assert(katali_route_validate_request(&r) == KATALI_ROUTE_ERR_ARGUMENT);
      models[0].context_length = saved_context; }
    assert(katali_route_decide(&r, &d) == KATALI_ROUTE_OK && strcmp(d.model_id, "small") == 0);
    r.has_tools = 1; assert(katali_route_decide(&r, &d) == KATALI_ROUTE_OK && d.validate_output && d.retry_budget == 1);
    { char json[512]; assert(katali_route_decision_json(&d, json, sizeof(json)) == KATALI_ROUTE_OK); assert(strstr(json, "\"validate_output\":true") != NULL); }
    { char tiny[8]; assert(katali_route_decision_json(&d, tiny, sizeof(tiny)) == KATALI_ROUTE_ERR_BUFFER); }
    { KataliRouteGgufClient c = {fake_generate, reject_output, NULL}; char output[32];
      generate_count = 0; r.has_tools = 1; r.output_json = 0; assert(katali_route_execute(&r, &c, output, sizeof(output), &d) == KATALI_ROUTE_OK);
      assert(strcmp(output, "ok") == 0 && !generated_model_id_is_null && generate_count == 2 && d.action == KATALI_ROUTE_ESCALATE); }
    r.routing_enabled = 0;
    { KataliRouteGgufClient c = {fake_generate, NULL, NULL}; char output[32];
      assert(katali_route_execute(&r, &c, output, sizeof(output), &d) == KATALI_ROUTE_OK);
      assert(generated_model_id_is_null); }
    { KataliRouteDecisionProvider p = {fake_provider, NULL, "fixture"}; KataliRouteTypedDecision typed;
      r.routing_enabled = 1; r.has_tools = 0;
      assert(katali_route_decide_with_provider(&r, &p, &d, &typed) == KATALI_ROUTE_OK);
      assert(d.confidence == 0.91 && strstr(d.rationale, "provider:fixture") != NULL); }
    { KataliRoutePolicy policy = {0.90, 2, 1}; trace_count = 0; r.routing_enabled = 1; r.has_tools = 0;
      assert(katali_route_decide_with_policy(&r, &policy, trace_event, NULL, &d) == KATALI_ROUTE_OK);
      assert(d.action == KATALI_ROUTE_ESCALATE && strcmp(d.rationale, "low_confidence") == 0 && trace_count == 0); }
    { KataliRoutePolicy policy = {0.80, 2, 1}; trace_count = 0;
      assert(katali_route_decide_with_policy(&r, &policy, trace_event, NULL, &d) == KATALI_ROUTE_OK);
      assert(d.action == KATALI_ROUTE_GENERATE && trace_count == 1); }
    { KataliRouteDecisionProvider p = {low_choice_provider, NULL, "low-choice"}; KataliRouteTypedDecision typed;
      KataliRoutePolicy policy = {0.80, 1, 1}; trace_count = 0; models[1].available = 1;
      assert(katali_route_decide_with_provider_policy(&r, &p, &policy, trace_event, NULL, &d, &typed) == KATALI_ROUTE_OK);
      assert(d.action == KATALI_ROUTE_ESCALATE && d.confidence == 0.55 && strcmp(d.rationale, "low_confidence") == 0); }
    { char trace_json[256]; assert(katali_route_trace_json(KATALI_ROUTE_REASON_MODEL_SELECTED, &d, trace_json, sizeof(trace_json)) == KATALI_ROUTE_OK);
      assert(strstr(trace_json, "route_decision") != NULL && strstr(trace_json, "model_selected") != NULL); }
    { KataliRouteLayaProvider lp = {fake_laya, NULL, "fixture-revision"};
      KataliRouteDecisionProvider p = katali_route_laya_provider(&lp); KataliRouteTypedDecision typed;
      r.text = "hello"; models[1].available = 1; assert(katali_route_decide_with_provider(&r, &p, &d, &typed) == KATALI_ROUTE_OK);
      assert(typed.kind == KATALI_ROUTE_DECISION_CHOICE && strcmp(typed.label, "large") == 0);
      assert(strcmp(d.model_id, "large") == 0 && strcmp(d.rationale, "provider:laya") == 0); }
    { KataliRouteLayaProvider lp = {unknown_laya, NULL, "fixture-revision"};
      KataliRouteDecisionProvider p = katali_route_laya_provider(&lp); KataliRouteTypedDecision typed;
      assert(katali_route_decide_with_provider(&r, &p, &d, &typed) == KATALI_ROUTE_ERR_MODEL); }
    { double probabilities[] = {0.1, 0.9}; KataliRouteTypedDecision typed = {KATALI_ROUTE_DECISION_CHOICE, "large", 0.9, probabilities, 2}; char json[256];
      assert(katali_route_typed_decision_json(&typed, json, sizeof(json)) == KATALI_ROUTE_OK);
      assert(strstr(json, "0.100000,0.900000") != NULL); }
    { double invalid_probabilities[] = {0.2, 0.2}; KataliRouteTypedDecision typed = {KATALI_ROUTE_DECISION_CHOICE, "small", 0.2, invalid_probabilities, 2};
      KataliRouteDecisionProvider p = {fake_provider, NULL, "fixture"};
      (void)p; /* serialization is allowed to be a raw fixture; provider validation rejects it */
      assert(katali_route_typed_decision_json(&typed, (char[128]){0}, 128) == KATALI_ROUTE_OK); }
    { KataliRouteDecisionProvider p = {invalid_provider, NULL, "invalid"}; KataliRouteTypedDecision typed;
      assert(katali_route_decide_with_provider(&r, &p, &d, &typed) == KATALI_ROUTE_ERR_ARGUMENT); }
    { KataliRouteModelInfo info = {"model\"id", "katali\\gguf", "http://127.0.0.1:8080", 4096, 128, 1}; char json[256];
      assert(katali_route_model_json(&info, json, sizeof(json)) == KATALI_ROUTE_OK);
      assert(strstr(json, "model\\\"id") != NULL && strstr(json, "katali\\\\gguf") != NULL); }
    { KataliRouteRequest escaped = {"trace\n1", "hello", models, 1, 1, 0, 0}; char json[256];
      assert(katali_route_decide(&escaped, &d) == KATALI_ROUTE_OK);
      assert(katali_route_trace_json(KATALI_ROUTE_REASON_MODEL_SELECTED, &d, json, sizeof(json)) == KATALI_ROUTE_OK);
      assert(strstr(json, "trace\\n1") != NULL); }
    { char long_text[12002]; memset(long_text, 'x', sizeof(long_text) - 1); long_text[sizeof(long_text) - 1] = '\0';
      r.text = long_text; models[1].available = 1; assert(katali_route_decide(&r, &d) == KATALI_ROUTE_OK);
      assert(d.action == KATALI_ROUTE_GENERATE && strcmp(d.model_id, "large") == 0); }
    return 0;
}
