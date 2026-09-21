#ifndef KATALI_ROUTE_LAYA_TOKENIZER_H
#define KATALI_ROUTE_LAYA_TOKENIZER_H

#include "katali_route_tokenizer.h"
#include "tokenizers_c.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    TokenizerHandle handle;
    int cls_id;
    int sep_id;
    int mask_id;
    int pad_id;
    size_t head_max_len;
    size_t max_len;
} KataliRouteLayaTokenizer;

KataliRouteStatus katali_route_laya_tokenizer_init(
    KataliRouteLayaTokenizer *tokenizer, const char *json, size_t json_len);
void katali_route_laya_tokenizer_dispose(KataliRouteLayaTokenizer *tokenizer);
int katali_route_laya_tokenize(void *user, const KataliRouteRequest *request,
                               KataliRouteTokenizerOutput *output);

#ifdef __cplusplus
}
#endif
#endif
