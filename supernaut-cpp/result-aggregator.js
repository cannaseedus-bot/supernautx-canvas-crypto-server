/**
 * Phase 7.5.4: Result Aggregator for Supernaut Orchestration
 * 
 * Merges results from multiple specialist micronauts using
 * confidence-weighted voting and consensus scoring.
 */

// ============================================================================
// RESULT AGGREGATOR
// ============================================================================

class ResultAggregator {
  /**
   * Aggregate results using confidence-weighted voting
   * 
   * Combines multiple specialist results into unified response.
   * Weight: specialist_confidence × result_confidence × specialist_health
   */
  static aggregateResults(results, specialistRegistry = null) {
    if (!results || results.length === 0) {
      return {
        status: 'no_results',
        result: '[No results available]',
        confidence: 0,
        count: 0
      };
    }

    // Single result: return as-is
    if (results.length === 1) {
      return {
        status: 'ok',
        result: results[0].result,
        confidence: results[0].confidence,
        primary: results[0].specialist,
        count: 1,
        method: 'single'
      };
    }

    // Multiple results: apply weighted voting
    const weights = results.map((r, i) => {
      let weight = r.confidence;

      // Boost weight if specialist is healthy
      if (specialistRegistry) {
        const metrics = specialistRegistry.getMetrics(r.specialist);
        if (metrics && metrics.health > 0.7) {
          weight *= (1 + 0.2); // +20% boost for healthy
        }
      }

      return {
        index: i,
        weight,
        specialist: r.specialist,
        result: r.result,
        confidence: r.confidence
      };
    });

    // Normalize weights
    const totalWeight = weights.reduce((sum, w) => sum + w.weight, 0);
    const normalizedWeights = weights.map(w => ({
      ...w,
      normalizedWeight: w.weight / totalWeight
    }));

    // Find result with highest weight
    const primaryIdx = normalizedWeights.reduce((maxIdx, w, i) =>
      w.normalizedWeight > normalizedWeights[maxIdx].normalizedWeight ? i : maxIdx
    , 0);

    const primary = normalizedWeights[primaryIdx];

    // Compute aggregate confidence
    const aggregateConfidence = normalizedWeights.reduce((sum, w) =>
      sum + w.normalizedWeight * w.confidence
    , 0);

    // Check consensus
    const consensus = this.checkConsensus(normalizedWeights);

    return {
      status: 'ok',
      result: primary.result,
      confidence: aggregateConfidence,
      primary_specialist: primary.specialist,
      consensus,
      weights: normalizedWeights.map(w => ({
        specialist: w.specialist,
        weight: w.normalizedWeight.toFixed(3),
        confidence: w.confidence.toFixed(3)
      })),
      count: results.length,
      method: 'weighted_voting'
    };
  }

  /**
   * Check consensus among results
   */
  static checkConsensus(weights) {
    if (weights.length < 2) return { type: 'single', score: 1.0 };

    const topWeight = Math.max(...weights.map(w => w.normalizedWeight));

    // High consensus: top result has >60% weight
    if (topWeight > 0.6) {
      return {
        type: 'high',
        score: topWeight,
        message: 'Strong agreement among specialists'
      };
    }

    // Moderate consensus: top result has >40% weight
    if (topWeight > 0.4) {
      return {
        type: 'moderate',
        score: topWeight,
        message: 'General agreement with some dissent'
      };
    }

    // Low consensus: split decision
    return {
      type: 'low',
      score: topWeight,
      message: 'Split decision - consider alternative results'
    };
  }

  /**
   * Merge text results (for multi-line aggregation)
   */
  static mergeTextResults(results) {
    if (results.length === 0) return '';

    const lines = results
      .filter(r => r.result && typeof r.result === 'string')
      .map((r, i) => `[${r.specialist}]: ${r.result}`)
      .join('\n');

    return lines;
  }

  /**
   * Find outliers (results inconsistent with consensus)
   */
  static findOutliers(results, threshold = 2.0) {
    if (results.length < 3) return [];

    // Compute average confidence
    const avgConfidence = results.reduce((sum, r) => sum + r.confidence, 0) / results.length;
    const stdDev = Math.sqrt(
      results.reduce((sum, r) => sum + Math.pow(r.confidence - avgConfidence, 2), 0) / results.length
    );

    // Find outliers (>2 std devs from mean)
    return results.filter(r =>
      Math.abs(r.confidence - avgConfidence) > threshold * stdDev
    );
  }

  /**
   * Aggregate with outlier removal
   */
  static aggregateWithOutlierRemoval(results, registry = null, threshold = 2.0) {
    const outliers = this.findOutliers(results, threshold);

    if (outliers.length === 0) {
      // No outliers: use all results
      return this.aggregateResults(results, registry);
    }

    // Remove outliers and re-aggregate
    const filtered = results.filter(r =>
      !outliers.find(o => o.specialist === r.specialist)
    );

    if (filtered.length === 0) {
      // All results are outliers: use original
      return this.aggregateResults(results, registry);
    }

    const aggregated = this.aggregateResults(filtered, registry);
    aggregated.outliers_removed = outliers.length;
    aggregated.method = 'weighted_voting_outlier_removed';

    return aggregated;
  }

  /**
   * Majority voting
   */
  static majorityVote(results) {
    if (results.length === 0) return null;

    // Group by result text
    const grouped = new Map();
    results.forEach(r => {
      const key = r.result;
      if (!grouped.has(key)) {
        grouped.set(key, []);
      }
      grouped.get(key).push(r);
    });

    // Find group with most votes
    let maxCount = 0;
    let majorityResult = null;

    grouped.forEach((group, result) => {
      if (group.length > maxCount) {
        maxCount = group.length;
        majorityResult = {
          result,
          count: group.length,
          percentage: (group.length / results.length) * 100,
          specialists: group.map(r => r.specialist),
          avg_confidence: group.reduce((sum, r) => sum + r.confidence, 0) / group.length
        };
      }
    });

    return majorityResult;
  }

  /**
   * Confidence-based selection (pick highest)
   */
  static selectHighestConfidence(results) {
    if (results.length === 0) return null;

    return results.reduce((best, current) =>
      current.confidence > best.confidence ? current : best
    );
  }
}

// ============================================================================
// AGGREGATION STRATEGIES
// ============================================================================

class AggregationStrategy {
  /**
   * WEIGHTED: Confidence-weighted voting (default)
   */
  static WEIGHTED = 'weighted';

  /**
   * MAJORITY: Majority voting by result text
   */
  static MAJORITY = 'majority';

  /**
   * HIGHEST: Select result with highest confidence
   */
  static HIGHEST = 'highest';

  /**
   * CONSENSUS: Weighted voting with outlier removal
   */
  static CONSENSUS = 'consensus';

  /**
   * Apply strategy to results
   */
  static apply(results, strategy = this.WEIGHTED, registry = null) {
    switch (strategy) {
      case this.MAJORITY:
        return ResultAggregator.majorityVote(results);

      case this.HIGHEST:
        return ResultAggregator.selectHighestConfidence(results);

      case this.CONSENSUS:
        return ResultAggregator.aggregateWithOutlierRemoval(results, registry);

      case this.WEIGHTED:
      default:
        return ResultAggregator.aggregateResults(results, registry);
    }
  }
}

// ============================================================================
// AGGREGATION METRICS
// ============================================================================

class AggregationMetrics {
  constructor() {
    this.aggregations = [];
    this.stats = {
      total: 0,
      avg_confidence: 0,
      avg_count: 0,
      consensus_high: 0,
      consensus_moderate: 0,
      consensus_low: 0
    };
  }

  /**
   * Record aggregation
   */
  record(aggregation) {
    this.aggregations.push({
      ...aggregation,
      timestamp: Date.now()
    });

    // Update stats
    this.stats.total++;
    this.stats.avg_confidence = (this.stats.avg_confidence * (this.stats.total - 1) + aggregation.confidence) / this.stats.total;
    this.stats.avg_count = (this.stats.avg_count * (this.stats.total - 1) + aggregation.count) / this.stats.total;

    if (aggregation.consensus) {
      this.stats[`consensus_${aggregation.consensus.type}`]++;
    }
  }

  /**
   * Get metrics
   */
  getMetrics() {
    return {
      total_aggregations: this.stats.total,
      avg_confidence: this.stats.avg_confidence.toFixed(3),
      avg_result_count: this.stats.avg_count.toFixed(2),
      consensus_breakdown: {
        high: this.stats.consensus_high,
        moderate: this.stats.consensus_moderate,
        low: this.stats.consensus_low
      }
    };
  }
}

// ============================================================================
// MIDDLEWARE
// ============================================================================

function createAggregatorMiddleware(registry = null) {
  const metrics = new AggregationMetrics();

  return {
    /**
     * Aggregation endpoint
     */
    aggregateHandler: (req, res) => {
      try {
        const { results, strategy = 'weighted' } = req.body;
        if (!results || !Array.isArray(results)) {
          return res.status(400).json({ error: 'Missing or invalid "results" array' });
        }

        const aggregated = AggregationStrategy.apply(results, strategy, registry);
        metrics.record(aggregated);

        res.json({ status: 'ok', aggregated });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    },

    /**
     * Metrics endpoint
     */
    metricsHandler: (req, res) => {
      const metricData = metrics.getMetrics();
      res.json({ status: 'ok', metrics: metricData });
    }
  };
}

// ============================================================================
// EXPORTS
// ============================================================================

export {
  ResultAggregator,
  AggregationStrategy,
  AggregationMetrics,
  createAggregatorMiddleware
};
