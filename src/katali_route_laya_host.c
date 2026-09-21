#include "katali_route_laya_host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef const OrtApiBase *(ORT_API_CALL *GetApiBaseFn)(void);

static void set_error(KataliRouteLayaHost *host, const char *message) {
    strncpy(host->error, message ? message : "Laya host initialization failed", sizeof(host->error)-1);
    host->error[sizeof(host->error)-1] = '\0';
}

static KataliRouteStatus ort_failure(KataliRouteLayaHost *host, OrtStatus *status) {
    if (status) { set_error(host, host->api ? host->api->GetErrorMessage(status) : "ONNX Runtime call failed"); if (host->api) host->api->ReleaseStatus(status); }
    return KATALI_ROUTE_ERR_MODEL;
}

static int read_file(const char *path, char **data, size_t *length) {
    FILE *file; long size; char *buffer;
    file=fopen(path,"rb"); if(!file)return -1; if(fseek(file,0,SEEK_END)!=0){fclose(file);return -1;} size=ftell(file); if(size<0){fclose(file);return -1;} rewind(file);
    buffer=(char *)malloc((size_t)size+1); if(!buffer){fclose(file);return -1;}
    if(fread(buffer,1,(size_t)size,file)!=(size_t)size){free(buffer);fclose(file);return -1;} fclose(file); buffer[size]='\0'; *data=buffer; *length=(size_t)size; return 0;
}

KataliRouteStatus katali_route_laya_host_init(KataliRouteLayaHost *host, const char *tokenizer_json_path, const char *onnx_path, const char *runtime_dll_path) {
    FARPROC raw; GetApiBaseFn get_api_base; const OrtApiBase *base; OrtStatus *status;
    size_t json_len; wchar_t wide_onnx_path[1024]; int wide_len;
    if(!host || !tokenizer_json_path || !onnx_path)return KATALI_ROUTE_ERR_ARGUMENT;
    memset(host,0,sizeof(*host));
    if(read_file(tokenizer_json_path,&host->tokenizer_json,&json_len)!=0){set_error(host,"could not read Laya tokenizer JSON");return KATALI_ROUTE_ERR_MODEL;}
    if(katali_route_laya_tokenizer_init(&host->tokenizer,host->tokenizer_json,json_len)!=KATALI_ROUTE_OK){set_error(host,"could not initialize Laya tokenizer");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    host->dll=LoadLibraryA(runtime_dll_path && runtime_dll_path[0] ? runtime_dll_path : "onnxruntime.dll");
    if(!host->dll){set_error(host,"could not load ONNX Runtime DLL");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    raw=GetProcAddress(host->dll,"OrtGetApiBase"); if(!raw){set_error(host,"ONNX Runtime DLL lacks OrtGetApiBase");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    memcpy(&get_api_base,&raw,sizeof(get_api_base)); base=get_api_base(); if(!base){set_error(host,"ONNX Runtime returned no API base");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    host->api=base->GetApi(ORT_API_VERSION); if(!host->api){set_error(host,"ONNX Runtime API version is unavailable");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    wide_len=MultiByteToWideChar(CP_UTF8,0,onnx_path,-1,wide_onnx_path,(int)(sizeof(wide_onnx_path)/sizeof(wide_onnx_path[0]))); if(!wide_len){set_error(host,"could not convert Laya ONNX path");katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_ARGUMENT;}
    status=host->api->CreateEnv(ORT_LOGGING_LEVEL_WARNING,"katali-route-laya-host",&host->env); if(status){ort_failure(host,status);katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    status=host->api->CreateSessionOptions(&host->options); if(status){ort_failure(host,status);katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    status=host->api->CreateSession(host->env,wide_onnx_path,host->options,&host->session); if(status){ort_failure(host,status);katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    status=host->api->CreateCpuMemoryInfo(OrtDeviceAllocator,OrtMemTypeDefault,&host->memory); if(status){ort_failure(host,status);katali_route_laya_host_dispose(host);return KATALI_ROUTE_ERR_MODEL;}
    host->output=(KataliRouteTokenizerOutput){host->input_ids,host->attention_mask,host->marker_pos,host->marker_mask,host->labels,512,64,0,0,0};
    host->provider.runtime=(KataliRouteLayaOrt){host->api,host->session,host->memory,0.0}; host->provider.tokenizer=&host->tokenizer; host->provider.output=&host->output;
    return KATALI_ROUTE_OK;
}

void katali_route_laya_host_dispose(KataliRouteLayaHost *host) {
    char error[sizeof(host->error)];
    if(!host)return;
    memcpy(error,host->error,sizeof(error));
    if(host->api && host->memory)host->api->ReleaseMemoryInfo(host->memory);
    if(host->api && host->session)host->api->ReleaseSession(host->session);
    if(host->api && host->options)host->api->ReleaseSessionOptions(host->options);
    if(host->api && host->env)host->api->ReleaseEnv(host->env);
    if(host->dll)FreeLibrary(host->dll);
    katali_route_laya_tokenizer_dispose(&host->tokenizer); free(host->tokenizer_json);
    memset(host,0,sizeof(*host)); memcpy(host->error,error,sizeof(host->error));
}

KataliRouteDecisionProvider katali_route_laya_host_provider(KataliRouteLayaHost *host) { return katali_route_laya_ort_tokenizer_provider(&host->provider); }
const char *katali_route_laya_host_error(const KataliRouteLayaHost *host) { return host ? host->error : "invalid Laya host"; }
