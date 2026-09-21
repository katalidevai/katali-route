#ifndef KATALI_ROUTE_CONFIG_H
#define KATALI_ROUTE_CONFIG_H

#include "katali_route.h"

#ifdef __cplusplus
extern "C" {
#endif

#define KATALI_ROUTE_CONFIG_MODEL_COUNT 4
#define KATALI_ROUTE_CONFIG_ID_CAP 64
#define KATALI_ROUTE_CONFIG_PATH_CAP 1024
#define KATALI_ROUTE_CONFIG_MODE_EAGER 0
#define KATALI_ROUTE_CONFIG_MODE_LAZY 1

typedef struct {
    char ids[KATALI_ROUTE_CONFIG_MODEL_COUNT][KATALI_ROUTE_CONFIG_ID_CAP];
    char paths[KATALI_ROUTE_CONFIG_MODEL_COUNT][KATALI_ROUTE_CONFIG_PATH_CAP];
    int context_lengths[KATALI_ROUTE_CONFIG_MODEL_COUNT];
    int max_output_tokens[KATALI_ROUTE_CONFIG_MODEL_COUNT];
    int available[KATALI_ROUTE_CONFIG_MODEL_COUNT];
    int model_count;
    int threads;
    int pool_load_mode;
    double minimum_confidence;
    int max_retries;
    int validate_structured_output;
} KataliRouteConfig;

KataliRouteStatus katali_route_config_load(const char *path, KataliRouteConfig *config);
const char *katali_route_config_error(void);
void katali_route_config_models(const KataliRouteConfig *config, KataliRouteModel *models);

#ifdef __cplusplus
}
#endif
#endif
