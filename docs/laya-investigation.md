# Laya investigation

Status: compatibility reviewed; ONNX weights downloaded with approval for local smoke testing.

The published English ONNX graph and external data are now staged locally for
smoke testing. A CPU ONNX Runtime run with the published tokenizer succeeded
with inputs shaped exactly as documented and returned logits `(1, 2)` plus action
probabilities. This was a synthetic two-option fixture, not a production
calibration result.

## Sources and licensing

The official runtime wrapper reviewed is [`receptron/laya`](https://github.com/receptron/laya).
Its wrapper/export code is MIT-licensed. The published Convai Innovations Laya
weights are Apache 2.0. The ONNX bundle is published as
[`receptron/laya-onnx`](https://huggingface.co/receptron/laya-onnx).

## Runtime and format

The official wrapper uses Node.js 20+ and ONNX Runtime; Python and PyTorch are
not needed at runtime. The bundle contains `laya.onnx`, `laya.onnx.data`,
`laya_config.json`, and tokenizer JSON files. The graph accepts batched
`input_ids`, `attention_mask`, `marker_pos`, `marker_mask`, and `qtype`, and
returns logits plus action probabilities. This is compatible with a future C
adapter through ONNX Runtime C API, but it is not a GGUF model and should stay
an optional decision-provider backend.

## Limits relevant to Katali-Route

- English checkpoint state is truncated to 512 tokens; the typed-decisions
  checkpoint is listed with 1024-token context.
- Each question has a head budget of roughly 192–256 tokens, and fewer than
  about 20 choice options is recommended.
- Questions in one call are batched; the published wrapper reports roughly
  140 ms for three warm Apple-silicon CPU questions and about 33 ms for one
  question on a T4. These are upstream figures, not Katali measurements.
- The default bundle is about 1.7 GB fp32 and needs roughly 2 GB RAM plus batch
  memory, so weights are intentionally not downloaded during this phase.
- CPU execution is explicitly supported through ONNX Runtime's `cpu` provider;
  GPU provider availability is runtime/platform dependent.

The published metadata was inspected without fetching weights: the English
export declares `max_len: 512`, `head_max_len: 192`, and per-cardinality
temperatures for `choice`, `score`, and `noul`. Its tokenizer metadata uses
`input_ids` and `attention_mask`; the graph-level marker/question inputs remain
as documented by the ONNX model card.

## Decision

Laya is a good fit for typed pre-generation decisions, but not for text
generation. The planned adapter should translate `choice`, `score`, and
`noul` into Katali decisions and preserve probability distributions and
confidence. Calibration must remain visible because the upstream typed-
decisions model reports overconfidence and recommends refitting temperature on
held-out data.

The native C bridge intentionally stops at a tokenizer callback boundary:
`KataliRouteLayaTokenizeFn` supplies token IDs, masks, marker positions, question
type, and labels, while `katali_route_laya_ort_choice` owns ONNX execution and
probability conversion. This keeps the tokenizer replaceable and avoids making
the Route core depend on a specific tokenizer implementation.

The published Hugging Face `tokenizers` project is Rust-first and currently
does not provide a stable native C API. The Laya tokenizer is a ByteLevel BPE
tokenizer, so production integration should use a small C ABI adapter around a
selected native tokenizer implementation rather than parsing `tokenizer.json`
inside the routing core or introducing a Python runtime.

The selected implementation is `mlc-ai/tokenizers-cpp`. Its C binding loads a
complete Hugging Face `tokenizer.json` in memory and exposes encode/free APIs.
The Rust binding was built for the local MinGW target and the adapter verified
Laya's tokenizer with a Unicode input, producing sequence length 21 and marker
positions 10 and 12 for two options. The default dependency-free build remains
unchanged; the tokenizer binding belongs to the optional Laya build.
