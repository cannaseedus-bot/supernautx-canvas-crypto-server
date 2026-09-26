/**
 * Phase 7.4: Supernaut API Client for run.mjs Integration
 * 
 * Provides HTTP client to communicate with Supernaut inference API
 * Handles request/response serialization, error recovery, and caching
 */

const axios = require('axios');

// ============================================================================
// CONFIGURATION
// ============================================================================

const SUPERNAUT_API = process.env.SUPERNAUT_API || 'http://localhost:5775';
const API_TIMEOUT = 5000;  // 5s timeout

// ============================================================================
// CLIENT
// ============================================================================

class SupernautAPIClient {
  constructor(baseURL = SUPERNAUT_API) {
    this.baseURL = baseURL;
    this.client = axios.create({
      baseURL: this.baseURL,
      timeout: API_TIMEOUT,
      validateStatus: () => true  // Don't throw on any status
    });
    this.cache = new Map();
    this.stats = {
      requests: 0,
      successes: 0,
      errors: 0,
      cacheHits: 0,
      totalLatency: 0
    };
  }

  /**
   * Check API health
   * @returns {Promise<{healthy: boolean, status: string}>}
   */
  async health() {
    try {
      const start = Date.now();
      const response = await this.client.get('/health');
      const latency = Date.now() - start;
      
      this.stats.requests++;
      if (response.status === 200) {
        this.stats.successes++;
        this.stats.totalLatency += latency;
        return { healthy: true, status: response.data.status };
      } else {
        this.stats.errors++;
        return { healthy: false, status: 'unhealthy' };
      }
    } catch (error) {
      this.stats.errors++;
      return { healthy: false, status: 'unreachable' };
    }
  }

  /**
   * Get model metadata
   * @returns {Promise<Object>}
   */
  async getModelInfo() {
    try {
      const start = Date.now();
      const response = await this.client.get('/model-info');
      const latency = Date.now() - start;
      
      this.stats.requests++;
      if (response.status === 200) {
        this.stats.successes++;
        this.stats.totalLatency += latency;
        return response.data.model;
      } else {
        throw new Error(`Model info failed: ${response.status}`);
      }
    } catch (error) {
      this.stats.errors++;
      throw error;
    }
  }

  /**
   * Tokenize text
   * @param {string} text - Input text
   * @returns {Promise<{tokens: number[], tokenCount: number}>}
   */
  async tokenize(text) {
    // Check cache
    const cacheKey = `tokenize:${text}`;
    if (this.cache.has(cacheKey)) {
      this.stats.cacheHits++;
      return this.cache.get(cacheKey);
    }

    try {
      const start = Date.now();
      const response = await this.client.post('/tokenize', { text });
      const latency = Date.now() - start;
      
      this.stats.requests++;
      if (response.status === 200) {
        this.stats.successes++;
        this.stats.totalLatency += latency;
        
        const result = {
          tokens: response.data.tokens,
          tokenCount: response.data.token_count
        };
        
        // Cache result
        this.cache.set(cacheKey, result);
        
        return result;
      } else {
        throw new Error(`Tokenize failed: ${response.status} - ${response.data?.error}`);
      }
    } catch (error) {
      this.stats.errors++;
      throw error;
    }
  }

  /**
   * Forward pass
   * @param {number[]} tokens - Token IDs
   * @returns {Promise<{logits: number[], confidence: number}>}
   */
  async forward(tokens) {
    // Check cache
    const cacheKey = `forward:${tokens.join(',')}`;
    if (this.cache.has(cacheKey)) {
      this.stats.cacheHits++;
      return this.cache.get(cacheKey);
    }

    try {
      const start = Date.now();
      const response = await this.client.post('/forward', { tokens });
      const latency = Date.now() - start;
      
      this.stats.requests++;
      if (response.status === 200) {
        this.stats.successes++;
        this.stats.totalLatency += latency;
        
        const result = {
          logits: response.data.logits,
          confidence: response.data.confidence
        };
        
        // Cache result
        this.cache.set(cacheKey, result);
        
        return result;
      } else {
        throw new Error(`Forward failed: ${response.status} - ${response.data?.error}`);
      }
    } catch (error) {
      this.stats.errors++;
      throw error;
    }
  }

  /**
   * Generate text (autoregressive)
   * @param {string} prompt - Input prompt
   * @param {number} maxLength - Maximum sequence length
   * @returns {Promise<{tokens: number[], text: string}>}
   */
  async generate(prompt, maxLength = 50) {
    try {
      const start = Date.now();
      const response = await this.client.post('/generate', { 
        prompt, 
        max_length: maxLength 
      });
      const latency = Date.now() - start;
      
      this.stats.requests++;
      if (response.status === 200) {
        this.stats.successes++;
        this.stats.totalLatency += latency;
        
        return {
          tokens: response.data.generated_tokens,
          text: response.data.generated_text
        };
      } else {
        throw new Error(`Generate failed: ${response.status} - ${response.data?.error}`);
      }
    } catch (error) {
      this.stats.errors++;
      throw error;
    }
  }

  /**
   * Get client statistics
   * @returns {Object}
   */
  getStats() {
    return {
      ...this.stats,
      avgLatency: this.stats.requests > 0 
        ? (this.stats.totalLatency / this.stats.successes).toFixed(2) 
        : 0,
      cacheSize: this.cache.size,
      hitRate: this.stats.requests > 0
        ? ((this.stats.cacheHits / this.stats.requests) * 100).toFixed(2)
        : 0
    };
  }

  /**
   * Clear cache
   */
  clearCache() {
    this.cache.clear();
  }
}

// ============================================================================
// MIDDLEWARE FOR run.mjs
// ============================================================================

/**
 * Create Supernaut middleware for Express
 * @param {Express.Application} app 
 */
function integrateSupernautAPI(app) {
  const client = new SupernautAPIClient();

  // Health endpoint
  app.get('/supernaut/health', async (req, res) => {
    try {
      const health = await client.health();
      res.json(health);
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // Model info endpoint
  app.get('/supernaut/model-info', async (req, res) => {
    try {
      const info = await client.getModelInfo();
      res.json({ status: 'ok', model: info });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // Tokenize endpoint
  app.post('/supernaut/tokenize', async (req, res) => {
    try {
      const { text } = req.body;
      if (!text) {
        return res.status(400).json({ error: 'Missing text field' });
      }
      const result = await client.tokenize(text);
      res.json({ status: 'ok', ...result });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // Forward endpoint
  app.post('/supernaut/forward', async (req, res) => {
    try {
      const { tokens } = req.body;
      if (!tokens) {
        return res.status(400).json({ error: 'Missing tokens field' });
      }
      const result = await client.forward(tokens);
      res.json({ status: 'ok', ...result });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // Generate endpoint
  app.post('/supernaut/generate', async (req, res) => {
    try {
      const { prompt, max_length } = req.body;
      if (!prompt) {
        return res.status(400).json({ error: 'Missing prompt field' });
      }
      const result = await client.generate(prompt, max_length || 50);
      res.json({ status: 'ok', ...result });
    } catch (error) {
      res.status(500).json({ error: error.message });
    }
  });

  // Stats endpoint
  app.get('/supernaut/stats', (req, res) => {
    res.json({ status: 'ok', stats: client.getStats() });
  });

  return client;
}

// ============================================================================
// EXPORTS
// ============================================================================

module.exports = {
  SupernautAPIClient,
  integrateSupernautAPI
};
