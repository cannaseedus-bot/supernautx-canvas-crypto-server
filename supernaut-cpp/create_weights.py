#!/usr/bin/env python3
"""
Phase 7.2: Create vocabulary and weight tensors
"""

import json
import numpy as np
from pathlib import Path

# Common English words for vocab
COMMON_WORDS = [
    # Top 100 words
    "the", "be", "to", "of", "and", "a", "in", "that", "have", "i",
    "it", "for", "not", "on", "with", "he", "as", "you", "do", "at",
    "this", "but", "his", "by", "from", "they", "we", "say", "her", "she",
    "or", "an", "will", "my", "one", "all", "would", "there", "their", "what",
    "so", "up", "out", "if", "about", "who", "get", "which", "go", "me",
    "when", "make", "can", "like", "time", "no", "just", "him", "know", "take",
    "people", "into", "year", "your", "good", "some", "could", "them", "see", "other",
    "than", "then", "now", "look", "only", "come", "its", "over", "think", "also",
    "back", "after", "use", "two", "how", "our", "work", "first", "well", "way",
    "even", "new", "want", "because", "any", "these", "give", "day", "most", "us",
    # Add more common words up to 256
    "hello", "world", "python", "code", "test", "model", "data", "neural", "network", "ai",
    "machine", "learning", "deep", "train", "inference", "vector", "matrix", "tensor", "cpu", "gpu",
    "api", "server", "client", "request", "response", "json", "xml", "parse", "encode", "decode",
    "error", "warning", "debug", "log", "print", "write", "read", "file", "path", "name",
    "type", "class", "function", "method", "object", "instance", "variable", "constant", "return", "value",
    "true", "false", "null", "none", "yes", "no", "ok", "fail", "pass", "break",
    "continue", "loop", "while", "for", "if", "else", "elif", "switch", "case", "default",
    "public", "private", "protected", "static", "final", "abstract", "interface", "import", "export", "module",
    "async", "await", "promise", "callback", "handler", "listener", "event", "emit", "subscribe", "publish",
    "cache", "memory", "storage", "database", "table", "row", "column", "query", "insert", "update",
    "delete", "select", "join", "group", "order", "limit", "offset", "where", "having", "aggregation",
    "sum", "count", "average", "min", "max", "std", "variance", "mean", "median", "mode",
    "dimension", "shape", "size", "length", "width", "height", "depth", "layer", "channel", "filter",
    "activation", "relu", "sigmoid", "tanh", "softmax", "dropout", "batch", "norm", "regularization", "loss",
    "gradient", "descent", "optimize", "learning", "rate", "epoch", "iteration", "step", "backprop", "forward",
    "weight", "bias", "parameter", "hyperparameter", "config", "setting", "option", "flag", "argument", "input",
    "output", "hidden", "embedding", "attention", "transformer", "encoder", "decoder", "sequence", "token", "vocab",
    "tokenize", "encode", "decode", "generate", "predict", "classify", "cluster", "regression", "score", "metric",
    "accuracy", "precision", "recall", "f1", "roc", "auc", "confusion", "matrix", "baseline", "benchmark",
    "baseline", "evaluate", "validate", "test", "train", "fine", "tune", "transfer", "pretrained", "checkpoint",
    "save", "load", "serialize", "deserialize", "dump", "restore", "backup", "archive", "compress", "decompress",
    "encrypt", "decrypt", "hash", "checksum", "signature", "verify", "authenticate", "authorize", "permission", "role",
]

def create_vocabulary(output_path: str = "vocab/tokens.json", vocab_size: int = 256) -> dict:
    """Create vocabulary file"""
    
    # Special tokens
    special_tokens = {
        "<pad>": 0,
        "<unk>": 1,
        "<eos>": 2,
        "<bos>": 3,
        "<sep>": 4,
        "<cls>": 5,
    }
    
    vocab = {}
    vocab.update(special_tokens)
    
    # Add common words
    for word in COMMON_WORDS[:vocab_size - len(special_tokens)]:
        if word not in vocab:
            vocab[word] = len(vocab)
    
    # Fill remaining slots with generated tokens
    while len(vocab) < vocab_size:
        token_id = len(vocab)
        vocab[f"<token_{token_id}>"] = token_id
    
    # Save
    Path(output_path).parent.mkdir(exist_ok=True, parents=True)
    with open(output_path, "w") as f:
        json.dump(vocab, f, indent=2)
    
    print(f"✓ Created vocabulary: {len(vocab)} tokens → {output_path}")
    print(f"  First 10 tokens: {list(vocab.items())[:10]}")
    
    return vocab

def create_embedding_tensor(output_path: str = "weights/embedding.bin", 
                           vocab_size: int = 256, hidden_dim: int = 320,
                           seed: int = 42) -> np.ndarray:
    """Create embedding weight tensor"""
    
    np.random.seed(seed)
    
    # Create INT8 tensor [vocab_size × hidden_dim]
    # Use small values to avoid saturation
    embedding = np.random.randint(-32, 32, size=(vocab_size, hidden_dim), dtype=np.int8)
    
    # Save
    Path(output_path).parent.mkdir(exist_ok=True, parents=True)
    embedding.tofile(output_path)
    
    print(f"✓ Created embedding tensor: {embedding.shape} → {output_path}")
    print(f"  Dtype: {embedding.dtype}, Min: {embedding.min()}, Max: {embedding.max()}")
    print(f"  File size: {Path(output_path).stat().st_size} bytes")
    
    return embedding

def create_projection_tensor(output_path: str = "weights/projection.bin",
                            hidden_dim: int = 320, vocab_size: int = 256,
                            seed: int = 42) -> np.ndarray:
    """Create projection (LM head) weight tensor"""
    
    np.random.seed(seed + 1)  # Different seed for different initialization
    
    # Create INT8 tensor [hidden_dim × vocab_size]
    projection = np.random.randint(-32, 32, size=(hidden_dim, vocab_size), dtype=np.int8)
    
    # Save
    Path(output_path).parent.mkdir(exist_ok=True, parents=True)
    projection.tofile(output_path)
    
    print(f"✓ Created projection tensor: {projection.shape} → {output_path}")
    print(f"  Dtype: {projection.dtype}, Min: {projection.min()}, Max: {projection.max()}")
    print(f"  File size: {Path(output_path).stat().st_size} bytes")
    
    return projection

def verify_tensors(vocab_path: str, embedding_path: str, projection_path: str):
    """Verify all created tensors"""
    
    print("\n=== VERIFICATION ===")
    
    # Load vocab
    with open(vocab_path, "r") as f:
        vocab = json.load(f)
    print(f"✓ Vocab verified: {len(vocab)} tokens")
    
    # Load embedding
    embedding = np.fromfile(embedding_path, dtype=np.int8)
    embedding = embedding.reshape(256, 320)
    print(f"✓ Embedding verified: shape {embedding.shape}, dtype {embedding.dtype}")
    
    # Load projection
    projection = np.fromfile(projection_path, dtype=np.int8)
    projection = projection.reshape(320, 256)
    print(f"✓ Projection verified: shape {projection.shape}, dtype {projection.dtype}")
    
    # Test forward pass
    print("\n=== TEST FORWARD PASS ===")
    
    # Simulate forward: token_id=5 → embedding → projection → logits
    token_id = 5
    token_embedding = embedding[token_id].astype(np.float32)  # [320]
    logits = token_embedding @ projection  # [256]
    
    print(f"✓ Forward pass successful")
    print(f"  Input: token_id {token_id}")
    print(f"  Embedding: shape {token_embedding.shape}")
    print(f"  Logits: shape {logits.shape}, min {logits.min():.2f}, max {logits.max():.2f}")
    print(f"  Top-3 tokens: {np.argsort(logits)[-3:][::-1]}")

if __name__ == "__main__":
    import os
    
    os.chdir("C:\\public_html\\MX2LM\\codex\\AS-XCFE\\micronaut\\s7-llm-mini\\supernaut-cpp")
    
    print("=== PHASE 7.2: CREATE WEIGHTS AND VOCAB ===\n")
    
    # Create files
    create_vocabulary("vocab/tokens.json", vocab_size=256)
    print()
    create_embedding_tensor("weights/embedding.bin", vocab_size=256, hidden_dim=320)
    print()
    create_projection_tensor("weights/projection.bin", hidden_dim=320, vocab_size=256)
    
    # Verify
    verify_tensors("vocab/tokens.json", "weights/embedding.bin", "weights/projection.bin")
    
    print("\n✅ Phase 7.2: Setup Complete!")
    print("\nNext step: python supernaut.py 'hello'")
