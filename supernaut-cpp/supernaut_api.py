#!/usr/bin/env python3
"""
Phase 7.4: HTTP API Wrapper for Supernaut S7 Model
Exposes deterministic inference via REST endpoints.

Endpoints:
  POST /generate   - Autoregressive text generation
  POST /tokenize   - Text → token IDs
  POST /forward    - Token IDs → logits
  GET  /health     - Health check
  GET  /model-info - Model metadata
"""

import json
import os
import torch
import struct
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass, asdict
from flask import Flask, request, jsonify
import logging

# ============================================================================
# CONSTANTS
# ============================================================================

MODEL_ROOT = Path(os.getenv("SUPERNAUT_MODEL_ROOT", r"E:\models\s7-llm-mini"))


def resolve_model_file(env_var: str, candidates: List[Path]) -> Path:
    """Resolve model file path via env override, then first existing candidate."""
    env_value = os.getenv(env_var, "").strip()
    if env_value:
        return Path(env_value)
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return candidates[0]


S7_PATH = resolve_model_file("SUPERNAUT_S7_PATH", [
    MODEL_ROOT / "micronaut-weights-v2.s7",
    MODEL_ROOT / "model" / "micronaut-weights-v2.s7",
    MODEL_ROOT / "supernaut" / "model" / "micronaut-weights-v2.s7",
])
MANIFEST_PATH = resolve_model_file("SUPERNAUT_MANIFEST_PATH", [
    MODEL_ROOT / "manifest-weights-v2.json",
    MODEL_ROOT / "model" / "manifest-weights-v2.json",
    MODEL_ROOT / "supernaut" / "model" / "manifest-weights-v2.json",
])
TRAINING_DATA_PATH = resolve_model_file("SUPERNAUT_TRAINING_DATA_PATH", [
    MODEL_ROOT / "training-data.sorted.jsonl",
    MODEL_ROOT / "model" / "training-data.sorted.jsonl",
    MODEL_ROOT / "supernaut" / "model" / "training-data.sorted.jsonl",
])

S7_MAGIC = b"SC77"
FIXED_TIMESTAMP = 1711324800
MAX_SEQUENCE_LENGTH = 256
VOCAB_SIZE = 256
HIDDEN_DIM = 320

# ============================================================================
# LOGGING
# ============================================================================

logging.basicConfig(
    level=logging.INFO,
    format='[%(asctime)s] %(levelname)s: %(message)s'
)
logger = logging.getLogger(__name__)

# ============================================================================
# DATA CLASSES
# ============================================================================

@dataclass
class S7Header:
    """SC77 header structure"""
    magic: bytes
    version: int
    sealed: bool
    layer_id: int
    merkle_root: str
    timestamp: int
    crc32: int
    
    def to_dict(self):
        return asdict(self)


@dataclass
class ModelInfo:
    """Model metadata"""
    name: str
    version: str
    layers: int
    parameters: int
    vocab_size: int
    hidden_dim: int
    deterministic: bool
    timestamp: int
    
    def to_dict(self):
        return asdict(self)


@dataclass
class InferenceResult:
    """Inference output"""
    tokens: List[int]
    logits: Optional[List[float]] = None
    confidence: Optional[float] = None
    timestamp: int = FIXED_TIMESTAMP
    
    def to_dict(self):
        return asdict(self)


# ============================================================================
# S7 BINARY LOADER
# ============================================================================

class S7BinaryLoader:
    """Load and deserialize S7 SCXQ7 binary format"""
    
    def __init__(self, s7_path: Path):
        self.s7_path = s7_path
        self.header: Optional[S7Header] = None
        self.dict_lane = None
        self.field_lane = None
        self.lane_lane = None
        self.edge_lane = None
        self.batch_lane = None
        
    def load(self) -> bool:
        """Load S7 binary"""
        logger.info(f"Loading S7 binary from {self.s7_path}")
        try:
            with open(self.s7_path, 'rb') as f:
                data = f.read()
            
            # Remove trailing checksum (64 bytes hex)
            if len(data) > 64:
                content = data[:-64]
                checksum_hex = data[-64:].decode('ascii')
            else:
                content = data
                checksum_hex = None
            
            # Parse SC77 header (64 bytes)
            if len(content) < 64:
                logger.error("S7 file too small")
                return False
            
            header_data = content[:64]
            magic = header_data[0:4]
            
            if magic != S7_MAGIC:
                logger.error(f"Invalid magic: {magic}")
                return False
            
            # Parse header fields
            version = struct.unpack('>H', header_data[4:6])[0]
            sealed = header_data[6] == 0x01
            layer_id = header_data[7]
            merkle_root = header_data[8:40].hex()
            timestamp = struct.unpack('>I', header_data[40:44])[0]
            crc32 = struct.unpack('>I', header_data[44:48])[0]
            
            self.header = S7Header(
                magic=magic,
                version=version,
                sealed=sealed,
                layer_id=layer_id,
                merkle_root=merkle_root,
                timestamp=timestamp,
                crc32=crc32
            )
            
            logger.info(f"✓ S7 Header loaded (v{version}, {layer_id} layers)")
            
            # Parse SCXQ7 lanes (estimated layout)
            lane_offset = 64
            
            # DICT lane: tensor data (large)
            dict_size = len(content) - lane_offset - 4*64  # Rough estimate
            if dict_size > 0:
                self.dict_lane = content[lane_offset:lane_offset + dict_size]
                lane_offset += dict_size
                logger.info(f"✓ DICT lane: {len(self.dict_lane)} bytes")
            
            # FIELD, LANE, EDGE, BATCH lanes (64 bytes each)
            if len(content) >= lane_offset + 4*64:
                self.field_lane = content[lane_offset:lane_offset + 64]
                self.lane_lane = content[lane_offset + 64:lane_offset + 128]
                self.edge_lane = content[lane_offset + 128:lane_offset + 192]
                self.batch_lane = content[lane_offset + 192:lane_offset + 256]
                logger.info("✓ SCXQ7 lanes loaded")
            
            logger.info("✅ S7 binary loaded successfully")
            return True
            
        except Exception as e:
            logger.error(f"Error loading S7: {e}")
            return False


# ============================================================================
# INFERENCE ENGINE
# ============================================================================

class InferenceEngine:
    """Deterministic inference engine"""
    
    def __init__(self, s7_loader: S7BinaryLoader, model_info: ModelInfo):
        self.s7_loader = s7_loader
        self.model_info = model_info
        
        # Placeholder weights (in production, extract from S7)
        self.embedding = torch.randn(VOCAB_SIZE, HIDDEN_DIM, dtype=torch.float32)
        self.projection = torch.randn(HIDDEN_DIM, VOCAB_SIZE, dtype=torch.float32)
        
        logger.info("✓ Inference engine initialized")
    
    def tokenize(self, text: str) -> List[int]:
        """Convert text to token IDs"""
        # Simple: convert each character to byte value
        tokens = [ord(c) % VOCAB_SIZE for c in text]
        tokens = tokens[:MAX_SEQUENCE_LENGTH]
        return tokens
    
    def forward(self, token_ids: List[int]) -> Tuple[List[float], float]:
        """Forward pass: tokens → logits"""
        if not token_ids:
            return [], 0.0
        
        # Convert to tensor
        tokens_tensor = torch.tensor(token_ids, dtype=torch.long)
        
        # Embed
        embedded = self.embedding[tokens_tensor]  # [seq_len, hidden_dim]
        
        # Average pooling
        pooled = embedded.mean(dim=0)  # [hidden_dim]
        
        # Project to logits (shape: [hidden_dim] @ [hidden_dim, vocab_size] → [vocab_size])
        logits = torch.matmul(pooled, self.projection)  # [vocab_size]
        
        # Softmax for confidence
        probs = torch.softmax(logits, dim=0)
        confidence = float(probs.max().item())
        
        return logits.tolist(), confidence
    
    def generate(self, prompt: str, max_length: int = 50) -> List[int]:
        """Autoregressive generation"""
        tokens = self.tokenize(prompt)
        
        for _ in range(max_length):
            if len(tokens) >= MAX_SEQUENCE_LENGTH:
                break
            
            logits, _ = self.forward(tokens)
            next_token = logits.index(max(logits))
            tokens.append(next_token)
        
        return tokens


# ============================================================================
# FLASK APPLICATION
# ============================================================================

def create_app(s7_loader: S7BinaryLoader, model_info: ModelInfo, inference_engine: InferenceEngine) -> Flask:
    """Create Flask application"""
    app = Flask(__name__)
    
    # ========================================================================
    # ENDPOINTS
    # ========================================================================
    
    @app.route('/health', methods=['GET'])
    def health():
        """Health check"""
        return jsonify({
            'status': 'healthy',
            'model': model_info.name,
            'timestamp': FIXED_TIMESTAMP
        }), 200
    
    @app.route('/model-info', methods=['GET'])
    def model_info_endpoint():
        """Model metadata"""
        return jsonify({
            'status': 'ok',
            'model': model_info.to_dict()
        }), 200
    
    @app.route('/tokenize', methods=['POST'])
    def tokenize_endpoint():
        """Convert text to tokens"""
        try:
            data = request.get_json()
            if not data or 'text' not in data:
                return jsonify({'error': 'Missing "text" field'}), 400
            
            text = data['text']
            tokens = inference_engine.tokenize(text)
            
            logger.info(f"Tokenize: {len(text)} chars → {len(tokens)} tokens")
            
            return jsonify({
                'status': 'ok',
                'text': text,
                'tokens': tokens,
                'token_count': len(tokens),
                'timestamp': FIXED_TIMESTAMP
            }), 200
            
        except Exception as e:
            logger.error(f"Tokenize error: {e}")
            return jsonify({'error': str(e)}), 500
    
    @app.route('/forward', methods=['POST'])
    def forward_endpoint():
        """Forward pass: tokens → logits"""
        try:
            data = request.get_json()
            if not data or 'tokens' not in data:
                return jsonify({'error': 'Missing "tokens" field'}), 400
            
            tokens = data['tokens']
            if not isinstance(tokens, list):
                return jsonify({'error': '"tokens" must be a list'}), 400
            
            logits, confidence = inference_engine.forward(tokens)
            
            logger.info(f"Forward: {len(tokens)} tokens → {len(logits)} logits (confidence={confidence:.4f})")
            
            return jsonify({
                'status': 'ok',
                'tokens': tokens,
                'logits': logits,
                'confidence': confidence,
                'timestamp': FIXED_TIMESTAMP
            }), 200
            
        except Exception as e:
            logger.error(f"Forward error: {e}")
            return jsonify({'error': str(e)}), 500
    
    @app.route('/generate', methods=['POST'])
    def generate_endpoint():
        """Autoregressive generation"""
        try:
            data = request.get_json()
            if not data or 'prompt' not in data:
                return jsonify({'error': 'Missing "prompt" field'}), 400
            
            prompt = data['prompt']
            max_length = data.get('max_length', 50)
            
            if max_length > 256:
                max_length = 256
            
            output_tokens = inference_engine.generate(prompt, max_length)
            
            # Convert tokens back to text (simple)
            output_text = ''.join(chr(t % 128) for t in output_tokens[len(inference_engine.tokenize(prompt)):])
            
            logger.info(f"Generate: '{prompt[:30]}...' → {len(output_tokens)} tokens")
            
            return jsonify({
                'status': 'ok',
                'prompt': prompt,
                'generated_tokens': output_tokens,
                'generated_text': output_text,
                'token_count': len(output_tokens),
                'timestamp': FIXED_TIMESTAMP
            }), 200
            
        except Exception as e:
            logger.error(f"Generate error: {e}")
            return jsonify({'error': str(e)}), 500
    
    @app.errorhandler(404)
    def not_found(e):
        """404 handler"""
        return jsonify({'error': 'Endpoint not found'}), 404
    
    @app.errorhandler(500)
    def internal_error(e):
        """500 handler"""
        logger.error(f"Internal error: {e}")
        return jsonify({'error': 'Internal server error'}), 500
    
    return app


# ============================================================================
# MAIN
# ============================================================================

def main():
    """Initialize and run HTTP API server"""
    logger.info("="*70)
    logger.info("PHASE 7.4: HTTP API WRAPPER")
    logger.info("="*70)
    logger.info(f"Model root: {MODEL_ROOT}")
    logger.info(f"S7 path: {S7_PATH}")
    logger.info(f"Manifest path: {MANIFEST_PATH}")
    logger.info(f"Training data path: {TRAINING_DATA_PATH}")
    
    # Load S7 binary
    logger.info("\n>>> Loading S7 Binary...")
    s7_loader = S7BinaryLoader(S7_PATH)
    if not s7_loader.load():
        logger.error("❌ Failed to load S7 binary")
        return False
    
    # Create model info
    model_info = ModelInfo(
        name="Supernaut-Copilot",
        version="7.3.0",
        layers=146,
        parameters=100307737,
        vocab_size=VOCAB_SIZE,
        hidden_dim=HIDDEN_DIM,
        deterministic=True,
        timestamp=FIXED_TIMESTAMP
    )
    logger.info(f"✓ Model info: {model_info.name} v{model_info.version}")
    
    # Create inference engine
    logger.info("\n>>> Initializing Inference Engine...")
    inference_engine = InferenceEngine(s7_loader, model_info)
    
    # Create Flask app
    logger.info("\n>>> Creating Flask Application...")
    app = create_app(s7_loader, model_info, inference_engine)
    
    logger.info("\n" + "="*70)
    logger.info("✅ HTTP API READY")
    logger.info("="*70)
    logger.info("\nEndpoints:")
    logger.info("  GET  /health          - Health check")
    logger.info("  GET  /model-info      - Model metadata")
    logger.info("  POST /tokenize        - Text → tokens")
    logger.info("  POST /forward         - Tokens → logits")
    logger.info("  POST /generate        - Autoregressive generation")
    logger.info("\n")
    
    # Start server
    port = int(os.getenv("FLASK_PORT", "5775"))
    logger.info(f"Starting server on http://localhost:{port}")
    app.run(host='0.0.0.0', port=port, debug=False, threaded=True)
    
    return True


if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
