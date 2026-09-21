# Benchmarking

`build_route.bat` builds and runs `build/benchmark-route.exe`. The benchmark
reports three CPU-only baselines:

- `route_single_ms`: deterministic policy cost for one request.
- `provider_single_ms`: policy plus replaceable-provider callback cost.
- `route_batch_64_ms`: cost of 64 sequential decisions.

These numbers measure routing overhead only; they do not include GGUF model
loading or generation. The optional four-model pool benchmark separately runs
10 warm generations per configured model and writes JSONL records with load
time, resident bytes, token counts, decode speed, route latency, generation
latency, and p50/p95/p99 summaries.

## Current machine

The development machine exposes an NVIDIA GeForce RTX 4060, but the current
Katali-GGUF SDK and Laya provider expose CPU execution only. There is no real
GPU/provider API to measure, so the pool benchmark reports CPU measurements
only.

The native Laya C smoke fixture currently measures approximately 55.6 ms per
warm CPU inference on this machine. This is a synthetic two-option input and
should not be treated as application-level routing latency.

The complete native tokenizer plus ORT fixture reports CPU process timings;
the machine-readable GGUF report is the production-sizing baseline for the
four configured models.
