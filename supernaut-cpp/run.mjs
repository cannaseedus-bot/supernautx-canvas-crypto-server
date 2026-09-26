#!/usr/bin/env node
/**
 * Phase 7.5: Standalone Supernaut Orchestration Service
 * 
 * Orchestrates 21 specialist micronauts for deep thinking mode.
 * Runs independently - users activate Supernaut mode when they need orchestration.
 * 
 * Architecture:
 * - Query Router: Domain classification + specialist selection
 * - Specialist Registry: 21 micronauts (7 coding, 7 math, 7 reasoning)
 * - Assembly Line Executor: Parallel execution with timeouts
 * - Result Aggregator: Consensus voting + confidence scoring
 * - Health Monitor: Auto-checks specialist health every 30s
 * 
 * REST API: 12 endpoints on /orchestrate/* (see below)
 */

import http from 'http';
import path from 'path';
import { fileURLToPath } from 'url';

import { SupernautOrchestrator } from './supernaut-orchestrator.js';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const PORT = Number(process.env.SUPERNAUT_PORT || 5774);

// ============================================================================
// STATE & UTILITIES
// ============================================================================

const state = {
  startedAt: Date.now(),
  requests: 0,
  errors: 0,
  latencies: [],
  orchestrator: null
};

function send(res, code, payload) {
  res.writeHead(code, { 'content-type': 'application/json' });
  res.end(JSON.stringify(payload, null, 2));
}

function parseBody(req) {
  return new Promise((resolve, reject) => {
    let raw = '';
    req.on('data', (chunk) => {
      raw += chunk;
      if (raw.length > 8_000_000) reject(new Error('Body too large'));
    });
    req.on('end', () => {
      if (!raw.trim()) return resolve({});
      try { resolve(JSON.parse(raw)); } catch { reject(new Error('Invalid JSON')); }
    });
    req.on('error', reject);
  });
}

/**
 * Parse URL to extract path and query parameters
 */
function parseUrl(url) {
  const [pathname, search] = url.split('?');
  const params = new URLSearchParams(search || '');
  return { pathname, params };
}

/**
 * Extract route parameters (e.g., /orchestrate/specialist/:id)
 */
function extractRouteParams(pattern, pathname) {
  const patternParts = pattern.split('/').filter(Boolean);
  const pathParts = pathname.split('/').filter(Boolean);

  if (patternParts.length !== pathParts.length) return null;

  const params = {};
  for (let i = 0; i < patternParts.length; i++) {
    if (patternParts[i].startsWith(':')) {
      const paramName = patternParts[i].slice(1);
      params[paramName] = pathParts[i];
    } else if (patternParts[i] !== pathParts[i]) {
      return null;
    }
  }
  return params;
}

// ============================================================================
// HTTP REQUEST HANDLER
// ============================================================================

const server = http.createServer(async (req, res) => {
  const started = Date.now();
  state.requests += 1;

  try {
    const { pathname, params: queryParams } = parseUrl(req.url);
    const method = req.method;

    // ===== ORCHESTRATION ENDPOINTS =====

    // GET /orchestrate/status
    if (method === 'GET' && pathname === '/orchestrate/status') {
      const status = state.orchestrator.getStatus();
      return send(res, 200, { status: 'ok', data: status });
    }

    // GET /orchestrate/registry-summary
    if (method === 'GET' && pathname === '/orchestrate/registry-summary') {
      const summary = state.orchestrator.getRegistrySummary();
      return send(res, 200, { status: 'ok', summary });
    }

    // GET /orchestrate/specialists
    if (method === 'GET' && pathname === '/orchestrate/specialists') {
      const domain = queryParams.get('domain');
      const specialists = state.orchestrator.getSpecialists(domain);
      return send(res, 200, {
        status: 'ok',
        domain: domain || 'all',
        count: specialists.length,
        specialists
      });
    }

    // GET /orchestrate/specialist/:id
    const specialistMatch = extractRouteParams('orchestrate/specialist/:id', pathname);
    if (method === 'GET' && specialistMatch) {
      const detail = state.orchestrator.getSpecialistDetail(specialistMatch.id);
      if (!detail) {
        return send(res, 404, { error: 'specialist_not_found' });
      }
      return send(res, 200, { status: 'ok', detail });
    }

    // POST /orchestrate/route
    if (method === 'POST' && pathname === '/orchestrate/route') {
      const body = await parseBody(req);
      if (!body.query) {
        return send(res, 400, { error: 'query_required' });
      }

      const topN = body.topN || 3;
      const route = await state.orchestrator.queryRouter.route(body.query, topN);

      return send(res, 200, {
        status: 'ok',
        query: body.query,
        domain: route.domain,
        confidence: route.domainConfidence,
        specialists: route.specialists
      });
    }

    // POST /orchestrate/execute
    if (method === 'POST' && pathname === '/orchestrate/execute') {
      const body = await parseBody(req);
      if (!body.query) {
        return send(res, 400, { error: 'query_required' });
      }

      const options = {
        topN: body.topN || 3,
        strategy: body.strategy || 'weighted',
        timeout: body.timeout || 5000
      };

      const result = await state.orchestrator.execute(body.query, options);

      return send(res, 200, {
        status: 'ok',
        query: body.query,
        result
      });
    }

    // POST /orchestrate/health-check
    if (method === 'POST' && pathname === '/orchestrate/health-check') {
      const health = await state.orchestrator.checkHealth();
      return send(res, 200, {
        status: 'ok',
        health
      });
    }

    // GET /orchestrate/stats
    if (method === 'GET' && pathname === '/orchestrate/stats') {
      const status = state.orchestrator.getStatus();
      return send(res, 200, {
        status: 'ok',
        stats: status.stats,
        specialists: status.specialists
      });
    }

    // GET /orchestrate/queue
    if (method === 'GET' && pathname === '/orchestrate/queue') {
      const queueStatus = state.orchestrator.queue.getStatus();
      return send(res, 200, {
        status: 'ok',
        queue: queueStatus
      });
    }

    // ===== SERVICE HEALTH ENDPOINTS =====

    if (method === 'GET' && pathname === '/health') {
      return send(res, 200, {
        service: 'supernaut-orchestrator',
        status: 'ok',
        port: PORT,
        mode: 'orchestration',
        role: 'control_plane',
        specialists_total: 21,
        specialists_healthy: state.orchestrator.registry.getHealthySpecialists().length
      });
    }

    if (method === 'GET' && pathname === '/metrics') {
      const uptime = Math.max(1, (Date.now() - state.startedAt) / 1000);
      const avgLatency = state.latencies.length
        ? (state.latencies.reduce((a, b) => a + b, 0) / state.latencies.length).toFixed(2)
        : 0;

      return send(res, 200, {
        service: 'supernaut-orchestrator',
        uptime_seconds: uptime,
        requests_total: state.requests,
        errors_total: state.errors,
        avg_latency_ms: avgLatency,
        query_cache_size: state.orchestrator.queryRouter.cache.size || 0
      });
    }

    // ===== ROOT/INFO =====

    if (method === 'GET' && (pathname === '/' || pathname === '/info')) {
      return send(res, 200, {
        service: 'supernaut-orchestrator',
        version: '7.5',
        mode: 'deep-thinking',
        description: 'Multi-micronaut orchestration service for deep thinking mode',
        endpoints: {
          status: 'GET /orchestrate/status',
          registry: 'GET /orchestrate/registry-summary',
          specialists: 'GET /orchestrate/specialists?domain=code_review',
          specialist_detail: 'GET /orchestrate/specialist/:id',
          route: 'POST /orchestrate/route',
          execute: 'POST /orchestrate/execute',
          health_check: 'POST /orchestrate/health-check',
          stats: 'GET /orchestrate/stats',
          queue: 'GET /orchestrate/queue',
          service_health: 'GET /health',
          metrics: 'GET /metrics'
        },
        specialists: {
          total: 21,
          domains: {
            coding: 7,
            math: 7,
            reasoning: 7
          }
        }
      });
    }

    // 404
    return send(res, 404, { error: 'not_found', path: pathname });

  } catch (err) {
    state.errors += 1;
    console.error('❌ Request error:', err.message);
    return send(res, 500, {
      error: 'internal_error',
      message: err.message
    });
  } finally {
    state.latencies.push(Date.now() - started);
    if (state.latencies.length > 500) state.latencies.shift();
  }
});

// ============================================================================
// STARTUP
// ============================================================================

server.listen(PORT, '0.0.0.0', () => {
  console.log('');
  console.log('╔═══════════════════════════════════════════════════════════════╗');
  console.log('║  Phase 7.5: Supernaut Orchestration Service - ACTIVE           ║');
  console.log('╚═══════════════════════════════════════════════════════════════╝');
  console.log('');
  console.log(`🚀 Listening on port ${PORT} (supernaut mode)`);
  console.log('');
  console.log('📋 Specialist Registry:');
  console.log('   - 7 Coding specialists (review, refactor, optimize, test, docs, debug, arch)');
  console.log('   - 7 Math specialists (math, calculus, algebra, geometry, stats, theory, linear)');
  console.log('   - 7 Reasoning specialists (logic, deduction, induction, analogy, synthesis, verify)');
  console.log('');
  console.log('⚙️  Health Monitoring:');
  console.log('   - Auto-checks every 30 seconds');
  console.log('   - Tracks: success_rate, accuracy, latency, uptime');
  console.log('');
  console.log('🔗 REST API:');
  console.log(`   POST  http://localhost:${PORT}/orchestrate/execute       (deep thinking)`);
  console.log(`   GET   http://localhost:${PORT}/orchestrate/status        (status check)`);
  console.log(`   GET   http://localhost:${PORT}/orchestrate/specialists   (list all)`);
  console.log(`   POST  http://localhost:${PORT}/orchestrate/route         (domain routing)`);
  console.log(`   GET   http://localhost:${PORT}/health                    (service health)`);
  console.log('');
  console.log('✨ Supernaut ready for orchestration queries!');
  console.log('');

  // Initialize orchestrator
  try {
    state.orchestrator = new SupernautOrchestrator({
      supernautClient: null,
      defaultTimeout: 5000,
      maxQueueSize: 100
    });
    console.log('✅ Orchestrator initialized with all 21 specialists');
  } catch (err) {
    console.error('❌ Failed to initialize orchestrator:', err.message);
    process.exit(1);
  }
});

// Graceful shutdown
process.on('SIGINT', () => {
  console.log('\n⏹️  Shutting down Supernaut orchestrator...');
  if (state.orchestrator) {
    state.orchestrator.shutdown();
  }
  server.close(() => {
    console.log('✅ Supernaut orchestrator stopped');
    process.exit(0);
  });
});

process.on('SIGTERM', () => {
  if (state.orchestrator) {
    state.orchestrator.shutdown();
  }
  server.close(() => process.exit(0));
});

export { server, state };
