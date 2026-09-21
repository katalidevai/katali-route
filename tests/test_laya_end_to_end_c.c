#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "katali_route_config.h"
#include "katali_route_laya_host.h"
#include "katali_route_gguf_sdk.h"

int main(void) {
    KataliRouteConfig config; KataliRouteModel models[4]; KataliRouteLayaHost laya;
    KataliRouteDecisionProvider provider; KataliRouteDecision decision; KataliRouteTypedDecision typed;
    KataliRouteGgufPool pool; KataliRouteGgufClient client; char generated[4096];
    KataliRouteRequest request; clock_t start; double tokenize_ms, route_ms;
    assert(katali_route_config_load("config\\route.conf.example", &config) == KATALI_ROUTE_OK);
    katali_route_config_models(&config, models);
    assert(katali_route_laya_host_init(&laya, "work\\laya-onnx\\tokenizer\\tokenizer.json", "work\\laya-onnx\\laya.onnx", "onnxruntime.dll") == KATALI_ROUTE_OK);
    provider=katali_route_laya_host_provider(&laya);
    request=(KataliRouteRequest){"e2e","Choose the best model for café summarization",models,4,1,0,0};
    start=clock(); assert(katali_route_laya_tokenize(&laya.tokenizer,&request,&laya.output)==0); tokenize_ms=(double)(clock()-start)*1000.0/CLOCKS_PER_SEC;
    start=clock(); assert(katali_route_decide_with_provider(&request,&provider,&decision,&typed)==KATALI_ROUTE_OK); route_ms=(double)(clock()-start)*1000.0/CLOCKS_PER_SEC;
    assert(typed.probability_count==4); assert(laya.output.sequence_length<=512);
    printf("laya_e2e_model=%s confidence=%.6f tokens=%zu markers=%lld,%lld tokenize_ms=%.3f route_ms=%.3f total_ms=%.3f\n",decision.model_id,typed.value,laya.output.sequence_length,(long long)laya.output.marker_pos[0],(long long)laya.output.marker_pos[1],tokenize_ms,route_ms,tokenize_ms+route_ms);
    assert(katali_route_gguf_pool_open_config(&pool,&config)==KATALI_ROUTE_OK); client=katali_route_gguf_pool_client(&pool);
    assert(katali_route_execute_decision(&request,&client,generated,sizeof(generated),&decision)==KATALI_ROUTE_OK); printf("laya_to_gguf_model=%s output=%s\n",decision.model_id,generated);
    { KataliRoutePolicy policy={0.80,1,1}; assert(katali_route_decide_with_provider_policy(&request,&provider,&policy,NULL,NULL,&decision,&typed)==KATALI_ROUTE_OK); assert(decision.action==KATALI_ROUTE_ESCALATE && decision.confidence<policy.minimum_confidence); }
    request.text="Return strict JSON with the selected model and a one-sentence reason."; request.output_json=1;
    assert(katali_route_decide_with_provider(&request,&provider,&decision,&typed)==KATALI_ROUTE_OK); assert(decision.validate_output && laya.output.sequence_length<=512);
    { char long_text[4096]; memset(long_text,'x',sizeof(long_text)-1); long_text[sizeof(long_text)-1]='\0'; request.text=long_text; request.output_json=0; assert(katali_route_decide_with_provider(&request,&provider,&decision,&typed)==KATALI_ROUTE_OK); assert(laya.output.sequence_length<=512 && laya.output.marker_pos[1]<(int64_t)laya.output.sequence_length); }
    katali_route_gguf_pool_close(&pool); katali_route_laya_host_dispose(&laya); return 0;
}
