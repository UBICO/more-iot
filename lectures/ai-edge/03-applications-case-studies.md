# IoT Edge AI Case Studies: System Analysis & Architecture

## Lecture Overview

This lecture analyzes real-world IoT Edge AI deployments through a **systems engineering lens**. Each case study examines the distributed architecture, network topology, data flows, fleet operations, and failure modes — using the **D·A·M (Data·Algorithm·Machine)** framework and **Iron Law** from MLSysBook to diagnose system behavior.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with IoT/distributed systems background  
**Reference:** *Machine Learning Systems at Scale* (MLSysBook Vol 2) — https://mlsysbook.ai/vol2/

---

## Part 1: System Analysis Framework for Edge AI (15 minutes)

### D·A·M Diagnostic Lens for Distributed Edge Systems

| Axis | Questions for Case Study Analysis |
|------|-----------------------------------|
| **Data (D)** | What data rates? Bandwidth per link? Intermittent connectivity? Schema evolution? Time sync? Privacy boundaries? |
| **Algorithm (A)** | Model size/complexity per tier? Split points? Adaptation strategy? Quantization? Ensemble vs single? |
| **Machine (M)** | Device heterogeneity? Compute per tier (TOPS)? Memory hierarchy? Thermal envelope? Power budget? Accelerator utilization? |

### Iron Law for Distributed Inference

```
T_end_to_end = max(
    T_edge_compute + T_transfer + T_server_compute,  # Split path
    T_local_only                                     # Fallback path
) + T_orchestration_overhead

Where:
  T_edge_compute    = O_edge / (R_edge · η_edge)
  T_transfer        = Activation_Size / BW_available
  T_server_compute  = O_server / (R_server · η_server)
  T_local_only      = O_full / (R_edge · η_edge)
```

### Key System Metrics for Evaluation

| Metric | Target | Measurement |
|--------|--------|-------------|
| **End-to-end latency (p99)** | < SLA | Distributed trace |
| **Availability** | 99.9%+ | Heartbeat + inference success rate |
| **Bandwidth efficiency** | Activations << Raw data | Network monitor |
| **Fleet homogeneity** | < 5% version skew | Orchestrator |
| **Rollout safety** | Zero-downtime, auto-rollback | Canary metrics |
| **Degradation gracefulness** | Local-only < 2× latency | Chaos testing |

---

## Part 2: Industrial IoT — Predictive Maintenance Fleet (30 minutes)

### System Context

**Deployment**: 500+ pump monitoring nodes across 12 oil refineries  
**Goal**: Detect bearing failures 24-48 hours before catastrophic failure

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         REFINERY EDGE FLEET                                  │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│   ┌─────────────────────────────────────────────────────────────────────┐   │
│   │                    REFINERY 1 (×12)                                  │   │
│   │  ┌──────────┐    ┌──────────┐    ┌──────────┐    ┌──────────────┐  │   │
│   │  │ Pump 1   │    │ Pump 2   │    │ Pump 3   │    │  Edge        │  │   │
│   │  │ [MEMS200│  [MEMS×4] │  [MEMS×4] │  [MEMS×4] │───→│  Gateway     │  │   │
│   │  +MCU  │    +MCU    │    +MCU    │    │  (Jetson    │  │   │
│   │        │    │        │    │        │    │   Nano)    │  │   │
│   │        │    │        │    │        │    │            │  │   │
│   │  In-   │    In-     │    In-     │    │  - Fusion  │  │   │
│   │  ference   ference    ference    │    │  - Buffer  │  │   │
│   │        │    │        │    │        │    │  - OTA     │  │   │
│   └────┬───┘    └────┬───┘    └────┬───┘    └──────┬─────┘  │   │
│        │             │             │             │         │   │
│        └─────────────┴─────────────┴─────────────┘         │   │
│                          │                                   │   │
│                          ▼                                   │   │
│                 ┌─────────────────┐                          │   │
│                 │  Site Server    │                          │   │
│                 │  (x86 + GPU)    │                          │   │
│                 │  - Training     │                          │   │
│                 │  - Aggregation  │                          │   │
│                 │  - Model Registry                           │   │
│                 └────────┬────────┘                          │   │
│                          │                                   │   │
│        ┌─────────────────┼─────────────────┐                 │   │
│        ▼                 ▼                 ▼                 │   │
│   ┌─────────┐       ┌─────────┐       ┌─────────┐          │   │
│   │ Cloud   │       │ Fleet   │       │ Alert   │          │   │
│   │ Train   │       │ Ops     │       │ System  │          │   │
│   └─────────┘       └─────────┘       └─────────┘          │   │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### D·A·M Analysis

#### Data (D) Axis

| Aspect | Specification |
|--------|---------------|
| **Sensor data rate** | 4 MEMS mics × 16 kHz × 2 bytes = 128 KB/s per pump |
| **Local buffering** | 5-second windows, 90% overlap → 80 windows/sec |
| **Upstream bandwidth** | Gateway → Site: 500 pumps × 50 KB/hr = 25 MB/hr (metadata only) |
| **Intermittency handling** | 24-hr local buffer on gateway (SD card, 32 GB) |
| **Time sync** | PTP (IEEE 1588) across refinery LAN; ±1 ms accuracy |
| **Privacy** | No raw audio leaves pump node; only anomaly scores + metadata |

#### Algorithm (A) Axis

| Tier | Model | Size | Split Strategy |
|------|-------|------|----------------|
| **Pump MCU (Tier 1)** | DS-CNN autoencoder (INT8) | 180 KB | Full local inference |
| **Gateway (Tier 2)** | LSTM classifier (INT8) | 2.1 MB | Receives features from 50 pumps |
| **Site Server (Tier 3)** | Transformer encoder (FP16) | 45 MB | Receives anomalies, re-trains weekly |

**Split Computing**: Pump MCU extracts 40-dim MFCC features → Gateway runs LSTM → Server correlates across pumps

#### Machine (M) Axis

| Device | Compute | Memory | Power | Utilization |
|--------|---------|--------|-------|-------------|
| **STM32H7 (MCU)** | 480 MHz Cortex-M7 | 1 MB SRAM | 200 mW | 15% (DSP) |
| **Jetson Nano (Gateway)** | 128-core Maxwell | 4 GB | 10 W | 40% (GPU) |
| **Dell R650 (Site)** | A100 40GB | 256 GB | 300 W | 60% (GPU) |

### Operational Characteristics

| Operational Aspect | Implementation |
|-------------------|----------------|
| **Model OTA** | Weekly retraining → delta update (bsdiff, 95% reduction) |
| **Rollout** | Canary: 1 refinery → 3 → all 12 (48 hrs total) |
| **Monitoring** | Per-pump: inference latency, anomaly score dist, MCU temp |
| **Failure domain** | Single pump failure → isolated; Gateway failure → 50 pumps buffered locally |
| **Degradation** | Cloud link down → Site server continues; Gateway down → MCU buffers 24 hrs |

### Key System Lessons

1. **Hierarchical buffering** absorbs connectivity blips without data loss
2. **Feature extraction at source** reduces bandwidth 1000× vs raw audio
3. **Tiered models** match compute to tier capabilities (MCU → Gateway → Server)
4. **Local autonomy** critical for safety-critical industrial environments

---

## Part 3: Smart City — Distributed Traffic Intelligence (25 minutes)

### System Context

**Deployment**: 2,000 smart cameras across metropolitan area  
**Goal**: Real-time traffic flow optimization, incident detection, privacy-preserving

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      CITY-WIDE TRAFFIC FLEET                                 │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  ZONE NORTH          ZONE SOUTH          ZONE EAST          ZONE WEST        │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐    ┌─────────────┐  │
│  │ Intersection│    │ Intersection│    │ Intersection│    │ Intersection│  │
│  │  Cam 1..50  │    │  Cam 51..100│    │  Cam 101..  │    │  Cam ...    │  │
│  │  (Tier 2)   │    │  (Tier 2)   │    │  (Tier 2)   │    │  (Tier 2)   │  │
│  └──────┬──────┘    └──────┬──────┘    └──────┬──────┘    └──────┬──────┘  │
│         │                 │                 │                 │          │
│         ▼                 ▼                 ▼                 ▼          │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │                    ZONE EDGE SERVER (×4)                            │ │
│  │  - Aggregates 50 cameras                                            │ │
│  │  - Regional traffic model (split: camera→server)                   │ │
│  │  - Coordinates signal timing                                        │ │
│  └──────────────────────┬──────────────────────────────────────────────┘ │
│                         │                                                │
│         ┌───────────────┼───────────────┐                                │
│         ▼               ▼               ▼                                │
│  ┌─────────────────────────────────────────────────────────────────────┐ │
│  │                   CITY OPERATIONS CENTER                            │ │
│  │  - Global traffic model (cloud)                                    │ │
│  │  - Fleet orchestration (KubeEdge)                                  │ │
│  │  - Incident management                                             │ │
│  └─────────────────────────────────────────────────────────────────────┘ │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### D·A·M Analysis

#### Data (D) Axis

| Aspect | Specification |
|--------|---------------|
| **Camera stream** | 1080p30 H.264 = 8 Mbps raw per camera |
| **Edge processing** | Camera runs YOLOv5s-INT8 (14 MB) → outputs tracks (JSON, 5 KB/s) |
| **Zone aggregation** | 50 cameras × 5 KB/s = 250 KB/s per zone server |
| **City-wide** | 4 zones × 250 KB/s = 1 MB/s to cloud (vs 16 Gbps raw) |
| **Split computing** | Camera: detection → Server: tracking + prediction |
| **Privacy** | **No video leaves camera**; faces/blates blurred on-device |

#### Algorithm (A) Axis

| Tier | Model | Split Point | Output |
|------|-------|-------------|--------|
| **Camera (Tier 2)** | YOLOv5s-INT8 (detection) | Layer 12/24 | Bounding boxes + features |
| **Zone Server (Tier 3)** | DeepSORT + LSTM (FP16) | Receives features | Tracks, trajectories, predictions |
| **Cloud (Tier 4)** | Graph Transformer (FP16) | Receives zone tracks | City-wide flow optimization |

**Adaptive Split Point**: Cameras adjust based on 5G/WiFi link quality
- Good link: Send features at layer 12 (0.8 MB activation)
- Poor link: Send features at layer 18 (2.1 MB) — more local compute
- No link: Local tracking only (Kalman filter), sync on reconnect

#### Machine (M) Axis

| Device | Specs | Deployment |
|--------|-------|------------|
| **Smart Camera** | Ambarella CV22 (1.5 TOPS), 2 GB | Pole-mounted, PoE+, IP67 |
| **Zone Server** | 2× Jetson AGX Orin (200 TOPS), 64 GB | Street cabinet, ruggedized |
| **Cloud** | GPU cluster (A100×8) | Regional DC |

### Network Fabric

| Link | Technology | Bandwidth | Latency | Redundancy |
|------|------------|-----------|---------|------------|
| Camera → Zone | 5G NR / WiFi 6 | 100-500 Mbps | < 20 ms | Dual SIM, failover |
| Zone → Cloud | Dedicated fiber | 10 Gbps | < 5 ms | Dual path |
| Inter-zone | VXLAN overlay | 1 Gbps | < 10 ms | Mesh |

### Key System Lessons

1. **Privacy-by-architecture**: Video never leaves camera — GDPR compliance baked into data flow
2. **Adaptive split computing** handles variable 5G/WiFi quality in urban canyons
3. **Hierarchical federation**: Zone servers coordinate locally; cloud optimizes globally
4. **Multi-homed connectivity** essential for city-scale reliability

---

## Part 4: Agricultural IoT — Autonomous Precision Spraying (20 minutes)

### System Context

**Deployment**: 50 autonomous sprayer robots across 5,000 hectares  
**Goal**: Real-time weed detection → targeted herbicide application (90% reduction)

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      AUTONOMOUS SPRAYER FLEET                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  FIELD CLUSTER (×10 per farm, ×5 farms)                                     │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  SPRAYER 1          SPRAYER 2          SPRAYER 3          ...        │  │
│  │  ┌───────────┐      ┌───────────┐      ┌───────────┐                 │  │
│  │  │ Multispec │      │ Multispec │      │ Multispec │                 │  │
│  │  │ Camera    │      │ Camera    │      │ Camera    │                 │  │
│  │  │ (4-band)  │      │ (4-band)  │      │ (4-band)  │                 │  │
│  │  └─────┬─────┘      └─────┬─────┘      └─────┬─────┘                 │  │
│  │        │                  │                  │                        │  │
│  │        ▼                  ▼                  ▼                        │  │
│  │  ┌────────────────────────────────────────────────────────────────┐   │  │
│  │  │              ON-BOARD JETSON AGX ORIN (Tier 3)                  │   │  │
│  │  │  - Weed/Crop classification (EfficientNet-B1 INT8, 1.2 GOPS)  │   │  │
│  │  │  - Real-time: 20 plants/sec, 1mm localization                  │   │  │
│  │  │  - Spray control (PWM, 100 Hz)                                 │   │  │
│  │  │  - SLAM + path planning                                        │   │  │
│  │  └────────────────────────────────────────────────────────────────┘   │  │
│  │        │                  │                  │                        │  │
│  └────────┼──────────────────┼──────────────────┼────────────────────────┘  │
│           │                  │                  │                           │
│           ▼                  ▼                  ▼                           │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                    FARM EDGE SERVER (Tier 3)                          │  │
│  │  - Model aggregation (federated averaging)                           │  │
│  │  - Field mapping + prescription generation                           │  │
│  │  - Fleet coordination (mission planning)                             │  │
│  └────────────────────────────┬──────────────────────────────────────────┘  │
│                               │                                            │
│                               ▼                                            │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                      CLOUD (Seasonal retraining)                     │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### D·A·M Analysis

#### Data (D) Axis

| Aspect | Specification |
|--------|---------------|
| **Camera data** | 4-band × 512×512 × 20 Hz = 20 MB/s per sprayer |
| **On-board processing** | Full inference on Jetson AGX (no split — latency critical) |
| **Upstream** | Detection results only: 50 KB/s per sprayer |
| **Farm server** | Aggregates 10 sprayers = 500 KB/s |
| **Connectivity** | LoRaWAN (telemetry) + 4G/LTE (model updates) — intermittent in fields |
| **Offline operation** | 8-hr autonomy; models cached; sync on return to barn |

#### Algorithm (A) Axis

| Component | Model | Optimization |
|-----------|-------|--------------|
| **Weed detection** | EfficientNet-B1 INT8 | TensorRT, batch=1, FP16 accumulation |
| **Spray control** | PID + MPC | 100 Hz loop, < 5 ms latency |
| **Federated learning** | FedAvg (weekly) | Local epochs=5, 10% devices/sampled |

**Model Adaptation**: Seasonal retraining (cloud) → OTA to farm server → federated to sprayers

#### Machine (M) Axis

| Device | Compute | Thermal | Power | Duty Cycle |
|--------|---------|---------|-------|------------|
| **Sprayer (Jetson AGX Orin)** | 200 TOPS INT8 | Active cooling | 60 W | 8 hrs/day |
| **Farm Server (x86+RTX)** | 50 TOPS | Forced air | 200 W | 24/7 |
| **Cloud** | A100 cluster | DC cooling | Grid | Batch |

### Key System Lessons

1. **No split computing on sprayer** — 50 ms latency budget demands full local inference
2. **Federated learning** adapts to field-specific weed phenotypes without raw data upload
3. **LoRaWAN + 4G hybrid** handles rural connectivity; store-and-forward for model updates
4. **Deterministic real-time** — spray control on dedicated CPU core, isolated from ML

---

## Part 5: Healthcare IoT — Continuous Glucose Monitoring Network (20 minutes)

### System Context

**Deployment**: 10,000+ patients with CGM + insulin pump + phone gateway  
**Goal**: Hypoglycemia prediction 30 min ahead; automated insulin suspension

### Architecture

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                       PATIENT EDGE NETWORK                                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  PATIENT (×10,000+)                                                         │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │  [CGM Sensor] ←──BLE──→ [Phone App (Tier 2)] ←──BLE──→ [Insulin Pump] │  │
│  │     │                    │                     │                      │  │
│  │     ▼                    ▼                     ▼                      │  │
│  │  Glucose          ConvLSTM INT8          Safety                    │  │
│  │  5-min            (1D, 3-layer,         Monitor                     │  │
│  │  samples          32 units)             - Rate limit            │  │
│  │                   - 30-min pred          - Max bolus             │  │
│  │                   - Confidence            - Override             │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                            │                                                │
│                            ▼ (HTTPS, cert-pinned)                           │
│  ┌──────────────────────────────────────────────────────────────────────┐  │
│  │                      CLOUD BACKEND (Tier 4)                          │  │
│  │  - Population model (Transformer, weekly retrain)                   │  │
│  │  - Clinician dashboard                                              │  │
│  │  - Regulatory audit trail (FDA 21 CFR Part 11)                     │  │
│  │  - OTA model updates (signed, staged rollout)                      │  │
│  └──────────────────────────────────────────────────────────────────────┘  │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### D·A·M Analysis

#### Data (D) Axis

| Aspect | Specification |
|--------|---------------|
| **CGM data rate** | 1 sample / 5 min = 288 samples/day |
| **Phone inference** | Runs every 5 min; < 2 ms on NPU |
| **Upstream** | Prediction + glucose + meta = 2 KB / 5 min = 576 KB/day |
| **Privacy** | **HIPAA/GDPR**: No PHI in telemetry; local inference only |
| **Regulatory** | FDA Class II — model changes require clinical validation |

#### Algorithm (A) Axis

| Tier | Model | Constraint |
|------|-------|------------|
| **Phone (Tier 2)** | ConvLSTM INT8 (1D, 3-layer, 32 units) | < 5 MB, < 10 ms, 5-year battery |
| **Cloud (Tier 4)** | Transformer (population) | Weekly retrain, clinical validation |

**No split computing** — phone runs full model; cloud only for population retraining

#### Machine (M) Axis

| Device | Compute | Battery Life | Regulatory |
|--------|---------|--------------|------------|
| **Phone (iOS/Android)** | Apple NE / Qualcomm Hexagon | 24+ hrs with app | App Store review |
| **CGM Sensor** | Custom ASIC | 10-14 days | FDA cleared |
| **Insulin Pump** | MCU (Cortex-M) | 7 days | FDA Class III |

### Key System Lessons

1. **Regulatory constraints dominate** — model changes = clinical trial; OTA only for validated versions
2. **Battery life > accuracy** — INT8 quantization mandatory; NPU usage optimized
3. **Safety monitor** — independent MCU on pump enforces hard limits regardless of ML
4. **Patient phone as gateway** — leverages existing compute, connectivity, UI

---

## Part 6: Comparative System Analysis (15 minutes)

### Cross-Case D·A·M Comparison

| Dimension | Industrial Predictive Maint | Smart City Traffic | Ag Sprayer Fleet | Healthcare CGM |
|-----------|----------------------------|-------------------|------------------|----------------|
| **Scale** | 500 nodes, 12 sites | 2,000 cameras, 4 zones | 50 robots, 5 farms | 10,000 patients |
| **Topology** | Hierarchical (3-tier) | Hierarchical (3-tier) | Flat + farm server | Star (phone gateway) |
| **Split Computing** | Yes (MCU→Gateway→Server) | Yes (adaptive, Camera↔Zone) | No (full on Orin) | No (full on phone) |
| **Latency Budget** | 100 ms (anomaly score) | 200 ms (signal timing) | 50 ms (spray control) | 30 min (prediction) |
| **Connectivity** | Wired (refinery LAN) | 5G/WiFi (variable) | LoRaWAN + 4G (rural) | BLE + HTTPS (phone) |
| **Model Update Freq** | Weekly | Daily | Seasonal | Monthly (validated) |
| **Regulatory** | IEC 62443 | GDPR (privacy) | None | FDA Class II, HIPAA |
| **Offline Tolerance** | 24 hrs (buffered) | 1 hr (local track) | 8 hrs (full autonomy) | Continuous (local) |

### Iron Law Comparison (Typical Inference Path)

| Case | T_edge | T_transfer | T_server | T_total | Bottleneck |
|------|--------|------------|----------|---------|------------|
| **Industrial** | 15 ms | 5 ms | 20 ms | 40 ms | Server queue |
| **Traffic** | 30 ms | 15 ms | 50 ms | 95 ms | Network (5G var) |
| **Sprayer** | 40 ms | N/A | N/A | 40 ms | Compute (Orin) |
| **CGM** | 2 ms | N/A | N/A | 2 ms | NPU efficiency |

### Fleet Operations Maturity Model

| Capability | Industrial | Traffic | Sprayer | Healthcare |
|------------|------------|---------|---------|------------|
| **Zero-touch provisioning** | ✅ | ✅ | ⚠️ (manual) | ✅ |
| **Delta OTA + rollback** | ✅ | ✅ | ✅ | ✅ (staged) |
| **Canary rollout** | ✅ | ✅ | ⚠️ | ✅ (clinical) |
| **Split computing** | ✅ | ✅ (adaptive) | ❌ | ❌ |
| **Federated learning** | ✅ | ❌ | ✅ | ❌ |
| **Distributed tracing** | ✅ | ✅ | ❌ | ❌ |
| **Chaos testing** | ⚠️ | ⚠️ | ❌ | ❌ |

---

## Part 7: Design Patterns for IoT Edge AI Systems (15 minutes)

### Pattern 1: Hierarchical Inference Pipeline

```
Tier 1 (MCU)          Tier 2 (Gateway)         Tier 3 (Server)         Tier 4 (Cloud)
     │                      │                        │                       │
  Feature              Aggregate              Correlate               Train
  Extract              Features               Across                  Global
     │                      │                        │                       │
     ▼                      ▼                        ▼                       ▼
┌─────────┐            ┌─────────┐            ┌─────────┐            ┌─────────┐
│ Anomaly │            │  Local  │            │  Fleet  │            │  Model  │
│ Score   │            │ Decision│            │  View   │            │ Registry│
└─────────┘            └─────────┘            └─────────┘            └─────────┘
```

**When to use**: Multi-sensor, high data rate, bandwidth-constrained uplinks

### Pattern 2: Adaptive Split Computing

```python
# Runtime decision based on SLA + network + device state
def compute_split_decision(sla_ms, network_bw, device_load, model_manifest):
    # Returns: (split_layer, expected_latency_ms)
    pass
```

**When to use**: Variable network conditions (5G, WiFi), heterogeneous edge devices

### Pattern 3: Federated Edge Learning

```
┌─────────────┐     ┌─────────────┐     ┌─────────────┐
│  Device A   │     │  Device B   │     │  Device C   │
│  Local data │     │  Local data │     │  Local data │
│  Local train│     │  Local train│     │  Local train│
└──────┬──────┘     └──────┬──────┘     └──────┬──────┘
       │                   │                   │
       └───────────────────┼───────────────────┘
                           ▼
              ┌─────────────────────┐
              │  Aggregation Server │  (FedAvg, Scaffold, FedProx)
              │  (Gateway/Cloud)    │
              └──────────┬──────────┘
                         │
                         ▼
              ┌─────────────────────┐
              │  Global Model v+1   │
              └──────────┬──────────┘
                         │
          ┌──────────────┼──────────────┐
          ▼              ▼              ▼
     ┌─────────┐    ┌─────────┐    ┌─────────┐
     │Device A │    │Device B │    │Device C │
     │  OTA    │    │  OTA    │    │  OTA    │
     └─────────┘    └─────────┘    └─────────┘
```

**When to use**: Data privacy, non-IID local data, regulatory constraints

### Pattern 4: Graceful Degradation Ladder

```yaml
degradation_levels:
  L0_normal:
    compute: split_optimal
    telemetry: full
    model_update: auto
    
  L1_degraded_network:
    compute: shift_local
    telemetry: reduced
    model_update: paused
    
  L2_intermittent:
    compute: local_only
    telemetry: buffered
    model_update: queued
    
  L3_disconnected:
    compute: local_only
    telemetry: persisted
    model_update: deferred
    autonomy: max
```

**When to use**: Any production edge deployment — design for L3 from day one

---

## Summary

**Cross-Cutting System Principles:**

1. **Match compute to tier** — MCU for features, Gateway for fusion, Server for correlation, Cloud for training
2. **Design split points explicitly** — with activation size, compute balance, privacy boundary
3. **Network is a first-class constraint** — adaptive split, buffering, graceful degradation
4. **Fleet operations = distributed systems** — OTA, canary, observability, certificate mgmt
5. **Regulatory shapes architecture** — FDA → local-only + safety monitor; GDPR → privacy-by-architecture
6. **Offline-first** — every edge device must function autonomously for hours/days

---

## Discussion Questions

- How do you handle model version skew when devices reconnect after weeks offline?
- What's the right granularity for federated learning: per-device, per-gateway, per-site?
- How do you test adaptive split computing under realistic network conditions?
- When does the cost of split computing (complexity, debugging) exceed its benefits?

---

## Further Reading (IoT Systems Case Studies)

- **MLSysBook Vol 2**: [Edge Intelligence](https://mlsysbook.ai/vol2/contents/vol2/edge_intelligence/edge_intelligence.html), [Fleet Orchestration](https://mlsysbook.ai/vol2/contents/vol2/fleet_orchestration/fleet_orchestration.html)
- **Industrial**: ISA/IEC 62443 case studies, NAMUR Open Architecture
- **Smart City**: ETSI MEC (Multi-access Edge Computing) whitepapers
- **Agricultural**: ISO 11783 (ISOBUS), ISO 18453 (sprayer control)
- **Healthcare**: FDA Digital Health Innovation Action Plan, IEC 62304
- **Split Computing**: "Split Learning over Edge Networks" (arXiv:2507.01041), BranchyNet, Neurosurgeon