#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "katali_route_laya_tokenizer.h"

int main(void) {
    FILE *file = fopen("work\\laya-onnx\\tokenizer\\tokenizer.json", "rb");
    long size; char *json; size_t i;
    int64_t ids[512], mask[512], markers[64]; unsigned char marker_mask[64];
    const char *labels[64];
    KataliRouteLayaTokenizer tokenizer;
    KataliRouteTokenizerOutput output = {ids, mask, markers, marker_mask, labels, 512, 64, 0, 0, 0};
    KataliRouteModel models[] = {{"small", "katali-gguf", 4096, 128, 1}, {"large", "katali-gguf", 32768, 512, 1}};
    KataliRouteRequest request = {"tok", "Choose a model for café", models, 2, 1, 0, 0};
    assert(file); fseek(file, 0, SEEK_END); size = ftell(file); rewind(file);
    json = (char *)malloc((size_t)size); assert(json); assert(fread(json, 1, (size_t)size, file) == (size_t)size); fclose(file);
    assert(katali_route_laya_tokenizer_init(&tokenizer, json, (size_t)size) == KATALI_ROUTE_OK);
    assert(katali_route_laya_tokenize(&tokenizer, &request, &output) == 0);
    assert(output.sequence_length > 0 && output.option_count == 2);
    for (i = 0; i < output.option_count; ++i) assert(output.marker_mask[i] && output.marker_pos[i] < (int64_t)output.sequence_length);
    assert(ids[0] == 50281 && ids[markers[0]] == 50284 && ids[markers[0] + 1] == 1355);
    assert(ids[markers[1]] == 50284 && ids[markers[1] + 1] == 1781);
    printf("laya_tokenizer_c sequence=%zu options=%zu marker0=%lld marker1=%lld\n", output.sequence_length, output.option_count, (long long)output.marker_pos[0], (long long)output.marker_pos[1]);
    katali_route_laya_tokenizer_dispose(&tokenizer); free(json); return 0;
}
