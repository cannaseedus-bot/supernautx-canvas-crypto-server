/**
 * Phase 7.5.3: Assembly Line Executor for Supernaut Orchestration
 * 
 * Executes queries through specialist micronauts in parallel,
 * with timeouts, error handling, and result aggregation.
 */

// ============================================================================
// ASSEMBLY LINE EXECUTOR
// ============================================================================

class AssemblyLineExecutor {
  constructor(options = {}) {
    this.queryRouter = options.queryRouter;
    this.registry = options.registry;
    this.supernautClient = options.supernautClient;
    this.defaultTimeout = options.defaultTimeout || 5000;
    this.maxParallel = options.maxParallel || 21;
    this.stats = {
      executions: 0,
      successes: 0,
      failures: 0,
      totalTime: 0
    };
  }

  /**
   * Execute query through single specialist (mock)
   */
  async executeSpecialist(specialistId, query, timeoutMs = 2000) {
    const specialist = this.registry.get(specialistId);
    if (!specialist) {
      throw new Error(`Specialist ${specialistId} not found`);
    }

    return new Promise((resolve, reject) => {
      const timer = setTimeout(() => {
        reject(new Error(`Timeout executing ${specialistId} (${timeoutMs}ms)`));
      }, timeoutMs);

      try {
        // Mock execution: simulate specialist analysis
        const mockResult = this.mockSpecialistResponse(specialist, query);
        clearTimeout(timer);
        
        // Record metrics
        const latency = Math.floor(Math.random() * 200) + 50;
        const success = true;
        this.registry.recordRequest(specialistId, success, latency, mockResult.confidence);

        resolve({
          specialist: specialistId,
          result: mockResult.result,
          confidence: mockResult.confidence,
          latency,
          timestamp: Date.now()
        });
      } catch (error) {
        clearTimeout(timer);
        this.registry.recordRequest(specialistId, false, 5000, 0);
        reject(error);
      }
    });
  }

  /**
   * Generate mock specialist response
   */
  mockSpecialistResponse(specialist, query) {
    // Mock logic: different responses per domain
    const responses = {
      code_review: {
        result: `Code review for: "${query.substring(0, 40)}..."`,
        confidence: 0.85 + Math.random() * 0.1
      },
      refactoring: {
        result: `Refactoring suggestion for: "${query.substring(0, 40)}..."`,
        confidence: 0.82 + Math.random() * 0.12
      },
      optimization: {
        result: `Optimization recommendation: "${query.substring(0, 40)}..."`,
        confidence: 0.88 + Math.random() * 0.1
      },
      testing: {
        result: `Test strategy for: "${query.substring(0, 40)}..."`,
        confidence: 0.80 + Math.random() * 0.15
      },
      math: {
        result: `Mathematical solution to: "${query.substring(0, 40)}..."`,
        confidence: 0.90 + Math.random() * 0.08
      },
      reasoning: {
        result: `Reasoning analysis: "${query.substring(0, 40)}..."`,
        confidence: 0.83 + Math.random() * 0.12
      }
    };

    const response = responses[specialist.domain] || {
      result: `Analysis: "${query.substring(0, 40)}..."`,
      confidence: 0.75 + Math.random() * 0.2
    };

    return {
      result: response.result,
      confidence: Math.min(response.confidence, 1.0)
    };
  }

  /**
   * Execute query through multiple specialists in parallel
   */
  async executeParallel(query, specialists, timeoutMs = 2000) {
    if (!specialists || specialists.length === 0) {
      return [];
    }

    // Limit concurrency
    const toExecute = specialists.slice(0, this.maxParallel);

    const promises = toExecute.map(specialistId =>
      this.executeSpecialist(specialistId, query, timeoutMs)
        .catch(error => ({
          specialist: specialistId,
          error: error.message,
          result: null,
          confidence: 0,
          timestamp: Date.now()
        }))
    );

    return Promise.all(promises);
  }

  /**
   * Full assembly line execution
   */
  async execute(query, options = {}) {
    const startTime = Date.now();

    try {
      this.stats.executions++;

      const {
        topN = 3,
        timeoutMs = this.defaultTimeout
      } = options;

      // Stage 1: Route query
      const route = await this.queryRouter.route(query, topN);
      const { specialists, domain, domainConfidence } = route;

      // Stage 2: Execute in parallel
      const execTimeout = Math.min(2000, timeoutMs / 2);
      const results = await this.executeParallel(query, specialists, execTimeout);

      // Stage 3: Filter successful results
      const successful = results.filter(r => !r.error && r.result);

      if (successful.length === 0) {
        // Fallback to MM-1
        return {
          query,
          domain,
          status: 'fallback_mm1',
          result: `[MM-1 Fallback] Could not get specialist responses for: "${query.substring(0, 40)}..."`,
          confidence: 0.5,
          specialists: [],
          execution_time_ms: Date.now() - startTime,
          timestamp: Date.now()
        };
      }

      const executionTime = Date.now() - startTime;
      this.stats.successes++;
      this.stats.totalTime += executionTime;

      return {
        query,
        domain,
        domainConfidence,
        status: 'ok',
        results: successful,
        final_result: successful[0].result, // Use highest confidence
        final_confidence: successful[0].confidence,
        execution_time_ms: executionTime,
        specialists_executed: successful.length,
        timestamp: Date.now()
      };

    } catch (error) {
      this.stats.failures++;
      throw error;
    }
  }

  /**
   * Get execution statistics
   */
  getStats() {
    return {
      executions: this.stats.executions,
      successes: this.stats.successes,
      failures: this.stats.failures,
      success_rate: this.stats.executions > 0 
        ? ((this.stats.successes / this.stats.executions) * 100).toFixed(2)
        : 0,
      avg_time_ms: this.stats.successes > 0
        ? (this.stats.totalTime / this.stats.successes).toFixed(2)
        : 0
    };
  }
}

// ============================================================================
// EXECUTION QUEUE (for handling bursts)
// ============================================================================

class ExecutionQueue {
  constructor(executor, maxQueue = 100) {
    this.executor = executor;
    this.maxQueue = maxQueue;
    this.queue = [];
    this.processing = false;
    this.stats = {
      queued: 0,
      processed: 0,
      dropped: 0
    };
  }

  /**
   * Enqueue query execution
   */
  enqueue(query, options = {}) {
    if (this.queue.length >= this.maxQueue) {
      this.stats.dropped++;
      return {
        status: 'dropped',
        reason: 'Queue full'
      };
    }

    const id = `q_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
    this.queue.push({
      id,
      query,
      options,
      timestamp: Date.now(),
      status: 'pending'
    });

    this.stats.queued++;
    this.processQueue();

    return { status: 'queued', id, position: this.queue.length };
  }

  /**
   * Process queue sequentially
   */
  async processQueue() {
    if (this.processing) return;
    this.processing = true;

    while (this.queue.length > 0) {
      const item = this.queue.shift();
      try {
        item.status = 'executing';
        const result = await this.executor.execute(item.query, item.options);
        item.status = 'done';
        item.result = result;
        this.stats.processed++;
      } catch (error) {
        item.status = 'error';
        item.error = error.message;
      }
    }

    this.processing = false;
  }

  /**
   * Get queue status
   */
  getStatus() {
    return {
      queued_count: this.queue.length,
      processing: this.processing,
      stats: this.stats
    };
  }
}

// ============================================================================
// MIDDLEWARE
// ============================================================================

function createExecutorMiddleware(executor, queue) {
  return {
    /**
     * Express middleware
     */
    middleware: (req, res, next) => {
      req.executor = executor;
      req.queue = queue;
      next();
    },

    /**
     * Execute query endpoint
     */
    executeHandler: async (req, res) => {
      try {
        const { query, topN = 3, timeout_ms = 5000 } = req.body;
        if (!query) {
          return res.status(400).json({ error: 'Missing "query" field' });
        }

        const result = await executor.execute(query, {
          topN,
          timeoutMs: timeout_ms
        });

        res.json({ status: 'ok', result });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    },

    /**
     * Queue endpoint
     */
    queueHandler: (req, res) => {
      const { query, topN = 3 } = req.body;
      if (!query) {
        return res.status(400).json({ error: 'Missing "query" field' });
      }

      const queueResult = queue.enqueue(query, { topN });
      res.status(202).json(queueResult);
    },

    /**
     * Queue status endpoint
     */
    queueStatusHandler: (req, res) => {
      const status = queue.getStatus();
      res.json({ status: 'ok', queue: status });
    },

    /**
     * Stats endpoint
     */
    statsHandler: (req, res) => {
      const stats = executor.getStats();
      res.json({ status: 'ok', stats });
    }
  };
}

// ============================================================================
// EXPORTS
// ============================================================================

export {
  AssemblyLineExecutor,
  ExecutionQueue,
  createExecutorMiddleware
};
