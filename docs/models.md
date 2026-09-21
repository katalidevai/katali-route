# Qwen GGUF downloads

Katali Route does not bundle the large Qwen model files. Download the official
Qwen Q4_K_M files into these paths:

- [Qwen3-0.6B-GGUF](https://huggingface.co/Qwen/Qwen3-0.6B-GGUF): `C:\models\qwen3-0.6b-gguf\Qwen3-0.6B-Q4_K_M.gguf`
- [Qwen3-1.7B-GGUF](https://huggingface.co/Qwen/Qwen3-1.7B-GGUF): `C:\models\qwen3-1.7b-gguf\Qwen3-1.7B-Q4_K_M.gguf`
- [Qwen3-4B-GGUF](https://huggingface.co/Qwen/Qwen3-4B-GGUF): `C:\models\qwen3-4b-gguf\Qwen3-4B-Q4_K_M.gguf`
- [Qwen3-8B-GGUF](https://huggingface.co/Qwen/Qwen3-8B-GGUF): `C:\models\qwen3-8b-gguf\Qwen3-8B-Q4_K_M.gguf`

Install the downloader:

```powershell
py -m pip install -U huggingface_hub
```

Download the files:

```powershell
hf download Qwen/Qwen3-0.6B-GGUF Qwen3-0.6B-Q4_K_M.gguf --local-dir C:\models\qwen3-0.6b-gguf
hf download Qwen/Qwen3-1.7B-GGUF Qwen3-1.7B-Q4_K_M.gguf --local-dir C:\models\qwen3-1.7b-gguf
hf download Qwen/Qwen3-4B-GGUF Qwen3-4B-Q4_K_M.gguf --local-dir C:\models\qwen3-4b-gguf
hf download Qwen/Qwen3-8B-GGUF Qwen3-8B-Q4_K_M.gguf --local-dir C:\models\qwen3-8b-gguf
```

The Qwen3 GGUF model distributions are Apache 2.0. Keep the model-card
attribution when redistributing them.
