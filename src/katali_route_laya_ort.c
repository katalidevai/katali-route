#include "katali_route_laya_ort.h"
#include <math.h>
#include <string.h>

double katali_route_laya_temperature_for_options(size_t option_count) {
    if (option_count == 2) return 1.9063563340981445;
    if (option_count >= 3 && option_count <= 5) return 1.7601518630981445;
    if (option_count >= 6 && option_count <= 10) return 1.0000158548355103;
    if (option_count >= 11) return 0.10058280825614929;
    return 1.0;
}

KataliRouteStatus katali_route_laya_tokens_from_tokenizer(
    const KataliRouteTokenizerOutput *output,
    KataliRouteLayaTokens *tokens) {
    if (katali_route_tokenizer_validate(output) != KATALI_ROUTE_OK || !tokens) {
        return KATALI_ROUTE_ERR_ARGUMENT;
    }
    tokens->input_ids = output->input_ids;
    tokens->attention_mask = output->attention_mask;
    tokens->sequence_length = output->sequence_length;
    tokens->marker_pos = output->marker_pos;
    tokens->marker_mask = output->marker_mask;
    tokens->option_count = output->option_count;
    tokens->question_type = output->question_type;
    tokens->labels = output->labels;
    return KATALI_ROUTE_OK;
}

static int ort_provider_evaluate(void *user, const KataliRouteRequest *request, KataliRouteTypedDecision *result) {
    KataliRouteLayaOrtProvider *provider = (KataliRouteLayaOrtProvider *)user;
    KataliRouteLayaTokens tokens;
    if (!provider || !provider->tokenize || !request || !result) return -1;
    memset(&tokens, 0, sizeof(tokens));
    if (provider->tokenize(provider->tokenizer_user, request, &tokens) != 0) return -1;
    if (katali_route_laya_ort_choice(&provider->runtime, tokens.input_ids, tokens.attention_mask,
                                     tokens.sequence_length, tokens.marker_pos, tokens.marker_mask,
                                     tokens.option_count, tokens.question_type, tokens.labels, result) != KATALI_ROUTE_OK) return -1;
    return 0;
}

static int ort_tokenizer_provider_evaluate(void *user, const KataliRouteRequest *request,
                                            KataliRouteTypedDecision *result) {
    KataliRouteLayaOrtTokenizerProvider *provider = (KataliRouteLayaOrtTokenizerProvider *)user;
    KataliRouteLayaTokens tokens;
    if (!provider || !provider->tokenizer || !provider->output || !request || !result) return -1;
    if (katali_route_laya_tokenize(provider->tokenizer, request, provider->output) != 0) return -1;
    if (katali_route_laya_tokens_from_tokenizer(provider->output, &tokens) != KATALI_ROUTE_OK) return -1;
    return katali_route_laya_ort_choice(&provider->runtime, tokens.input_ids, tokens.attention_mask,
                                        tokens.sequence_length, tokens.marker_pos, tokens.marker_mask,
                                        tokens.option_count, tokens.question_type, tokens.labels, result) == KATALI_ROUTE_OK ? 0 : -1;
}

KataliRouteDecisionProvider katali_route_laya_ort_provider(const KataliRouteLayaOrtProvider *provider) {
    KataliRouteDecisionProvider result = {ort_provider_evaluate, (void *)provider, "laya-ort"};
    return result;
}

KataliRouteDecisionProvider katali_route_laya_ort_tokenizer_provider(
    const KataliRouteLayaOrtTokenizerProvider *provider) {
    KataliRouteDecisionProvider result = {ort_tokenizer_provider_evaluate, (void *)provider, "laya-ort-tokenizer"};
    return result;
}

KataliRouteStatus katali_route_laya_ort_choice(
    const KataliRouteLayaOrt *runtime,
    const int64_t *input_ids, const int64_t *attention_mask, size_t sequence_length,
    const int64_t *marker_pos, const unsigned char *marker_mask, size_t option_count,
    int64_t question_type, const char *const *labels,
    KataliRouteTypedDecision *result) {
    const char *input_names[] = {"input_ids", "attention_mask", "marker_pos", "marker_mask", "qtype"};
    const char *output_names[] = {"logits", "act_probs"};
    int64_t input_shape[] = {1, (int64_t)sequence_length};
    int64_t option_shape[] = {1, (int64_t)option_count};
    int64_t q_shape[] = {1};
    int64_t q_data = question_type;
    size_t bytes[] = {sequence_length * sizeof(int64_t), sequence_length * sizeof(int64_t), option_count * sizeof(int64_t), option_count, sizeof(int64_t)};
    void *data[] = {(void *)input_ids, (void *)attention_mask, (void *)marker_pos, (void *)marker_mask, &q_data};
    const int64_t *shapes[] = {input_shape, input_shape, option_shape, option_shape, q_shape};
    size_t ranks[] = {2, 2, 2, 2, 1};
    ONNXTensorElementDataType types[] = {ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64, ONNX_TENSOR_ELEMENT_DATA_TYPE_BOOL, ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64};
    OrtValue *inputs[5] = {0}; OrtValue *outputs[2] = {0}; OrtStatus *status = NULL; void *logits = NULL; size_t i;
    double total = 0.0, max_logit = -INFINITY;
    double temperature = runtime ? runtime->temperature : 1.0;
    if (!runtime || !runtime->api || !runtime->session || !runtime->memory || !result ||
        !input_ids || !attention_mask || !marker_pos || !marker_mask || !labels ||
        !sequence_length || sequence_length > 512 || !option_count || option_count > 64) {
        return KATALI_ROUTE_ERR_ARGUMENT;
    }
    if (!(temperature > 0.0) || !isfinite(temperature)) temperature = katali_route_laya_temperature_for_options(option_count);
    for (i = 0; i < option_count; ++i) {
        if (!labels[i] || marker_pos[i] < 0 || (size_t)marker_pos[i] >= sequence_length) {
            return KATALI_ROUTE_ERR_ARGUMENT;
        }
    }
    memset(result, 0, sizeof(*result)); result->kind = KATALI_ROUTE_DECISION_CHOICE; result->probability_count = option_count;
    if ((status = runtime->api->CreateTensorWithDataAsOrtValue(runtime->memory, data[0], bytes[0], shapes[0], ranks[0], types[0], &inputs[0]))) goto ort_error;
    for (i = 1; i < 5; ++i) if ((status = runtime->api->CreateTensorWithDataAsOrtValue(runtime->memory, data[i], bytes[i], shapes[i], ranks[i], types[i], &inputs[i]))) goto ort_error;
    if ((status = runtime->api->Run(runtime->session, NULL, input_names, (const OrtValue *const *)inputs, 5, output_names, 2, outputs))) goto ort_error;
    if ((status = runtime->api->GetTensorMutableData(outputs[0], &logits))) goto ort_error;
    for (i = 0; i < option_count; ++i) if (((float *)logits)[i] / temperature > max_logit) max_logit = ((float *)logits)[i] / temperature;
    for (i = 0; i < option_count; ++i) { result->probabilities = NULL; total += exp((double)((float *)logits)[i] / temperature - max_logit); }
    { static double probabilities[64]; size_t best = 0; for (i = 0; i < option_count; ++i) { probabilities[i] = exp((double)((float *)logits)[i] / temperature - max_logit) / total; if (probabilities[i] > probabilities[best]) best = i; } result->probabilities = probabilities; result->label = labels[best]; result->value = probabilities[best]; }
    for (i = 0; i < 5; ++i) runtime->api->ReleaseValue(inputs[i]);
    runtime->api->ReleaseValue(outputs[0]);
    runtime->api->ReleaseValue(outputs[1]);
    return KATALI_ROUTE_OK;
ort_error:
    for (i = 0; i < 5; ++i) if (inputs[i]) runtime->api->ReleaseValue(inputs[i]);
    if (outputs[0]) runtime->api->ReleaseValue(outputs[0]);
    if (outputs[1]) runtime->api->ReleaseValue(outputs[1]);
    if (status) runtime->api->ReleaseStatus(status);
    return KATALI_ROUTE_ERR_MODEL;
}
