#include "katali_route_config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char config_error[256];
static void set_error(const char *s) { strncpy(config_error, s ? s : "configuration error", sizeof(config_error)-1); config_error[sizeof(config_error)-1] = '\0'; }
const char *katali_route_config_error(void) { return config_error; }
static char *trim(char *s) { char *e; while (*s && isspace((unsigned char)*s)) ++s; e=s+strlen(s); while (e>s && isspace((unsigned char)e[-1])) --e; *e='\0'; return s; }
static int int_value(const char *s, int *out) { char *e; long n; if (!s || !*s) return -1; n=strtol(s,&e,10); if (*e || n < -2147483647L || n > 2147483647L) return -1; *out=(int)n; return 0; }
static int double_value(const char *s, double *out) { char *e; double n; if (!s || !*s) return -1; n=strtod(s,&e); if (*e) return -1; *out=n; return 0; }
static int copy_value(char *dst, size_t cap, const char *s) { size_t n=strlen(s); if (!n || n>=cap) return -1; memcpy(dst,s,n+1); return 0; }

static int set_model_value(const char *key, const char *value, KataliRouteConfig *c) {
    static const char *names[] = {"0_6b", "1_7b", "4b", "8b"}; int i; char k[96];
    for (i=0; i<KATALI_ROUTE_CONFIG_MODEL_COUNT; ++i) {
        snprintf(k,sizeof(k),"gguf_qwen3_%s_id",names[i]); if (!strcmp(key,k)) return copy_value(c->ids[i],sizeof(c->ids[i]),value);
        snprintf(k,sizeof(k),"gguf_qwen3_%s_path",names[i]); if (!strcmp(key,k)) return copy_value(c->paths[i],sizeof(c->paths[i]),value);
        snprintf(k,sizeof(k),"gguf_qwen3_%s_context_length",names[i]); if (!strcmp(key,k)) return int_value(value,&c->context_lengths[i]);
        snprintf(k,sizeof(k),"gguf_qwen3_%s_max_output_tokens",names[i]); if (!strcmp(key,k)) return int_value(value,&c->max_output_tokens[i]);
        snprintf(k,sizeof(k),"gguf_qwen3_%s_available",names[i]); if (!strcmp(key,k)) return int_value(value,&c->available[i]);
    }
    return 1;
}

static int set_value(const char *key, const char *value, KataliRouteConfig *c) {
    if (!strcmp(key,"routing_enabled") || !strcmp(key,"laya_onnx_path") || !strcmp(key,"laya_tokenizer_json_path") || !strcmp(key,"onnxruntime_dll_path") || !strcmp(key,"tokenizers_c_library_path")) return 0;
    if (!strcmp(key,"threads")) return int_value(value,&c->threads);
    if (!strcmp(key,"pool_load_mode")) { if (!strcmp(value,"eager")) c->pool_load_mode=0; else if (!strcmp(value,"lazy")) c->pool_load_mode=1; else return -1; return 0; }
    if (!strcmp(key,"minimum_confidence")) return double_value(value,&c->minimum_confidence);
    if (!strcmp(key,"max_retries")) return int_value(value,&c->max_retries);
    if (!strcmp(key,"validate_structured_output")) return int_value(value,&c->validate_structured_output);
    return set_model_value(key,value,c);
}

static int validate(KataliRouteConfig *c) {
    int i,j; FILE *f;
    if (c->threads<0 || (c->pool_load_mode!=0 && c->pool_load_mode!=1) || c->minimum_confidence<0.0 || c->minimum_confidence>1.0 || c->max_retries<0 || (c->validate_structured_output!=0 && c->validate_structured_output!=1)) { set_error("invalid global pool setting"); return -1; }
    for (i=0;i<KATALI_ROUTE_CONFIG_MODEL_COUNT;++i) {
        if (!c->ids[i][0] || !c->paths[i][0] || c->context_lengths[i]<=0 || c->max_output_tokens[i]<=0 || (c->available[i]!=0 && c->available[i]!=1)) { set_error("invalid model id, path, context, output limit, or availability"); return -1; }
        for (j=0;j<i;++j) if (!strcmp(c->ids[i],c->ids[j])) { set_error("duplicate GGUF model id"); return -1; }
        if (c->available[i]) { f=fopen(c->paths[i],"rb"); if (!f) { snprintf(config_error,sizeof(config_error),"model file unavailable: %s",c->paths[i]); return -1; } fclose(f); }
    }
    return 0;
}

KataliRouteStatus katali_route_config_load(const char *path, KataliRouteConfig *config) {
    FILE *f; char line[1400]; int line_no=0,i;
    if (!path || !config) return KATALI_ROUTE_ERR_ARGUMENT;
    memset(config,0,sizeof(*config)); config->model_count=4; config->pool_load_mode=0; config->minimum_confidence=0.80; config->max_retries=1; config->validate_structured_output=1;
    for (i=0;i<4;++i) { config->context_lengths[i]=2048; config->max_output_tokens[i]=32; config->available[i]=1; }
    f=fopen(path,"rb"); if (!f) { set_error("could not open route.conf"); return KATALI_ROUTE_ERR_MODEL; }
    while (fgets(line,sizeof(line),f)) { char *eq,*comment,*key,*value; int r; ++line_no; comment=strchr(line,'#'); if(comment)*comment='\0'; key=trim(line); if(!*key)continue; eq=strchr(key,'='); if(!eq){fclose(f);snprintf(config_error,sizeof(config_error),"line %d has no '='",line_no);return KATALI_ROUTE_ERR_ARGUMENT;} *eq='\0'; value=trim(eq+1); key=trim(key); r=set_value(key,value,config); if(r==1){fclose(f);snprintf(config_error,sizeof(config_error),"unknown key on line %d: %s",line_no,key);return KATALI_ROUTE_ERR_ARGUMENT;} if(r<0){fclose(f);snprintf(config_error,sizeof(config_error),"invalid value on line %d: %s",line_no,key);return KATALI_ROUTE_ERR_ARGUMENT;} }
    fclose(f); return validate(config)==0 ? KATALI_ROUTE_OK : KATALI_ROUTE_ERR_MODEL;
}

void katali_route_config_models(const KataliRouteConfig *c, KataliRouteModel *models) { int i; if(!c||!models)return; for(i=0;i<4;++i){models[i].id=c->ids[i];models[i].backend="katali-gguf";models[i].context_length=c->context_lengths[i];models[i].max_output_tokens=c->max_output_tokens[i];models[i].available=c->available[i];} }
