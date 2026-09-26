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
  MAX_PEER_ID_LEN: 64
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
    peerCount: peers.length
  };
}

function handleRegister_(data) {
  var peerId = requirePeerId_(data.peerId || data.id, 'peerId');
  var now = Date.now();
  var peerMeta = asObject_(data.meta);

  var peerData = {
    id: peerId,
    lastSeen: now,
    registeredAt: now,
    meta: peerMeta
  };

  var existing = getPeerData_(peerId);
  if (existing) {
    peerData.registeredAt = existing.registeredAt || now;
  }

  setPeerData_(peerId, peerData);
  addPeerToList_(peerId);

  // Ensure queue key exists.
  setQueue_(peerId, getQueue_(peerId));
  cleanupOldPeers_();

  return {
    peerId: peerId,
    registered: true
  };
}

function handlePoll_(data) {
  var peerId = requirePeerId_(data.peerId || data.id, 'peerId');
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
  if (peerId) {
    touchPeer_(peerId);
  }

  cleanupOldPeers_();
  var peers = listActivePeers_();
  var includeMeta = !!data.includeMeta;

  return {
    peers: includeMeta
      ? peers
      : peers.map(function (p) { return p.id; }),
    count: peers.length
  };
}

// -----------------------------------------------------------------------------
// Auth / Endpoint / Parsing
// -----------------------------------------------------------------------------

function isAuthorized_(payload, endpoint) {
  // Public health/status endpoint.
  if (endpoint === 'status' || endpoint === 'health') {
    return true;
  }

  var configuredSecret = PROPERTIES.getProperty('MESHNET_SHARED_SECRET');
  if (!configuredSecret) {
    // Open mode if no shared secret configured.
    return true;
  }

  var incoming = '';
  if (payload.auth && typeof payload.auth.secret === 'string') {
    incoming = payload.auth.secret;
  } else if (typeof payload.secret === 'string') {
    incoming = payload.secret;
  }

  return incoming === configuredSecret;
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

function peerKey_(peerId) {
  return 'peer_' + peerId;
}

function queueKey_(peerId) {
  return 'messages_' + peerId;
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
