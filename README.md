# AnitoTracer
Repository for the Anito Tracer

## Script editor

Double-click an `.ascript` file in the File Explorer to open or focus it in the
Script Editor. The panel is also available from the Windows menu. Each file has
its own tab, selection, and undo history. Closing a modified tab offers Save,
Discard, and Cancel; closing the panel hides it without closing its tabs.

Use Ctrl+S to save, Ctrl+Z and Ctrl+Y (or Ctrl+Shift+Z) to undo and redo,
Ctrl+C/X/V to copy, cut, and paste, and Ctrl+D to duplicate the selection or
current line. Shift+arrow extends the selection, and Ctrl+Shift+Left/Right
extends it by word. Ctrl+A selects all. Mouse drag selects text; double-click
selects a word and triple-click selects a line.

Save modified scripts before exiting the engine: unsaved tabs are held in memory
and are not restored on restart. The editor preserves a UTF-8 BOM and consistent
LF or CRLF line endings; files with mixed line endings are normalized on save.
There is currently no syntax highlighting.
