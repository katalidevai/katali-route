#include <stdio.h>
#include "katali_route_config.h"
#include "katali_route_gguf_sdk.h"

/* A host normally obtains `decision` from the native Laya/ONNX provider.
 * This dependency-light example uses the same route decision interface. */
int main(int argc, char **argv) {
    KataliRouteConfig config; KataliRouteModel models[4]; KataliRouteGgufPool pool;
    KataliRouteGgufClient client; KataliRouteRequest request; KataliRouteDecision decision; char output[4096];
    const char *path=argc>1?argv[1]:"config\\route.conf.example";
    if(katali_route_config_load(path,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"startup error: %s\n",katali_route_config_error());return 1;}
    katali_route_config_models(&config,models);
    if(katali_route_gguf_pool_open_config(&pool,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"pool startup error: %s\n",katali_route_gguf_sdk_error());return 2;}
    request=(KataliRouteRequest){"host-example","Choose the smallest suitable Qwen model.",models,4,1,0,0};
    if(katali_route_decide(&request,&decision)!=KATALI_ROUTE_OK){katali_route_gguf_pool_close(&pool);return 3;}
    printf("provider_decision model=%s confidence=%.3f action=%s\n",decision.model_id,decision.confidence,katali_route_action_name(decision.action));
    client=katali_route_gguf_pool_client(&pool);
    if(katali_route_execute_decision(&request,&client,output,sizeof(output),&decision)!=KATALI_ROUTE_OK){katali_route_gguf_pool_close(&pool);return 4;}
    printf("generated_model=%s output=%s\n",decision.model_id,output);
    katali_route_gguf_pool_close(&pool); return 0;
}
