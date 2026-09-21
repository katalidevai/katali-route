#include <stdio.h>
#include "katali_route_config.h"
#include "katali_route_gguf_sdk.h"
#include "katali_route_laya_host.h"

int main(int argc, char **argv) {
    const char *config_path=argc>1?argv[1]:"config\\route.conf.example";
    KataliRouteConfig config; KataliRouteModel models[4]; KataliRouteLayaHost laya;
    KataliRouteGgufPool pool; KataliRouteGgufClient client; KataliRouteDecisionProvider provider;
    KataliRouteRequest request; KataliRouteDecision decision; KataliRouteTypedDecision typed; char output[4096];
    if(katali_route_config_load(config_path,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"config error: %s\n",katali_route_config_error());return 1;}
    katali_route_config_models(&config,models);
    if(katali_route_laya_host_init(&laya,"work\\laya-onnx\\tokenizer\\tokenizer.json","work\\laya-onnx\\laya.onnx","onnxruntime.dll")!=KATALI_ROUTE_OK){fprintf(stderr,"Laya startup error: %s\n",katali_route_laya_host_error(&laya));return 2;}
    if(katali_route_gguf_pool_open_config(&pool,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"GGUF pool startup error: %s\n",katali_route_gguf_sdk_error());katali_route_laya_host_dispose(&laya);return 3;}
    request=(KataliRouteRequest){"laya-host-example","Choose the best Qwen model for this request.",models,4,1,0,0}; provider=katali_route_laya_host_provider(&laya);
    if(katali_route_decide_with_provider(&request,&provider,&decision,&typed)!=KATALI_ROUTE_OK){fprintf(stderr,"Laya decision failed: %s\n",katali_route_laya_host_error(&laya));katali_route_gguf_pool_close(&pool);katali_route_laya_host_dispose(&laya);return 4;}
    printf("laya_decision model=%s confidence=%.6f choices=%zu\n",decision.model_id,typed.value,typed.probability_count);
    client=katali_route_gguf_pool_client(&pool);
    if(katali_route_execute_decision(&request,&client,output,sizeof(output),&decision)!=KATALI_ROUTE_OK){fprintf(stderr,"GGUF generation failed: %s\n",katali_route_gguf_sdk_error());katali_route_gguf_pool_close(&pool);katali_route_laya_host_dispose(&laya);return 5;}
    printf("laya_gguf_model=%s output=%s\n",decision.model_id,output);
    katali_route_gguf_pool_close(&pool); katali_route_laya_host_dispose(&laya); return 0;
}
