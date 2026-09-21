# Implementation status

## Complete

- Native C request, model, decision, policy, and provider contracts.
- Safe JSON serialization for decisions, metadata, and typed probabilities.
- Explicit error codes and deterministic contract tests.
- Katali-GGUF client boundary with retry, validation, escalation, and bypass.
- Laya adapter boundary with typed decision and probability mapping.
- Stable rationale codes, trace callback, confidence thresholds, and retry policy.
- CPU routing benchmark and build/test automation.
- Laya compatibility investigation without downloading weights.

## External integration status

- ONNX Runtime CPU 1.30.0 is installed and its bundled sigmoid model was
  successfully executed through `CPUExecutionProvider`. The official native
  NuGet package is also staged under `work/ort-package`; a C smoke test loads
  the DLL and reports C API version 30 / runtime 1.30.0. A second C smoke test
  creates a session, runs the bundled sigmoid graph, and returns `0.500000`.
  The full Laya graph is also loaded through the optional native C bridge.
- The published Laya ONNX graph and external data are staged under
  `work/laya-onnx`. The native C bridge runs the graph, applies automatic
  option-count temperature calibration, and maps logits into a typed choice.
- `mlc-ai/tokenizers-cpp` is built for the local MinGW target and loads Laya's
  Hugging Face `tokenizer.json` through its C ABI. The native Laya adapter builds
  the `[CLS] ... [MASK] option ... [SEP]` sequence, enforces the 512-token
  limit, and passes validated caller-owned buffers into ONNX Runtime.
- The complete native path is verified with short, Unicode, structured-output,
  long-input, and low-confidence escalation scenarios. One current baseline is
  `tokenize_ms=1.000`, `route_ms=77.000`, `total_ms=78.000` on CPU.
- The optional Katali-GGUF SDK adapter is now verified with all four local Qwen3
  Q4_K_M checkpoints through the config-driven pool, including eager/lazy
  lifecycle, unavailable-model fallback, confidence escalation, and exact Laya
  model dispatch.
- GPU measurements are intentionally omitted: the current Katali-GGUF SDK and
  native Laya path expose CPU execution only and no GPU/provider API.
- The reusable Laya host helper, real `examples/laya_gguf_pool.c`, and the
  top-level `build_release_verification.bat` release sequence are complete.
- `package_release.bat` produces a runnable package from its own directory,
  including the supplied application icons and Laya external tensor data.
