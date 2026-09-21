#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "katali_route_config.h"
#include "katali_route_gguf_sdk.h"

#define RUNS 10
static double ms(void) { return (double)clock()*1000.0/CLOCKS_PER_SEC; }
static int compare_double(const void *a,const void *b){double x=*(const double*)a,y=*(const double*)b;return x<y?-1:x>y;}
static double percentile(double *v,int n,double p){int i=(int)(p*(n-1));qsort(v,(size_t)n,sizeof(*v),compare_double);return v[i];}
static void json_path(const char *input,char *output,size_t cap){size_t i=0,j=0;while(input&&input[i]&&j+2<cap){if(input[i]=='\\'||input[i]=='"')output[j++]='\\';output[j++]=input[i++];}output[j]='\0';}

int main(int argc, char **argv) {
    KataliRouteConfig config; KataliRouteGgufPool pool; KataliRouteGgufClient client;
    KataliRouteModel models[4]; KataliRouteRequest request; KataliRouteDecision decision; char output[4096];
    const char *config_path=argc>1?argv[1]:"config\\route.conf.example"; const char *report_path=argc>2?argv[2]:"build\\benchmark-gguf-pool.jsonl"; FILE *report; size_t i; int run;
    if(katali_route_config_load(config_path,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"config_error=%s\n",katali_route_config_error());return 1;}
    katali_route_config_models(&config,models);
    if(katali_route_gguf_pool_open_config(&pool,&config)!=KATALI_ROUTE_OK){fprintf(stderr,"pool_error=%s\n",katali_route_gguf_sdk_error());return 2;}
    report=fopen(report_path,"wb");if(!report){katali_route_gguf_pool_close(&pool);return 3;}
    client=katali_route_gguf_pool_client(&pool);request=(KataliRouteRequest){"benchmark","Answer with one number: 2+2",models,4,1,0,0};
    { char escaped_config[2048]; json_path(config_path,escaped_config,sizeof(escaped_config)); fprintf(report,"{\"type\":\"metadata\",\"runs\":%d,\"config\":\"%s\",\"load_mode\":\"%s\"}\n",RUNS,escaped_config,config.pool_load_mode?"lazy":"eager"); }
    printf("gguf_pool_report=%s runs=%d mode=%s\n",report_path,RUNS,config.pool_load_mode?"lazy":"eager");
    for(i=0;i<pool.count;++i){double lat[RUNS];const KataliRouteGgufSdk *entry=&pool.entries[i];size_t j;int completed=0;if(!entry->available)continue;for(j=0;j<pool.count;++j)models[j].available=(j==i);
        for(run=0;run<RUNS;++run){double start=ms(),route_start,route_ms,total_ms;katali_sdk_stats stats;route_start=ms();if(katali_route_decide(&request,&decision)!=KATALI_ROUTE_OK||katali_route_execute_decision(&request,&client,output,sizeof(output),&decision)!=KATALI_ROUTE_OK){fprintf(report,"{\"type\":\"failure\",\"model_id\":\"%s\",\"run\":%d}\n",models[i].id,run+1);break;}route_ms=ms()-route_start;total_ms=ms()-start;lat[completed++]=total_ms;memset(&stats,0,sizeof(stats));stats.struct_size=sizeof(stats);katali_sdk_session_stats(pool.entries[i].session,&stats);fprintf(report,"{\"type\":\"run\",\"model_id\":\"%s\",\"run\":%d,\"route_ms\":%.3f,\"total_ms\":%.3f,\"load_ms\":%.3f,\"resident_bytes\":%llu,\"prompt_tokens\":%d,\"generated_tokens\":%d,\"decode_tps\":%.3f}\n",models[i].id,run+1,route_ms,total_ms,entry->load_ms,(unsigned long long)stats.resident_bytes,stats.last_prompt_tokens,stats.last_generated_tokens,stats.last_decode_tps);}
        if(completed==RUNS){double p50=percentile(lat,completed,.50),p95=percentile(lat,completed,.95),p99=percentile(lat,completed,.99);fprintf(report,"{\"type\":\"summary\",\"model_id\":\"%s\",\"p50_ms\":%.3f,\"p95_ms\":%.3f,\"p99_ms\":%.3f,\"load_ms\":%.3f,\"resident_bytes\":%llu}\n",models[i].id,p50,p95,p99,entry->load_ms,(unsigned long long)entry->resident_bytes);printf("gguf_pool_model=%s p50_ms=%.3f p95_ms=%.3f p99_ms=%.3f load_ms=%.3f resident_mb=%.1f\n",models[i].id,p50,p95,p99,entry->load_ms,entry->resident_bytes/1048576.0);}
    }
    fclose(report);katali_route_gguf_pool_close(&pool);return 0;
}
