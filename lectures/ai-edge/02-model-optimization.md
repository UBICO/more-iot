# Edge AI Model Optimization and Deployment

## Lecture Overview

This lecture dives deep into the techniques and methodologies for optimizing machine learning models for edge deployment from an **ML Systems Engineering** perspective. We will cover quantization, pruning, knowledge distillation, neural architecture search for edge, and deployment strategies using the **D·A·M (Data·Algorithm·Machine)** taxonomy and the **Iron Law** of ML performance.

By the end of this session, students will understand the quantitative trade-offs and engineering principles required for preparing models for resource-constrained environments.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with prior ML experience  
**Reference:** *Machine Learning Systems* (MLSysBook) by Vijay Janapa Reddi et al., MIT Press 2026 — https://mlsysbook.ai/

---

## Part 1: Quantization — Theory and Practice (30 minutes)

### Numerical Precision in Neural Networks: The Physics

From MLSysBook: **"Arithmetic is nearly free; data movement dominates cost."** Quantization attacks both sides of the Roofline:

- **Bandwidth side**: Reduced bitwidth → less data movement (Dvol/BW ↓)
- **Compute side**: Integer MACs → higher Rpeak, lower energy/op (O/(R·η) ↓)

### Precision Evolution and HW/SW Co-design

**Core Insight**: SW proved reduced precision works → HW added native support. This is **HW/SW Co-design** (Def 1.2): algorithm constraints inform silicon, hardware shapes algorithms (e.g., INT8 quantization enables dense tensor-core packing).

| NVIDIA Tensor Core Trajectory | Precision | Impact |
|-------------------------------|-----------|--------|
| Volta (2017) | FP16 | 2× bandwidth reduction |
| Turing (2018) | INT8/4/1 | Integer tensor cores |
| Ampere (2020) | TF32, BF16, FP64 | Mixed precision flexibility |
| Hopper (2022) | FP8 | 8-bit floating point |
| Blackwell (2024) | FP4 | 4-bit floating point |

**Energy impact**: INT8 **~30× less energy/op** than FP32. FP32→FP16 halves memory traffic; INT8 halves it again.

### Quantization Fundamentals

#### Symmetric vs Asymmetric Quantization

**Symmetric (per-tensor, typical for weights):**
```
q = round(r / s)          where s = max(|r|) / (2^(b-1) - 1)
r ≈ s × q                 Zero-point = 0
```

**Asymmetric (per-tensor, typical for activations):**
```
q = round(r / s) + z      where s = (max(r) - min(r)) / (2^b - 1)
                          z = -round(min(r) / s)
r ≈ s × (q - z)
```

#### Per-Channel vs Per-Tensor Quantization

| Aspect | Per-Tensor | Per-Channel (weights only) |
|--------|------------|---------------------------|
| **Scale/Zero-point** | 1 per tensor | 1 per output channel |
| **Accuracy** | Lower for weights | Better preserves weight distribution |
| **Hardware** | Simpler | Requires per-channel dequantization |
| **Edge support** | Universal | Tensor Cores, Edge TPU, ONNX Runtime |

**Example**: Weight tensor shape [C_out, C_in, K, K]
- Per-tensor: 1 scale for all C_out × C_in × K × K elements
- Per-channel: C_out scales (one per output channel)

### Post-Training Quantization (PTQ)

PTQ applies quantization after model training without retraining:

**Process:**
1. **Calibration**: Run representative dataset through model (typically 100-500 samples)
2. **Range estimation**: Determine min/max values for each tensor
3. **Scale calculation**: Compute quantization scale and zero-point
4. **Replacement**: Replace FP32 operations with quantized equivalents

**Quantized matrix multiplication:**
```
output = Scale_w × Scale_x × (int8_W - ZP_w) × (int8_X - ZP_x) + bias

Where:
- Scale_w: Weight quantization scale
- Scale_x: Activation quantization scale  
- ZP_w: Weight zero-point (0 for symmetric)
- ZP_x: Activation zero-point
- int8_W, int8_X: Quantized integer values
```

**Accuracy impact (MLSysBook data):**
- Typical loss: 1-3% for well-trained models
- ResNet/VGG: Usually < 2% drop
- MobileNet: Often < 1% drop  
- RNNs/LSTMs: Can suffer 5-10% drop without QAT
- Transformers: Attention scores sensitive; may need QAT

### Quantization-Aware Training (QAT)

QAT simulates quantization during training using **fake quantization nodes**:

**Key techniques:**
- **Fake quantization nodes**: Added to forward pass to simulate quantization errors
- **Quantization simulation**: Training sees same errors as inference
- **Backpropagation through quantization**: Gradients flow through fake ops (straight-through estimator)

**TensorFlow Lite QAT workflow:**
```python
import tensorflow as tf
import tensorflow_model_optimization as tfmot

# Step 1: Prepare model
model = tf.keras.models.load_model('path/to/model')

# Step 2: Apply quantization-aware training
qat_model = tfmot.quantization.keras.quantize_model(model)

# Step 3: Fine-tune with representative dataset
qat_model.compile(optimizer='adam', loss='mse')
qat_model.fit(rep_dataset, epochs=5)

# Step 4: Convert to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(qat_model)
converter.optimizations = [tf.lite.Optimize.DEFAULT]
tflite_model = converter.convert()
```

**When QAT is worth the effort (MLSysBook guidance):**
- PTQ accuracy drop > 2-3%
- Models with sensitive layers (attention, small activations)
- Ultra-low precision (INT4, FP4)
- Regulatory accuracy requirements

---

## Part 2: Model Pruning and Sparsification (20 minutes)

### The Lottery Ticket Hypothesis (Frankle & Carbin, 2019)

Dense, randomly initialized networks contain subnetworks ("winning tickets") that can match or exceed original accuracy when trained with matching initializations.

**Implications for edge AI (MLSysBook):**
- Smaller models can achieve same accuracy
- Pruning should be **iterative** (re-training between pruning steps)
- Network connectivity matters more than raw parameter count
- **Critical**: A pruning strategy that reduces FLOPs but destroys memory access patterns can *slow down* execution on real hardware (D·A·M intersection)

### Unstructured vs Structured Pruning

| Aspect | Unstructured | Structured |
|--------|--------------|------------|
| **Granularity** | Individual weights | Filters, channels, layers |
| **FLOP reduction** | Theoretical only | Direct, measurable |
| **Memory savings** | Requires sparse kernels | Natural size reduction |
| **Hardware support** | Rare on edge | Universal |
| **Speedup on edge** | Often negative | Positive |

**Edge reality**: Most edge accelerators don't support sparse inference efficiently. **Use structured pruning.**

### Structured Pruning Workflow

```python
import torch.nn.utils.prune as prune

# Filter pruning: remove entire convolutional filters
prune.ln_structured(module, name='weight', n=2, dim=0, amount=0.3)

# Iterative pruning schedule
for epoch in range(total_epochs):
    if epoch % prune_freq == 0:
        current_sparsity = min(0.5, epoch * 0.01)
        prune.ln_structured(module, 'weight', n=2, dim=0, amount=current_sparsity)
        fine_tune(model, epochs=1)
```

### Pruning Decision Framework (D·A·M)

```
Target: Reduce O (Algorithm) while preserving memory access patterns (Machine)

Step 1: Profile baseline — measure T = Dvol/BW + O/(R·η) + Llat
Step 2: Apply structured pruning → measure new O, R·η (may change!)
Step 3: If latency increases despite O↓ → pruning hurt memory patterns → revert
Step 4: Iterate with fine-tuning until accuracy/latency target met
```

---

## Part 3: Knowledge Distillation (20 minutes)

### Teacher-Student Architecture

Large "teacher" model trains small "student" model to mimic its behavior:

```
Teacher Model (ResNet-50, BERT-Large) → Soft Labels + Intermediate Features
                                           ↓
Student Model (MobileNet, TinyBERT) ← Hard Labels + Distillation Loss
```

**Soft labels contain richer information than hard labels:**
- Correct class probability: 95%
- Alternative class: 3%
- Third class: 2%
- Student learns **relative confidences**, not just top prediction

### Distillation Loss Functions

**Standard distillation loss (Hinton et al., 2015):**
```
L_distill = KL-divergence(softmax(student/T) || softmax(teacher/T))
L_task    = Cross-entropy(student || ground_truth)
L_total   = α × L_distill + (1-α) × L_task

Where T = temperature (typically 2-20)
```

**Advanced techniques (MLSysBook Algorithm∩Machine):**
- **Attention transfer**: Match attention maps in transformers (Zagoruyko & Komodakis, 2017)
- **Feature map matching**: Intermediate activation distillation (Romero et al., 2015)
- **Contrastive distillation**: Preserve relative distance relationships

### Edge-Specific Distillation Strategies

**Progressive distillation (for extreme compression):**
1. Teacher: Large model (high accuracy, O_large)
2. Student-1: Medium model (moderate accuracy, O_medium)  
3. Student-2: Tiny model (edge deployment, O_tiny)
4. Each step distills from previous student

**Multi-task distillation:**
- Single teacher, multiple students with different specializations
- Each student optimized for specific edge device class (MCU, SBC, Server)
- Shared knowledge base across device types

**Distillation for quantization (MLSysBook):**
- Teacher: FP32 model
- Student: Quantized architecture (INT8-aware)
- Distillation helps recover quantization accuracy loss

---

## Part 4: Neural Architecture Search for Edge (15 minutes)

### The D·A·M View of NAS

NAS optimizes at **Algorithm ∩ Machine** intersection: How to Execute Efficiently.

**Constraints for edge (from MLSysBook):**
- **Latency**: Must meet real-time SLAs (Llat constraint)
- **Model size**: Fit within storage (Dvol constraint)
- **Power consumption**: Energy budget (Machine constraint)
- **Accuracy**: Cannot sacrifice too much (Algorithm quality)

### EfficientNet: Compound Scaling (Tan & Le, 2019)

EfficientNet discovers optimal scaling between depth, width, and resolution:

```
depth: d = α^φ
width: w = β^φ  
resolution: r = γ^φ

Subject to: α·β²·γ² ≈ 2 (for 2× FLOPs)
```

**Edge-specific modifications:**
- Reduce resolution scaling (γ closer to 1) — memory bound
- Emphasize width scaling (wider shallow networks) — better parallelism
- Limit depth for latency-constrained devices (Llat constraint)

### MobileNet Architecture (Howard et al., 2017)

Uses **depthwise separable convolutions** to reduce computation:

| Convolution Type | Operations | Reduction |
|------------------|------------|-----------|
| Standard | H×W×K×C | 1× (baseline) |
| Depthwise Separable | H×W×(K + K×C) | ~1/K + 1/C |

For K=32, C=64: **~30× fewer operations**

### Neural Architecture Search Constraints

**ProxylessNAS approach (Cai et al., 2019):**
- Search directly on target hardware (no proxy)
- Use weight sharing to reduce search cost
- Include hardware metrics in loss function: `L = L_task + λ·latency(device)`

**Once-for-All (OFA) Network (Cai et al., 2020):**
- Train single supernet supporting many sub-networks
- Extract specialized subnets for each edge device
- Avoids retraining for each deployment target

---

## Part 5: Model Compilation and Runtime Optimization (20 minutes)

### The Compilation Stack (MLSysBook: Algorithm ∩ Machine)

```
Model (PyTorch/TF) → Graph Optimizations → Kernel Selection → Code Generation
                        ↓                      ↓                 ↓
                 Operator Fusion        Hardware-specific    Memory Planning
                 Constant Folding       Kernels (TensorRT,   (Static for MCU)
                 Layout Transforms      TVM, XLA)            
```

### TensorFlow Lite Conversion with Optimizations

```python
import tensorflow as tf

converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Optimization flags
converter.optimizations = [tf.lite.Optimize.DEFAULT]

# Representative dataset for PTQ
def representative_dataset():
    for input_value in calibration_data:
        yield [input_value]

converter.representative_dataset = representative_dataset

# Quantization configuration
converter.target_spec.supported_ops = [
    tf.lite.OpsSet.TFLITE_BUILTINS_INT8
]
converter.inference_input_type = tf.int8
converter.inference_output_type = tf.int8

# Experimental new converter (recommended)
converter._experimental_new_converter = True

# Convert
tflite_model = converter.convert()
```

### ONNX Runtime for Edge

```bash
# Install
pip install onnxruntime onnxruntime-tools

# Optimize with graph transformations
python -c "
import onnx
from onnxruntime.transformers import optimizer

# Fusion, constant folding, layout optimization
optimized_model = optimizer.optimize_model('model.onnx')
optimized_model.save_model_to_file('model_optimized.onnx')
"
```

### Apache TVM: End-to-End Compilation

```python
import tvm
from tvm import relay

# Load model
mod, params = relay.frontend.from_tensorflow(model_path)

# Target-specific compilation
target = tvm.target.Target("llvm -mt=armv7l -mattr=+neon")  # ARM Cortex-A
# target = tvm.target.Target("cuda -arch=sm_70")  # Jetson GPU

# Compile with optimizations
with tvm.transform.PassContext(opt_level=3):
    lib = relay.build(mod, target=target, params=params)

# Deploy artifacts: lib.so (compiled kernels), graph.json, params
lib.export_library("model.so")
```

### Runtime Performance Profiling (MLSysBook: Benchmarking)

**Micro-benchmarking rules (Systems Detective's Rules):**
1. **Warm-up rule** — Don't measure cold-start iterations
2. **Variance rule** — Report Coefficient of Variation (CV = σ/μ); CV > 5% = noisy
3. **Speed-of-light check** — Compare achieved throughput vs roofline
4. **Flush rule** — Flush L2 cache between memory bandwidth runs

**TensorFlow Lite benchmark tool:**
```bash
./benchmark_model --graph=model.tflite \
    --num_threads=4 \
    --warmup_runs=10 \
    --num_runs=100 \
    --enable_op_profiling=true
```

**Metrics to track:**
| Metric | Iron Law Term | Target |
|--------|---------------|--------|
| Average inference time (ms) | T | < SLA |
| Standard deviation | Consistency | CV < 5% |
| Peak memory usage (RAM) | Dvol | < Device RAM |
| Model load time | Llat | < 100 ms |
| Power consumption | Energy/inference | < Budget |

---

## Part 6: Deployment Strategies (15 minutes)

### Over-the-Air (OTA) Model Updates

**Secure update pipeline (D·A·M lifecycle):**
1. **Model signing** (Algorithm integrity): Digital signature on model binary
2. **Delta updates** (Data efficiency): Binary diff for bandwidth efficiency
3. **Staged rollout** (Machine safety): Canary deployment to subset of devices
4. **Rollback mechanism** (Machine reliability): Automatic revert on performance degradation
5. **Integrity verification** (Algorithm validity): Hash check before activation

**OTA architecture with D·A·M monitoring:**
```
[Cloud Model Registry] → [Update Service] → [Edge Device]
                              ↓
                    [OTA Agent (background)]
                              ↓
                   [Model Validator/Benchmark]
                              ↓
                     [A/B Testing Framework]
                              ↓
              [Telemetry: Accuracy, Latency, Power, Memory]
```

### Model Versioning and A/B Testing

**Version format (semantic + platform):**
```
v<major>.<minor>.<patch>-<platform>-<quantization>-<optimization>

Examples:
v1.2.3-rpi4-int8-pruned30
v1.2.4-jetson-fp16-distilled
v2.0.0-esp32-uint8-nas
```

**A/B testing with D·A·M metrics:**
```python
class ModelABTester:
    def __init__(self, model_a, model_b):
        self.models = {'a': model_a, 'b': model_b}
        self.metrics = {'a': [], 'b': []}
    
    def run_inference(self, input_data, ground_truth):
        variant = random.choice(['a', 'b'])
        prediction = self.models[variant].predict(input_data)
        
        # Log D·A·M metrics
        self.metrics[variant].append({
            'accuracy': (prediction == ground_truth),     # Algorithm quality
            'latency_ms': measured_latency,               # Machine throughput
            'power_mw': measured_power,                   # Machine efficiency
            'memory_mb': peak_memory,                     # Data volume
            'timestamp': time.time()
        })
```

### Edge Monitoring and Feedback Loops

**Telemetry collection (D·A·M observability):**
| Axis | Metrics | Purpose |
|------|---------|---------|
| **Data (D)** | Input distribution shift, feature drift | Detect data quality issues |
| **Algorithm (A)** | Accuracy, confidence scores, calibration | Detect model degradation |
| **Machine (M)** | Latency (p50, p99), throughput, power, memory, temperature | Detect hardware issues |

**Federated learning integration (Data privacy + Algorithm improvement):**
- Periodically collect **gradients** (not raw data) — Data (D) privacy
- Aggregate updates in cloud — Algorithm (A) improvement
- Push improved models to fleet — Machine (M) deployment
- Handle device heterogeneity (different Rpeak, memory)

---

## Part 7: Hands-on Optimization Case Study (10 minutes)

### Image Classification: ResNet-18 → Edge Optimized

| Stage | Model | Size | Accuracy | Latency (Pi 4) | Ops (GFLOPs) |
|-------|-------|------|----------|----------------|--------------|
| **Baseline** | ResNet-18 FP32 | 45 MB | 93% | 150 ms | 1.8 |
| **Architecture** | MobileNetV3 FP32 | 12 MB | 91% | 25 ms | 0.22 |
| **Quantization** | MobileNetV3 INT8 | 3 MB | 90% | 12 ms | 0.22 |
| **Pruning** | MobileNetV3 INT8 + 40% structured | 1.8 MB | 89% | 8 ms | 0.13 |
| **Distillation** | TinyNet (student) INT8 | 1.2 MB | 88% | 6 ms | 0.08 |

**Iron Law Analysis:**
```
Baseline: T = 45MB/BW + 1.8G/(R·η) + Llat = 150ms
Optimized: T = 1.2MB/BW + 0.08G/(R·η) + Llat = 6ms
```

### Audio Processing: Keyword Spotting on MCU

```python
# Edge Impulse configuration for voice commands
{
  "model": {
    "type": "keras",
    "architecture": "ds-cnn",
    "window_size": 1000,  # ms
    "window_increase": 500,
    "frequency": 16000,
    "quantized": true
  },
  "deployment": {
    "target": "arduino-nano-33-ble",
    "memory": "256KB RAM",
    "flash": "1MB Flash"
  }
}
```

**Results on Cortex-M4 (256 KB RAM, 1 MB Flash):**
- Model: DS-CNN, 150 KB INT8
- Keywords: 4 ("start", "stop", "up", "down")
- Latency: 20 ms per inference
- Power: < 5 mA active, < 1 µA sleep
- Accuracy: 98% in 60 dB factory noise

---

## Summary and Questions

**Key optimization techniques with D·A·M analysis:**

| Technique | Algorithm (O) | Machine (R·η) | Data (Dvol/BW) |
|-----------|---------------|---------------|----------------|
| **Quantization (INT8)** | O unchanged | Rpeak↑ 4-8× (Tensor Cores) | Dvol↓ 4× |
| **Structured Pruning** | O↓ 30-50% | R·η may change | Dvol↓ proportional |
| **Distillation** | O↓ via smaller arch | Rpeak↑ (smaller model) | Dvol↓ proportional |
| **NAS / EfficientNet** | O↓ optimal scaling | R·η↑ (better HW fit) | Dvol↓ |
| **Operator Fusion** | O unchanged | ηhw↑ (less kernel launch) | BW↓ (fewer intermediates) |

**Discussion points:**
- When does quantization-aware training provide >1% accuracy recovery over PTQ?
- How do you verify that pruning improved R·η and not just O?
- What benchmarking methodology ensures reproducible edge measurements?

---

## Practical Exercises

Students should attempt these optimizations on provided models:

1. **Quantize a pre-trained model**trained model** using TensorFlow Lite PTQ and QAT; compare accuracy
2. **Apply structured pruning** to remove 30% of filters; measure actual latency change
3. **Implement distillation** from ResNet-50 to MobileNetV3; evaluate student performance
4. **Run TVM compilation** for ARM Cortex-A and measure speedup over TFLite
5. **Design OTA update** with delta compression and A/B testing framework

---

## Further Reading (MLSysBook Primary Source)

- **Primary**: *Machine Learning Systems* (MLSysBook) — https://mlsysbook.ai/
  - Volume I: [Model Compression](https://mlsysbook.ai/vol1/model_compression/model_compression.html) — Quantization, Pruning, Distillation
  - Volume I: [Hardware Acceleration](https://mlsysbook.ai/vol1/hw_acceleration/hw_acceleration.html) — Tensor Cores, Numerics, Memory Wall
  - Volume I: [Benchmarking](https://mlsysbook.ai/vol1/benchmarking/benchmarking.html) — D·A·M Validation, Micro/Macro/End-to-End
  - Volume I: [Model Training](https://mlsysbook.ai/vol1/training/training.html) — Iron Law, Mixed Precision
  - Appendix: [D·A·M Taxonomy](https://mlsysbook.ai/vol1/backmatter/appendix_dam.html)
- "Quantization and Training of Neural Networks for Efficient Integer-Arithmetic-Only Inference" — Jacob et al., 2018
- "The Lottery Ticket Hypothesis" — Frankle & Carbin, 2019
- "MobileNets: Efficient Convolutional Neural Networks for Mobile Vision Applications" — Howard et al., 2017
- "Once for All: Train One Network and Specialize it for Efficient Deployment" — Cai et al., 2020