# AI on the Edge: Fundamentals and Architecture

## Lecture Overview

This lecture provides a comprehensive introduction to Artificial Intelligence on Edge computing platforms from an **ML Systems Engineering** perspective. We will explore the fundamental concepts, architectural patterns, hardware considerations, and the critical distinctions between cloud-based AI and edge AI deployments using the **D·A·M (Data·Algorithm·Machine)** taxonomy from Harvard's *Machine Learning Systems* textbook.

By the end of this session, students will understand the engineering trade-offs and quantitative analysis required for deploying AI models on resource-constrained devices.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with basic understanding of IoT systems and machine learning concepts  
**Reference:** *Machine Learning Systems* (MLSysBook) by Vijay Janapa Reddi et al., MIT Press 2026 — https://mlsysbook.ai/

---

## Part 1: Understanding Edge AI through the D·A·M Taxonomy (30 minutes)

### The D·A·M Framework for Edge AI

The **Data · Algorithm · Machine (D·A·M) taxonomy** is the primary diagnostic framework for ML systems engineering. It formalizes the interdependence between:

| Axis | Domain | Primary Constraint | Edge AI Relevance |
|------|--------|-------------------|-------------------|
| **Data (D)** | Information (The Fuel) | Bandwidth (BW) | Edge reduces Dvol/BW by processing locally |
| **Algorithm (A)** | Logic (The Blueprint) | Operations (O) | Model compression reduces O |
| **Machine (M)** | Physics (The Engine) | Throughput (Rpeak) | Edge hardware has lower Rpeak |

**Core Principle**: "In production, 'it is slow' and 'it is wrong' are rarely informative symptoms." Without a taxonomy, teams optimize the wrong thing—buying faster accelerators to fix a slow input pipeline, or rewriting kernels when the model is simply too large for the latency budget.

### What is Edge AI?

Edge AI refers to the deployment and execution of artificial intelligence algorithms directly on edge devices—computing nodes located at the periphery of the network, physically close to data sources. From a D·A·M perspective:

- **Data (D)**: Process locally → reduces Dvol/BW by orders of magnitude (million-fold for video)
- **Algorithm (A)**: Compressed models (quantization, pruning, distillation) → reduces O
- **Machine (M)**: Specialized edge accelerators (NPUs, TPUs) → improves Rpeak and ηhw for specific workloads

### Edge AI vs. Cloud AI: A D·A·M Comparison

| Aspect | Cloud AI | Edge AI | D·A·M Analysis |
|--------|----------|---------|----------------|
| **Latency** | High (network round-trip + processing) | Ultra-low (local only) | Eliminates network latency term |
| **Bandwidth** | Requires constant high-BW | Minimal, intermittent | Dvol/BW → near zero |
| **Privacy** | Data leaves device | Data stays local | Data (D) sovereignty |
| **Compute (Rpeak)** | Unlimited (server GPUs) | Constrained | Machine (M) limited |
| **Power** | Unlimited (grid) | Battery/solar | Machine (M) energy budget |
| **Operations (O)** | Large models | Compressed | Algorithm (A) must reduce O |

---

## Part 2: The Iron Law of ML Performance (20 minutes)

### The Iron Law Equation

The performance of any ML task is governed by the **Iron Law**:

```
T = Dvol/BW + O/(Rpeak·ηhw) + Llat
```

| Term | Meaning | Edge AI Optimization |
|------|---------|---------------------|
| **Dvol/BW** | Data movement time | **Edge advantage**: Dvol → metadata only |
| **O/(Rpeak·ηhw)** | Compute time | **Edge challenge**: Rpeak limited, must reduce O |
| **Llat** | Overhead (kernel launch, sync) | Critical for real-time edge |

### Iron Law Applied to Edge vs Cloud

```
Cloud: T = Dvol/BW_network + O/(Rpeak_cloud·ηhw) + Llat
Edge:  T = Dvol/BW_memory  + O/(Rpeak_edge·ηhw)  + Llat
```

**Key insight**: For edge, the data term shifts from network bandwidth to memory bandwidth. The compute term becomes dominant because Rpeak_edge ≪ Rpeak_cloud. This forces **Algorithm (A)** optimization: reduce O via quantization, pruning, distillation.

### The Memory Wall

From MLSysBook: **"Arithmetic is nearly free; data movement dominates cost (time, energy)."**

- DRAM access (~640 pJ) costs **100–20,000×** more energy than a MAC (~3.7 pJ) or SRAM access (~0.5 pJ)
- For edge devices on battery, **memory access is the primary energy consumer**
- **Arithmetic Intensity (AI)** = FLOP/byte moved from memory
  - High AI (> Ridge Point) → Compute-bound (benefits from TFLOP/s)
  - Low AI (< Ridge Point) → Memory-bound (benefits from bandwidth/reuse)

**Edge implication**: Most edge models are **memory-bound**. Optimization must focus on:
1. Reducing model size (weights fit in SRAM/flash)
2. Operator fusion (reduce intermediate memory traffic)
3. Data reuse (tiling, caching)

---

## Part 3: Hardware Acceleration for Edge (25 minutes)

### Specialization Waves (Post-Dennard Scaling ~2005)

1. **FP Coprocessors** (1980): 100× gain for FP
2. **Parallel GPUs** (1999→2006): Massive lightweight parallelism
3. **Domain-Specific Architectures (DSAs)** (TPUv1, 2015): **15–30× faster, 30–80× perf/watt** vs K80 GPU
4. **ML-Specific Refinement**: Tensor Cores, Systolic Arrays, Sparsity, Mixed Precision

### Dark Silicon & Efficiency Requirements

**Hennessy/Patterson**: DSAs require **≥10× efficiency** over general-purpose to justify dedicated silicon. Thermal limits prevent powering >30–50% transistors simultaneously ("Dark Silicon").

### Edge Hardware Landscape: Compute Primitives

| Primitive | Operation Type | Hardware Mapping | Edge Examples |
|-----------|----------------|------------------|---------------|
| **Matrix** | Many-to-Many (GEMM, Conv, Attention) | **Tensor Cores / Systolic Arrays** | Jetson Tensor Cores, Edge TPU, Apple Neural Engine |
| **Vector** | One-to-One (Activations, Norm, EW) | **SIMD / SIMT Units** | ARM NEON, RISC-V Vector Ext |
| **Special Function (SFU)** | Nonlinear (Exp, Sqrt, Reciprocal) | **Dedicated Units** | SFUs in NPUs |

### Numerics & Precision Evolution (HW/SW Co-design)

**Core Insight**: SW proved reduced precision works → HW added native support. This is **HW/SW Co-design** (Def 1.2): Violates traditional abstraction layers; algorithm constraints inform silicon, hardware shapes algorithms (e.g., INT8 quantization enables dense tensor-core packing).

| Precision | Memory Traffic | Energy/Op (vs FP32) | Edge Hardware Support |
|-----------|----------------|----------------------|----------------------|
| FP32 | 1× | 1× | All |
| FP16/BF16 | 0.5× | ~2× better | Tensor Cores, Jetson, Coral |
| INT8 | 0.25× | **~30× less energy** | Tensor Cores, Edge TPU, ARM NEON |
| INT4/FP4 | 0.125× | ~60× less energy | Latest (H100, Blackwell, Orin) |

**Impact**: FP32→INT8 **halves memory traffic again**; INT8 **~30× less energy/op** than FP32. Attacks both sides of Roofline (Bandwidth + Compute Density).

### Edge Platform Classification

| Platform | Compute | Memory | Power | Rpeak (INT8) | Best For |
|----------|---------|--------|-------|--------------|----------|
| **MCU** (ESP32, STM32, RP2040) | Cortex-M (16-300 MHz) | 32 KB - 2 MB | 10-100 mW | ~0.01-0.1 TOPS | Keyword spotting, simple anomaly |
| **SBC** (Pi 4/5, Jetson Nano) | Cortex-A (1-2 GHz) | 1-8 GB | 2-10 W | 0.5-10 TOPS | Object detection, audio, sensor fusion |
| **Edge Server** (Jetson AGX, x86+GPU) | Multi-core + GPU | 16-128 GB | 30-300 W | 20-200 TOPS | Multi-camera, complex models |

---

## Part 4: Why Edge AI Matters — Quantitative Analysis (20 minutes)

### The Bandwidth Challenge: Iron Law Data Term

Consider a surveillance camera capturing 1080p video at 30fps:
- Raw video stream: ~1 Gbps (Dvol/BW_network)
- Transmitting to cloud: Requires high-bandwidth, expensive network connections
- Edge AI solution: Process locally, transmit only events (metadata) - < 1 Kbps
- **Reduction**: Dvol/BW_network → **million-fold reduction**

### Latency Requirements: The Llat Constraint

| Domain | Required Latency | Cloud Feasible? | Edge Necessary? | Constraint |
|--------|------------------|-----------------|-----------------|------------|
| Industrial automation | < 1 ms | ❌ | ✅ | Llat dominates |
| Autonomous vehicles | < 100 ms | ❌ | ✅ | Dvol/BW + O/R |
| Augmented reality | < 20 ms | ❌ | ✅ | Llat dominates |
| Smart home control | < 1 s | ⚠️ | ✅ | Dvol/BW acceptable |
| Predictive maintenance | < 10 s | ✅ | ✅ | Either works |
| Energy optimization | < 1 min | ✅ | ⚠️ | Cloud acceptable |

### Privacy and Security: Data (D) Sovereignty

- **On-device processing** of Personally Identifiable Information (PII)
- **Federated learning**: Models improve without sharing data
- **Regulatory compliance**: GDPR, HIPAA, local data residency laws

---

## Part 5: Edge AI Architectures (15 minutes)

### Architecture Pattern 1: Cloud-Edge Hybrid (D·A·M Coordination)

```
[IoT Devices] → [Edge Nodes] → [Cloud Backend]
     Data              AI              Model
                     Inference       Training
                     (reduce O)      (full O)
```

- **Data (D)**: Edge filters Dvol; only anomalies/metadata to cloud
- **Algorithm (A)**: Edge runs compressed model (reduced O); Cloud trains full model
- **Machine (M)**: Edge uses NPU for inference; Cloud uses GPU cluster for training
- **Pipelining**: `T = max(Dvol/BW_edge, O_edge/R_edge) + O_cloud/R_cloud`

### Architecture Pattern 2: Hierarchical Edge

```
[Sensor Layer] → [Gateway Layer] → [Edge Server Layer] → [Cloud (optional)]
   (MCU)            (SBC)              (Server)
   O~10^6           O~10^9             O~10^12
   R~0.01 TOPS      R~1 TOPS           R~100 TOPS
```

- **Selective offloading**: Only complex tasks go upstream
- Each layer handles its O within local Rpeak budget

### Architecture Pattern 3: Fully Distributed Edge

```
[Device 1] → [Device 2] → [Device 3]
     │              │              │
     └─────────── [Peer-to-Peer] ──┘
```

- **Collaborative inference**: Distributed O across devices
- **Consensus mechanisms** for critical applications
- **Fail-safe operation** even if individual nodes fail

---

## Part 6: Frameworks and Tools through D·A·M Lens (10 minutes)

| Framework | Algorithm (A) Support | Machine (M) Target | Data (D) Handling |
|-----------|----------------------|-------------------|-------------------|
| **TensorFlow Lite** | PTQ, QAT, Pruning, Clustering | GPU, DSP, NPU delegates; TFLite Micro for MCU | Representative dataset for calibration |
| **ONNX Runtime** | Quantization-aware training | Execution providers (CPU, CUDA, TensorRT, NPU) | Cross-platform model format |
| **Edge Impulse** | Auto-optimization, NAS | MCU, SBC, GPU targets | End-to-end data pipeline |
| **Apache TVM** | Kernel tuning, operator fusion | AOT compilation to target ISA | Memory planning for constrained devices |
| **MLSys·im** | First-principles performance modeling | Simulates Rpeak, BW, ηhw | Predicts T = max(D/BW, O/R) |

---

## Summary and Questions

**Key takeaways from this lecture:**

1. **D·A·M Taxonomy**: Every edge AI decision involves Data, Algorithm, Machine trade-offs
2. **Iron Law**: `T = Dvol/BW + O/(Rpeak·ηhw) + Llat` — Edge shifts balance toward compute term
3. **Memory Wall**: Data movement dominates energy; arithmetic intensity determines bound
4. **HW/SW Co-design**: Quantization enables hardware specialization (INT8 → Tensor Cores)
5. **Architecture choice**: Hybrid, Hierarchical, or Distributed based on O, Rpeak, BW constraints

**Discussion questions:**
- For a given edge device (Rpeak, BW, power budget), how do you determine max feasible O?
- What happens when quantization reduces O but destroys memory access patterns?
- How do you pipeline D, A, M stages to achieve `T = max(...)` instead of `T = sum(...)`?

---

## Further Reading

- **Primary**: *Machine Learning Systems* (MLSysBook) — https://mlsysbook.ai/
  - Volume I: Chapters on Hardware Acceleration, Benchmarking, Model Compression
  - D·A·M Taxonomy Appendix
  - Iron Law and Roofline Model
- TensorFlow Lite documentation: https://www.tensorflow.org/lite
- NVIDIA Jetson developer resources: https://developer.nvidia.com/jetson
- "TinyML: Machine Learning with TensorFlow Lite on Arduino and Ultra-Low-Power Microcontrollers" — Warden & Situnayake, 2020