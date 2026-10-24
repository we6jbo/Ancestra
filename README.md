# Ancestra v0.2

Qt 6 Widgets/C++17 offline family research viewer. Reads /var/lib/kinmap/kinmap_state.json, /tmp/kinmap_family_tree_26-27.json, /home/we6jbo/.family_story/real stories.db without modifying them. Refreshes every 10 seconds; absent tree file produces a clearly marked missing state. Reads stories by identifier code; no guessing based on a matching name.

Research coverage: 50% relationship coverage for original tree members (one parent and one child relationship recorded), 50% story coverage of all encountered people. This is a mechanical coverage metric and does not establish that everyone had a child. If the baseline tree is unavailable, percentage should not be treated as complete.

Tab 2 initializes /home/we6jbo/.treeresearchoct2326/init.md (never overwrites it) and /tmp/oct23-vibe.sh. It copies the launcher text on request. Vibe CLI invocation cannot force its first prompt; manually tell Vibe to open init.md. Vibe prepares other markdown files and DOCX per the prompt (requires appropriate Vibe tooling). No remote services or network ports used by Ancestra.

Only TG918273 and TG654147 are embedded as project provenance. Other identifiers are read at runtime from input files.


## v0.3 transient-source cache
The application checks `/tmp/kinmap_family_tree_26-27.json` every 10 seconds. When it is present and valid, it takes precedence and is atomically mirrored to `~/.cache/ancestra`. When missing or invalid, the last valid snapshot in that cache is loaded. The `/tmp/oct23-vibe.sh` launcher is also cached and can be restored after reboot, without overwriting a customized launcher. Status text identifies live versus fallback tree source. Original `/var/lib/kinmap/kinmap_state.json` and SQLite story DB remain untouched.

## v0.4 recurring research report
`research_protocol.md` is installed alongside the source and copied into the workflow directory by the installer. Vibe should read it before other research steps and update `Ancestra_Manual_Updates.docx` on **every** session. A Reddit genealogy post is a lead, not independently verified evidence. Existing user-written planning files remain untouched.

## v0.5 External and Researched tabs
External sources are imported locally, stored in the durable workflow directory, and tracked using evidence-driven red/green flags. ChatGPT attachments do not sync automatically. A source bundled with this release seeds the first row. Research exhaustion and interactive requests persist in researched_status.json. The DOCX report becomes step 5.

## v0.6 Luedtke source
EXTERNAL row 2 imports the Luedtke-Rice subdomain research inventory: structured JSON, readable text, 28 candidate people and outbound/internal links CSVs. Both indicators initially red, pending Vibe evidence. Upgrade installer merges by stable ID and preserves user/Vibe edits.


## v0.7 External family groups
Eight additional unconfirmed group rows were added (Lester, Norma, Burke, Amelia, Percy, Wilhelmina, Mary Johannsen, and supplementary Amelia parent research). Double-click an External row to read the source. Installer seeds `unconfirmed.md` and `confirmed.md` if absent, without replacing the user's files. Vibe must update both claim-level ledgers and the DOCX during research; merely installing the update does not verify claims or run Vibe.


## v0.7.2 interactive Vibe startup
The existing Vibe Research Copy button copies `bash "/home/we6jbo/.treeresearchoct2326/ancestra_start_vibe.sh"`. Only the user manually pastes and executes that command in their own terminal. The launcher uses the positional `vibe "$PROMPT"` interactive CLI form; not `-p` and never auto-approval. It reads init.md and research_protocol.md, continues plan.md -> moltbook.md -> tree.md -> EXTERNAL -> DOCX, retaining privacy and manual-update boundaries. The managed launcher lives outside /tmp and survives reboot.
