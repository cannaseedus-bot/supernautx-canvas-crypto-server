# Phase 7.5/7.7: Quick Reference Guide

## 🚀 Start Services

### ESM Orchestrator (Phase 7.5)
```bash
node supernaut-cpp/run.mjs
# Port: 5774
```

### Native C++ Orchestrator (Phase 7.7)
```bash
cd supernaut/supernaut-cpp/native/build/bin/Debug
./supernaut_native.exe
# Port: 5776
```

---

## 🎯 Common Operations (Native Layer)

### 1. Semantic Edict (Primary Dispatch)
```bash
curl -X POST http://localhost:5776/edict \
  -H "Content-Type: application/json" \
  -d '{"edict": "optimize the kernel subspace"}'
```

### 2. Semantic Kernel Invoke (Plugin Call)
```bash
curl -X POST http://localhost:5776/kernel/invoke \
  -H "Content-Type: application/json" \
  -d '{
    "plugin": "micro_build_agent",
    "function": "patch",
    "arguments": {"query": "fix code"}
  }'
```

### 3. Stateful Agent Interaction
```bash
# 1. Create context
curl http://localhost:5776/thread/create

# 2. Invoke Career Coach (Resume Intelligence)
curl -X POST http://localhost:5776/agent/invoke \
  -H "Content-Type: application/json" \
  -d '{
    "agent_id": "CareerCoach",
    "thread_id": "thread_123",
    "input": "ExtractSkills from this JSON resume"
  }'

# 3. Invoke Manifest Architect
curl -X POST http://localhost:5776/agent/invoke \
  -H "Content-Type: application/json" \
  -d '{
    "agent_id": "ManifestArchitect",
    "thread_id": "thread_123",
    "input": "BuildAgentManifest for a GPU Optimizer"
  }'
```

### 4. Discovery Endpoints
- `GET /health` → Kernel status
- `GET /agents` → Active swarm registry
- `GET /tools` → Semantic tool map
- `GET /skills` → Micronaut skill database
- `GET /commands` → Opcode registry

---

## 📋 21 Specialists Available

### Coding (7)
- `code-review-1` - Code quality review
- `refactoring-1` - Code restructuring
- `optimization-1` - Performance tuning
- `testing-1` - Test coverage
- `documentation-1` - API documentation
- `debugging-1` - Bug finding
- `architecture-1` - System design

### Math (7)
- `math-1` - General math
- `calculus-1` - Derivatives & integrals
- `algebra-1` - Symbolic math
- `geometry-1` - Spatial reasoning
- `statistics-1` - Data analysis
- `number-theory-1` - Prime factorization
- `linear-algebra-1` - Matrices & tensors

### Reasoning (7)
- `reasoning-1` - Logical inference
- `logic-1` - Formal logic
- `deduction-1` - Top-down reasoning
- `induction-1` - Bottom-up patterns
- `analogy-1` - Similarity mapping
- `synthesis-1` - Combination
- `verification-1` - Proof checking

---

## ⚙️ Configuration

### Environment Variables
```bash
# Custom port
SUPERNAUT_PORT=5775

# Run with custom port
SUPERNAUT_PORT=5775 node run.mjs
```

### Request Options
```json
{
  "query": "your question",
  "topN": 3,              // 1-21 specialists
  "strategy": "weighted", // weighted|majority|highest|consensus
  "timeout": 5000         // milliseconds
}
```

---

## 📊 Performance Targets

| Metric | Value |
|--------|-------|
| Startup time | <2 seconds |
| Single specialist latency | 200-400ms |
| Full orchestration (3 specialists) | 215-325ms |
| Throughput | 3-5 queries/second |
| Query cache hit rate | ~25% |
| Memory usage | 50-150 MB |

---

## 🏥 Health Monitoring

Auto-checks run every 30 seconds.

### Manual Health Check
```bash
curl -X POST http://localhost:5774/orchestrate/health-check
```

### View Specialist Health
```bash
curl http://localhost:5774/orchestrate/specialist/optimization-1
```

---

## 📝 Documentation

- **Main Guide:** `PHASE_7_5_STANDALONE_SERVICE.md` (comprehensive)
- **API Examples:** See PHASE_7_5_STANDALONE_SERVICE.md
- **Architecture:** Two-mode system (orchestration + micro-fast)

---

## 🔧 Troubleshooting

### Service won't start
```bash
# Check port is available
netstat -an | grep 5774

# Try different port
SUPERNAUT_PORT=5775 node run.mjs
```

### Query timing out
```bash
# Increase timeout
{"query": "...", "timeout": 10000}
```

### Specialists showing unhealthy
```bash
# Restart service (Ctrl+C then restart)
# Or manual health check
POST /orchestrate/health-check
```

---

## 🎓 Two Operating Modes

### Mode 1: Supernaut Orchestration (This Service)
```
port: 5774
query → multiple specialists → consensus → result
latency: 215-325ms
use for: complex decisions, deep thinking
```

### Mode 2: Micro-Fast Mode (Individual Micronauts)
```
port: varies by micronaut
query → single specialist → result
latency: 100-200ms
use for: quick questions, specific domains
```

---

## 🚦 Status Indicator

All systems operational if:
- ✅ Service starts without errors
- ✅ `/health` returns 21/21 specialists healthy
- ✅ `/orchestrate/execute` responds in <500ms
- ✅ `/metrics` shows success rate >90%

---

## 📞 Integration

For other services to call Supernaut when needed:

```javascript
const response = await fetch('http://localhost:5774/orchestrate/execute', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    query: userQuery,
    topN: 5,
    strategy: 'weighted'
  })
});

const result = await response.json();
console.log(result.result);  // The orchestrated response
```

---

**Phase 7.5: Complete & Production-Ready** ✅

For detailed documentation, see `PHASE_7_5_STANDALONE_SERVICE.md`
