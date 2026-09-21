#ifndef KATALI_ROUTE_GGUF_SDK_H
#define KATALI_ROUTE_GGUF_SDK_H

#include "katali_route.h"
#include "katali_route_config.h"
#include "katali_gguf_sdk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    katali_model *model;
    katali_session *session;
    const char *model_id;
    const char *model_path;
    char model_id_storage[KATALI_ROUTE_CONFIG_ID_CAP];
    char model_path_storage[KATALI_ROUTE_CONFIG_PATH_CAP];
    int threads;
    int context_length;
    int available;
    int loaded;
    double load_ms;
    uint64_t resident_bytes;
    char error[256];
} KataliRouteGgufSdk;

#define KATALI_ROUTE_GGUF_POOL_MAX 8
typedef struct {
    KataliRouteGgufSdk entries[KATALI_ROUTE_GGUF_POOL_MAX];
    size_t count;
} KataliRouteGgufPool;

KataliRouteStatus katali_route_gguf_sdk_open(
    KataliRouteGgufSdk *adapter, const char *model_id,
    const char *model_path, int threads, int context_length);
void katali_route_gguf_sdk_close(KataliRouteGgufSdk *adapter);
KataliRouteGgufClient katali_route_gguf_sdk_client(KataliRouteGgufSdk *adapter);
KataliRouteStatus katali_route_gguf_pool_open(
    KataliRouteGgufPool *pool, const KataliRouteModel *models,
    const char *const *model_paths, size_t model_count,
    int threads, int context_length);
KataliRouteStatus katali_route_gguf_pool_open_config(
    KataliRouteGgufPool *pool, const KataliRouteConfig *config);
KataliRouteStatus katali_route_gguf_pool_ensure_loaded(
    KataliRouteGgufPool *pool, const char *model_id);
const KataliRouteGgufSdk *katali_route_gguf_pool_entry(
    const KataliRouteGgufPool *pool, const char *model_id);
const char *katali_route_gguf_pool_next_available(
    const KataliRouteGgufPool *pool, const char *model_id);
KataliRouteStatus katali_route_gguf_pool_apply_escalation(
    const KataliRouteGgufPool *pool, double minimum_confidence,
    KataliRouteDecision *decision);
void katali_route_gguf_pool_close(KataliRouteGgufPool *pool);
KataliRouteGgufClient katali_route_gguf_pool_client(KataliRouteGgufPool *pool);
const char *katali_route_gguf_sdk_error(void);

#ifdef __cplusplus
}
#endif
#endif
