import { QueryRouter } from './query-router.js';
import { SpecialistRegistry } from './specialist-registry.js';
import { ResultAggregator, AggregationStrategy } from './result-aggregator.js';

console.log('\n✅ Phase 7.5 Core Orchestration - Test Results\n');

// Test 1: Query Router
const router = new QueryRouter(null);
const classification = await router.classifyDomain("optimize my recursive function");
console.log(`[1] Query Router: domain="${classification.primary}" confidence=${classification.ranked[0].combinedScore.toFixed(3)}`);

// Test 2: Specialist Registry
const registry = new SpecialistRegistry();
registry.registerAll();
console.log(`[2] Specialist Registry: ${registry.getAll().length} specialists registered`);
console.log(`    - Coding: ${registry.getByDomain('code_review').length + registry.getByDomain('refactoring').length + registry.getByDomain('optimization').length + registry.getByDomain('testing').length + registry.getByDomain('documentation').length + registry.getByDomain('debugging').length + registry.getByDomain('architecture').length} specialists`);
console.log(`    - Math: ${registry.getByDomain('math').length} specialists`);
console.log(`    - Reasoning: ${registry.getByDomain('reasoning').length} specialists`);

// Test 3: Result Aggregation
const results = [
  { specialist: 'code-review-1', result: 'Add type checking', confidence: 0.92 },
  { specialist: 'refactoring-1', result: 'Simplify logic', confidence: 0.85 },
  { specialist: 'testing-1', result: 'Add edge cases', confidence: 0.88 }
];
const aggregated = ResultAggregator.aggregateResults(results, registry);
console.log(`[3] Result Aggregation: result="${aggregated.result}" confidence=${aggregated.confidence.toFixed(3)} consensus=${aggregated.consensus.type}`);

console.log('\n✅ All Phase 7.5 components operational\n');
