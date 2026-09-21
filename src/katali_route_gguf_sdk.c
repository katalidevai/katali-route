#include "katali_route_gguf_sdk.h"
#include <string.h>
#include <time.h>

static double now_ms(void) { return (double)clock() * 1000.0 / CLOCKS_PER_SEC; }

static int sdk_generate(void *user, const char *model_id, const char *prompt,
                        char *output, size_t output_cap) {
    KataliRouteGgufSdk *adapter = (KataliRouteGgufSdk *)user;
    size_t out_len = 0;
    if (!adapter || !adapter->loaded || !adapter->session || !prompt || !output || !output_cap) return -1;
    if (model_id && adapter->model_id && strcmp(model_id, adapter->model_id) != 0) return -1;
    return katali_sdk_generate(adapter->session, prompt, output, output_cap, &out_len) < 0 ? -1 : 0;
}

static int sdk_validate(void *user, const char *output) {
    (void)user;
    return output && output[0] ? 0 : -1;
}

static KataliRouteGgufSdk *pool_find(KataliRouteGgufPool *pool, const char *model_id) {
    size_t i;
    if (!pool || !model_id) return NULL;
    for (i = 0; i < pool->count; ++i) if (pool->entries[i].model_id && strcmp(pool->entries[i].model_id, model_id) == 0) return &pool->entries[i];
    return NULL;
}

static int pool_generate(void *user, const char *model_id, const char *prompt,
                         char *output, size_t output_cap) {
    KataliRouteGgufPool *pool = (KataliRouteGgufPool *)user;
    KataliRouteGgufSdk *entry = pool_find(pool, model_id);
    if (entry && !entry->loaded && katali_route_gguf_pool_ensure_loaded(pool, model_id) != KATALI_ROUTE_OK) return -1;
    return entry ? sdk_generate(entry, model_id, prompt, output, output_cap) : -1;
}

static int pool_validate(void *user, const char *output) { return sdk_validate(user, output); }

KataliRouteStatus katali_route_gguf_sdk_open(
    KataliRouteGgufSdk *adapter, const char *model_id,
    const char *model_path, int threads, int context_length) {
    katali_model_options options;
    katali_gen_options generation;
    if (!adapter || !model_id || !model_path || !model_id[0] || !model_path[0] ||
        strlen(model_id) >= KATALI_ROUTE_CONFIG_ID_CAP || strlen(model_path) >= KATALI_ROUTE_CONFIG_PATH_CAP) return KATALI_ROUTE_ERR_ARGUMENT;
    {
        double start = now_ms();
        memset(adapter, 0, sizeof(*adapter));
    strcpy(adapter->model_id_storage, model_id); strcpy(adapter->model_path_storage, model_path);
    adapter->model_id = adapter->model_id_storage; adapter->model_path = adapter->model_path_storage;
        adapter->threads = threads; adapter->context_length = context_length; adapter->available = 1;
    memset(&options, 0, sizeof(options));
    options.struct_size = sizeof(options); options.model_path = adapter->model_path;
    options.n_threads = threads; options.context_length = context_length; options.use_mmap = 1;
        adapter->model = katali_sdk_model_open(&options);
        if (!adapter->model) { strncpy(adapter->error, katali_sdk_last_error(), sizeof(adapter->error)-1); return KATALI_ROUTE_ERR_MODEL; }
    memset(&generation, 0, sizeof(generation));
    generation.struct_size = sizeof(generation); generation.max_tokens = 8;
        adapter->session = katali_sdk_session_create(adapter->model, &generation);
        if (!adapter->session) { strncpy(adapter->error, katali_sdk_last_error(), sizeof(adapter->error)-1); katali_sdk_model_close(adapter->model); adapter->model = NULL; return KATALI_ROUTE_ERR_MODEL; }
        adapter->loaded = 1; adapter->load_ms = now_ms() - start;
        { katali_model_info info; memset(&info, 0, sizeof(info)); info.struct_size = sizeof(info); if (katali_sdk_model_info(adapter->model, &info) == KATALI_SDK_OK) adapter->resident_bytes = info.resident_bytes; }
    }
    return KATALI_ROUTE_OK;
}

void katali_route_gguf_sdk_close(KataliRouteGgufSdk *adapter) {
    if (!adapter) return;
    if (adapter->session) katali_sdk_session_destroy(adapter->session);
    if (adapter->model) katali_sdk_model_close(adapter->model);
    memset(adapter, 0, sizeof(*adapter));
}

KataliRouteGgufClient katali_route_gguf_sdk_client(KataliRouteGgufSdk *adapter) {
    KataliRouteGgufClient client = {sdk_generate, sdk_validate, adapter};
    return client;
}

KataliRouteStatus katali_route_gguf_pool_open(
    KataliRouteGgufPool *pool, const KataliRouteModel *models,
    const char *const *model_paths, size_t model_count,
    int threads, int context_length) {
    size_t i;
    if (!pool || !models || !model_paths || !model_count || model_count > KATALI_ROUTE_GGUF_POOL_MAX) return KATALI_ROUTE_ERR_ARGUMENT;
    memset(pool, 0, sizeof(*pool));
    for (i = 0; i < model_count; ++i) {
        if (!models[i].id || !model_paths[i] || katali_route_gguf_sdk_open(&pool->entries[i], models[i].id, model_paths[i], threads, context_length) != KATALI_ROUTE_OK) {
            katali_route_gguf_pool_close(pool); return KATALI_ROUTE_ERR_MODEL;
        }
        pool->count++;
    }
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_gguf_pool_open_config(KataliRouteGgufPool *pool, const KataliRouteConfig *config) {
    KataliRouteModel models[KATALI_ROUTE_CONFIG_MODEL_COUNT]; int i;
    if (!pool || !config || config->model_count != KATALI_ROUTE_CONFIG_MODEL_COUNT) return KATALI_ROUTE_ERR_ARGUMENT;
    katali_route_config_models(config, models); memset(pool, 0, sizeof(*pool));
    for (i=0; i<config->model_count; ++i) {
        KataliRouteGgufSdk *entry=&pool->entries[i]; memset(entry,0,sizeof(*entry));
        entry->threads=config->threads; entry->context_length=models[i].context_length; entry->available=models[i].available; pool->count++;
        if (strlen(models[i].id) >= KATALI_ROUTE_CONFIG_ID_CAP || strlen(config->paths[i]) >= KATALI_ROUTE_CONFIG_PATH_CAP) { katali_route_gguf_pool_close(pool); return KATALI_ROUTE_ERR_ARGUMENT; }
        strcpy(entry->model_id_storage, models[i].id); strcpy(entry->model_path_storage, config->paths[i]); entry->model_id=entry->model_id_storage; entry->model_path=entry->model_path_storage;
        if (config->pool_load_mode==KATALI_ROUTE_CONFIG_MODE_EAGER && entry->available && katali_route_gguf_sdk_open(entry,models[i].id,config->paths[i],entry->threads,entry->context_length)!=KATALI_ROUTE_OK) { katali_route_gguf_pool_close(pool); return KATALI_ROUTE_ERR_MODEL; }
    }
    return KATALI_ROUTE_OK;
}

KataliRouteStatus katali_route_gguf_pool_ensure_loaded(KataliRouteGgufPool *pool, const char *model_id) {
    KataliRouteGgufSdk *entry=pool_find(pool,model_id);
    char id[KATALI_ROUTE_CONFIG_ID_CAP], path[KATALI_ROUTE_CONFIG_PATH_CAP];
    if (!entry || !entry->available) return KATALI_ROUTE_ERR_MODEL;
    if (entry->loaded) return KATALI_ROUTE_OK;
    strcpy(id, entry->model_id); strcpy(path, entry->model_path);
    return katali_route_gguf_sdk_open(entry,id,path,entry->threads,entry->context_length);
}

const KataliRouteGgufSdk *katali_route_gguf_pool_entry(const KataliRouteGgufPool *pool, const char *model_id) { return pool_find((KataliRouteGgufPool *)pool,model_id); }

const char *katali_route_gguf_pool_next_available(const KataliRouteGgufPool *pool, const char *model_id) {
    size_t i; int found=0;
    if (!pool) return NULL;
    for(i=0;i<pool->count;++i) { if(!found && model_id && pool->entries[i].model_id && !strcmp(pool->entries[i].model_id,model_id)){found=1;continue;} if(found && pool->entries[i].available)return pool->entries[i].model_id; }
    for(i=pool->count;i>0;--i) if(pool->entries[i-1].available)return pool->entries[i-1].model_id;
    return NULL;
}

KataliRouteStatus katali_route_gguf_pool_apply_escalation(const KataliRouteGgufPool *pool, double minimum_confidence, KataliRouteDecision *decision) {
    const char *next;
    if (!pool || !decision || minimum_confidence < 0.0 || minimum_confidence > 1.0) return KATALI_ROUTE_ERR_ARGUMENT;
    if (decision->confidence >= minimum_confidence) return KATALI_ROUTE_OK;
    next = katali_route_gguf_pool_next_available(pool, decision->model_id);
    if (!next) return KATALI_ROUTE_ERR_MODEL;
    decision->model_id = next; decision->action = KATALI_ROUTE_ESCALATE;
    strncpy(decision->rationale, "low_confidence_next_available_model", sizeof(decision->rationale)-1);
    decision->rationale[sizeof(decision->rationale)-1] = '\0';
    return KATALI_ROUTE_OK;
}

void katali_route_gguf_pool_close(KataliRouteGgufPool *pool) {
    size_t i;
    if (!pool) return;
    for (i = 0; i < pool->count; ++i) katali_route_gguf_sdk_close(&pool->entries[i]);
    memset(pool, 0, sizeof(*pool));
}

KataliRouteGgufClient katali_route_gguf_pool_client(KataliRouteGgufPool *pool) {
    KataliRouteGgufClient client = {pool_generate, pool_validate, pool};
    return client;
}

const char *katali_route_gguf_sdk_error(void) { return katali_sdk_last_error(); }
