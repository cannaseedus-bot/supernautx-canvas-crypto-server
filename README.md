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

### Deployment

Deploy Apps Script as Web App:

- **Execute as:** Me
- **Who has access:** Anyone (or your intended public scope)

Optional auth:

- Set Script Property `MESHNET_SHARED_SECRET`.
- Send `auth.secret` (or `secret`) in request payloads.

## Crypto-network integration guidance

- GAS node = control plane (state/dispatch), not transport plane.
- Native executables remain transport/security participants.
- For multi-process local mesh:
  - use unique local ports per process,
  - keep protocol/auth semantics consistent,
  - keep shared trust/issuer policy aligned.

