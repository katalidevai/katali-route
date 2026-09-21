#include "katali_route.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    KataliRouteModel model = {"default", "katali-gguf", 32768, 512, 1};
    KataliRouteRequest request = {"cli-1", "", &model, 1, 1, 0, 0};
    KataliRouteDecision decision;
    char json[1024];
    if (argc < 2) { fprintf(stderr, "usage: route_main.exe REQUEST\n"); return 2; }
    request.text = argv[1];
    if (katali_route_decide(&request, &decision) != 0 ||
        katali_route_decision_json(&decision, json, sizeof(json)) != 0) return 1;
    puts(json);
    return 0;
}
