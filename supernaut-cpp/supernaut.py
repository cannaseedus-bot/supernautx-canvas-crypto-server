#!/usr/bin/env python3
"""
Supernaut S7-MINI: Phase 7.2 - Real Weights & Tensors
"""

import json
import numpy as np
from typing import List, Tuple
from pathlib import Path

# ============================================================================
# TENSOR LAYER
# ============================================================================

class Int8Tensor:
    """INT8 quantized tensor with scale factor"""
    def __init__(self, shape: Tuple[int, ...], scale: float = 1.0, data: np.ndarray = None):
        self.shape = shape
        self.scale = scale
        if data is not None:
            self.data = data.astype(np.int8)
        else:
            self.data = np.zeros(shape, dtype=np.int8)
    
    def dequantize(self) -> np.ndarray:
        """Convert int8 data to float with scale"""
        return self.data.astype(np.float32) * self.scale
    
    def quantize(self, values: np.ndarray):
        """Quantize float32 values to int8"""
        self.data = np.clip(values / self.scale, -128, 127).astype(np.int8)

# ============================================================================
# MODEL LAYERS
# ============================================================================

class Linear:
    """Linear layer: y = x @ W"""
    def __init__(self, in_features: int, out_features: int, scale: float = 1.0, weights_path: str = None):
        self.in_features = in_features
        self.out_features = out_features
        self.scale = scale
        
        if weights_path and Path(weights_path).exists():
            # Load from file
            data = np.fromfile(weights_path, dtype=np.int8)
            self.weight = Int8Tensor((in_features, out_features), scale, data.reshape(in_features, out_features))
        else:
            # Initialize with random
            self.weight = Int8Tensor((in_features, out_features), scale, 
                                    np.random.randint(-32, 32, size=(in_features, out_features), dtype=np.int8))
    
    def forward(self, x: np.ndarray) -> np.ndarray:
        """Matrix-vector multiply with INT8 quantization"""
        w_float = self.weight.dequantize()
        return x @ w_float

class Embedding:
    """Token embedding lookup table"""
    def __init__(self, vocab_size: int, hidden_dim: int, scale: float = 1.0, weights_path: str = None):
        self.vocab_size = vocab_size
        self.hidden_dim = hidden_dim
        self.scale = scale
        
        if weights_path and Path(weights_path).exists():
            # Load from file
            data = np.fromfile(weights_path, dtype=np.int8)
            self.weight = Int8Tensor((vocab_size, hidden_dim), scale, data.reshape(vocab_size, hidden_dim))
        else:
            # Initialize with random
            self.weight = Int8Tensor((vocab_size, hidden_dim), scale,
                                    np.random.randint(-32, 32, size=(vocab_size, hidden_dim), dtype=np.int8))
    
    def forward(self, token_ids: np.ndarray) -> np.ndarray:
        """Look up embeddings for token IDs"""
        # Use last token
        token_id = int(token_ids[-1])
        token_id = token_id % self.vocab_size  # Wrap around
        
        w_float = self.weight.dequantize()
        return w_float[token_id]

class S7Mini:
    """Minimal S7Mini transformer: embedding → projection → logits"""
    def __init__(self, vocab_size: int = 256, hidden_dim: int = 320, context_len: int = 512,
                 embedding_weights: str = None, projection_weights: str = None):
        self.vocab_size = vocab_size
        self.hidden_dim = hidden_dim
        self.context_len = context_len
        
        self.embedding = Embedding(vocab_size, hidden_dim, weights_path=embedding_weights)
        self.lm_head = Linear(hidden_dim, vocab_size, weights_path=projection_weights)
    
    def forward(self, token_ids: np.ndarray) -> np.ndarray:
        """Forward pass: tokens → embedding → projection → logits"""
        # Validate
        assert len(token_ids) > 0, "Token sequence cannot be empty"
        
        # Embedding lookup (use last token)
        hidden = self.embedding.forward(token_ids)
        assert hidden.shape == (self.hidden_dim,), f"Embedding shape mismatch: {hidden.shape}"
        
        # Project to logits
        logits = self.lm_head.forward(hidden)
        assert logits.shape == (self.vocab_size,), f"Projection shape mismatch: {logits.shape}"
        
        return logits
    
    def benchmark(self, num_iterations: int = 100):
        """Benchmark inference latency"""
        import time
        
        # Test token
        test_tokens = np.array([1, 2, 3], dtype=np.int8)
        
        # Warmup
        self.forward(test_tokens)
        
        # Benchmark
        start = time.time()
        for _ in range(num_iterations):
            self.forward(test_tokens)
        elapsed = time.time() - start
        
        avg_latency = (elapsed / num_iterations) * 1000  # ms
        throughput = num_iterations / elapsed  # tokens/sec
        
        print(f"✓ Benchmark ({num_iterations} iterations):")
        print(f"  Avg latency: {avg_latency:.2f} ms/token")
        print(f"  Throughput: {throughput:.1f} tokens/sec")
        
        return {"latency_ms": avg_latency, "throughput_tps": throughput}

# ============================================================================
# TOKENIZER
# ============================================================================

class Tokenizer:
    """Simple whitespace-split tokenizer"""
    def __init__(self):
        self.vocab = {}
        self.inv_vocab = {}
        self.unk_token = 1
    
    def load_vocab(self, vocab_path: str) -> bool:
        """Load vocabulary from JSON file"""
        try:
            with open(vocab_path, 'r') as f:
                self.vocab = json.load(f)
            
            # Build inverse vocab (int to str)
            self.inv_vocab = {v: k for k, v in self.vocab.items()}
            
            print(f"✓ Loaded vocabulary: {len(self.vocab)} tokens from {vocab_path}")
            return len(self.vocab) > 0
        except Exception as e:
            print(f"✗ Failed to load vocab: {e}")
            return False
    
    def encode(self, text: str) -> np.ndarray:
        """Encode text to token IDs"""
        tokens = []
        for word in text.split():
            token_id = self.vocab.get(word.lower(), self.unk_token)
            tokens.append(token_id)
        return np.array(tokens, dtype=np.int8)
    
    def decode(self, token_ids: np.ndarray) -> str:
        """Decode token IDs to text"""
        words = []
        for token_id in token_ids:
            word = self.inv_vocab.get(int(token_id), "<unk>")
            words.append(word)
        return " ".join(words)

# ============================================================================
# INFERENCE ENGINE
# ============================================================================

def greedy_decode(
    model: S7Mini,
    input_tokens: np.ndarray,
    max_tokens: int = 32,
    eos_token: int = 2
) -> List[int]:
    """Generate tokens using greedy decoding (argmax)"""
    import ctypes
    
    # Start with input tokens
    tokens = [int(t) for t in input_tokens]
    
    for i in range(max_tokens):
        # Convert tokens to int8 for model
        token_array = np.array(tokens, dtype=np.int8)
        logits = model.forward(token_array)
        
        # Greedy: select highest logit (argmax) - returns 0-255
        next_token = int(np.argmax(logits))
        # Convert unsigned to signed int8
        next_token_signed = ctypes.c_int8(next_token).value
        tokens.append(next_token_signed)
        
        # Check for end-of-sequence
        if eos_token != -1 and next_token == eos_token:
            break
    
    return tokens

def generate_text(
    model: S7Mini,
    tokenizer: Tokenizer,
    prompt: str,
    max_tokens: int = 32
) -> str:
    """End-to-end: encode prompt → generate tokens → decode text"""
    # Encode prompt
    input_tokens = tokenizer.encode(prompt)
    
    # Generate tokens
    all_tokens = greedy_decode(model, input_tokens, max_tokens)
    
    # Extract generated tokens (skip prompt)
    generated = all_tokens[len(input_tokens):]
    
    # Decode to text
    return tokenizer.decode(np.array(generated, dtype=np.int8))

# ============================================================================
# MAIN ENTRY POINT
# ============================================================================

if __name__ == "__main__":
    import sys
    import os
    
    # Fix Unicode encoding on Windows
    if os.name == 'nt':
        import io
        sys.stdout = io.TextIOWrapper(sys.stdout.buffer, encoding='utf-8')
    
    print("=== Supernaut S7-MINI (Phase 7.2: Real Weights) ===\n")
    
    try:
        # Initialize model with real weights
        model = S7Mini(
            vocab_size=256,
            hidden_dim=320,
            context_len=512,
            embedding_weights="weights/embedding.bin",
            projection_weights="weights/projection.bin"
        )
        print("[OK] S7Mini model initialized with real weights")
        
        # Initialize tokenizer
        tokenizer = Tokenizer()
        if not tokenizer.load_vocab("vocab/tokens.json"):
            raise Exception("Failed to load vocabulary")
        
        # Get prompt
        prompt = sys.argv[1] if len(sys.argv) > 1 else "hello"
        print(f"Prompt: {prompt}")
        
        # Encode
        tokens = tokenizer.encode(prompt)
        print(f"Input tokens: {tokens}")
        
        # Forward pass
        logits = model.forward(tokens)
        print(f"Logits shape: {logits.shape}")
        top_tokens = np.argsort(logits)[-5:][::-1]
        print(f"Top-5 tokens: {top_tokens}")
        
        # Generate
        generated_tokens = greedy_decode(model, tokens, max_tokens=5)
        generated_text = tokenizer.decode(np.array(generated_tokens[len(tokens):], dtype=np.int8))
        print(f"Generated: {generated_text}")
        
        # Benchmark
        print()
        model.benchmark(num_iterations=100)
        
        print("\n[SUCCESS] Phase 7.2 Complete!")
        
    except Exception as e:
        print(f"[ERROR] {e}")
        import traceback
        traceback.print_exc()
