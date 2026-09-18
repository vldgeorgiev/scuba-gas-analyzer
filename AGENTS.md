# Agent Instructions

- For LVGL APIs, widgets, behavior, and version compatibility, consult the `lvgl` MCP server before answering or modifying code.
- This project uses LVGL 9.5.0. Verify recommendations against that version and do not assume newer APIs are available.
- The active UI is the LVGL Editor export in `lvgl-ui-project`. Do not hand-edit generated C, fonts,
	or images. Change layouts in the Editor sources, export them, and keep application bindings in
	`src/display/UiAdapter.cpp`.
- The analyzer task owns ADCs, sensor power, calibration, effective settings, and Preferences. The UI
	task owns LVGL and editable settings drafts. Preserve the fixed command/result and latest-reading
	queues instead of sharing mutable state across tasks.
- Calibration and warm-up constants belong in `src/app/AnalyzerPolicy.h`. Manual and enabled cold-boot
	air calibration must continue to use the same incremental validation path.
- Keep generated UI integration through `gas-analyzer-ui=symlink://lvgl-ui-project`; do not add staging
	copies, generated-source rewriting, or a second UI runtime.
- Before completing firmware changes, run the focused test first, then as appropriate:
	`pio test -e native -e native-ui` and
	`pio run -e t-display-s3 -e t-display-s3-release`. These checks do not replace device validation.
- Consult `docs/testing.md` before test or CI work. Keep its suite counts, coverage map, environment
	selection, CI steps, and validation boundaries current whenever related code or configuration changes.
- Treat `docs/improvement-plan.md` as the current scope, `docs/architecture.md` as the current design,
	and the dated sections of `docs/progress.md` as historical snapshots. Do not infer current pending
	work from an older dated entry when the status table says it is complete.
