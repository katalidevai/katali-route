#ifndef KATALI_ROUTE_LAYA_ORT_H
#define KATALI_ROUTE_LAYA_ORT_H

#include "katali_route.h"
#include "katali_route_tokenizer.h"
#include "katali_route_laya_tokenizer.h"
#include "onnxruntime_c_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const OrtApi *api;
    OrtSession *session;
    OrtMemoryInfo *memory;
    double temperature;
} KataliRouteLayaOrt;

typedef struct {
    const int64_t *input_ids;
    const int64_t *attention_mask;
    size_t sequence_length;
    const int64_t *marker_pos;
    const unsigned char *marker_mask;
    size_t option_count;
    int64_t question_type;
    const char *const *labels;
} KataliRouteLayaTokens;

KataliRouteStatus katali_route_laya_tokens_from_tokenizer(
    const KataliRouteTokenizerOutput *output,
    KataliRouteLayaTokens *tokens);

typedef int (*KataliRouteLayaTokenizeFn)(void *user,
                                         const KataliRouteRequest *request,
                                         KataliRouteLayaTokens *tokens);

typedef struct {
    KataliRouteLayaOrt runtime;
    KataliRouteLayaTokenizeFn tokenize;
    void *tokenizer_user;
} KataliRouteLayaOrtProvider;

typedef struct {
    KataliRouteLayaOrt runtime;
    KataliRouteLayaTokenizer *tokenizer;
    KataliRouteTokenizerOutput *output;
} KataliRouteLayaOrtTokenizerProvider;

double katali_route_laya_temperature_for_options(size_t option_count);

KataliRouteDecisionProvider katali_route_laya_ort_provider(
    const KataliRouteLayaOrtProvider *provider);
KataliRouteDecisionProvider katali_route_laya_ort_tokenizer_provider(
    const KataliRouteLayaOrtTokenizerProvider *provider);

KataliRouteStatus katali_route_laya_ort_choice(
    const KataliRouteLayaOrt *runtime,
    const int64_t *input_ids, const int64_t *attention_mask, size_t sequence_length,
    const int64_t *marker_pos, const unsigned char *marker_mask, size_t option_count,
    int64_t question_type, const char *const *labels,
    KataliRouteTypedDecision *result);

#ifdef __cplusplus
}
#endif
#endif
