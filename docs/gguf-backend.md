# Katali-GGUF backend handoff

Katali-Route does not load GGUF weights or own text generation. A host runtime
supplies a `KataliRouteGgufClient` through `include/katali_route.h`.

The host must provide:

- `generate(user, model_id, prompt, output, output_cap)`, which runs the
  selected GGUF model and writes a terminated response into the caller-owned
  output buffer.
- `validate(user, output)`, when structured-output validation is enabled.

The route decision carries the selected model ID, action, confidence, retry
budget, and rationale. `katali_route_execute` passes a null model ID only for
an explicit bypass; otherwise it passes the selected model ID unchanged. A
validation failure consumes the retry budget and then produces an escalation
decision.

The optional SDK adapter is verified against the local Katali-GGUF DLL and
`C:\models\qwen3-0.6b-gguf\Qwen3-0.6B-Q4_K_M.gguf`. One end-to-end run produced
an 8-token answer in `1124.344 ms` SDK-reported total time, with 33 prompt
tokens and `9.554` decode tokens/sec. The measurement is a CPU baseline; the
current SDK exposes no GPU backend or provider API.

The four-model resident pool also loads successfully. Pool load was `377 ms`
with the SDK's memory-mapped model loading. Three warm one-token probes reported
approximately `282–347 ms` for 0.6B, `798–894 ms` for 1.7B, `1928–2116 ms` for
4B, and `3837–4188 ms` for 8B. The pool dispatch selected each model by ID and
reported decode rates of roughly `9.3`, `3.4`, `1.4`, and `0.79` tokens/sec.
The benchmark now measures route selection separately; it was below the
`0.001 ms` display resolution for all four models in the latest run.

## Configured four-model pool

`include/katali_route_config.h` and `src/katali_route_config.c` provide a
strict C loader for `config/route.conf`. The loader validates all four IDs,
paths, context sizes, output limits, duplicate IDs, and available files before
startup. Unknown keys and malformed values are startup errors.

`katali_route_gguf_pool_open_config` supports `pool_load_mode=eager` (the
default) and `pool_load_mode=lazy`. Each pool entry records load time, loaded
state, resident bytes from the SDK, and the SDK error text when loading fails.
`katali_route_gguf_pool_apply_escalation` selects the next larger available
model when a provider confidence is below the configured threshold. Hosts must
dispatch a provider-produced decision through `katali_route_execute_decision`
so the selected Laya model is preserved exactly.

`tests/benchmark_gguf_pool.c` writes one JSONL record per warm generation and
summary records with p50, p95, and p99 latency. The current SDK exposes only
CPU model/session APIs; no GPU/provider API is present, so these measurements
are CPU-only by design.
