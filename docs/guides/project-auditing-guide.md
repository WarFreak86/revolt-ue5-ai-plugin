# Project Auditing Guide

Audits are read-only.

## Commands

- `run_project_audit`
- `run_full_project_audit`
- `audit_current_level`
- `audit_selected_actors`
- `audit_blueprints`
- `audit_assets`
- `audit_generated_content`
- `generate_fix_plan`
- `export_audit_report`

## Fix Plans

`generate_fix_plan` returns recommendations only. It does not modify assets, actors, or packages.

Each plan item includes severity, affected asset or actor, recommended fix, automation hint, permission level, and suggested bridge command where applicable.

## Editor UI

Open `Tools > Revolt Bridge`, then use the `Project Audit` section.

Available buttons:

- Audit Current Level
- Audit Selected Actors
- Audit Assets
- Audit Blueprints
- Audit Generated Content
- Run Full Project Audit
- Generate Fix Plan
- Export Audit JSON

Audit JSON is local-only and read-only. Export copies the latest audit JSON locally and does not modify project content.
