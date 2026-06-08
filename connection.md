# Task: Make Unreal Connection Setup Beginner-Friendly

The desktop app and Unreal plugin exist, but connecting the desktop app to Unreal is too confusing for a beginner.

## Goal

Make the process of connecting the desktop app to Unreal Engine as simple as possible for someone with no Unreal Engine experience.

The user should be guided step-by-step from “I opened the app” to “Connected to Unreal,” with clear status messages and no confusing technical language.

## Core Requirements

1. Do not add unrelated features.
2. Do not redesign the entire app.
3. Do not add cloud calls.
4. Do not add telemetry.
5. Do not require an AI model to connect to Unreal.
6. Do not require MCP to connect to Unreal.
7. Preserve offline-first behavior.
8. The app must work even when Unreal is not running.
9. The connection flow must never crash with undefined/null errors.

---

# Part 1: Add a Beginner Connection Wizard

Add a new page or modal called:

`Connect to Unreal`

This should be accessible from:

- Dashboard
- Setup Checklist
- Unreal Connection page
- Beginner Mode home screen, if it exists

The wizard should use plain language.

## Wizard Steps

### Step 1 — Open Unreal

Title:

`Step 1: Open your Unreal project`

Body:

`Open the Unreal Engine project you want this tool to control. Keep Unreal open while using the desktop app.`

Button:

`I opened Unreal`

---

### Step 2 — Check Plugin

Title:

`Step 2: Make sure the plugin is enabled`

Body:

`In Unreal, go to Edit → Plugins and search for RevoltEditorBridge. Make sure it is enabled. If Unreal asks you to restart, restart Unreal and come back here.`

Buttons:

- `Plugin is enabled`
- `I need help finding it`

If the user clicks `I need help finding it`, show extra instructions:

1. In Unreal, click `Edit`.
2. Click `Plugins`.
3. Use the search box.
4. Type `RevoltEditorBridge`.
5. Check the Enabled box.
6. Restart Unreal if asked.

---

### Step 3 — Open Revolt Bridge in Unreal

Title:

`Step 3: Open the Revolt Bridge panel`

Body:

`In Unreal, open Tools → Revolt Bridge.`

Button:

`I opened Revolt Bridge`

Add note:

`If you do not see Revolt Bridge under Tools, the plugin may not be enabled or Unreal may need to restart.`

---

### Step 4 — Start Bridge

Title:

`Step 4: Start the local bridge`

Body:

`In the Revolt Bridge panel inside Unreal, click Start Bridge. This lets the desktop app talk to Unreal on your own computer.`

Button:

`I clicked Start Bridge`

Add safety note:

`This uses localhost only by default. It does not require internet.`

---

### Step 5 — Test Connection

Title:

`Step 5: Test the connection`

Body:

`Click the button below to check whether the desktop app can reach Unreal.`

Button:

`Test Connection`

When clicked, call the existing Unreal ping/test endpoint.

Default endpoint:

`http://127.0.0.1:17777`

If the endpoint is editable, hide it behind:

`Advanced connection settings`

---

# Part 2: Connection Result Messages

Use beginner-friendly results.

## Success

Show:

`Connected to Unreal`

Then show:

`You are ready to use Manual Command Mode or AI-assisted tools.`

Buttons:

- `Open Manual Command Mode`
- `Go to Dashboard`
- `Run First Test`

`Run First Test` should call a safe read-only command such as `get_project_summary` or `get_open_level_summary`.

---

## Unreal Not Running / Bridge Not Started

Show:

`The desktop app could not reach Unreal.`

Then show:

`Most likely, Unreal is not open or the Start Bridge button has not been clicked inside Unreal.`

Show fix steps:

1. Open your Unreal project.
2. Go to Tools → Revolt Bridge.
3. Click Start Bridge.
4. Come back here and click Test Connection again.

Buttons:

- `Try Again`
- `Show Advanced Details`

---

## Plugin Not Enabled

If detectable, show:

`The Unreal plugin does not appear to be running.`

Then show:

`Open Unreal, go to Edit → Plugins, enable RevoltEditorBridge, then restart Unreal.`

If not detectable, show this as a possible cause.

---

## Invalid Endpoint

Show:

`The connection address is not valid.`

Then show:

`Use the default address unless you know you changed it inside Unreal.`

Default:

`http://127.0.0.1:17777`

Button:

`Reset to Default`

---

## Port Blocked / Wrong Port

Show:

`The desktop app could not connect on this port.`

Then show:

`Make sure the port shown in the Unreal Revolt Bridge panel matches the port in this app.`

Buttons:

- `Reset to Default`
- `Open Advanced Settings`

---

# Part 3: Simplify the Unreal Connection Page

Update the existing Unreal Connection page so it is beginner-friendly.

It should show:

1. Big connection status card:

   - Connected
   - Not Connected
   - Checking
   - Error

2. Simple instruction card:

   `To connect: Open Unreal → Tools → Revolt Bridge → Start Bridge → Test Connection`

3. Main button:

   `Connect to Unreal`

4. Secondary button:

   `Test Connection`

5. Advanced section collapsed by default:

   - Endpoint URL
   - Port
   - Timeout
   - Last response
   - Last error
   - Raw JSON response

6. Troubleshooting section collapsed by default.

---

# Part 4: Add First-Time Setup Prompt

If the app launches and Unreal is not connected, show a friendly setup card on the dashboard:

Title:

`Connect Unreal to get started`

Body:

`This app works with Unreal through a local bridge. Open your Unreal project, start the Revolt Bridge panel, then test the connection here.`

Button:

`Connect to Unreal`

Do not show scary errors on the dashboard by default.

---

# Part 5: Add One-Click Default Connection Test

Add a simple function:

`testDefaultUnrealConnection()`

It should:

1. Use default endpoint:

   `http://127.0.0.1:17777`

2. Call the existing ping endpoint.

3. Return one of:

   - connected
   - unreachable
   - invalid_response
   - timeout
   - invalid_endpoint
   - unknown_error

4. Never throw uncaught errors.

5. Update connection state safely.

6. Show friendly messages.

---

# Part 6: Add Advanced Connection Settings

Advanced users should still be able to change endpoint.

Add collapsed section:

`Advanced connection settings`

Fields:

- Host
- Port
- Full endpoint URL
- Timeout
- Reset to default

Defaults:

- Host: `127.0.0.1`
- Port: `17777`
- Endpoint: `http://127.0.0.1:17777`
- Timeout: `5000`

Validation:

1. Endpoint must be valid.
2. Default mode only allows localhost / 127.0.0.1.
3. Public external URLs are blocked unless online providers are explicitly enabled.
4. Invalid values show friendly messages.
5. Reset to default restores safe default settings.

---

# Part 7: Add Safe Connection State

Make sure the Unreal connection state always has safe defaults.

Example:

```ts
const defaultUnrealConnection = {
  status: "disconnected",
  endpoint: "http://127.0.0.1:17777",
  host: "127.0.0.1",
  port: 17777,
  timeoutMs: 5000,
  lastCheckedAt: null,
  lastSuccessAt: null,
  lastError: null,
  lastResponse: null
};