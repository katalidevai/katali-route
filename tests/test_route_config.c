#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "katali_route_config.h"

int main(void) {
    KataliRouteConfig config; KataliRouteModel models[4];
    FILE *bad;
    assert(katali_route_config_load("config\\route.conf.example", &config) == KATALI_ROUTE_OK);
    assert(config.model_count == 4 && config.pool_load_mode == KATALI_ROUTE_CONFIG_MODE_EAGER);
    assert(!strcmp(config.ids[0], "qwen3-0.6b"));
    assert(config.context_lengths[3] == 2048 && config.available[3]);
    katali_route_config_models(&config, models);
    assert(!strcmp(models[2].id, "qwen3-4b") && models[2].available);
    assert(katali_route_config_load("build\\does-not-exist.conf", &config) != KATALI_ROUTE_OK);
    bad = fopen("build\\invalid-route.conf", "wb"); assert(bad);
    fputs("unknown_production_key=1\n", bad); fclose(bad);
    assert(katali_route_config_load("build\\invalid-route.conf", &config) != KATALI_ROUTE_OK);
    remove("build\\invalid-route.conf");
    printf("route_config_models=%d mode=eager\n", config.model_count);
    return 0;
}
