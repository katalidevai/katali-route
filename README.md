# Katali-Route

**Developed By : Joan Apita**

Katali-Route is a local decision and routing sidecar for Katali-GGUF. It chooses
an execution path; Katali-GGUF remains responsible for model loading and text
generation.

## Current scope

The first implementation provides a dependency-free native C core and CLI:

```bat
build_route.bat
```

The default policy is deterministic and observable. It can select a model,
request validation, retry once for low confidence, and escalate when no model
meets the request constraints. No model weights are downloaded or required.

## Architecture

```text
request + model metadata
        |
        v
  RouteRequest -> PolicyEngine -> RouteDecision
                                      |
                         Katali-GGUF adapter boundary
```

The decision model is represented by the public C routing API in
`include/katali_route.h`. The default provider is deterministic; the optional
native Laya provider loads the published tokenizer JSON, builds typed choice
inputs, and runs through ONNX Runtime without adding Python to the inference
path.

## Run checks

Run `build_route.bat` from a developer prompt to build the CLI and deterministic
C test executable. A native library target can be added once the toolchain's
archiver is stable.

See `docs/architecture.md` and `schemas/route.schema.json` for the contract.
The GGUF host-runtime handoff is described in `docs/gguf-backend.md`.

For integration details, see `docs/integration.md`; for Laya compatibility and
remaining external dependencies, see `docs/laya-investigation.md` and
`docs/status.md`.

After the optional ONNX Runtime package and Laya bundle are staged under
`work/`, run `build_laya_ort_smoke.bat` to build and run the native Laya bridge
fixture.

For the full native tokenizer path, run `build_laya_tokenizer_e2e.bat` after
staging the MinGW-compatible tokenizers-cpp library, ONNX Runtime package, and
Laya bundle under `work/`.

The optional Katali-GGUF SDK pool benchmark is `build\benchmark-gguf-pool.exe`;
it reads `config\route.conf.example` by default and writes
`build\benchmark-gguf-pool.jsonl` with 10 warm runs per model and p50/p95/p99
summaries. The pool/host example is `examples\route_gguf_pool.c`, and the
real native Laya host example is `examples\laya_gguf_pool.c`. The repeatable
Laya-to-four-model verification target is `build_laya_gguf_pool.bat`; the full
release verification sequence is `build_release_verification.bat`.
To assemble a portable host package, run `package_release.bat`. It includes
the supplied `icon.ico` and `icon_head_950.png`, Laya's external ONNX tensor
data, runtime DLLs, configuration template, and documentation; the large
Qwen GGUF files remain external under the configured model paths.
it requires the sibling Katali-GGUF SDK headers/import library and the four
Qwen checkpoints under `C:\models`.
