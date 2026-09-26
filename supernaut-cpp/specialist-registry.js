/**
 * Phase 7.5.2: Specialist Registry for Supernaut Orchestration
 * 
 * Manages 21 specialist micronauts with health tracking, capabilities,
 * and performance metrics.
 */

// ============================================================================
// SPECIALIST REGISTRY
// ============================================================================

class SpecialistRegistry {
  constructor() {
    this.specialists = new Map();
    this.healthChecks = new Map();
    this.metrics = new Map();
    this.lastUpdate = Date.now();
  }

  /**
   * Register a specialist
   */
  register(specialist) {
    const {
      id,
      name,
      domain,
      port,
      capabilities = [],
      endpoints = {}
    } = specialist;

    this.specialists.set(id, {
      id,
      name,
      domain,
      port,
      capabilities,
      endpoints,
      registered: Date.now()
    });

    // Initialize metrics
    this.metrics.set(id, {
      health: 1.0,
      latency_ms: 0,
      success_rate: 1.0,
      accuracy: 0.95,
      requests: 0,
      successes: 0,
      failures: 0,
      totalLatency: 0,
      lastCheck: Date.now()
    });

    return true;
  }

  /**
   * Register all 21 specialist micronauts
   */
  registerAll() {
    // Coding Specialists (7)
    this.register({
      id: 'code-review-1',
      name: 'Code Review Specialist',
      domain: 'code_review',
      port: 8001,
      capabilities: ['lint', 'style', 'quality', 'standards'],
      endpoints: { health: '/health', analyze: '/analyze' }
    });

    this.register({
      id: 'refactoring-1',
      name: 'Refactoring Specialist',
      domain: 'refactoring',
      port: 8002,
      capabilities: ['refactor', 'simplify', 'clean-code'],
      endpoints: { health: '/health', refactor: '/refactor' }
    });

    this.register({
      id: 'optimization-1',
      name: 'Optimization Specialist',
      domain: 'optimization',
      port: 8003,
      capabilities: ['performance', 'memory', 'speed', 'efficiency'],
      endpoints: { health: '/health', optimize: '/optimize' }
    });

    this.register({
      id: 'testing-1',
      name: 'Testing Specialist',
      domain: 'testing',
      port: 8004,
      capabilities: ['unit-testing', 'integration', 'coverage', 'mocking'],
      endpoints: { health: '/health', test: '/test' }
    });

    this.register({
      id: 'documentation-1',
      name: 'Documentation Specialist',
      domain: 'documentation',
      port: 8005,
      capabilities: ['docs', 'comments', 'examples', 'guides'],
      endpoints: { health: '/health', document: '/document' }
    });

    this.register({
      id: 'debugging-1',
      name: 'Debugging Specialist',
      domain: 'debugging',
      port: 8006,
      capabilities: ['debug', 'trace', 'error', 'diagnosis'],
      endpoints: { health: '/health', debug: '/debug' }
    });

    this.register({
      id: 'architecture-1',
      name: 'Architecture Specialist',
      domain: 'architecture',
      port: 8007,
      capabilities: ['design', 'patterns', 'structure', 'modules'],
      endpoints: { health: '/health', design: '/design' }
    });

    // Math Specialists (7)
    this.register({
      id: 'math-1',
      name: 'General Math Specialist',
      domain: 'math',
      port: 8008,
      capabilities: ['calculate', 'solve', 'formula'],
      endpoints: { health: '/health', solve: '/solve' }
    });

    this.register({
      id: 'calculus-1',
      name: 'Calculus Specialist',
      domain: 'math',
      port: 8009,
      capabilities: ['derivative', 'integral', 'limit', 'series'],
      endpoints: { health: '/health', compute: '/compute' }
    });

    this.register({
      id: 'algebra-1',
      name: 'Algebra Specialist',
      domain: 'math',
      port: 8010,
      capabilities: ['equation', 'polynomial', 'factorization', 'expansion'],
      endpoints: { health: '/health', solve: '/solve' }
    });

    this.register({
      id: 'geometry-1',
      name: 'Geometry Specialist',
      domain: 'math',
      port: 8011,
      capabilities: ['shape', 'spatial', 'transform', 'projection'],
      endpoints: { health: '/health', visualize: '/visualize' }
    });

    this.register({
      id: 'statistics-1',
      name: 'Statistics Specialist',
      domain: 'math',
      port: 8012,
      capabilities: ['probability', 'distribution', 'analysis', 'inference'],
      endpoints: { health: '/health', analyze: '/analyze' }
    });

    this.register({
      id: 'number-theory-1',
      name: 'Number Theory Specialist',
      domain: 'math',
      port: 8013,
      capabilities: ['primes', 'divisibility', 'modular', 'cryptography'],
      endpoints: { health: '/health', analyze: '/analyze' }
    });

    this.register({
      id: 'linear-algebra-1',
      name: 'Linear Algebra Specialist',
      domain: 'math',
      port: 8014,
      capabilities: ['matrix', 'vector', 'determinant', 'eigenvalue'],
      endpoints: { health: '/health', compute: '/compute' }
    });

    // Reasoning Specialists (7)
    this.register({
      id: 'reasoning-1',
      name: 'General Reasoning Specialist',
      domain: 'reasoning',
      port: 8015,
      capabilities: ['logic', 'inference', 'reasoning'],
      endpoints: { health: '/health', reason: '/reason' }
    });

    this.register({
      id: 'logic-1',
      name: 'Logic Specialist',
      domain: 'reasoning',
      port: 8016,
      capabilities: ['propositional', 'predicate', 'proof', 'validity'],
      endpoints: { health: '/health', verify: '/verify' }
    });

    this.register({
      id: 'deduction-1',
      name: 'Deductive Reasoning Specialist',
      domain: 'reasoning',
      port: 8017,
      capabilities: ['deduction', 'syllogism', 'modus-ponens'],
      endpoints: { health: '/health', deduce: '/deduce' }
    });

    this.register({
      id: 'induction-1',
      name: 'Inductive Reasoning Specialist',
      domain: 'reasoning',
      port: 8018,
      capabilities: ['induction', 'generalization', 'pattern'],
      endpoints: { health: '/health', infer: '/infer' }
    });

    this.register({
      id: 'analogy-1',
      name: 'Analogical Reasoning Specialist',
      domain: 'reasoning',
      port: 8019,
      capabilities: ['analogy', 'similarity', 'mapping'],
      endpoints: { health: '/health', analogize: '/analogize' }
    });

    this.register({
      id: 'synthesis-1',
      name: 'Synthesis Specialist',
      domain: 'reasoning',
      port: 8020,
      capabilities: ['synthesis', 'combination', 'integration'],
      endpoints: { health: '/health', synthesize: '/synthesize' }
    });

    this.register({
      id: 'verification-1',
      name: 'Verification Specialist',
      domain: 'reasoning',
      port: 8021,
      capabilities: ['verify', 'validate', 'check', 'prove'],
      endpoints: { health: '/health', verify: '/verify' }
    });
  }

  /**
   * Get specialist by ID
   */
  get(id) {
    return this.specialists.get(id);
  }

  /**
   * Get all specialists
   */
  getAll() {
    return Array.from(this.specialists.values());
  }

  /**
   * Get specialists by domain
   */
  getByDomain(domain) {
    return this.getAll().filter(s => s.domain === domain);
  }

  /**
   * Update specialist metrics
   */
  recordRequest(specialistId, success, latencyMs, accuracy = 0.95) {
    const metrics = this.metrics.get(specialistId);
    if (!metrics) return;

    metrics.requests++;
    metrics.totalLatency += latencyMs;
    metrics.latency_ms = metrics.totalLatency / metrics.requests;

    if (success) {
      metrics.successes++;
      metrics.success_rate = metrics.successes / metrics.requests;
    } else {
      metrics.failures++;
    }

    if (accuracy > 0) {
      metrics.accuracy = (metrics.accuracy * (metrics.requests - 1) + accuracy) / metrics.requests;
    }

    // Compute health score
    metrics.health = 0.5 * metrics.success_rate + 0.3 * metrics.accuracy + 0.2 * (1 - Math.min(metrics.latency_ms / 1000, 1));
    metrics.lastCheck = Date.now();
  }

  /**
   * Get specialist metrics
   */
  getMetrics(specialistId) {
    return this.metrics.get(specialistId);
  }

  /**
   * Get all metrics
   */
  getAllMetrics() {
    return Array.from(this.metrics.entries()).map(([id, metrics]) => ({
      specialist: this.specialists.get(id),
      metrics
    }));
  }

  /**
   * Check specialist health
   */
  isHealthy(specialistId) {
    const metrics = this.metrics.get(specialistId);
    if (!metrics) return false;
    
    // Healthy if health score > 0.7
    return metrics.health >= 0.7;
  }

  /**
   * Get healthy specialists
   */
  getHealthySpecialists() {
    return this.getAll().filter(s => this.isHealthy(s.id));
  }

  /**
   * Rank specialists by health
   */
  rankByHealth() {
    return this.getAll()
      .map(specialist => ({
        ...specialist,
        health: this.metrics.get(specialist.id)?.health || 0.5
      }))
      .sort((a, b) => b.health - a.health);
  }

  /**
   * Get registry summary
   */
  getSummary() {
    const all = this.getAll();
    const healthy = this.getHealthySpecialists();
    
    return {
      total: all.length,
      healthy: healthy.length,
      unhealthy: all.length - healthy.length,
      healthPercentage: ((healthy.length / all.length) * 100).toFixed(1),
      lastUpdate: this.lastUpdate,
      averageHealth: (all.reduce((sum, s) => sum + (this.metrics.get(s.id)?.health || 0.5), 0) / all.length).toFixed(3)
    };
  }
}

// ============================================================================
// HEALTH MONITOR
// ============================================================================

class SpecialistHealthMonitor {
  constructor(registry) {
    this.registry = registry;
    this.checks = new Map();
  }

  /**
   * Simulate health check (in production, would call actual specialist endpoints)
   */
  async checkHealth(specialistId, client = null) {
    const specialist = this.registry.get(specialistId);
    if (!specialist) return false;

    try {
      // Simulate check: random success (90% success rate for healthy specialist)
      const success = Math.random() < 0.9;
      const latencyMs = Math.floor(Math.random() * 200) + 50;
      const accuracy = 0.9 + Math.random() * 0.1;

      this.registry.recordRequest(specialistId, success, latencyMs, accuracy);
      return success;
    } catch (error) {
      this.registry.recordRequest(specialistId, false, 5000, 0);
      return false;
    }
  }

  /**
   * Check all specialists (parallel)
   */
  async checkAll() {
    const specialists = this.registry.getAll();
    const checks = specialists.map(s => this.checkHealth(s.id));
    
    const results = await Promise.allSettled(checks);
    return results.map((r, i) => ({
      specialist: specialists[i].id,
      healthy: r.status === 'fulfilled' && r.value === true
    }));
  }

  /**
   * Start periodic health checks
   */
  startPeriodicChecks(intervalMs = 30000) {
    this.interval = setInterval(() => this.checkAll(), intervalMs);
    console.log(`Health checks started (interval: ${intervalMs}ms)`);
  }

  /**
   * Stop periodic checks
   */
  stopPeriodicChecks() {
    if (this.interval) {
      clearInterval(this.interval);
      this.interval = null;
      console.log('Health checks stopped');
    }
  }
}

// ============================================================================
// MIDDLEWARE
// ============================================================================

function createRegistryMiddleware(registry, monitor) {
  return {
    /**
     * Express middleware to attach registry to request
     */
    middleware: (req, res, next) => {
      req.registry = registry;
      req.monitor = monitor;
      next();
    },

    /**
     * Get all specialists endpoint
     */
    getAllHandler: (req, res) => {
      const specialists = registry.getAll();
      res.json({ status: 'ok', specialists });
    },

    /**
     * Get specialist by ID endpoint
     */
    getSpecialistHandler: (req, res) => {
      const { id } = req.params;
      const specialist = registry.get(id);
      if (!specialist) {
        return res.status(404).json({ error: 'Specialist not found' });
      }
      const metrics = registry.getMetrics(id);
      res.json({ status: 'ok', specialist, metrics });
    },

    /**
     * Get registry summary endpoint
     */
    summaryHandler: (req, res) => {
      const summary = registry.getSummary();
      res.json({ status: 'ok', summary });
    },

    /**
     * Get ranked specialists endpoint
     */
    rankedHandler: (req, res) => {
      const ranked = registry.rankByHealth();
      res.json({ status: 'ok', ranked });
    },

    /**
     * Health check endpoint
     */
    healthHandler: async (req, res) => {
      try {
        const results = await monitor.checkAll();
        res.json({ status: 'ok', checks: results });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    }
  };
}

// ============================================================================
// EXPORTS
// ============================================================================

export {
  SpecialistRegistry,
  SpecialistHealthMonitor,
  createRegistryMiddleware
};
