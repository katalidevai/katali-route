#include "katali_route_laya_tokenizer.h"
#include <stdio.h>
#include <string.h>

static int token_id(TokenizerHandle handle, const char *text) {
    int32_t id = -1;
    tokenizers_token_to_id(handle, text, strlen(text), &id);
    return id;
}

static size_t encode(TokenizerHandle handle, const char *text, int *ids, size_t cap) {
    TokenizerEncodeResult result = {0};
    size_t n;
    tokenizers_encode(handle, text, strlen(text), 0, &result);
    n = result.len < cap ? result.len : cap;
    if (n) memcpy(ids, result.token_ids, n * sizeof(*ids));
    tokenizers_free_encode_results(&result, 1);
    return n;
}

KataliRouteStatus katali_route_laya_tokenizer_init(
    KataliRouteLayaTokenizer *tokenizer, const char *json, size_t json_len) {
    if (!tokenizer || !json || !json_len) return KATALI_ROUTE_ERR_ARGUMENT;
    memset(tokenizer, 0, sizeof(*tokenizer));
    tokenizer->handle = tokenizers_new_from_str(json, json_len);
    if (!tokenizer->handle) return KATALI_ROUTE_ERR_MODEL;
    tokenizer->cls_id = token_id(tokenizer->handle, "[CLS]");
    tokenizer->sep_id = token_id(tokenizer->handle, "[SEP]");
    tokenizer->mask_id = token_id(tokenizer->handle, "[MASK]");
    tokenizer->pad_id = token_id(tokenizer->handle, "[PAD]");
    tokenizer->head_max_len = 192;
    tokenizer->max_len = KATALI_ROUTE_TOKENIZER_MAX_SEQUENCE;
    if (tokenizer->cls_id < 0 || tokenizer->sep_id < 0 || tokenizer->mask_id < 0 || tokenizer->pad_id < 0) {
        katali_route_laya_tokenizer_dispose(tokenizer);
        return KATALI_ROUTE_ERR_MODEL;
    }
    return KATALI_ROUTE_OK;
}

void katali_route_laya_tokenizer_dispose(KataliRouteLayaTokenizer *tokenizer) {
    if (tokenizer && tokenizer->handle) tokenizers_free(tokenizer->handle);
    if (tokenizer) memset(tokenizer, 0, sizeof(*tokenizer));
}

int katali_route_laya_tokenize(void *user, const KataliRouteRequest *request,
                               KataliRouteTokenizerOutput *output) {
    KataliRouteLayaTokenizer *tok = (KataliRouteLayaTokenizer *)user;
    int head[192], option[64], state[512];
    char head_text[1024];
    size_t i, j, nhead, noption, nstate, used = 0, option_budget;
    if (!tok || !tok->handle || !request || !request->text || !output ||
        request->model_count == 0 || request->model_count > output->option_capacity ||
        output->sequence_capacity < tok->max_len) return -1;
    if (snprintf(head_text, sizeof(head_text), "choice question: %s", request->text) < 0) return -1;
    nhead = encode(tok->handle, head_text, head, sizeof(head) / sizeof(head[0]));
    option_budget = tok->head_max_len;
    for (i = 0; i < request->model_count; ++i) option_budget -= 1;
    if (nhead > option_budget) nhead = option_budget;
    output->input_ids[used++] = tok->cls_id;
    for (j = 0; j < nhead; ++j) output->input_ids[used++] = head[j];
    output->input_ids[used++] = tok->sep_id;
    for (i = 0; i < request->model_count; ++i) {
        char text[256];
        output->marker_pos[i] = (int64_t)used;
        output->marker_mask[i] = 1;
        output->labels[i] = request->models[i].id;
        output->input_ids[used++] = tok->mask_id;
        if (snprintf(text, sizeof(text), " %s", request->models[i].id) < 0) return -1;
        noption = encode(tok->handle, text, option, sizeof(option) / sizeof(option[0]));
        if (noption > 48) noption = 48;
        if (used + noption + 1 > tok->max_len) return -1;
        for (j = 0; j < noption; ++j) output->input_ids[used++] = option[j];
    }
    if (used + 1 > tok->max_len) return -1;
    output->input_ids[used++] = tok->sep_id;
    nstate = encode(tok->handle, request->text, state, sizeof(state) / sizeof(state[0]));
    if (nstate > tok->max_len - used - 1) nstate = tok->max_len - used - 1;
    for (i = 0; i < nstate; ++i) output->input_ids[used++] = state[i];
    output->input_ids[used++] = tok->sep_id;
    for (i = 0; i < used; ++i) output->attention_mask[i] = 1;
    output->sequence_length = used;
    output->option_count = request->model_count;
    output->question_type = 0;
    return katali_route_tokenizer_validate(output) == KATALI_ROUTE_OK ? 0 : -1;
}
