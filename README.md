# AnitoTracer
Repository for the Anito Tracer

## Project assets

Models and audio clips are identified by the GUID in their adjacent `.ani`
sidecar, not by the path saved in a scene or prefab. Keep each sidecar with its
asset when moving, copying, or sharing a project. New sidecars record paths
relative to their asset root; older sidecars with absolute paths remain valid.
If a sidecar is missing, a new GUID is created and existing references to the
old GUID cannot resolve. Restore the original sidecar from a backup or version
control to recover those references. Conflicting or invalid sidecars are
reported and left unchanged rather than being assigned new GUIDs.

Use **File > Rescan Current Folder for Assets** or Ctrl+R to recursively import
new models (`.obj`, `.fbx`) and audio clips (`.wav`) in the folder currently
open in the File Explorer. The scan does not reload changed assets already in
memory; check the log for import errors.

## Window layouts

Docked panels keep their horizontal and vertical split proportions when the
application window is resized. Drag a splitter to change those proportions;
they are also respected when a saved layout is loaded at a different window
size. ImGui's minimum panel sizes may limit the proportions in very small
windows. Floating windows retain their normal independent sizes.

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
