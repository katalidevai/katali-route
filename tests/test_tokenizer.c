#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "katali_route_tokenizer.h"

int main(void) {
    int64_t ids[8] = {1};
    int64_t mask[8] = {1};
    int64_t markers[2] = {2, 5};
    unsigned char marker_mask[2] = {1, 1};
    const char *labels[2] = {"small", "large"};
    KataliRouteTokenizerOutput output = {ids, mask, markers, marker_mask, labels, 8, 2, 8, 2, 0};
    assert(katali_route_tokenizer_validate(&output) == KATALI_ROUTE_OK);
    output.marker_pos[1] = 8;
    assert(katali_route_tokenizer_validate(&output) == KATALI_ROUTE_ERR_ARGUMENT);
    output.marker_pos[1] = 5; output.labels[1] = NULL;
    assert(katali_route_tokenizer_validate(&output) == KATALI_ROUTE_ERR_ARGUMENT);
    output.labels[1] = "large"; output.sequence_length = KATALI_ROUTE_TOKENIZER_MAX_SEQUENCE + 1;
    assert(katali_route_tokenizer_validate(&output) == KATALI_ROUTE_ERR_ARGUMENT);
    output.sequence_length = 8; output.option_count = KATALI_ROUTE_TOKENIZER_MAX_OPTIONS + 1;
    assert(katali_route_tokenizer_validate(&output) == KATALI_ROUTE_ERR_ARGUMENT);
    puts("tokenizer_abi_tests=passed");
    return 0;
}
