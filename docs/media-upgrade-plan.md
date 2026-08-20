# Omawrite media upgrade plan

Omawrite will reintroduce images one small, reviewable stage at a time. The
slash menu remains limited to its eight core writing commands until each stage
is visually approved and passes automated tests.

## Design rules

- Keep writing primary. Images should never dominate the page or cover text.
- Use standard Markdown image syntax with document-relative paths whenever
  possible so files remain portable outside Omawrite.
- Bind every surface, control, and state to Omawrite's semantic Omarchy theme
  roles. Do not add literal interface colors.
- Default to a compact preview. Extra controls appear only when an image is
  selected or hovered.
- Add screenshot capture only after ordinary image insertion is stable.
- Keep Code and Table out of this media project; they require separate plans.

## Stage 1 — Read-only image preview

Render one existing Markdown image inline without adding a slash command or
file picker. The source Markdown remains accessible for editing. Constrain the
preview to the writing width and use a modest default height so it does not
take over the document.

Acceptance gate:

- Local relative paths, absolute file URLs, and missing files behave safely.
- Loading a document does not alter its Markdown or modified state.
- Light and dark Omarchy themes remain readable after a live theme switch.
- Undo, cursor movement, search, and scrolling continue to work normally.

## Stage 2 — Minimal image insertion

Add an Image command behind a development flag. A file picker inserts standard
Markdown with useful alt text and prefers a path relative to the saved note.
Do not add screenshot capture or resize controls yet.

Acceptance gate:

- Canceling the picker leaves the document unchanged.
- Paths containing spaces and special characters reopen correctly.
- Save, Save As, recovery, and external-change detection remain correct.

## Stage 3 — Calm sizing controls

Add a compact control shown only for the selected image. Prefer three clear
presets—Small, Medium, and Page width—over persistent plus/minus buttons. Store
the chosen width in portable markup and keep Page width bounded by the inner
writing area.

Acceptance gate:

- The default preview is visibly smaller than Page width.
- Resizing is one undoable edit and never changes the source path or alt text.
- Controls are keyboard accessible, centered, and themed through semantic
  Omarchy roles.

## Stage 4 — Omarchy screenshot insertion

Add Screenshot as a separate command using the supported Omarchy capture
command. Reuse the proven image insertion and sizing path instead of creating a
second renderer.

Acceptance gate:

- Cancel, capture failure, and missing command states are non-destructive.
- A successful capture inserts exactly one image at the original cursor.
- The application never blocks while the capture tool is active.

## Stage 5 — Release hardening

Test multiple images, long documents, theme changes, fractional scaling,
renamed or moved notes, missing assets, printing, and crash recovery. Remove
the development flag and restore Image and Screenshot to the slash menu only
after visual review confirms that the experience remains quiet and simple.
