#ifndef KATALI_ROUTE_TOKENIZER_H
#define KATALI_ROUTE_TOKENIZER_H

#include "katali_route.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KATALI_ROUTE_TOKENIZER_MAX_SEQUENCE 512
#define KATALI_ROUTE_TOKENIZER_MAX_OPTIONS 64

/* Caller-owned output buffers. A tokenizer must not retain these pointers. */
typedef struct {
    int64_t *input_ids;
    int64_t *attention_mask;
    int64_t *marker_pos;
    unsigned char *marker_mask;
    const char **labels;
    size_t sequence_capacity;
    size_t option_capacity;
    size_t sequence_length;
    size_t option_count;
    int64_t question_type;
} KataliRouteTokenizerOutput;

typedef int (*KataliRouteTokenizerFn)(void *user,
                                      const KataliRouteRequest *request,
                                      KataliRouteTokenizerOutput *output);

KataliRouteStatus katali_route_tokenizer_validate(
    const KataliRouteTokenizerOutput *output);

#ifdef __cplusplus
}
#endif
#endif
