# Katali-GGUF integration contract

Katali-Route is the caller-side decision layer. Katali-GGUF remains the
generation engine. The current native boundary is:

```c
KataliRouteGgufClient client = {
    .generate = host_generate,
    .validate = host_validate_or_null,
    .user = host_context
};
katali_route_execute(&request, &client, output, output_cap, &decision);
```

`host_generate` may call the existing Katali-GGUF loopback API or the binary
SDK. The Route library does not include GGUF headers and does not load model
weights.

## Metadata

The host converts GGUF status metadata into `KataliRouteModel` fields:

| Route field | GGUF source/meaning |
| --- | --- |
| `id` | Host model identifier or path alias |
| `backend` | `katali-gguf` |
| `context_length` | Effective context available to the session |
| `max_output_tokens` | Host-configured generation limit |
| `available` | Model loaded and usable |

For a loopback host, `KataliRouteModelInfo.endpoint` may carry the server
endpoint and `katali_route_model_json` provides a transport-neutral metadata
object.

## Execution semantics

- `generate` passes the selected model ID to the host callback.
- Validation failure causes another generation attempt while the retry budget is
  positive.
- Exhausted retries return an `escalate` decision.
- `routing_enabled == 0` returns `bypass` and calls the host with a null model
  ID, preserving the host's direct-generation path.
- Route never interprets generated text as a replacement inference engine; the
  optional validator belongs to the host or an integration layer.

When a provider has already produced a decision, hosts should call
`katali_route_execute_decision`. It executes that exact selected model and does
not recompute the deterministic fallback route. The native Laya-to-GGUF path
and resident multi-model pool use this API.

The current Katali-GGUF loopback API exposes health/status, load, generate,
reset, cancel, and shutdown operations. A future host adapter can map these
operations to `KataliRouteGgufClient` without changing the Route ABI.

For the native SDK pool, load `config/route.conf` with
`katali_route_config_load`, convert the configuration with
`katali_route_config_models`, and initialize with
`katali_route_gguf_pool_open_config`. The pool owns all SDK model/session
handles and must be closed with `katali_route_gguf_pool_close` on every exit
path. The host-facing sequence is demonstrated in
`examples/route_gguf_pool.c`.

Pool entries copy their configured IDs and paths during initialization, so the
caller may release or reuse its configuration storage after
`katali_route_gguf_pool_open_config` returns.
