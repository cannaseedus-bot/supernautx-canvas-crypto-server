/**
 * Phase 7.5: Supernaut Control Plane - Unified Orchestrator
 * 
 * Coordinates all 21 specialist micronauts using:
 * - Query routing (domain classification)
 * - Specialist selection (health + confidence scoring)
 * - Assembly line execution (parallel processing)
 * - Result aggregation (consensus voting)
 */

import { QueryRouter, ConfidenceScorer } from './query-router.js';
import { SpecialistRegistry, SpecialistHealthMonitor } from './specialist-registry.js';
import { AssemblyLineExecutor, ExecutionQueue } from './assembly-line-executor.js';
import { ResultAggregator, AggregationStrategy } from './result-aggregator.js';

// ============================================================================
// SUPERNAUT ORCHESTRATOR
// ============================================================================

class SupernautOrchestrator {
  constructor(options = {}) {
    const {
      supernautClient,
      defaultTimeout = 5000,
      maxQueueSize = 100
    } = options;

    // Initialize components
    this.queryRouter = new QueryRouter(supernautClient);
    this.registry = new SpecialistRegistry();
    this.monitor = new SpecialistHealthMonitor(this.registry);
    this.executor = new AssemblyLineExecutor({
      queryRouter: this.queryRouter,
      registry: this.registry,
      supernautClient,
      defaultTimeout
    });
    this.queue = new ExecutionQueue(this.executor, maxQueueSize);
    
    // Register all 21 specialists
    this.registry.registerAll();

    // Start health monitoring
    this.monitor.startPeriodicChecks(30000);

    // Global stats
    this.stats = {
      queries: 0,
      successes: 0,
      failures: 0,
      totalTime: 0,
      startTime: Date.now()
    };
  }

  /**
   * Execute query through full orchestration pipeline
   */
  async execute(query, options = {}) {
    const startTime = Date.now();

    try {
      this.stats.queries++;

      const {
        topN = 3,
        strategy = 'weighted',
        includeAll = false,
        timeout = 5000
      } = options;

      // Stage 1: Route query
      const route = await this.queryRouter.route(query, topN);
      const { domain, domainConfidence, specialists } = route;

      // Get specialist health for selection
      const rankedSpecialists = this.registry.rankByHealth()
        .filter(s => specialists.includes(s.id))
        .slice(0, topN);

      const selectedIds = rankedSpecialists.map(s => s.id);

      // Stage 2: Execute in parallel
      const results = await this.executor.executeParallel(query, selectedIds, timeout / 2);

      // Stage 3: Filter successful results
      const successful = results.filter(r => !r.error && r.result);

      if (successful.length === 0) {
        // Fallback to MM-1
        this.stats.failures++;
        return this.createResponse('fallback_mm1', {
          query,
          domain,
          result: '[MM-1 Fallback] No specialist responses available',
          confidence: 0.5,
          specialists: [],
          startTime
        });
      }

      // Stage 4: Aggregate results
      const aggregated = AggregationStrategy.apply(successful, strategy, this.registry);

      this.stats.successes++;
      const executionTime = Date.now() - startTime;
      this.stats.totalTime += executionTime;

      return this.createResponse('ok', {
        query,
        domain,
        domainConfidence,
        aggregated,
        execution_time_ms: executionTime,
        specialists_executed: successful.length,
        startTime
      });

    } catch (error) {
      this.stats.failures++;
      throw error;
    }
  }

  /**
   * Create unified response
   */
  createResponse(status, data) {
    return {
      status,
      query: data.query,
      domain: data.domain,
      domainConfidence: data.domainConfidence || 0.5,
      result: data.aggregated?.result || data.result,
      confidence: data.aggregated?.confidence || data.confidence || 0.5,
      consensus: data.aggregated?.consensus,
      specialists_executed: data.specialists_executed || data.specialists?.length || 0,
      weights: data.aggregated?.weights,
      execution_time_ms: data.execution_time_ms || 0,
      timestamp: Date.now()
    };
  }

  /**
   * Route query without execution
   */
  async route(query, topN = 3) {
    return this.queryRouter.route(query, topN);
  }

  /**
   * Get orchestration status
   */
  getStatus() {
    const healthy = this.registry.getHealthySpecialists().length;
    const total = this.registry.getAll().length;
    const summary = this.registry.getSummary();

    return {
      status: 'ready',
      uptime_seconds: Math.floor((Date.now() - this.stats.startTime) / 1000),
      specialists: {
        total,
        healthy,
        unhealthy: total - healthy,
        health_percentage: summary.healthPercentage
      },
      stats: {
        queries: this.stats.queries,
        successes: this.stats.successes,
        failures: this.stats.failures,
        success_rate: this.stats.queries > 0 
          ? ((this.stats.successes / this.stats.queries) * 100).toFixed(2)
          : 0,
        avg_time_ms: this.stats.successes > 0
          ? (this.stats.totalTime / this.stats.successes).toFixed(2)
          : 0
      },
      queue: this.queue.getStatus()
    };
  }

  /**
   * Get specialist information
   */
  getSpecialists(domain = null) {
    if (domain) {
      return this.registry.getByDomain(domain);
    }
    return this.registry.rankByHealth();
  }

  /**
   * Get specialist detail
   */
  getSpecialistDetail(id) {
    const specialist = this.registry.get(id);
    if (!specialist) return null;

    const metrics = this.registry.getMetrics(id);
    return {
      specialist,
      metrics,
      healthy: this.registry.isHealthy(id)
    };
  }

  /**
   * Manually check health of all specialists
   */
  async checkHealth() {
    return this.monitor.checkAll();
  }

  /**
   * Get registry summary
   */
  getRegistrySummary() {
    return this.registry.getSummary();
  }

  /**
   * Shutdown orchestrator
   */
  shutdown() {
    this.monitor.stopPeriodicChecks();
    console.log('Supernaut orchestrator shut down');
  }
}

// ============================================================================
// EXPRESS INTEGRATION
// ============================================================================

function integrateSupernautOrchestrator(app, orchestrator) {
  // ========================================================================
  // STATUS & INFO ENDPOINTS
  // ========================================================================

  app.get('/orchestrate/status', (req, res) => {
    const status = orchestrator.getStatus();
    res.json(status);
  });

  app.get('/orchestrate/registry-summary', (req, res) => {
    const summary = orchestrator.getRegistrySummary();
    res.json({ status: 'ok', summary });
  });

  // ========================================================================
  // SPECIALIST ENDPOINTS
  // ========================================================================

  app.get('/orchestrate/specialists', (req, res) => {
    const { domain } = req.query;
    const specialists = orchestrator.getSpecialists(domain);
    res.json({ status: 'ok', specialists, count: specialists.length });
  });

  app.get('/orchestrate/specialist/:id', (req, res) => {
    const detail = orchestrator.getSpecialistDetail(req.params.id);
    if (!detail) {
      return res.status(404).json({ error: 'Specialist not found' });
    }
    res.json({ status: 'ok', ...detail });
  });

  // ========================================================================
  // ROUTING ENDPOINTS
  // ========================================================================

  app.post('/orchestrate/route', async (req, res) => {
    try {
      const { query, topN = 3 } = req.body;
      if (!query) {
        return res.status(400).json({ error: 'Missing "query" field' });
      }

      const route = await orchestrator.route(query, topN);
      res.json({ status: 'ok', route });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // ========================================================================
  // EXECUTION ENDPOINTS
  // ========================================================================

  app.post('/orchestrate/execute', async (req, res) => {
    try {
      const {
        query,
        topN = 3,
        strategy = 'weighted',
        timeout = 5000
      } = req.body;

      if (!query) {
        return res.status(400).json({ error: 'Missing "query" field' });
      }

      const result = await orchestrator.execute(query, {
        topN,
        strategy,
        timeout
      });

      res.json(result);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  app.post('/orchestrate/queue', (req, res) => {
    const { query, topN = 3 } = req.body;
    if (!query) {
      return res.status(400).json({ error: 'Missing "query" field' });
    }

    const queueResult = orchestrator.queue.enqueue(query, { topN });
    res.status(202).json(queueResult);
  });

  app.get('/orchestrate/queue-status', (req, res) => {
    const status = orchestrator.queue.getStatus();
    res.json({ status: 'ok', queue: status });
  });

  // ========================================================================
  // HEALTH ENDPOINTS
  // ========================================================================

  app.post('/orchestrate/health-check', async (req, res) => {
    try {
      const results = await orchestrator.checkHealth();
      const healthy = results.filter(r => r.healthy).length;
      res.json({
        status: 'ok',
        healthy,
        total: results.length,
        checks: results
      });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // ========================================================================
  // STATS ENDPOINTS
  // ========================================================================

  app.get('/orchestrate/stats', (req, res) => {
    const status = orchestrator.getStatus();
    res.json({ status: 'ok', stats: status.stats });
  });

  app.get('/orchestrate/router-stats', (req, res) => {
    const stats = orchestrator.queryRouter.getStats();
    res.json({ status: 'ok', router_stats: stats });
  });

  app.get('/orchestrate/executor-stats', (req, res) => {
    const stats = orchestrator.executor.getStats();
    res.json({ status: 'ok', executor_stats: stats });
  });

  console.log('✓ Supernaut orchestrator endpoints registered');
}

// ============================================================================
// EXPORTS
// ============================================================================

export {
  SupernautOrchestrator,
  integrateSupernautOrchestrator
};
