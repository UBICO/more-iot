# IoT Edge Architecture & Distributed Intelligence

## Lecture Overview

This lecture examines Edge AI from an **IoT Systems Engineering** perspective — focusing on distributed system architecture, networked device fleets, split computing, and the infrastructure that enables intelligence at the edge. We move beyond single-device ML optimization to understand how IoT systems coordinate computation across heterogeneous edge nodes, gateways, and cloud.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with IoT/distributed systems background  
**Reference:** *Machine Learning Systems at Scale* (MLSysBook Vol 2) — https://mlsysbook.ai/vol2/

---

## Part 1: The Edge as a Distributed System (25 minutes)

### From Centralized Factory to Decentralized Network

**Traditional ML (Centralized):**
```
[Data Collection] → [Cloud Training] → [Cloud Inference] → [Results]
         ↓               ↓                  ↓
    IoT Devices    GPU Cluster         API Service
```

**Edge Intelligence (Decentralized - MLSysBook Vol 2):**
```
┌─────────────────────────────────────────────────────────────┐
│                    EDGE FLEET                                │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Sensor  │  │ Gateway │  │ Edge    │  │ Cloud   │        │
│  │ Node    │──│ / Edge  │──│ Server  │──│ (Train) │        │
│  │ (MCU)   │  │ (SBC)   │  │ (GPU)   │  │         │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
│       │            │            │            │              │
│       └────────────┴────────────┴────────────┘              │
│                    FEDERATED COORDINATION                    │
└─────────────────────────────────────────────────────────────┘
```

**Key Shift (MLSysBook):** ML moves from a *centralized factory model* to a *decentralized, continuously adapting network*.

### The Fleet Stack (Bottom-Up)

From MLSysBook Vol 2 Part I — The Fleet:

| Layer | Components | Role in Edge AI |
|-------|------------|-----------------|
| **Silicon** | MCU, NPU, GPU, CPU | Heterogeneous compute |
| **Node** | Raspberry Pi, Jetson, Industrial PC | Single device runtime |
| **Rack/Cluster** | Edge server racks, micro-datacenters | Local aggregation |
| **Fabric** | WiFi, 5G, LoRaWAN, Ethernet, TSN | Inter-node communication |
| **Orchestration** | K3s, KubeEdge, EdgeX, Balena | Fleet management |

---

## Part 2: Split Computing & Model Partitioning (30 minutes)

### Why Split Computing?

**Constraints driving split inference:**
| Constraint | Cloud-Only | Edge-Only | Split Computing |
|------------|------------|-----------|-----------------|
| **Model Size** | Unlimited | < Device Memory | Partitioned |
| **Latency** | High (RTT) | Low | Early exit / partial |
| **Privacy** | Data leaves | Data stays | Raw data local |
| **Bandwidth** | High | Zero | Activations only |
| **Energy** | N/A | Battery-limited | Offload heavy layers |

### Partitioning Strategies

#### 1. Early Exit (BranchyNet)
```
Input → [Layer 1] → [Layer 2] → [Exit 1?] → [Layer 3] → [Exit 2?] → [Layer 4] → Output
              ↑                              ↑
         Easy samples                  Hard samples
         (low latency)                 (high accuracy)
```

**IoT Application**: Keyword spotting on MCU → exit early for confident predictions; offload uncertain to gateway.

#### 2. Layer-wise Partition (Split Point)
```
┌─────────────────────┐     Activations      ┌─────────────────────┐
│      EDGE DEVICE    │ ──────────────────→  │     EDGE SERVER     │
│  [Layers 1...k]     │   (tensor, < 1MB)    │  [Layers k+1...N]   │
│                     │ ←──────────────────  │                     │
│  - Feature extract  │   Gradients /        │  - Classification   │
│  - Privacy preserve │   Results            │  - Heavy compute    │
└─────────────────────┘                      └─────────────────────┘
```

**Split Point Selection Criteria (MLSysBook):**
- **Communication volume**: Activation tensor size at split
- **Compute balance**: O_edge vs O_server
- **Privacy boundary**: Raw data never leaves edge
- **Network reliability**: Can tolerate intermittent connectivity

#### 3. Collaborative Inference (Multi-Device)
```
[Camera 1] ──→ [Edge Gateway] ←── [Camera 2]
                │
                ↓
         [Fused Inference]
                │
                ↓
         [Decision/Action]
```

**IoT Example**: Multi-camera object tracking in warehouse — each camera runs detection, gateway fuses tracks.

### Quantifying Split Computing Trade-offs

**Iron Law for Split Inference:**
```
T_total = T_edge_compute + T_transfer + T_server_compute + T_return

T_edge_compute    = O_edge / (R_edge · η_edge)
T_transfer        = Activation_Size / BW_network
T_server_compute  = O_server / (R_server · η_server)
```

**Optimization Problem:**
```
minimize T_total
subject to:
  - Activation_Size ≤ BW_network × Latency_Budget
  - Edge_Memory_Peak ≤ Device_RAM
  - Privacy: Raw_Data never transmitted
```

---

## Part 3: Edge Fleet Architecture & Device Hierarchy (20 minutes)

### Device Tier Classification (MLSysBook Vol 2)

| Tier | Device Class | Compute | Memory | Power | Typical Role |
|------|--------------|---------|--------|-------|--------------|
| **Tier 1** | Microcontroller (ESP32, STM32, RP2040) | ~0.03 TOPS | 256 KB - 2 MB SRAM | 10-100 mW | Sensor preprocessing, keyword spot, simple anomaly |
| **Tier 2** | SBC / Edge Gateway (Pi 4/5, Jetson Nano, Industrial) | 0.5-10 TOPS | 1-8 GB | 2-15 W | Model partition head, sensor fusion, protocol translation |
| **Tier 3** | Edge Server (Jetson AGX, x86+GPU, Micro-DC) | 20-200 TOPS | 16-128 GB | 30-300 W | Model partition tail, multi-stream, training coordination |
| **Tier 4** | Cloud / Regional DC | Unlimited | Unlimited | Grid | Full training, global model registry, fleet orchestration |

### IoT Protocol Stack for Edge AI

```
┌────────────────────────────────────────────────────────────┐
│                    APPLICATION LAYER                        │
│  MQTT Topics: inference/request, inference/response,       │
│  model/update, telemetry/metrics, health/status            │
├────────────────────────────────────────────────────────────┤
│                    TRANSPORT LAYER                          │
│  MQTT over TLS/TCP, CoAP over DTLS/UDP, gRPC, WebSocket    │
├────────────────────────────────────────────────────────────┤
│                    NETWORK LAYER                            │
│  WiFi (802.11n/ac/ax), 5G NR, LoRaWAN, Ethernet, TSN       │
├────────────────────────────────────────────────────────────┤
│                    LINK / PHYSICAL                          │
│  IEEE 802.15.4, Bluetooth LE, Matter/Thread, CAN bus       │
└────────────────────────────────────────────────────────────┘
```

**Edge AI Payload Considerations:**
- **Activation tensors**: Binary (protobuf/flatbuffers), not JSON
- **Compression**: Quantized INT8 activations + entropy coding
- **QoS**: MQTT QoS 1 for inference requests, QoS 0 for telemetry
- **Topic design**: `edge/{device_id}/inference/{model_id}/{split_point}`

---

## Part 4: Fleet Management & Orchestration (20 minutes)

### The Edge Fleet Control Plane

From MLSysBook Vol 2: **Fleet Orchestration** coordinates thousands of heterogeneous devices.

```
┌─────────────────────────────────────────────────────────────┐
│                    CLOUD CONTROL PLANE                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │ Model       │  │ Fleet       │  │ Monitoring &        │  │
│  │ Registry    │  │ Orchestrator│  │ Observability       │  │
│  │ (versions,  │  │ (KubeEdge,  │  │ (Prometheus,        │  │
│  │  artifacts) │  │  K3s,       │  │  Grafana, Loki)     │  │
│  └──────┬──────┘  └──────┬──────┘  └──────────┬──────────┘  │
└─────────┼────────────────┼─────────────────────┼─────────────┘
          │                │                     │
          ▼                ▼                     ▼
┌─────────────────────────────────────────────────────────────┐
│                    EDGE DATA PLANE                          │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐        │
│  │ Device  │  │ Device  │  │ Device  │  │ ...     │        │
│  │ Agent   │  │ Agent   │  │ Agent   │  │         │        │
│  │ (OTA,   │  │ (OTA,   │  │ (OTA,   │  │         │        │
│  │  health,│  │  health,│  │  health,│  │         │        │
│  │  exec)  │  │  exec)  │  │  exec)  │  │         │        │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────┬────┘        │
└───────┼────────────┼────────────┼─────────────┼─────────────┘
        │            │            │             │
        ▼            ▼            ▼             ▼
   ┌─────────────────────────────────────────────┐
   │          EDGE WORKLOADS (Pods/Containers)   │
   │  ┌────────┐ ┌────────┐ ┌────────┐          │
   │  │Inference│ │Data    │ │Model   │  ...    │
   │  │Runtime │ │Ingest  │ │Adapter │         │
   │  └────────┘ └────────┘ └────────┘          │
   └─────────────────────────────────────────────┘
```

### Device Agent Responsibilities (Edge Side)

| Function | Implementation |
|----------|----------------|
| **Model OTA** | Delta updates (bsdiff), signature verification, rollback |
| **Health Monitoring** | Heartbeat, resource usage (CPU, RAM, GPU, temp, power) |
| **Workload Execution** | Container runtime (containerd, K3s), WASM for isolation |
| **Telemetry** | Structured logs, metrics push (Prometheus pushgateway) |
| **Local Autonomy** | Cache model, queue requests during disconnect, FIFO retry |

### Fleet Orchestration Patterns

| Pattern | Use Case | Tooling |
|---------|----------|---------|
| **GitOps** | Declarative model/desired state in Git | Flux, ArgoCD on K3s |
| **Blue/Green** | Zero-downtime model rollout | Dual model slots, traffic switch |
| **Canary** | Risk mitigation for new models | 5% → 25% → 100% rollout |
| **Shadow** | Validate without affecting production | Mirror traffic to new model |

---

## Part 5: Data Engineering for Edge Fleets (15 minutes)

### The Data Path: Sensor → Inference → Action

```
[Sensor] → [Preprocess] → [Queue/Buffer] → [Inference] → [Postprocess] → [Actuate/Alert]
   │           │              │              │              │               │
   │           │              │              │              │               │
   ▼           ▼              ▼              ▼              ▼               ▼
Raw Data  Normalized    Time-series    Tensor          Decision       MQTT/HTTP/
(MQTT/     (scale,       buffer with    (INT8,          logic,       CoAP to
CoAP)      crop,         backpressure   FP16)           confidence   Cloud/SCADA
          augment)       handling
```

### Edge Data Challenges (MLSysBook D·A·M)

| Challenge | Data (D) Axis Impact | System Solution |
|-----------|---------------------|-----------------|
| **Intermittent connectivity** | Dvol/BW spikes on reconnect | Local buffering, store-and-forward |
| **Heterogeneous sensors** | Schema mismatch | Protocol translation at gateway (EdgeX) |
| **Time synchronization** | Distributed trace corruption | PTP/NTP, logical clocks for ordering |
| **Data gravity** | Cost to move vs compute | Compute at source, move results |
| **Privacy/Compliance** | Data residency | Local processing, federated learning |

### Time-Series Data at the Edge

**InfluxDB / TimescaleDB / VictoriaMetrics** on edge gateway:
- **Retention policies**: Raw (7d) → Aggregated (1y)
- **Downsampling**: 1Hz → 1min avg, max, min, stddev
- **Continuous queries**: Pre-compute features for ML

---

## Part 6: Network Fabric for Distributed Edge AI (10 minutes)

### Connectivity Requirements by Tier

| Link | Bandwidth | Latency | Reliability | Use Case |
|------|-----------|---------|-------------|----------|
| **Sensor → Gateway** | 100 Kbps - 10 Mbps | < 10 ms | Best-effort | MQTT/CoAP telemetry |
| **Gateway ↔ Edge Server** | 100 Mbps - 10 Gbps | < 5 ms | High (TSN) | Split computing activations |
| **Edge Server ↔ Cloud** | 100 Mbps - 10 Gbps | 10-100 ms | Standard | Model updates, fleet mgmt |

### Network-Aware Split Computing

**Adaptive split point based on network conditions:**
```python
def select_split_point(network_bw_mbps, latency_budget_ms):
    """
    Choose split layer based on current network.
    Returns layer index where activation fits in budget.
    """
    activation_sizes = get_activation_sizes_per_layer()  # MB per layer
    compute_times = get_compute_time_per_layer()  # ms per layer
    
    for i, (act_size, comp_time) in enumerate(zip(activation_sizes, compute_times)):
        transfer_time = (act_size * 8) / network_bw_mbps  # ms
        total_edge_time = sum(compute_times[:i+1])
        if total_edge_time + transfer_time < latency_budget_ms * 0.7:
            return i
    return 0  # Fallback: all on edge
```

---

## Summary

**Key IoT Systems Takeaways:**

1. **Edge is a distributed system** — not a single device; design for fleet heterogeneity
2. **Split computing** — partition models across tiers based on compute/memory/bandwidth
3. **Fleet orchestration** — GitOps, OTA, health monitoring, canary deployments
4. **Protocol stack** — MQTT/CoAP for telemetry, binary protocols for activations
5. **Data gravity** — compute at source, move only results; local buffering for resilience
6. **Network-aware adaptation** — dynamic split points based on real-time link quality

---

## Discussion Questions

- How do you handle model version skew across a fleet of 10,000 heterogeneous devices?
- What happens when the edge-server link fails during split inference?
- How do you synchronize time across devices for distributed trace correlation?
- When does federated learning make sense vs. cloud retraining + OTA?

---

## Further Reading (IoT Systems Focus)

- **MLSysBook Vol 2**: [Edge Intelligence](https://mlsysbook.ai/vol2/contents/vol2/edge_intelligence/edge_intelligence.html), [Compute Infrastructure](https://mlsysbook.ai/vol2/contents/vol2/compute_infrastructure/compute_infrastructure.html), [Fleet Orchestration](https://mlsysbook.ai/vol2/contents/vol2/fleet_orchestration/fleet_orchestration.html)
- **EdgeX Foundry**: Industrial IoT edge framework — https://www.edgexfoundry.org/
- **KubeEdge / K3s**: Kubernetes at the edge — https://kubeedge.io/, https://k3s.io/
- **Eclipse ioFog**: Edge compute platform — https://iofog.org/
- **Split Computing Surveys**: "Split Learning over Edge Networks" (arXiv:2507.01041)