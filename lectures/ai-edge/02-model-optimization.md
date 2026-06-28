# Edge AI Deployment & Operations in IoT Systems

## Lecture Overview

This lecture covers the **systems engineering** of deploying and operating AI models across heterogeneous IoT edge fleets. We focus on the full lifecycle: model packaging, OTA delivery, runtime execution, observability, and fleet-wide operations — from an IoT infrastructure perspective.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with IoT/devops/distributed systems background  
**Reference:** *Machine Learning Systems at Scale* (MLSysBook Vol 2) — https://mlsysbook.ai/vol2/

---

## Part 1: Model Packaging & Artifact Management (25 minutes)

### The Model as a Deployable Artifact

In IoT systems, a model is not just weights — it's a **versioned, signed, platform-specific artifact** with dependencies.

```
┌─────────────────────────────────────────────────────────────────┐
│                    MODEL ARTIFACT BUNDLE                         │
├─────────────────────────────────────────────────────────────────┤
│  model.tflite / model.onnx      ← Compiled model (INT8/FP16)   │
│  metadata.json                  ← Schema, version, requirements │
│  preprocessing.yaml             ← Normalization, tokenization  │
│  postprocessing.yaml            ← Thresholds, NMS, decoding    │
│  signature.sha256               ← Integrity verification        │
│  requirements.txt               ← Runtime deps (TFLite 2.14+)   │
│  platform_manifest.yaml         ← Target: rpi4-arm64, jetson   │
└─────────────────────────────────────────────────────────────────┘
```

### Metadata Schema (Interoperability)

```yaml
# metadata.json
{
  "model_id": "defect-detector-v3",
  "version": "3.2.1",
  "framework": "tflite",
  "precision": "int8",
  "input_schema": {
    "name": "image",
    "shape": [1, 224, 224, 3],
    "dtype": "uint8",
    "normalization": {"mean": [123.675, 116.28, 103.53], "std": [58.395, 57.12, 57.375]}
  },
  "output_schema": {
    "name": "defect_class",
    "shape": [1, 7],
    "dtype": "float32",
    "classes": ["normal", "scratch", "dent", "crack", "corrosion", "discolor", "foreign"]
  },
  "requirements": {
    "runtime": "tflite >= 2.14.0",
    "delegates": ["gpu", "nnapi"],
    "memory_mb": 150,
    "compute_tops": 0.5
  },
  "split_config": {
    "supports_split": true,
    "split_points": [12, 18, 24],
    "activation_sizes_mb": [0.8, 2.1, 4.5]
  },
  "signature": "sha256:abc123...",
  "built_at": "2026-01-15T10:30:00Z",
  "built_by": "ci-pipeline-42"
}
```

### Multi-Platform Build Pipeline

```
┌────────────────────────────────────────────────────────────────────┐
│                    CI/CD FOR EDGE MODELS                            │
├────────────────────────────────────────────────────────────────────┤
│                                                                    │
│  [Train] → [Export ONNX] → [Quantize] → [Compile per Platform]   │
│    │           │            │              │                       │
│    ▼           ▼            ▼              ▼                       │
│  PyTorch    ONNX          TFLite         ┌──────────────────┐     │
│  Checkpoint            PTQ/QAT           │ rpi4-arm64       │     │
│                                        │ jetson-arm64     │     │
│                                        │ x86_64-cuda      │     │
│                                        │ esp32-xtensa     │     │
│                                        └────────┬─────────┘     │
│                                                 │               │
│                                                 ▼               │
│                                        [Sign & Package]         │
│                                                 │               │
│                                                 ▼               │
│                                        [Model Registry]         │
│                                                 │               │
│                                                 ▼               │
│                                        [OTA Delivery]           │
│                                                                    │
└────────────────────────────────────────────────────────────────────┘
```

**Platform-Specific Compilation:**
| Platform | Toolchain | Output | Delegates |
|----------|-----------|--------|-----------|
| Raspberry Pi 4/5 | TVM / TFLite | `.tflite` / `.so` | GPU (V3D), NNAPI |
| Jetson Orin/NX | TensorRT / TFLite | `.plan` / `.tflite` | GPU, DLA |
| x86_64 Server | ONNX Runtime / TensorRT | `.onnx` / `.plan` | CUDA, TensorRT |
| ESP32 / MCU | TFLite Micro / TVM | `.tflite` / `.c` | CMSIS-NN |

---

## Part 2: Over-the-Air (OTA) Delivery System (30 minutes)

### OTA Architecture for Model Updates

```
┌────────────────────────────────────────────────────────────────────────┐
│                    OTA DELIVERY PIPELINE                                │
├────────────────────────────────────────────────────────────────────────┤
│                                                                        │
│  [Model Registry]                                                     │
│       │                                                               │
│       ▼                                                               │
│  [Delta Generator] ──→ [bsdiff / xdelta] ──→ [Delta Package]         │
│       │                      (90% size reduction)                     │
│       ▼                                                               │
│  [Signing Service] ──→ [ECDSA-P256] ──→ [Signed Delta]               │
│       │                                                               │
│       ▼                                                               │
│  [CDN / Edge Cache] ──→ [Geo-distributed, TLS]                       │
│       │                                                               │
├───────┼───────────────────────────────────────────────────────────────┤
│       │              NETWORK (MQTT/HTTPS/CoAP)                        │
│       ▼                                                               │
│  ┌────────────────────────────────────────────────────────────────┐  │
│  │                    DEVICE AGENT (per device)                    │  │
│  │  [OTA Client] → [Verifier] → [Stager] → [Applier] → [Validator]│  │
│  │       │            │           │           │            │       │  │
│  │       ▼            ▼           ▼           ▼            ▼       │  │
│  │  Download      Verify      Write to    Swap active   Health     │  │
│  │  delta         signature   staging    model slot     check      │  │
│  │                                                                  │  │
│  │  [Rollback Manager] ──→ [Previous slot] ──→ [Auto-revert]      │  │
│  └────────────────────────────────────────────────────────────────┘  │
│                                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

### Delta Update Strategy (Bandwidth Optimization)

| Strategy | Size Reduction | Complexity | Use Case |
|----------|----------------|------------|----------|
| **Full replacement** | 0% | Low | Major version changes |
| **bsdiff (binary diff)** | 85-95% | Medium | Weight updates, fine-tunes |
| **Layer-wise delta** | 90-99% | High | Structured pruning, quantization changes |
| **Quantization delta** | 50-75% | Medium | FP32→INT8 conversion |

**bsdiff Example:**
```
Original model: 15.2 MB
Updated model:  15.4 MB (fine-tuned weights)
bsdiff delta:   0.8 MB  (95% reduction)
```

### OTA Rollout Strategies

```yaml
# rollout.yaml
strategy: canary
phases:
  - name: "internal"
    percentage: 5
    devices: ["device-group:internal-test"]
    duration: "2h"
    success_criteria:
      - inference_latency_p99 < 100ms
      - error_rate < 0.1%
      - memory_usage < 80%
      - no_crash_loop
  
  - name: "canary"
    percentage: 10
    devices: ["region:eu-west", "device-type:jetson-orin"]
    duration: "6h"
    success_criteria:
      - inference_latency_p99 < 100ms
      - accuracy_drift < 2%
      - power_draw < budget
  
  - name: "progressive"
    percentage: [25, 50, 75, 100]
    duration_per_phase: "4h"
    auto_promote: true
    rollback_on_failure: true
```

---

## Part 3: Edge Runtime & Execution Environment (25 minutes)

### Containerized Inference Runtime

**Why containers at the edge?**
- Dependency isolation (TFLite vs ONNX Runtime vs TensorRT)
- Resource limits (CPU, RAM, GPU memory)
- Security sandboxing
- Consistent deployment across heterogeneous fleet

```yaml
# docker-compose.edge.yml
version: '3.8'
services:
  inference:
    image: registry.example.com/edge-inference:tflite-2.14
    deploy:
      resources:
        limits:
          cpus: '2'
          memory: 1G
          reservations:
            devices:
              - driver: nvidia
                count: 1
                capabilities: [gpu]
    environment:
      - MODEL_PATH=/models/defect-detector-v3.tflite
      - DELEGATE=gpu
      - BATCH_SIZE=1
      - NUM_THREADS=4
    volumes:
      - /dev/video0:/dev/video0  # Camera access
      - model-store:/models
    restart: unless-stopped
    healthcheck:
      test: ["CMD", "grpc_health_probe", "-addr=:50051"]
      interval: 30s
      timeout: 10s
      retries: 3

  model-adapter:
    image: registry.example.com/model-adapter:v2
    depends_on:
      inference:
        condition: service_healthy
    environment:
      - SPLIT_MODE=auto
      - NETWORK_MONITOR_INTERVAL=5s

volumes:
  model-store:
    driver: local
```

### Lightweight Alternatives: WASM & Unikernels

| Runtime | Overhead | Isolation | Hardware Access | Best For |
|---------|----------|-----------|-----------------|----------|
| **Docker/containerd** | Medium | Strong | Full (GPU, devices) | General edge servers |
| **K3s (K8s)** | Low | Strong | Full | Gateway clusters |
| **WASM (wasmEdge, Wasmer)** | **Minimal** | Strong | Limited (WASI + extensions) | MCUs, constrained |
| **Unikernel (Unikraft)** | **Minimal** | Strong | Direct | Specialized appliances |
| **Bare metal / systemd** | None | None | Full | Single-purpose devices |

**WASM for Edge Inference:**
```rust
// wasm module for TFLite inference
#[wasm_bindgen]
pub fn infer(input: &[u8]) -> Vec<f32> {
    let model = Model::load_from_bytes(include_bytes!("model.tflite"))?;
    let mut interpreter = Interpreter::new(&model)?;
    interpreter.set_input(input)?;
    interpreter.invoke()?;
    interpreter.get_output()
}
```
- **Size**: ~2 MB (vs 100+ MB container)
- **Cold start**: < 10 ms
- **Runs on**: WASM runtime (wasmtime, wasmedge) on Linux/RTOS

---

## Part 4: Observability & Fleet Monitoring (20 minutes)

### The Four Golden Signals for Edge AI (Adapted from Google SRE)

| Signal | Edge AI Metric | Collection |
|--------|----------------|------------|
| **Latency** | `inference_latency_ms` (p50, p95, p99) | Per-request, pushed via MQTT |
| **Traffic** | `inference_requests_total`, `batch_size` | Counter, histogram |
| **Errors** | `inference_errors_total` (by type: OOM, timeout, delegate_fail) | Counter with labels |
| **Saturation** | `gpu_utilization`, `memory_usage_pct`, `thermal_throttling` | Gauge, 10s interval |

### Telemetry Pipeline

```
┌─────────────────────────────────────────────────────────────────┐
│                    TELEMETRY ARCHITECTURE                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  [Device]                                                         │
│     │                                                              │
│     ├──→ [Metrics] ──→ [Prometheus Pushgateway] ──→ [Prometheus] │
│     │       (10s)         (batch, retry)           (scrape)      │
│     │                                                              │
│     ├──→ [Logs] ──→ [Fluent Bit] ──→ [Loki / Elasticsearch]      │
│     │       (structured)   (tail, parse)        (query)          │
│     │                                                              │
│     ├──→ [Traces] ──→ [OpenTelemetry] ──→ [Jaeger / Tempo]       │
│     │       (W3C)          (SDK)           (distributed trace)   │
│     │                                                              │
│     └──→ [Health] ──→ [MQTT heartbeat] ──→ [Alertmanager]        │
│             (30s)         (topic: health)      (rules)           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Critical Edge AI Dashboards

**1. Fleet Health Overview**
- Devices online / offline / degraded
- Model version distribution (pie chart)
- Rollout progress bars

**2. Inference Performance**
- Latency heatmap by device type / region
- Throughput (inferences/sec) per device
- Delegate utilization (GPU vs CPU fallback)

**3. Resource Saturation**
- Memory pressure (OOM kills count)
- Thermal throttling events
- Power consumption trends

**4. Model Quality Drift**
- Confidence distribution shifts
- Prediction entropy over time
- Human-label feedback loop (when available)

### Alerting Rules (PrometheusRule)

```yaml
groups:
- name: edge-ai-alerts
  rules:
  - alert: DeviceOffline
    expr: time() - device_last_heartbeat > 120
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Device {{ $labels.device_id }} offline"
      
  - alert: InferenceLatencyHigh
    expr: histogram_quantile(0.99, rate(inference_latency_bucket[5m])) > 200
    for: 10m
    labels:
      severity: warning
    annotations:
      summary: "High latency on {{ $labels.device_type }}"
      
  - alert: ModelAccuracyDrift
    expr: avg(prediction_entropy) by (model_version) > 1.5
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "Accuracy drift detected for {{ $labels.model_version }}"
      
  - alert: ThermalThrottling
    expr: thermal_throttling_events_total > 0
    for: 1m
    labels:
      severity: critical
    annotations:
      summary: "Thermal throttling on {{ $labels.device_id }}"
```

---

## Part 5: Device Lifecycle Management (20 minutes)

### Device Provisioning & Onboarding

```
┌─────────────────────────────────────────────────────────────────┐
│                    DEVICE ONBOARDING FLOW                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  1. MANUFACTURING                                                │
│     ├─ Burn device certificate (X.509, TPM)                     │
│     ├─ Flash bootloader + base OS (Yocto / BalenaOS)            │
│     └─ Provision hardware ID → Device Registry                  │
│                                                                  │
│  2. FIRST BOOT (Zero-Touch Provisioning)                        │
│     ├─ DHCP + DNS → Discover provisioning server                │
│     ├─ Mutual TLS auth with device cert                         │
│     ├─ Pull device config (fleet, region, model assignments)    │
│     ├─ Install device agent (K3s agent / custom)                │
│     └─ Register with Fleet Orchestrator                         │
│                                                                  │
│  3. MODEL DEPLOYMENT                                             │
│     ├─ Orchestrator assigns models based on device caps         │
│     ├─ Device pulls model artifacts (delta if update)           │
│     ├─ Verify signature, stage, validate                        │
│     └─ Report "model_ready" to orchestrator                     │
│                                                                  │
│  4. ONGOING OPERATIONS                                           │
│     ├─ Heartbeat + telemetry (30s interval)                     │
│     ├─ OTA updates (model, OS, agent)                           │
│     ├─ Config changes (split point, thresholds)                 │
│     └─ Certificate rotation (90-day expiry)                     │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### Certificate Management at Scale

| Challenge | Solution |
|-----------|----------|
| **10,000+ device certs** | PKI with intermediate CAs per region/fleet |
| **Rotation without downtime** | Dual-certificate overlap (new + old valid) |
| **Revocation** | CRL/OCSP stapled to MQTT broker; short-lived certs (24h) + renewal |
| **Edge CA** | Local CA at gateway for leaf device certs (reduces cloud dependency) |

### Decommissioning & Secure Erasure

```bash
# Secure decommission workflow
1. Drain: Stop accepting inference requests
2. Flush: Complete in-flight requests, upload final telemetry
3. Revoke: Invalidate device certificate in PKI
4. Wipe: Cryptographic erase of model storage (AES-256 key destruction)
5. Unregister: Remove from fleet orchestrator, device registry
6. Physical: Factory reset for redeployment
```

---

## Part 6: Split Computing Operations (15 minutes)

### Runtime Split Point Adaptation

```python
# Dynamic split point selection based on runtime conditions
class AdaptiveSplitInference:
    def __init__(self, model_manifest):
        self.split_points = model_manifest['split_config']['split_points']
        self.activation_sizes = model_manifest['split_config']['activation_sizes_mb']
        self.current_split = 0  # Start conservative
        
    def select_split_point(self, network_monitor, device_metrics):
        """
        Choose optimal split point based on:
        - Current bandwidth (Mbps)
        - Latency budget (ms)
        - Edge device load (CPU/GPU %)
        - Server queue depth
        """
        bw_mbps = network_monitor.current_bandwidth_mbps()
        latency_budget = 100  # ms
        edge_load = device_metrics.gpu_utilization_pct
        
        for i, (split_layer, act_size_mb) in enumerate(
            zip(self.split_points, self.activation_sizes)
        ):
            # Estimate transfer time
            transfer_ms = (act_size_mb * 8) / bw_mbps
            
            # Estimate edge compute (increases with split layer)
            edge_compute_ms = self.estimate_edge_compute(i, edge_load)
            
            # Estimate server queue time
            server_queue_ms = network_monitor.server_queue_estimate()
            
            total_estimated = edge_compute_ms + transfer_ms + server_queue_ms
            
            if total_estimated < latency_budget * 0.8:
                return i
                
        return len(self.split_points) - 1  # Max offload
```

### Handling Network Partitions

| Scenario | Behavior |
|----------|----------|
| **Edge→Gateway link down** | Fall back to local-only inference (early exit / full model if cached) |
| **Gateway→Cloud link down** | Queue model updates locally; continue inference with current model |
| **Intermittent connectivity** | Exponential backoff for telemetry; prioritize model updates over logs |
| **High latency / packet loss** | Reduce split point (more local compute), increase compression |

**Graceful Degradation Policy:**
```yaml
degradation:
  level_0:  # Normal
    split_point: auto
    telemetry_interval: 30s
  level_1:  # High latency
    split_point: -1  # One step more local
    telemetry_interval: 60s
  level_2:  # Packet loss > 10%
    split_point: -2
    telemetry_interval: 120s
    compression: high
  level_3:  # Disconnected
    split_point: local_only
    telemetry_interval: 0  # Queue locally
    model_update: paused
```

---

## Part 7: Security & Compliance (15 minutes)

### Threat Model for Edge AI

| Attack Vector | Impact | Mitigation |
|---------------|--------|------------|
| **Model extraction** | IP theft, adversarial attacks | Encryption at rest, TPM-backed keys, obfuscation |
| **Adversarial input** | Wrong predictions | Input validation, detection, ensemble voting |
| **Model tampering (OTA)** | Malicious behavior | Signed artifacts, secure boot, measured boot |
| **Data exfiltration** | Privacy violation | Local processing, differential privacy, no raw data egress |
| **Side-channel (power/timing)** | Key extraction | Constant-time inference, noise injection |
| **Supply chain** | Compromised base image | SBOM, image signing, reproducible builds |

### Compliance at the Edge

| Regulation | Edge Requirement | Implementation |
|------------|------------------|----------------|
| **GDPR** | Data minimization, right to deletion | No PII in telemetry; local inference; secure erase on decommission |
| **HIPAA** | PHI protection | Encrypted model + data; audit logs; access controls |
| **IEC 62443** | Industrial cybersecurity | Zone/conduit model; secure comms; patch management |
| **NIS2** | Critical infrastructure resilience | HA edge pairs; automated failover; incident reporting |

---

## Summary

**Key Operational Takeaways:**

1. **Model as artifact** — Versioned, signed, platform-specific, with metadata
2. **Delta OTA** — 90%+ bandwidth reduction; staged rollouts with auto-rollback
3. **Container/WASM runtime** — Isolation, resource limits, hardware access
4. **Four golden signals** — Latency, traffic, errors, saturation per device
5. **Zero-touch provisioning** — Certificates, config, model assignment automated
6. **Adaptive split computing** — Runtime split point based on network/device conditions
7. **Graceful degradation** — Local-only fallback when cloud disconnected
8. **Security by design** — Signed models, encrypted storage, minimal attack surface

---

## Discussion Questions

- How do you test OTA updates across 50+ device hardware variants?
- What's the right telemetry granularity: per-inference vs. aggregated?
- How do you handle model rollback when the new model breaks only 1% of devices?
- When does WASM make sense vs. containers for edge inference?

---

## Further Reading (Systems/Operations Focus)

- **MLSysBook Vol 2**: [Fleet Orchestration](https://mlsysbook.ai/vol2/contents/vol2/fleet_orchestration/fleet_orchestration.html), [Inference at Scale](https://mlsysbook.ai/vol2/contents/vol2/inference_at_scale/inference_at_scale.html), [Performance Engineering](https://mlsysbook.ai/vol2/contents/vol2/performance_engineering/performance_engineering.html)
- **KubeEdge**: Kubernetes-native edge orchestration — https://kubeedge.io/
- **K3s**: Lightweight Kubernetes for edge — https://k3s.io/
- **EdgeX Foundry**: Vendor-neutral IoT edge framework — https://www.edgexfoundry.org/
- **OpenTelemetry for Edge**: https://opentelemetry.io/docs/instrumentation/
- **TUF / in-toto**: Secure software supply chain — https://theupdateframework.io/