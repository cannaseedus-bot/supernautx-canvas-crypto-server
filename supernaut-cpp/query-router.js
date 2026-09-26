/**
 * Phase 7.5.1: Query Router for Supernaut Orchestration
 * 
 * Classifies user queries by domain and selects specialist micronauts.
 * Uses S7 model logits + keyword matching for deterministic routing.
 */

// ============================================================================
// DOMAIN DEFINITIONS
// ============================================================================

const DOMAIN_KEYWORDS = {
  code_review: {
    keywords: ['review', 'quality', 'lint', 'check', 'verify', 'standard', 'convention', 'style'],
    weight: 1.0,
    specialists: ['code-review', 'testing', 'refactoring']
  },
  refactoring: {
    keywords: ['refactor', 'rewrite', 'clean', 'simplify', 'improve', 'restructure'],
    weight: 1.0,
    specialists: ['refactoring', 'code-review', 'optimization']
  },
  optimization: {
    keywords: ['optimize', 'performance', 'faster', 'efficient', 'memory', 'speed', 'cache'],
    weight: 1.0,
    specialists: ['optimization', 'refactoring', 'testing']
  },
  testing: {
    keywords: ['test', 'unit', 'integration', 'coverage', 'mock', 'suite', 'edge'],
    weight: 1.0,
    specialists: ['testing', 'code-review', 'debugging']
  },
  documentation: {
    keywords: ['doc', 'comment', 'readme', 'guide', 'example', 'tutorial', 'explain'],
    weight: 0.8,
    specialists: ['documentation', 'code-review', 'testing']
  },
  debugging: {
    keywords: ['debug', 'error', 'bug', 'crash', 'fix', 'issue', 'problem', 'trace'],
    weight: 1.0,
    specialists: ['debugging', 'testing', 'code-review']
  },
  architecture: {
    keywords: ['architecture', 'design', 'pattern', 'structure', 'module', 'layer', 'coupling'],
    weight: 1.0,
    specialists: ['architecture', 'refactoring', 'code-review']
  },
  math: {
    keywords: ['calculate', 'solve', 'equation', 'integral', 'derivative', 'formula', 'math', 'number'],
    weight: 1.0,
    specialists: ['math', 'calculus', 'algebra']
  },
  reasoning: {
    keywords: ['why', 'explain', 'reason', 'logic', 'think', 'infer', 'conclude', 'deduce'],
    weight: 1.0,
    specialists: ['reasoning', 'logic', 'verification']
  },
  other: {
    keywords: [],
    weight: 0.5,
    specialists: ['mm-1'] // Fallback to base model
  }
};

// ============================================================================
// QUERY ROUTER
// ============================================================================

class QueryRouter {
  constructor(supernautClient) {
    this.supernautClient = supernautClient;
    this.routeCache = new Map();
    this.stats = {
      routesComputed: 0,
      cacheHits: 0,
      avgConfidence: 0
    };
  }

  /**
   * Tokenize query and get S7 logits
   */
  async getQueryLogits(query) {
    try {
      if (!this.supernautClient) {
        // Fallback when client not available (testing)
        return { logits: [], confidence: 0.5 };
      }
      const tokens = await this.supernautClient.tokenize(query);
      if (!tokens.tokens || tokens.tokens.length === 0) {
        return { logits: [], confidence: 0.5 };
      }
      
      // Take first 5 tokens for forward pass (representative sample)
      const sample = tokens.tokens.slice(0, 5);
      const forward = await this.supernautClient.forward(sample);
      
      return {
        logits: forward.logits,
        confidence: forward.confidence
      };
    } catch (error) {
      console.error('Error getting logits:', error);
      return { logits: [], confidence: 0.5 };
    }
  }

  /**
   * Extract keywords from query
   */
  extractKeywords(query) {
    const normalized = query.toLowerCase();
    const words = normalized.split(/\W+/).filter(w => w.length > 2);
    return new Set(words);
  }

  /**
   * Compute keyword match score for a domain
   */
  scoreKeywordMatch(queryWords, domain) {
    const domainKeywords = DOMAIN_KEYWORDS[domain].keywords;
    if (domainKeywords.length === 0) return 0;

    const matches = domainKeywords.filter(k => queryWords.has(k)).length;
    return matches / domainKeywords.length;
  }

  /**
   * Classify query into domain(s)
   */
  async classifyDomain(query) {
    // Check cache
    const cacheKey = `classify:${query}`;
    if (this.routeCache.has(cacheKey)) {
      this.stats.cacheHits++;
      return this.routeCache.get(cacheKey);
    }

    const queryWords = this.extractKeywords(query);
    const scores = {};

    // Score each domain
    for (const [domain, config] of Object.entries(DOMAIN_KEYWORDS)) {
      const keywordScore = this.scoreKeywordMatch(queryWords, domain);
      scores[domain] = {
        keywordScore,
        weight: config.weight,
        specialists: config.specialists,
        combinedScore: keywordScore > 0 ? keywordScore * config.weight : 0
      };
    }

    // Rank domains by combined score
    const ranked = Object.entries(scores)
      .map(([domain, score]) => ({
        domain,
        ...score,
        logitsConfidence: 0.5 // Will be updated after logit fetch
      }))
      .sort((a, b) => b.combinedScore - a.combinedScore);

    // Get S7 logits for confidence boosting
    const logits = await this.getQueryLogits(query);
    
    // Boost top domain with logits confidence
    if (ranked.length > 0 && ranked[0].combinedScore === 0) {
      // No keyword matches - boost by logits alone
      ranked[0].logitsConfidence = logits.confidence;
      ranked[0].combinedScore = logits.confidence;
    } else if (ranked.length > 0) {
      // Has keyword matches - combine with logits
      ranked[0].logitsConfidence = logits.confidence;
      ranked[0].combinedScore = 0.6 * ranked[0].combinedScore + 0.4 * logits.confidence;
      ranked.sort((a, b) => b.combinedScore - a.combinedScore);
    }

    const result = {
      primary: ranked[0]?.domain || 'other',
      ranked: ranked,
      timestamp: Date.now()
    };

    // Cache result
    this.routeCache.set(cacheKey, result);

    this.stats.routesComputed++;
    this.stats.avgConfidence = (this.stats.avgConfidence * (this.stats.routesComputed - 1) + (ranked[0]?.combinedScore || 0)) / this.stats.routesComputed;

    return result;
  }

  /**
   * Route query to specialist(s)
   */
  async route(query, topN = 2) {
    const classification = await this.classifyDomain(query);
    const primaryDomain = classification.primary;
    const domainConfig = DOMAIN_KEYWORDS[primaryDomain];

    // Get primary specialists
    const specialists = domainConfig.specialists.slice(0, topN);

    return {
      query,
      domain: primaryDomain,
      domainConfidence: classification.ranked[0]?.combinedScore || 0.5,
      specialists,
      allRanked: classification.ranked,
      timestamp: Date.now()
    };
  }

  /**
   * Get routing statistics
   */
  getStats() {
    return {
      routesComputed: this.stats.routesComputed,
      cacheHits: this.stats.cacheHits,
      cacheSize: this.routeCache.size,
      avgConfidence: this.stats.avgConfidence.toFixed(4),
      hitRate: this.stats.routesComputed > 0 
        ? ((this.stats.cacheHits / this.stats.routesComputed) * 100).toFixed(2)
        : 0
    };
  }

  /**
   * Clear cache
   */
  clearCache() {
    this.routeCache.clear();
  }
}

// ============================================================================
// CONFIDENCE SCORING (COMPOSITE)
// ============================================================================

class ConfidenceScorer {
  /**
   * Compute composite confidence
   * 
   * Score = 0.4 × logits_confidence 
   *       + 0.3 × domain_match 
   *       + 0.2 × specialist_health 
   *       + 0.1 × recency
   */
  static computeConfidence(options = {}) {
    const {
      logitsConfidence = 0.5,
      domainMatch = 0.5,
      specialistHealth = 0.9,
      recency = 0.8
    } = options;

    return (
      0.4 * logitsConfidence +
      0.3 * domainMatch +
      0.2 * specialistHealth +
      0.1 * recency
    );
  }

  /**
   * Validate confidence in range [0, 1]
   */
  static validate(confidence) {
    return Math.max(0, Math.min(1, confidence));
  }

  /**
   * Get confidence level (qualitative)
   */
  static getLevel(confidence) {
    if (confidence >= 0.9) return 'very_high';
    if (confidence >= 0.75) return 'high';
    if (confidence >= 0.6) return 'moderate';
    if (confidence >= 0.4) return 'low';
    return 'very_low';
  }
}

// ============================================================================
// ROUTING MIDDLEWARE
// ============================================================================

function createRouterMiddleware(queryRouter) {
  return {
    /**
     * Express middleware to attach router to request
     */
    middleware: (req, res, next) => {
      req.queryRouter = queryRouter;
      next();
    },

    /**
     * Route endpoint handler
     */
    routeHandler: async (req, res) => {
      try {
        const { query, topN = 2 } = req.body;
        if (!query) {
          return res.status(400).json({ error: 'Missing "query" field' });
        }

        const route = await queryRouter.route(query, topN);
        res.json({ status: 'ok', route });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    },

    /**
     * Classification endpoint
     */
    classifyHandler: async (req, res) => {
      try {
        const { query } = req.body;
        if (!query) {
          return res.status(400).json({ error: 'Missing "query" field' });
        }

        const classification = await queryRouter.classifyDomain(query);
        res.json({ status: 'ok', classification });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    },

    /**
     * Stats endpoint
     */
    statsHandler: (req, res) => {
      res.json({ status: 'ok', stats: queryRouter.getStats() });
    }
  };
}

// ============================================================================
// EXPORTS
// ============================================================================

export {
  QueryRouter,
  ConfidenceScorer,
  DOMAIN_KEYWORDS,
  createRouterMiddleware
};
