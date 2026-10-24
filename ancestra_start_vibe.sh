#!/usr/bin/env bash
set -euo pipefail
cd /home/we6jbo/.treeresearchoct2326
if ! command -v vibe >/dev/null 2>&1; then
  echo 'Mistral Vibe CLI (vibe) not found in PATH.' >&2
  exit 1
fi
if [[ ! -f init.md || ! -f research_protocol.md ]]; then
  echo 'Missing init.md or research_protocol.md. Open Ancestra and reinstall its research files before proceeding.' >&2
  exit 2
fi
printf 'Ancestra research starting in %s\n' "$PWD"
# Positional initial prompt starts the normal INTERACTIVE Vibe UI.
# Do not use -p: that switches Vibe into non-interactive mode.
PROMPT=$(cat <<'ANCESTRA_RESEARCH_PROMPT'
Begin the Ancestra family research workflow NOW; do not stop at a blank prompt.
Read init.md and research_protocol.md first, then explain briefly what you found and begin the first actionable step. Do not skip the interactive approvals.
1. Review plan.md for website permissions, search sources, research plan and evidence. If missing, create it interactively with me; do not replace existing plans.
2. Review moltbook.md for privacy, potential outreach, and my explicit permission before any publishing or other outward sharing. Never publish automatically.
3. Review tree.md for missing relationships and stories, plus the priorities and status checkboxes in researched_status.json. Ask me questions for interactive-requested people and wait for my response.
4. Review external/manifest.json and linked External source files, including Luedtke and grouped family leads. Treat all newly supplied claims as unconfirmed until corroborated. Distinguish Vibe read/retained status from actually posting to Moltbook and record evidence before changing statuses.
5. Read and reconcile unconfirmed.md and confirmed.md; preserve prior work, sources, and uncertainty. Look for missing stories and family connections under plan.md-approved research sources.
6. At the end of every research session, update Ancestra_Manual_Updates.docx (including no-findings sessions). Keep previous people and open tasks and give carefully sourced instructions for ME to manually update the JSON and SQLite source files. If DOCX tooling is unavailable, explain the blocker and preserve a Markdown draft without falsely claiming a DOCX update.
Do not modify /var/lib/kinmap/kinmap_state.json, the family-tree JSON, or the real stories.db database. Preserve and follow the exact privacy restrictions in the research protocol.
Start by opening init.md and research_protocol.md, then proceed interactively with the first unresolved step.
ANCESTRA_RESEARCH_PROMPT
)
exec vibe "$PROMPT"
