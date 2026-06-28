# Edge AI Model Optimization and Deployment

## Lecture Overview

This lecture dives deep into the techniques and methodologies for optimizing machine learning models for edge deployment. We will cover quantization, pruning, knowledge distillation, neural architecture search for edge, and deployment strategies including OTA updates, model versioning, and monitoring. This session is designed to give students practical skills for preparing models for resource-constrained environments.

**Duration:** 2 hours  
**Target audience:** Computer Science graduate students with prior ML experience

---

## Part 1: Quantization Techniques (30 minutes)

### Understanding Numerical Precision in Neural Networks

Neural networks traditionally use 32-bit floating point (FP32) for weights and activations. This provides approximately 7 decimal digits of precision—far more than needed for most inference tasks. Quantization reduces precision to:

- **INT8**: 8-bit integers (-128 to 127)
- **UINT8**: Unsigned 8-bit (0 to 255)
- **INT16/UINT16**: 16-bit integers for mixed precision
- **FP16**: 16-bit floating point
- **Mixed precision**: Different precisions for different layers

### Post-Training Quantization (PTQ)

PTQ applies quantization after model training without retraining:

**Process:**
1. **Calibration**: Run representative dataset through model
2. **Range estimation**: Determine min/max values for each tensor
3. **Scale calculation**: Compute quantization scale and zero-point
4. **Replacement**: Replace FP32 operations with quantized equivalents

**Quantized matrix multiplication:**
```
FP32: output = Scale_w × Scale_x × (int8_W - ZP_w) × (int8_X - ZP_x) + bias
    
Where:
- Scale_w: Weight quantization scale
- Scale_x: Activation quantization scale  
- ZP_w: Weight zero-point
- ZP_x: Activation zero-point
- int8_W, int8_X: Quantized integer values
```

**Accuracy impact:**
- Typical loss: 1-3% for well-trained models
- Can degrade to 10-15% for certain architectures
- ResNet/VGG: Usually < 2% drop
- MobileNet: Often < 1% drop
- RNNs/LSTMs: Can suffer 5-10% drop without quantization-aware training

### Quantization-Aware Training (QAT)

QAT simulates quantization during training:

**Key techniques:**
- **Fake quantization nodes**: Added to forward pass to simulate quantization errors
- **Quantization simulation**: Training sees same errors as inference
- **Backpropagation through quantization**: Gradients flow through fake ops

**TensorFlow Lite QAT workflow:**
```python
import tensorflow as tf

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

### Per-Channel vs. Per-Tensor Quantization

**Per-tensor quantization:**
- Single scale and zero-point for entire tensor
- Simpler implementation
- Lower accuracy for weights
- Used for activations typically

**Per-channel quantization (weights only):**
- Separate scale/zero-point per output channel
- Better accuracy preservation
- More complex dequantization
- Standard for convolutional weight quantization

**Example comparison:**
```
Original weight range: [-2.3, 4.1, -0.8, 6.7] (mixed magnitudes)

Per-tensor: 
- Scale: 0.05, Zero-point: 128
- All values share same quantization parameters

Per-channel:
- Channel 1 scale: 0.02, ZP: 125
- Channel 2 scale: 0.08, ZP: 126
- Better represents each weight distribution
```

---

## Part 2: Model Pruning and Sparsification (20 minutes)

### Unstructured Pruning

Removes individual weights based on magnitude:

**Magnitude-based pruning:**
```python
import torch.nn.utils.prune as prune

# Prune 50% of smallest weights
prune.l1_unstructured(module, name='weight', amount=0.5)
```

**Challenges for edge deployment:**
- Sparse matrices require specialized kernels
- Memory savings only realized with sparse kernels
- Most edge accelerators don't support sparse inference efficiently

### Structured Pruning

Removes entire filters, channels, or layers:

**Filter pruning:**
```python
# Remove entire convolutional filters
prune.ln_structured(module, name='weight', n=2, dim=0, amount=0.3)
```

**Benefits:**
- Removes entire computational paths
- Direct speedup on all hardware
- Reduces model size naturally
- No specialized sparse kernels needed

### Lottery Ticket Hypothesis

Research by Frankle & Carbin (2019) showed that dense, randomly initialized networks contain subnetworks ("winning tickets") that can match or exceed original accuracy when trained with matching initializations.

**Implications for edge AI:**
- Smaller models can achieve same accuracy
- Pruning should be iterative (re-training between pruning steps)
- Network connectivity matters more than raw parameter count

### Pruning Workflow for Edge

1. **Train full model** to convergence
2. **Apply pruning schedule**: Start 10-20%, gradually increase
3. **Fine-tune pruned model**: Recover from quantization errors
4. **Verify hardware support**: Ensure target supports pruned architecture
5. **Export and test**: Measure real-world performance

---

## Part 3: Knowledge Distillation (20 minutes)

### Teacher-Student Architecture

Large "teacher" model trains small "student" model to mimic its behavior:

```
Teacher Model (ResNet-50, BERT-Large)
    ↓ (soft labels + intermediate features)
Student Model (MobileNet, TinyBERT)
```

**Soft labels contain richer information than hard labels:**
- Correct class probability: 95%
- Alternative class: 3%
- Third class: 2%
- Student learns relative confidences, not just top prediction

### Distillation Loss Functions

**Standard distillation loss:**
```
L_distill = KL-divergence(softmax(student/T) || softmax(teacher/T))
L_task = Cross-entropy(student || ground_truth)
L_total = α × L_distill + (1-α) × L_task
```

Where T is temperature (typically 2-20).

**Advanced techniques:**
- **Attention transfer**: Match attention maps in transformers
- **Feature map matching**: Intermediate activation distillation
- **Contrastive distillation**: Preserve relative distance relationships

### Edge-Specific Distillation Strategies

**Progressive distillation:**
1. Teacher: Large model (high accuracy)
2. Student-1: Medium model (moderate accuracy)
3. Student-2: Tiny model (edge deployment)
4. Each step distills from previous student

**Multi-task distillation:**
- Single teacher, multiple students with different specializations
- Each student optimized for specific edge device class
- Shared knowledge base across device types

---

## Part 4: Neural Architecture Search for Edge (15 minutes)

### EfficientNet: Scaling Laws for Edge

EfficientNet discovers optimal scaling between depth, width, and resolution:

**Scaling formula:**
```
depth: d = α^φ
width: w = β^φ  
resolution: r = γ^φ

Where φ is a user-specified coefficient controlling model size
```

**Edge-specific modifications:**
- Reduce resolution scaling (γ closer to 1)
- Emphasize width scaling (wider shallow networks)
- Limit depth for latency-constrained devices

### MobileNet Architecture

Uses depthwise separable convolutions to reduce computation:

**Standard convolution:**
- Input: H×W×K channels
- Output: H×W×C channels
- Operations: H×W×K×C (expensive)

**Depthwise separable:**
1. **Depthwise**: K separate convolutions, H×W×K operations
2. **Pointwise**: 1×1 convolution, H×W×K×C operations
3. **Total**: H×W×(K + K×C) vs H×W×K×C
4. **Reduction**: ~1/K + 1/C of original (e.g., 1/512 + 1/1024 ≈ 1/300)

### Neural Architecture Search (NAS) Constraints

For edge deployment, NAS must optimize:
- **Latency**: Must meet real-time requirements
- **Model size**: Fit within storage constraints
- **Power consumption**: Critical for battery devices
- **Accuracy**: Cannot sacrifice too much performance

**ProxylessNAS approach:**
- Search directly on target hardware (no proxy)
- Use weight sharing to reduce search cost
- Include hardware metrics in loss function

---

## Part 5: Model Compilation and Runtime Optimization (20 minutes)

### TensorFlow Lite Conversion Process

**Full workflow:**
```python
# Convert Keras model to TFLite
converter = tf.lite.TFLiteConverter.from_keras_model(model)

# Optimization flags
converter.optimizations = [
    tf.lite.Optimize.DEFAULT  # Enable default optimizations
]

# Representative dataset for quantization
def representative_dataset():
    for input_value in calibration_data:
        yield [input_value]

converter.representative_dataset = representative_dataset

# Quantization types
converter.target_spec.supported_types = [tf.int8]  # Full INT8 quantization

# Experimental new converter (recommended)
converter._experimental_new_converter = True

# Convert
tflite_model = converter.convert()
```

### ONNX Runtime for Edge

**Compilation with optimizations:**
```bash
# Install ONNX Runtime
pip install onnxruntime onnxruntime-tools

# Optimize model
python -c "
import onnx
from onnxruntime.transformers import optimizer

optimized_model = optimizer.optimize_model('model.onnx')
optimized_model.save_model_to_file('model_optimized.onnx')
"
```

### Runtime Performance Profiling

**TensorFlow Lite benchmark tool:**
```bash
# Build benchmark tool
git clone https://github.com/tensorflow/tensorflow
cd tensorflow/build
make -j benchmark_model

# Run on target hardware
./benchmark_model --graph=model.tflite \
    --num_threads=4 \
    --warmup_runs=10 \
    --num_runs=100
```

**Metrics to track:**
- Average inference time (ms)
- Standard deviation (consistency)
- Peak memory usage (RAM)
- Model load time
- Power consumption during inference

---

## Part 6: Deployment Strategies (15 minutes)

### Over-the-Air (OTA) Model Updates

**Secure update pipeline:**
1. **Model signing**: Digital signature on model binary
2. **Delta updates**: Binary diff for bandwidth efficiency
3. **Staged rollout**: Canary deployment to subset of devices
4. **Rollback mechanism**: Automatic revert on performance degradation
5. **Integrity verification**: Hash check before activation

**OTA update architecture:**
```
[Cloud Model Registry] → [Update Service] → [Edge Device]
                              ↓
                    [OTA Agent (background)]
                              ↓
                   [Model Validator/Benchmark]
                              ↓
                     [A/B Testing Framework]
```

### Model Versioning and A/B Testing

**Version format:**
```
v<major>.<minor>.<patch>-<platform>-<quantization>

Examples:
v1.2.3-rpi4-int8
v1.2.4-jetson-fp16
v2.0.0-esp32-uint8
```

**A/B testing framework:**
```python
class ModelABTester:
    def __init__(self, model_a, model_b):
        self.models = {'a': model_a, 'b': model_b}
        self.metrics = {'a': [], 'b': []}
    
    def run_inference(self, input_data, ground_truth):
        # Random assignment for A/B test
        variant = random.choice(['a', 'b'])
        prediction = self.models[variant].predict(input_data)
        
        # Log accuracy/throughput
        self.metrics[variant].append({
            'accuracy': (prediction == ground_truth),
            'latency': measured_latency,
            'power': measured_power
        })
```

### Edge Monitoring and Feedback

**Telemetry collection:**
- Accuracy drift detection (confidence scores)
- Latency monitoring (real-time performance)
- Memory pressure reporting
- Temperature and power metrics
- Error rate tracking

**Federated learning integration:**
- Periodically collect gradients (not raw data)
- Aggregate updates in cloud
- Push improved models to fleet
- Handle device heterogeneity

---

## Part 7: Hands-on Examples (10 minutes)

### Example 1: Image Classification Optimization

**Original ResNet-18:**
- Size: 45 MB
- Accuracy: 93% (ImageNet)
- Latency: 150 ms (Pi 4)

**Optimized MobileNetV3:**
- Size: 12 MB
- Accuracy: 91% (ImageNet)  
- Latency: 25 ms (Pi 4)

**Fully optimized for edge:**
- Quantization: INT8
- Pruning: 40% structured
- Size: 7 MB
- Accuracy: 89%
- Latency: 15 ms

### Example 2: Audio Processing Pipeline

**Voice command recognition:**
- Model: 1D CNN on spectrograms
- Original: 2.3 MB FP32, 500 ms latency
- Optimized: 1.1 MB INT8, 120 ms latency
- MCU deployment: 300 KB, 300 ms latency (Arduino Nano 33 BLE)

```python
# Edge impulse configuration for voice commands
{
  "model": {
    "type": "keras",
    "architecture": "cnn-1d",
    "window_size": 2,
    "window_increase": 0.5,
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

---

## Summary and Questions

**Key optimization techniques covered:**

1. **Quantization**: INT8/FP16 reduces size/compute with minimal accuracy loss
2. **Pruning**: Structured pruning for direct speedup on edge hardware
3. **Distillation**: Teacher-student training for smaller, efficient models
4. **NAS**: Architecture search with edge constraints
5. **Compilation**: Framework-specific optimizations for target runtime
6. **Deployment**: Secure OTA updates with A/B testing

**Discussion points:**
- When is quantization-aware training worth the extra effort?
- How do you balance model compression against accuracy?
- What monitoring is essential for production edge AI systems?

---

## Practical Exercises

Students should attempt these optimizations on provided models:

1. **Quantize a pre-trained model** using TensorFlow Lite PTQ
2. **Apply structured pruning** to remove 30% of parameters
3. **Compare inference latency** across different optimization levels
4. **Implement delta update mechanism** for model deployment

---

## Further Reading

- "Quantization and Training of Neural Networks for Efficient Integer-Arithmetic-Only Inference" - Jacob et al., 2018
- "The Lottery Ticket Hypothesis" - Frankle & Carbin, 2019
- "MobileNets: Efficient Convolutional Neural Networks for Mobile Vision Applications" - Howard et al., 2017
- TensorFlow Model Optimization Toolkit documentation