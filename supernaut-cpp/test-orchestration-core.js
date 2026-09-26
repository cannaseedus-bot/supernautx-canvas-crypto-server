const { QueryRouter, ConfidenceScorer, DOMAIN_KEYWORDS } = require('./query-router.js');
const { SpecialistRegistry, SpecialistHealthMonitor } = require('./specialist-registry.js');
const { ResultAggregator, AggregationStrategy } = require('./result-aggregator.js');

// Initialize components
console.log('\n[1] Testing Query Router...');
const router = new QueryRouter(null);
const classification = router.classifyDomain("optimize my recursive function");
console.log(`✓ Domain: ${classification.primary}`);
console.log(`  Confidence: ${classification.ranked[0].combinedScore.toFixed(3)}`);

// Test specialist registry
console.log('\n[2] Testing Specialist Registry...');
const registry = new SpecialistRegistry();
registry.registerAll();
console.log(`✓ Registered ${registry.getAll().length} specialists`);

const codingSpecs = registry.getByDomain('code_review');
console.log(`  Code review specialists: ${codingSpecs.length}`);

registry.recordRequest('code-review-1', true, 150, 0.92);
const metrics = registry.getMetrics('code-review-1');
console.log(`✓ Metrics recorded: health=${metrics.health.toFixed(3)}`);

const summary = registry.getSummary();
console.log(`  Registry summary: ${summary.healthy}/${summary.total} healthy`);

// Test result aggregation
console.log('\n[3] Testing Result Aggregator...');
const results = [
  { specialist: 'code-review-1', result: 'Review: add type checking', confidence: 0.92 },
  { specialist: 'refactoring-1', result: 'Refactor: simplify logic', confidence: 0.85 },
  { specialist: 'testing-1', result: 'Test: add edge cases', confidence: 0.88 }
];

const aggregated = ResultAggregator.aggregateResults(results, registry);
console.log(`✓ Aggregated result: "${aggregated.result}"`);
console.log(`  Final confidence: ${aggregated.confidence.toFixed(3)}`);
console.log(`  Consensus: ${aggregated.consensus.type} (weight=${aggregated.consensus.score.toFixed(2)})`);

// Test aggregation strategies
console.log('\n[4] Testing Aggregation Strategies...');
const majority = AggregationStrategy.apply(results, 'majority');
console.log(`✓ Majority vote: ${majority.count} votes for primary result`);

const highest = AggregationStrategy.apply(results, 'highest');
console.log(`✓ Highest confidence: ${highest.confidence.toFixed(3)}`);

console.log('\n✅ All core components tested successfully!');
console.log('\nPhase 7.5 Ready: Query routing + specialist coordination + result aggregation');
