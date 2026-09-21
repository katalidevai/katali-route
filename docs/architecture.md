# Architecture

Katali-Route is a sidecar, not an inference engine. Its input is a user
request plus metadata for available Katali-GGUF execution paths. Its output is
a typed decision that the caller can execute or inspect.

The policy layer depends only on the schemas. A future Laya-backed provider
must implement the same decision boundary and remain optional, so offline
operation and deterministic tests continue to work without model weights.

The example policy values are in `config/route.conf.example`; a host may parse
them into `KataliRoutePolicy` before calling the C API.

Integration should initially use the existing Katali-GGUF loopback API. The
direct path remains available whenever `routing_enabled` is false.

The native integration boundary is `KataliRouteGgufClient`: the host supplies
generation and optional output-validation callbacks. Routed execution passes a
selected model id; bypass execution passes a null model id so the host can
preserve its normal direct-generation path. Model metadata can be exchanged as
the JSON object emitted by `katali_route_model_json`.

Deferred until Laya compatibility is confirmed: ONNX Runtime dependencies,
checkpoint downloads, batching behavior, and CPU/GPU benchmarks.
