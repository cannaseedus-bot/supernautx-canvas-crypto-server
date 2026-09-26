/**
 * GASNodes.gs
 * -----------
 * Google Apps Script control-plane node for MeshNet JSON peers.
 *
 * Deploy as Web App:
 * - Execute as: Me
 * - Access: Anyone (or restricted domain/user if desired)
 *
 * IMPORTANT CORS NOTE:
 * Apps Script ContentService responses do not support arbitrary response
 * headers, so classic browser CORS preflight handling is not available here.
 * Use server-to-server calls (preferred) or a proxy if browser CORS is needed.
 */

var CACHE = CacheService.getScriptCache();
var PROPERTIES = PropertiesService.getScriptProperties();

var SETTINGS = {
  PEER_ACTIVE_MS: 5 * 60 * 1000,
  PEER_STALE_MS: 10 * 60 * 1000,
  QUEUE_TTL_SEC: 6 * 60 * 60,
  MAX_QUEUE_LENGTH: 100,
  MAX_MESSAGE_BYTES: 16 * 1024,
  MAX_QUEUE_BYTES: 90 * 1024,
  MAX_PEER_ID_LEN: 64,
  MAX_BOOT_ID_LEN: 128,
  MAX_NONCE_LEN: 128,
  DEFAULT_SESSION_TTL_SEC: 60 * 60,
  DEFAULT_NONCE_TTL_SEC: 15 * 60,
  DEFAULT_ALLOWED_SKEW_MS: 5 * 60 * 1000
};

/**
 * Security bootstrap defaults ("up top"):
 * - If script properties are not set, these defaults apply.
 * - Keep SHARED_SECRET empty in source control; host should set either:
 *   1) Script Property MESHNET_SHARED_SECRET (recommended), or
 *   2) SECURITY_BOOTSTRAP.SHARED_SECRET directly in this file.
 */
var SECURITY_BOOTSTRAP = {
  SHARED_SECRET: '',
  ENFORCE_SESSIONS: true,
  REQUIRE_REGISTER_HANDSHAKE: true,
  SESSION_TTL_SEC: 60 * 60,
  NONCE_TTL_SEC: 15 * 60,
  TIMESTAMP_SKEW_MS: 5 * 60 * 1000
};

var KHANARY_CONTROL_GRAMMAR = {
  VERSION: 'khanary.control.v1',
  TOKEN_TYPES: [
    'verb',
    'command',
    'function',
    'tool_call',
    'args',
    'capability',
    'flag',
    'literal',
    'peer',
    'route'
  ],
  VERB_OPCODE: {
    invoke: 'CALL',
    call: 'CALL',
    send: 'SEND',
    query: 'QUERY',
    route: 'ROUTE',
    execute: 'EXEC',
    run: 'EXEC',
    emit: 'EMIT',
    register: 'REGISTER'
  }
};

function doGet(e) {
  return routeRequest_('GET', e || {});
}

function doPost(e) {
  return routeRequest_('POST', e || {});
}

function routeRequest_(method, e) {
  try {
    var payload = parsePayload_(method, e);
    var endpoint = resolveEndpoint_(e, payload);
    if (!endpoint) endpoint = 'status';

    validateSecurityConfiguration_(endpoint);

    if (!isAuthorized_(payload, endpoint)) {
      return jsonResponse_({
        success: false,
        error: 'unauthorized',
        message: 'Invalid shared secret'
      });
    }

    var responseData;
    if (isMutationEndpoint_(endpoint)) {
      responseData = withScriptLock_(function () {
        return dispatchEndpoint_(endpoint, payload);
      });
    } else {
      responseData = dispatchEndpoint_(endpoint, payload);
    }

    return jsonResponse_({
      success: true,
      endpoint: endpoint,
      timestamp: Date.now(),
      data: responseData
    });
  } catch (err) {
    return jsonResponse_({
      success: false,
      error: err.code || 'internal_error',
      message: err.message || String(err),
      timestamp: Date.now()
    });
  }
}

function dispatchEndpoint_(endpoint, payload) {
  switch (endpoint) {
    case 'status':
    case 'health':
      return handleStatus_();
    case 'register':
      return handleRegister_(payload);
    case 'poll':
      return handlePoll_(payload);
    case 'send':
      return handleSend_(payload);
    case 'peers':
      return handleGetPeers_(payload);
    case 'transpile':
      return handleTranspile_(payload);
    default:
      throw appError_('invalid_endpoint', 'Invalid endpoint: ' + endpoint);
  }
}

function handleStatus_() {
  cleanupOldPeers_();
  var peers = listActivePeers_();
  return {
    service: 'MeshNet GAS Backend',
    online: true,
    peers: peers.map(function (p) { return p.id; }),
    peerCount: peers.length,
    sessionEnforced: isSessionEnforced_(),
    registerHandshakeEnforced: isRegisterHandshakeEnforced_(),
    codeHashAllowlistConfigured: getCodeHashAllowlist_().length > 0,
    securityMisconfigured: isSessionEnforced_() && !getSharedSecret_()
  };
}

function handleRegister_(data) {
  var peerId = requirePeerId_(data.peerId || data.id, 'peerId');
  var now = Date.now();
  var peerMeta = asObject_(data.meta);

  var registerContext = buildRegisterContext_(
    data,
    peerId,
    isRegisterHandshakeEnforced_()
  );

  var peerData = {
    id: peerId,
    lastSeen: now,
    registeredAt: now,
    lastRegisterAt: now,
    meta: peerMeta,
    bootId: registerContext.bootId,
    codeHash: registerContext.codeHash
  };

  var existing = getPeerData_(peerId);
  if (existing) {
    peerData.registeredAt = existing.registeredAt || now;
  }

  revokePeerSession_(peerId);
  var session = issueSession_(peerId, registerContext.bootId, registerContext.codeHash);
  peerData.sessionIssuedAt = session.issuedAt;
  peerData.sessionExpiresAt = session.expiresAt;

  setPeerData_(peerId, peerData);
  addPeerToList_(peerId);

  // Ensure queue key exists.
  setQueue_(peerId, getQueue_(peerId));
  cleanupOldPeers_();

  return {
    peerId: peerId,
    registered: true,
    bootId: registerContext.bootId,
    codeHash: registerContext.codeHash,
    sessionToken: session.token,
    sessionIssuedAt: session.issuedAt,
    sessionExpiresAt: session.expiresAt,
    sessionTtlSec: session.ttlSec
  };
}

function handlePoll_(data) {
  var peerId = requirePeerId_(data.peerId || data.id, 'peerId');
  requireSessionForPeer_(data, 'poll', peerId);
  touchPeer_(peerId);

  var messages = getQueue_(peerId);
  clearQueue_(peerId);
  cleanupOldPeers_();

  return {
    peerId: peerId,
    messages: messages,
    messageCount: messages.length
  };
}

function handleSend_(data) {
  var from = requirePeerId_(data.peerId || data.from, 'peerId');
  requireSessionForPeer_(data, 'send', from);

  var type = requiredString_(data.type, 'type', 64);
  var toRaw = data.to || 'broadcast';
  var payloadData = data.data === undefined ? null : data.data;
  var now = Date.now();

  touchPeer_(from);

  var message = {
    id: Utilities.getUuid(),
    from: from,
    type: type,
    data: payloadData,
    timestamp: now
  };

  enforceMessageSize_(message);

  var recipients = [];
  if (String(toRaw).toLowerCase() === 'broadcast') {
    var peers = listActivePeers_();
    peers.forEach(function (peer) {
      if (peer.id !== from) recipients.push(peer.id);
    });
  } else {
    recipients.push(requirePeerId_(toRaw, 'to'));
  }

  recipients.forEach(function (peerId) {
    addMessageToQueue_(peerId, message);
  });

  cleanupOldPeers_();

  return {
    accepted: true,
    from: from,
    type: type,
    recipients: recipients,
    recipientCount: recipients.length
  };
}

function handleGetPeers_(data) {
  var peerId = normalizePeerId_(data.peerId || data.id);
  if (isSessionEnforced_()) {
    if (!peerId) {
      throw appError_('invalid_peer_id', 'peerId is required when sessions are enforced');
    }
    requireSessionForPeer_(data, 'peers', peerId);
    touchPeer_(peerId);
  } else if (peerId) {
    touchPeer_(peerId);
  }

  cleanupOldPeers_();
  var peers = listActivePeers_();
  var includeMeta = toBoolean_(data.includeMeta, false);

  return {
    peers: includeMeta
      ? peers
      : peers.map(function (p) { return p.id; }),
    count: peers.length
  };
}

function handleTranspile_(data) {
  var peerId = normalizePeerId_(data.peerId || data.id);
  if (isSessionEnforced_()) {
    if (!peerId) {
      throw appError_('invalid_peer_id', 'peerId is required when sessions are enforced');
    }
    requireSessionForPeer_(data, 'transpile', peerId);
    touchPeer_(peerId);
  } else if (peerId) {
    touchPeer_(peerId);
  }

  return transpileControlGrammar_(data || {});
}

// -----------------------------------------------------------------------------
// Auth / Endpoint / Parsing
// -----------------------------------------------------------------------------

function isAuthorized_(payload, endpoint) {
  // Public health/status endpoint.
  if (endpoint === 'status' || endpoint === 'health') {
    return true;
  }

  var configuredSecret = getSharedSecret_();
  if (!configuredSecret) {
    // Open mode if no shared secret configured.
    return true;
  }

  var incoming = readIncomingSecret_(payload);
  if (incoming && secureEquals_(incoming, configuredSecret)) {
    return true;
  }

  // When session mode is active, allow request through auth gate and
  // enforce identity in endpoint handlers using session tokens.
  if (isSessionEnforced_() && endpoint !== 'register') {
    return true;
  }

  return false;
}

function readIncomingSecret_(payload) {
  if (payload.auth && typeof payload.auth.secret === 'string') {
    return payload.auth.secret.trim();
  }
  if (typeof payload.secret === 'string') {
    return payload.secret.trim();
  }
  return '';
}

function resolveEndpoint_(e, payload) {
  if (payload && typeof payload.endpoint === 'string' && payload.endpoint.trim()) {
    return sanitizeEndpoint_(payload.endpoint);
  }

  if (e && e.parameter && typeof e.parameter.endpoint === 'string' && e.parameter.endpoint.trim()) {
    return sanitizeEndpoint_(e.parameter.endpoint);
  }

  if (e && typeof e.pathInfo === 'string' && e.pathInfo.trim()) {
    var first = e.pathInfo.split('/')[0];
    return sanitizeEndpoint_(first);
  }

  return '';
}

function sanitizeEndpoint_(value) {
  return String(value || '')
    .trim()
    .toLowerCase()
    .replace(/^\/+|\/+$/g, '');
}

function parsePayload_(method, e) {
  if (method !== 'POST') {
    if (e && e.parameter && typeof e.parameter === 'object') {
      var fromQuery = {};
      Object.keys(e.parameter).forEach(function (key) {
        if (key !== 'endpoint') {
          fromQuery[key] = e.parameter[key];
        }
      });
      return fromQuery;
    }
    return {};
  }

  if (!e || !e.postData || typeof e.postData.contents !== 'string') {
    return {};
  }

  var raw = e.postData.contents.trim();
  if (!raw) return {};

  try {
    var parsed = JSON.parse(raw);
    if (!parsed || typeof parsed !== 'object' || Array.isArray(parsed)) {
      throw appError_('invalid_json', 'JSON body must be an object');
    }
    return parsed;
  } catch (err) {
    if (err && err.code) throw err;
    throw appError_('invalid_json', 'Invalid JSON body');
  }
}

function isMutationEndpoint_(endpoint) {
  return endpoint === 'register' ||
         endpoint === 'poll' ||
         endpoint === 'send' ||
         endpoint === 'peers';
}

// -----------------------------------------------------------------------------
// KHANARY Control Grammar Transpiler
// -----------------------------------------------------------------------------

function transpileControlGrammar_(data) {
  var normalized = normalizeProgramInput_(data || {});
  if (normalized.statements.length === 0) {
    throw appError_(
      'invalid_program',
      'Provide statements[] or tokens[] with at least one actionable intent'
    );
  }

  var astBody = [];
  var plan = [];
  var warnings = normalized.warnings.slice();

  for (var i = 0; i < normalized.statements.length; i++) {
    var statement = normalizeStatement_(normalized.statements[i], i);
    var node = buildIntentAstNode_(statement, i);
    var op = buildExecutionPlanOp_(node, i);
    astBody.push(node);
    plan.push(op);
  }

  return {
    grammar: KHANARY_CONTROL_GRAMMAR.VERSION,
    tokenTypes: KHANARY_CONTROL_GRAMMAR.TOKEN_TYPES,
    sourceMode: normalized.sourceMode,
    tokenCount: normalized.tokenCount,
    statementCount: plan.length,
    warnings: warnings,
    ast: {
      type: 'Program',
      dialect: KHANARY_CONTROL_GRAMMAR.VERSION,
      body: astBody
    },
    plan: plan
  };
}

function normalizeProgramInput_(data) {
  var warnings = [];
  var statements = [];
  var tokenCount = 0;
  var sourceMode = '';

  if (Array.isArray(data.statements)) {
    sourceMode = 'statements';
    statements = data.statements.slice();
  } else if (Array.isArray(data.tokens)) {
    sourceMode = 'tokens';
    tokenCount = data.tokens.length;
    statements = tokensToStatements_(data.tokens, warnings);
  } else {
    sourceMode = 'none';
  }

  return {
    sourceMode: sourceMode,
    tokenCount: tokenCount,
    statements: statements,
    warnings: warnings
  };
}

function tokensToStatements_(tokens, warnings) {
  var statements = [];
  var current = createEmptyStatement_();

  for (var i = 0; i < tokens.length; i++) {
    var token = normalizeToken_(tokens[i], i);
    if (token.type === 'literal') {
      // Statement boundary markers.
      if (token.value === ';' || token.value === 'EOL' || token.value === 'NEWLINE') {
        if (statementHasContent_(current)) {
          statements.push(current);
          current = createEmptyStatement_();
        }
        continue;
      }
      current.literals.push(token.value);
      continue;
    }

    if (token.type === 'verb' && statementHasContent_(current) && current.verb) {
      statements.push(current);
      current = createEmptyStatement_();
    }

    assignTokenToStatement_(current, token, warnings);
  }

  if (statementHasContent_(current)) {
    statements.push(current);
  }

  return statements;
}

function normalizeToken_(token, index) {
  if (!token || typeof token !== 'object' || Array.isArray(token)) {
    throw appError_('invalid_token', 'Token at index ' + index + ' must be an object');
  }

  var rawType = token.type || token.kind || token.token;
  var type = normalizeTokenType_(rawType);
  if (!type) {
    throw appError_('invalid_token', 'Unknown token type at index ' + index + ': ' + rawType);
  }

  var value = token.value;
  if (type === 'args') {
    return {
      type: type,
      value: normalizeArgs_(value, index)
    };
  }

  if (type === 'flag') {
    return {
      type: type,
      value: normalizeIdentifierLike_(value, 'flag', 128)
    };
  }

  if (type === 'literal') {
    return {
      type: type,
      value: String(value === undefined || value === null ? '' : value).trim()
    };
  }

  return {
    type: type,
    value: normalizeIdentifierLike_(value, type, 256)
  };
}

function normalizeTokenType_(rawType) {
  var t = String(rawType || '').trim().toLowerCase();
  if (!t) return '';

  if (t === 'tool' || t === 'toolcall' || t === 'tool-call') return 'tool_call';
  if (t === 'fn' || t === 'func') return 'function';
  if (t === 'cmd') return 'command';
  if (t === 'arg' || t === 'params' || t === 'arguments') return 'args';
  if (t === 'cap' || t === 'scope') return 'capability';
  if (t === 'route_id') return 'route';

  for (var i = 0; i < KHANARY_CONTROL_GRAMMAR.TOKEN_TYPES.length; i++) {
    if (KHANARY_CONTROL_GRAMMAR.TOKEN_TYPES[i] === t) {
      return t;
    }
  }
  return '';
}

function createEmptyStatement_() {
  return {
    verb: '',
    command: '',
    'function': '',
    tool_call: '',
    capability: '',
    peer: '',
    route: '',
    args: {},
    flags: [],
    literals: []
  };
}

function statementHasContent_(statement) {
  return !!(
    statement.verb ||
    statement.command ||
    statement['function'] ||
    statement.tool_call ||
    statement.capability ||
    statement.peer ||
    statement.route ||
    statement.flags.length ||
    statement.literals.length ||
    Object.keys(statement.args).length
  );
}

function assignTokenToStatement_(statement, token, warnings) {
  switch (token.type) {
    case 'verb':
      statement.verb = normalizeVerb_(token.value);
      return;
    case 'command':
      statement.command = token.value;
      return;
    case 'function':
      statement['function'] = token.value;
      return;
    case 'tool_call':
      statement.tool_call = token.value;
      return;
    case 'capability':
      statement.capability = token.value;
      return;
    case 'peer':
      statement.peer = token.value;
      return;
    case 'route':
      statement.route = token.value;
      return;
    case 'args':
      statement.args = mergePlainObjects_(statement.args, token.value);
      return;
    case 'flag':
      if (statement.flags.indexOf(token.value) === -1) {
        statement.flags.push(token.value);
      } else {
        warnings.push('Duplicate flag ignored: ' + token.value);
      }
      return;
    default:
      warnings.push('Unhandled token type ignored: ' + token.type);
  }
}

function normalizeStatement_(raw, index) {
  if (!raw || typeof raw !== 'object' || Array.isArray(raw)) {
    throw appError_('invalid_statement', 'Statement at index ' + index + ' must be an object');
  }

  var normalized = createEmptyStatement_();
  normalized.verb = normalizeVerb_(raw.verb || raw.action || raw.op || '');
  normalized.command = optionalIdentifierLike_(raw.command || raw.cmd || '', 'command', 256);
  normalized['function'] = optionalIdentifierLike_(raw['function'] || raw.fn || '', 'function', 256);
  normalized.tool_call = optionalIdentifierLike_(raw.tool_call || raw.tool || raw.toolcall || '', 'tool_call', 256);
  normalized.capability = optionalIdentifierLike_(raw.capability || raw.scope || '', 'capability', 128);
  normalized.peer = optionalIdentifierLike_(raw.peer || raw.targetPeer || '', 'peer', 128);
  normalized.route = optionalIdentifierLike_(raw.route || raw.routeId || '', 'route', 128);
  normalized.args = normalizeArgs_(raw.args || raw.arguments || raw.params || {}, index);

  if (Array.isArray(raw.flags)) {
    for (var i = 0; i < raw.flags.length; i++) {
      var flagValue = optionalIdentifierLike_(raw.flags[i], 'flag', 128);
      if (flagValue && normalized.flags.indexOf(flagValue) === -1) {
        normalized.flags.push(flagValue);
      }
    }
  }

  if (Array.isArray(raw.literals)) {
    for (var j = 0; j < raw.literals.length; j++) {
      var lit = String(raw.literals[j] === undefined || raw.literals[j] === null ? '' : raw.literals[j]).trim();
      if (lit) normalized.literals.push(lit);
    }
  }

  if (!normalized.verb) {
    normalized.verb = deriveVerb_(normalized);
  }

  var hasAction = !!(
    normalized.command ||
    normalized['function'] ||
    normalized.tool_call ||
    normalized.route
  );
  if (!hasAction) {
    throw appError_(
      'invalid_statement',
      'Statement at index ' + index + ' has no command/function/tool_call/route'
    );
  }

  return normalized;
}

function buildIntentAstNode_(statement, index) {
  return {
    type: 'Intent',
    index: index,
    verb: statement.verb,
    command: statement.command || null,
    'function': statement['function'] || null,
    tool_call: statement.tool_call || null,
    route: statement.route || null,
    peer: statement.peer || null,
    capability: statement.capability || null,
    args: statement.args,
    flags: statement.flags,
    literals: statement.literals
  };
}

function buildExecutionPlanOp_(node, index) {
  return {
    index: index,
    opcode: resolveOpcodeForVerb_(node.verb),
    verb: node.verb,
    target: node.tool_call || node['function'] || node.command || node.route,
    command: node.command,
    'function': node['function'],
    tool_call: node.tool_call,
    route: node.route,
    peer: node.peer,
    capability: node.capability,
    args: node.args,
    flags: node.flags
  };
}

function deriveVerb_(statement) {
  if (statement.route) return 'route';
  if (statement.tool_call || statement['function']) return 'invoke';
  if (statement.command) return 'execute';
  return 'invoke';
}

function normalizeVerb_(value) {
  var v = optionalIdentifierLike_(value, 'verb', 64).toLowerCase();
  if (!v) return '';
  return v;
}

function resolveOpcodeForVerb_(verb) {
  if (KHANARY_CONTROL_GRAMMAR.VERB_OPCODE[verb]) {
    return KHANARY_CONTROL_GRAMMAR.VERB_OPCODE[verb];
  }
  return 'EXEC';
}

function optionalIdentifierLike_(value, fieldName, maxLen) {
  if (value === undefined || value === null || value === '') return '';
  return normalizeIdentifierLike_(value, fieldName, maxLen);
}

function normalizeIdentifierLike_(value, fieldName, maxLen) {
  var out = String(value === undefined || value === null ? '' : value).trim();
  if (!out) {
    throw appError_('invalid_' + fieldName, 'Missing or invalid ' + fieldName);
  }
  if (out.length > maxLen) {
    throw appError_('invalid_' + fieldName, fieldName + ' too long');
  }
  if (!/^[A-Za-z0-9._:-]+$/.test(out)) {
    throw appError_('invalid_' + fieldName, fieldName + ' contains unsupported characters');
  }
  return out;
}

function normalizeArgs_(argsValue, index) {
  if (argsValue === undefined || argsValue === null || argsValue === '') {
    return {};
  }

  if (typeof argsValue === 'string') {
    var raw = argsValue.trim();
    if (!raw) return {};
    try {
      argsValue = JSON.parse(raw);
    } catch (err) {
      throw appError_('invalid_args', 'args at index ' + index + ' is not valid JSON');
    }
  }

  if (!argsValue || typeof argsValue !== 'object' || Array.isArray(argsValue)) {
    throw appError_('invalid_args', 'args at index ' + index + ' must be an object');
  }

  return clonePlainObject_(argsValue);
}

function mergePlainObjects_(left, right) {
  var out = clonePlainObject_(left || {});
  var src = clonePlainObject_(right || {});
  var keys = Object.keys(src);
  for (var i = 0; i < keys.length; i++) {
    out[keys[i]] = src[keys[i]];
  }
  return out;
}

function clonePlainObject_(obj) {
  return JSON.parse(JSON.stringify(obj || {}));
}

function isSessionEnforced_() {
  var override = PROPERTIES.getProperty('MESHNET_ENFORCE_SESSIONS');
  if (override !== null && override !== undefined && String(override).trim() !== '') {
    return toBoolean_(override, false);
  }
  return toBoolean_(SECURITY_BOOTSTRAP.ENFORCE_SESSIONS, true);
}

function isRegisterHandshakeEnforced_() {
  var override = PROPERTIES.getProperty('MESHNET_REQUIRE_REGISTER_HANDSHAKE');
  if (override !== null && override !== undefined && String(override).trim() !== '') {
    return toBoolean_(override, false);
  }
  return toBoolean_(SECURITY_BOOTSTRAP.REQUIRE_REGISTER_HANDSHAKE, true);
}

function hasSharedSecret_() {
  return !!getSharedSecret_();
}

function getSharedSecret_() {
  var raw = PROPERTIES.getProperty('MESHNET_SHARED_SECRET');
  if (typeof raw === 'string' && raw.trim()) {
    return raw.trim();
  }

  if (typeof SECURITY_BOOTSTRAP.SHARED_SECRET === 'string' &&
      SECURITY_BOOTSTRAP.SHARED_SECRET.trim()) {
    return SECURITY_BOOTSTRAP.SHARED_SECRET.trim();
  }

  return '';
}

function validateSecurityConfiguration_(endpoint) {
  if (endpoint === 'status' || endpoint === 'health') {
    return;
  }

  if (isSessionEnforced_() && !getSharedSecret_()) {
    throw appError_(
      'security_misconfigured',
      'Session enforcement is enabled but no shared secret is configured. Set MESHNET_SHARED_SECRET or SECURITY_BOOTSTRAP.SHARED_SECRET.'
    );
  }
}

// -----------------------------------------------------------------------------
// Session + Register Handshake
// -----------------------------------------------------------------------------

function buildRegisterContext_(data, peerId, strictHandshake) {
  if (strictHandshake) {
    return verifyRegisterHandshake_(data, peerId);
  }

  return {
    bootId: normalizeBootId_(data.bootId) || Utilities.getUuid(),
    codeHash: normalizeSha256_(data.codeHash || data.sha256 || '') || '',
    timestamp: parseEpochMs_(data.ts || data.timestamp) || Date.now(),
    nonce: normalizeTokenPart_(data.nonce, SETTINGS.MAX_NONCE_LEN) || ''
  };
}

function verifyRegisterHandshake_(data, peerId) {
  var sharedSecret = getSharedSecret_();
  if (!sharedSecret) {
    throw appError_(
      'handshake_unavailable',
      'Handshake enforcement requires MESHNET_SHARED_SECRET'
    );
  }

  var bootId = requiredBootId_(data.bootId);
  var codeHash = requiredSha256_(data.codeHash || data.sha256 || data.hash, 'codeHash');
  enforceCodeHashPolicy_(codeHash);

  var timestamp = parseEpochMs_(data.ts || data.timestamp);
  if (!timestamp) {
    throw appError_('invalid_timestamp', 'Missing or invalid timestamp (ts)');
  }

  var now = Date.now();
  var maxSkew = resolveAllowedSkewMs_();
  if (Math.abs(now - timestamp) > maxSkew) {
    throw appError_('stale_timestamp', 'Timestamp outside allowed clock skew');
  }

  var nonce = requiredNonce_(data.nonce);
  claimRegisterNonce_(peerId, nonce);

  var signature = requiredSignature_(data);
  var signingPayload = buildRegisterSigningPayload_(peerId, bootId, codeHash, timestamp, nonce);
  if (!verifyHmacSignature_(signingPayload, signature, sharedSecret)) {
    throw appError_('invalid_signature', 'Signature verification failed');
  }

  return {
    bootId: bootId,
    codeHash: codeHash,
    timestamp: timestamp,
    nonce: nonce
  };
}

function buildRegisterSigningPayload_(peerId, bootId, codeHash, timestamp, nonce) {
  return [
    peerId,
    bootId,
    codeHash,
    String(timestamp),
    nonce
  ].join('|');
}

function enforceCodeHashPolicy_(codeHash) {
  var allowlist = getCodeHashAllowlist_();
  if (allowlist.length === 0) {
    return;
  }
  if (allowlist.indexOf(codeHash) === -1) {
    throw appError_('code_hash_not_allowed', 'Unapproved codeHash');
  }
}

function getCodeHashAllowlist_() {
  var raw = PROPERTIES.getProperty('MESHNET_CODEHASH_ALLOWLIST');
  if (!raw || typeof raw !== 'string') return [];

  var seen = {};
  var hashes = [];
  raw.split(/[\s,;]+/).forEach(function (part) {
    var normalized = normalizeSha256_(part);
    if (normalized && !seen[normalized]) {
      seen[normalized] = true;
      hashes.push(normalized);
    }
  });
  return hashes;
}

function resolveSessionTtlSec_() {
  return resolveNumberProperty_(
    'MESHNET_SESSION_TTL_SEC',
    Number(SECURITY_BOOTSTRAP.SESSION_TTL_SEC) || SETTINGS.DEFAULT_SESSION_TTL_SEC,
    60,
    24 * 60 * 60
  );
}

function resolveNonceTtlSec_() {
  return resolveNumberProperty_(
    'MESHNET_NONCE_TTL_SEC',
    Number(SECURITY_BOOTSTRAP.NONCE_TTL_SEC) || SETTINGS.DEFAULT_NONCE_TTL_SEC,
    30,
    24 * 60 * 60
  );
}

function resolveAllowedSkewMs_() {
  return resolveNumberProperty_(
    'MESHNET_TIMESTAMP_SKEW_MS',
    Number(SECURITY_BOOTSTRAP.TIMESTAMP_SKEW_MS) || SETTINGS.DEFAULT_ALLOWED_SKEW_MS,
    5000,
    24 * 60 * 60 * 1000
  );
}

function resolveNumberProperty_(name, fallback, minValue, maxValue) {
  var raw = PROPERTIES.getProperty(name);
  var value = Number(raw);
  if (!isFinite(value) || value <= 0) {
    value = fallback;
  }
  value = Math.floor(value);
  if (value < minValue) value = minValue;
  if (value > maxValue) value = maxValue;
  return value;
}

function claimRegisterNonce_(peerId, nonce) {
  var key = nonceKey_(peerId, nonce);
  if (CACHE.get(key)) {
    throw appError_('replay_nonce', 'Nonce already used');
  }
  CACHE.put(key, '1', resolveNonceTtlSec_());
}

function issueSession_(peerId, bootId, codeHash) {
  var now = Date.now();
  var ttlSec = resolveSessionTtlSec_();
  var token = generateSessionToken_(peerId, bootId, now);
  var session = {
    token: token,
    peerId: peerId,
    bootId: bootId,
    codeHash: codeHash || '',
    issuedAt: now,
    expiresAt: now + ttlSec * 1000,
    ttlSec: ttlSec
  };

  CACHE.put(sessionKey_(token), JSON.stringify(session), ttlSec);
  PROPERTIES.setProperty(peerSessionKey_(peerId), token);
  return session;
}

function revokePeerSession_(peerId) {
  var key = peerSessionKey_(peerId);
  var token = PROPERTIES.getProperty(key);
  if (token) {
    CACHE.remove(sessionKey_(token));
  }
  PROPERTIES.deleteProperty(key);
}

function readSessionToken_(payload) {
  if (typeof payload.sessionToken === 'string' && payload.sessionToken.trim()) {
    return payload.sessionToken.trim();
  }
  if (typeof payload.session === 'string' && payload.session.trim()) {
    return payload.session.trim();
  }
  if (payload.auth && typeof payload.auth.sessionToken === 'string' && payload.auth.sessionToken.trim()) {
    return payload.auth.sessionToken.trim();
  }
  return '';
}

function requireSessionForPeer_(payload, endpoint, peerId) {
  if (!isSessionEnforced_()) {
    return null;
  }

  var token = readSessionToken_(payload);
  if (!token) {
    throw appError_('unauthorized', 'Missing sessionToken for endpoint: ' + endpoint);
  }

  var session = getSession_(token);
  if (!session) {
    throw appError_('unauthorized', 'Invalid or expired sessionToken');
  }

  if (session.peerId !== peerId) {
    throw appError_('session_peer_mismatch', 'sessionToken does not match peerId');
  }

  if (Date.now() >= session.expiresAt) {
    CACHE.remove(sessionKey_(token));
    throw appError_('session_expired', 'sessionToken expired');
  }

  var current = PROPERTIES.getProperty(peerSessionKey_(peerId));
  if (current && !secureEquals_(current, token)) {
    throw appError_('session_revoked', 'sessionToken was replaced by a newer registration');
  }

  return session;
}

function getSession_(token) {
  var raw = CACHE.get(sessionKey_(token));
  if (!raw) return null;
  try {
    var session = JSON.parse(raw);
    if (!session || typeof session !== 'object') return null;
    return session;
  } catch (err) {
    return null;
  }
}

function generateSessionToken_(peerId, bootId, nowMs) {
  var material = [
    Utilities.getUuid(),
    Utilities.getUuid(),
    peerId,
    bootId,
    String(nowMs),
    String(Math.random())
  ].join('|');

  var digest = Utilities.computeDigest(Utilities.DigestAlgorithm.SHA_256, material);
  return trimBase64Padding_(Utilities.base64EncodeWebSafe(digest));
}

function verifyHmacSignature_(message, incomingSig, sharedSecret) {
  var normalizedIncoming = normalizeSignatureText_(incomingSig);
  if (!normalizedIncoming) return false;

  var bytes = Utilities.computeHmacSha256Signature(message, sharedSecret);
  var expectedHex = bytesToHex_(bytes);
  var expectedB64 = trimBase64Padding_(Utilities.base64Encode(bytes));
  var expectedB64Web = trimBase64Padding_(Utilities.base64EncodeWebSafe(bytes));

  var incomingNoPad = trimBase64Padding_(normalizedIncoming);
  var incomingHex = normalizedIncoming.toLowerCase();

  return secureEquals_(incomingHex, expectedHex) ||
         secureEquals_(incomingNoPad, expectedB64) ||
         secureEquals_(incomingNoPad, expectedB64Web);
}

function normalizeSignatureText_(value) {
  if (typeof value !== 'string') return '';
  return value
    .trim()
    .replace(/^hmac-sha256[:=]/i, '')
    .replace(/^sha256[:=]/i, '')
    .replace(/\s+/g, '');
}

// -----------------------------------------------------------------------------
// Peer / Queue Storage
// -----------------------------------------------------------------------------

function listActivePeers_() {
  var now = Date.now();
  var peerIds = getPeerList_();
  var active = [];

  peerIds.forEach(function (peerId) {
    var peer = getPeerData_(peerId);
    if (!peer || !peer.lastSeen) return;
    if ((now - peer.lastSeen) < SETTINGS.PEER_ACTIVE_MS) {
      active.push(peer);
    }
  });

  return active;
}

function touchPeer_(peerId) {
  var now = Date.now();
  var peer = getPeerData_(peerId) || { id: peerId, registeredAt: now, meta: {} };
  peer.id = peerId;
  peer.lastSeen = now;
  if (!peer.registeredAt) peer.registeredAt = now;
  if (!peer.meta || typeof peer.meta !== 'object') peer.meta = {};

  setPeerData_(peerId, peer);
  addPeerToList_(peerId);
}

function cleanupOldPeers_() {
  var now = Date.now();
  var peerIds = getPeerList_();
  var activeIds = [];

  peerIds.forEach(function (peerId) {
    var peer = getPeerData_(peerId);
    if (!peer || !peer.lastSeen) {
      deletePeer_(peerId);
      return;
    }

    if ((now - peer.lastSeen) > SETTINGS.PEER_STALE_MS) {
      deletePeer_(peerId);
      return;
    }

    activeIds.push(peerId);
  });

  savePeerList_(activeIds);
}

function deletePeer_(peerId) {
  revokePeerSession_(peerId);
  PROPERTIES.deleteProperty(peerKey_(peerId));
  CACHE.remove(queueKey_(peerId));
}

function getPeerData_(peerId) {
  var raw = PROPERTIES.getProperty(peerKey_(peerId));
  if (!raw) return null;
  try {
    var parsed = JSON.parse(raw);
    if (parsed && typeof parsed === 'object') return parsed;
  } catch (err) {
    // Drop corrupt peer entry.
  }
  return null;
}

function setPeerData_(peerId, data) {
  PROPERTIES.setProperty(peerKey_(peerId), JSON.stringify(data || {}));
}

function getPeerList_() {
  var raw = PROPERTIES.getProperty('peer_list');
  if (!raw) return [];
  try {
    var parsed = JSON.parse(raw);
    if (!Array.isArray(parsed)) return [];
    return dedupeStrings_(parsed)
      .filter(function (v) { return !!normalizePeerId_(v); });
  } catch (err) {
    return [];
  }
}

function savePeerList_(peerIds) {
  PROPERTIES.setProperty('peer_list', JSON.stringify(dedupeStrings_(peerIds)));
}

function addPeerToList_(peerId) {
  var list = getPeerList_();
  if (list.indexOf(peerId) === -1) {
    list.push(peerId);
    savePeerList_(list);
  }
}

function addMessageToQueue_(peerId, message) {
  var queue = getQueue_(peerId);
  queue.push(message);

  if (queue.length > SETTINGS.MAX_QUEUE_LENGTH) {
    queue = queue.slice(queue.length - SETTINGS.MAX_QUEUE_LENGTH);
  }

  // Keep queue under cache payload limits.
  while (queue.length > 0 && byteLen_(JSON.stringify(queue)) > SETTINGS.MAX_QUEUE_BYTES) {
    queue.shift();
  }

  setQueue_(peerId, queue);
}

function getQueue_(peerId) {
  var raw = CACHE.get(queueKey_(peerId));
  if (!raw) return [];
  try {
    var parsed = JSON.parse(raw);
    return Array.isArray(parsed) ? parsed : [];
  } catch (err) {
    return [];
  }
}

function setQueue_(peerId, queue) {
  CACHE.put(queueKey_(peerId), JSON.stringify(queue || []), SETTINGS.QUEUE_TTL_SEC);
}

function clearQueue_(peerId) {
  CACHE.put(queueKey_(peerId), '[]', SETTINGS.QUEUE_TTL_SEC);
}

// -----------------------------------------------------------------------------
// Validation / Helpers
// -----------------------------------------------------------------------------

function requirePeerId_(value, fieldName) {
  var peerId = normalizePeerId_(value);
  if (!peerId) {
    throw appError_('invalid_peer_id', 'Invalid ' + fieldName + ' value');
  }
  return peerId;
}

function normalizePeerId_(value) {
  if (typeof value !== 'string') return '';
  var peerId = value.trim();
  if (!peerId || peerId.length > SETTINGS.MAX_PEER_ID_LEN) return '';
  if (!/^[A-Za-z0-9._:-]+$/.test(peerId)) return '';
  return peerId;
}

function requiredString_(value, fieldName, maxLen) {
  if (typeof value !== 'string') {
    throw appError_('invalid_' + fieldName, 'Missing or invalid ' + fieldName);
  }
  var out = value.trim();
  if (!out) {
    throw appError_('invalid_' + fieldName, 'Missing or invalid ' + fieldName);
  }
  if (maxLen && out.length > maxLen) {
    throw appError_('invalid_' + fieldName, fieldName + ' too long');
  }
  return out;
}

function requiredBootId_(value) {
  var bootId = normalizeBootId_(value);
  if (!bootId) {
    throw appError_('invalid_boot_id', 'Missing or invalid bootId');
  }
  return bootId;
}

function normalizeBootId_(value) {
  if (typeof value !== 'string') return '';
  var bootId = value.trim();
  if (!bootId || bootId.length > SETTINGS.MAX_BOOT_ID_LEN) return '';
  if (!/^[A-Za-z0-9._:-]+$/.test(bootId)) return '';
  return bootId;
}

function requiredNonce_(value) {
  var nonce = normalizeTokenPart_(value, SETTINGS.MAX_NONCE_LEN);
  if (!nonce) {
    throw appError_('invalid_nonce', 'Missing or invalid nonce');
  }
  return nonce;
}

function normalizeTokenPart_(value, maxLen) {
  if (typeof value !== 'string') return '';
  var out = value.trim();
  if (!out || out.length > maxLen) return '';
  if (!/^[A-Za-z0-9._:-]+$/.test(out)) return '';
  return out;
}

function requiredSha256_(value, fieldName) {
  var hash = normalizeSha256_(value);
  if (!hash) {
    throw appError_('invalid_' + fieldName, 'Missing or invalid ' + fieldName + ' (expected SHA-256 hex)');
  }
  return hash;
}

function normalizeSha256_(value) {
  if (typeof value !== 'string') return '';
  var out = value.trim().toLowerCase();
  if (!/^[0-9a-f]{64}$/.test(out)) return '';
  return out;
}

function requiredSignature_(payload) {
  var sig = '';

  if (typeof payload.sig === 'string') {
    sig = payload.sig;
  } else if (typeof payload.signature === 'string') {
    sig = payload.signature;
  } else if (payload.auth && typeof payload.auth.sig === 'string') {
    sig = payload.auth.sig;
  } else if (payload.auth && typeof payload.auth.signature === 'string') {
    sig = payload.auth.signature;
  }

  sig = String(sig || '').trim();
  if (!sig) {
    throw appError_('invalid_signature', 'Missing signature');
  }
  return sig;
}

function parseEpochMs_(value) {
  if (value === null || value === undefined || value === '') return 0;
  var n = Number(value);
  if (!isFinite(n) || n <= 0) return 0;
  if (n < 100000000000) { // probably seconds
    n = n * 1000;
  }
  return Math.floor(n);
}

function toBoolean_(value, fallback) {
  if (value === null || value === undefined) return fallback;
  if (typeof value === 'boolean') return value;
  var text = String(value).trim().toLowerCase();
  if (!text) return fallback;
  if (text === '1' || text === 'true' || text === 'yes' || text === 'on') return true;
  if (text === '0' || text === 'false' || text === 'no' || text === 'off') return false;
  return fallback;
}

function asObject_(value) {
  if (!value || typeof value !== 'object' || Array.isArray(value)) return {};
  return value;
}

function enforceMessageSize_(message) {
  if (byteLen_(JSON.stringify(message)) > SETTINGS.MAX_MESSAGE_BYTES) {
    throw appError_('message_too_large', 'Message payload exceeds max size');
  }
}

function byteLen_(str) {
  return Utilities.newBlob(str || '').getBytes().length;
}

function dedupeStrings_(arr) {
  var seen = {};
  var out = [];
  (arr || []).forEach(function (v) {
    var s = String(v);
    if (!seen[s]) {
      seen[s] = true;
      out.push(s);
    }
  });
  return out;
}

function bytesToHex_(bytes) {
  var hex = '';
  for (var i = 0; i < bytes.length; i++) {
    var value = bytes[i];
    if (value < 0) value += 256;
    var part = value.toString(16);
    if (part.length < 2) part = '0' + part;
    hex += part;
  }
  return hex;
}

function trimBase64Padding_(value) {
  return String(value || '').replace(/=+$/, '');
}

function secureEquals_(a, b) {
  if (typeof a !== 'string' || typeof b !== 'string') return false;
  var lenA = a.length;
  var lenB = b.length;
  var maxLen = Math.max(lenA, lenB);
  var diff = lenA ^ lenB;

  for (var i = 0; i < maxLen; i++) {
    var ca = i < lenA ? a.charCodeAt(i) : 0;
    var cb = i < lenB ? b.charCodeAt(i) : 0;
    diff |= (ca ^ cb);
  }

  return diff === 0;
}

function peerKey_(peerId) {
  return 'peer_' + peerId;
}

function queueKey_(peerId) {
  return 'messages_' + peerId;
}

function sessionKey_(token) {
  return 'session_' + token;
}

function peerSessionKey_(peerId) {
  return 'peer_session_' + peerId;
}

function nonceKey_(peerId, nonce) {
  return 'nonce_' + peerId + '_' + nonce;
}

function withScriptLock_(fn) {
  var lock = LockService.getScriptLock();
  lock.waitLock(10000);
  try {
    return fn();
  } finally {
    lock.releaseLock();
  }
}

function appError_(code, message) {
  var err = new Error(message);
  err.code = code;
  return err;
}

function jsonResponse_(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj))
    .setMimeType(ContentService.MimeType.JSON);
}
