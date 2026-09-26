# Supernaut Runtime Docs (KHANARY)

This document reflects the **current** Supernaut + Canvas.X workflow in this repository.

## Runtime model (current)

- **UI host:** custom native C++ window/runtime (`canvas-x.exe`) with Win32 + D3D11 rendering.
- **Not WPF:** the terminal UI is app-rendered, not a WPF/XAML console.
- **HTML role:** optional content source for tabs/previews; runtime does not require HTML to boot.
- **Control plane:** JSON-first configuration and orchestration.

## JSON-first UI bootstrap

Canvas.X reads bootstrap config from:

- `...\Debug\temp\canvas-runtime-bootstrap.json` (or Release equivalent)

Minimal shape:

```json
{
  "schema": "canvas.runtime.bootstrap.v1",
  "tabs": [
    { "html": "mx2lm-os-v17.html", "label": "MX2LM OS v17 Bootstrap" },
    { "html": "GUIX-AI-CHAT-BOX.html", "label": "GUIX AI Chat Box", "active": true }
  ]
}
```

Notes:

- Paths are resolved relative to the bootstrap JSON location.
- If no loadable HTML tabs are found, Canvas.X falls back to internal default tabs.

## Build + package flow (`buildUI.bat`)

`buildUI.bat` (in the Canvas runtime source tree) now performs:

1. Rebuild `canvas-x` via `build.bat`.
2. Rebuild HTML sidecar `.bin` files when requested.
3. Package a runnable app folder from existing built executables + JSON/HTML assets.

### Usage

```bat
buildUI.bat [Debug|Release|Both] [auto|text|binary]
```

- `auto` (default): rebuild `file.html.bin` only if sidecar exists.
- `text`: process HTML with no binary sidecar rebuild.
- `binary`: force `.bin` sidecar rebuild for each HTML file.

### Package output

For each config, output is generated under:

- `...\<Config>\app\`
  - `app.manifest.json` (entry program + packaged assets)
  - `launch-canvas-x.bat`
  - `bin\*.exe`, `bin\*.dll`
  - `bin\temp\*.json`, `*.html`, optional `*.bin`

## GAS control-plane node (`GASNodes.gs`)

File:

- `tools\supernaut\GASNodes.gs`

Purpose:

- JSON control-plane for node registration, queueing, and message relay.
- **Not** a UDP/P2P transport node; it coordinates over HTTPS.

### Endpoints (JSON payload `endpoint` field)

- `status` / `health`
- `register`
- `poll`
- `send`
- `peers`
- `transpile` (KHANARY control grammar → AST + execution plan)

### Deployment

Deploy Apps Script as Web App:

- **Execute as:** Me
- **Who has access:** Anyone (or your intended public scope)

Optional auth:

- Set Script Property `MESHNET_SHARED_SECRET`.
- `register` accepts `auth.secret` (or `secret`) and issues a short-lived `sessionToken`.
- When session mode is enforced, `poll` / `send` / `peers` require that `sessionToken`.

### Restart-aware session handshake

For strict register validation, send:

```json
{
  "endpoint": "register",
  "peerId": "canvas-x-node-1",
  "bootId": "2f2c4f5d-7fd1-4694-a0b8-2f0c7fcb1166",
  "codeHash": "<sha256-hex-of-node-build>",
  "ts": 1790378056518,
  "nonce": "n-0001",
  "sig": "<hmac-sha256-signature>",
  "auth": { "secret": "<shared-secret>" }
}
```

Signature base string:

`peerId|bootId|codeHash|ts|nonce`

`sig` is verified as HMAC-SHA256 (hex, base64, or websafe base64 accepted).
Reused nonces are rejected (anti-replay), and reconnecting with a new `bootId`
revokes the old session for that `peerId`.

### Script properties recognized by `GASNodes.gs`

- `MESHNET_SHARED_SECRET`
- `MESHNET_ENFORCE_SESSIONS` (`true|false`; default from `SECURITY_BOOTSTRAP.ENFORCE_SESSIONS`)
- `MESHNET_REQUIRE_REGISTER_HANDSHAKE` (`true|false`; default from `SECURITY_BOOTSTRAP.REQUIRE_REGISTER_HANDSHAKE`)
- `MESHNET_SESSION_TTL_SEC` (default 3600)
- `MESHNET_NONCE_TTL_SEC` (default 900)
- `MESHNET_TIMESTAMP_SKEW_MS` (default 300000)
- `MESHNET_CODEHASH_ALLOWLIST` (comma/space separated SHA-256 hex list)

### Secure defaults already applied "up top"

`GASNodes.gs` now includes a top-level `SECURITY_BOOTSTRAP` block with:

- `ENFORCE_SESSIONS: true`
- `REQUIRE_REGISTER_HANDSHAKE: true`

This means non-status endpoints fail closed until a shared secret is configured.
Set either:

1. Script Property `MESHNET_SHARED_SECRET` (recommended), or
2. `SECURITY_BOOTSTRAP.SHARED_SECRET` directly in `GASNodes.gs`.

### `transpile` endpoint (agent control grammar)

`transpile` lets nodes send KHANARY control tokens/intent statements and receive a
deterministic AST plus normalized execution plan.

Token families supported:
- `verb`
- `command`
- `function`
- `tool_call`
- `args`
- `capability`
- `flag`
- `literal`
- `peer`
- `route`

Example:

```json
{
  "endpoint": "transpile",
  "peerId": "canvas-x-node-1",
  "sessionToken": "<session token from register>",
  "tokens": [
    { "type": "verb", "value": "invoke" },
    { "type": "function", "value": "khanary.dispatch" },
    { "type": "tool_call", "value": "canvas.render" },
    { "type": "command", "value": "draw" },
    { "type": "args", "value": { "tab": "main", "mode": "safe" } },
    { "type": "capability", "value": "mesh.exec" }
  ]
}
```

Response includes:
- `grammar` (`khanary.control.v1`)
- `ast` (`Program` + `Intent[]`)
- `plan` (opcode/target/args per statement)

## Crypto-network integration guidance

- GAS node = control plane (state/dispatch), not transport plane.
- Native executables remain transport/security participants.
- For multi-process local mesh:
  - use unique local ports per process,
  - keep protocol/auth semantics consistent,
  - keep shared trust/issuer policy aligned.
