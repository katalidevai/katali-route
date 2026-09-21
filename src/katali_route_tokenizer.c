#include "katali_route_tokenizer.h"

KataliRouteStatus katali_route_tokenizer_validate(
    const KataliRouteTokenizerOutput *output) {
    size_t i;
    if (!output || !output->input_ids || !output->attention_mask ||
        !output->marker_pos || !output->marker_mask || !output->labels ||
        !output->sequence_length || output->sequence_length > KATALI_ROUTE_TOKENIZER_MAX_SEQUENCE ||
        !output->option_count || output->option_count > KATALI_ROUTE_TOKENIZER_MAX_OPTIONS ||
        output->sequence_length > output->sequence_capacity ||
        output->option_count > output->option_capacity) return KATALI_ROUTE_ERR_ARGUMENT;
    for (i = 0; i < output->option_count; ++i) {
        if (!output->labels[i] || output->marker_pos[i] < 0 ||
            (size_t)output->marker_pos[i] >= output->sequence_length) {
            return KATALI_ROUTE_ERR_ARGUMENT;
        }
    }
    return KATALI_ROUTE_OK;
}
