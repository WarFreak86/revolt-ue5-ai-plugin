# Manual Command Mode Guide

Manual Command Mode makes the system useful without internet, cloud providers, or local models.

## Open

1. Launch the desktop runtime.
2. Open `Manual Command Mode`.
3. Pick a common tool.
4. Review the raw JSON request.
5. Click `Run Manual Command`.

## Mutating Commands

Mutating commands default to `dry_run: true` and require explicit approval before changes apply.

Use dry-run first, inspect the response, then check `Approved` only when the action is safe.

## Raw JSON

The UI shows request and response JSON for advanced users and debugging.

## History Export

Open `Command History` and click `Export History JSON` to write a local JSON file.
