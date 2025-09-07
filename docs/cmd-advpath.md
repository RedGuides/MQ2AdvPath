---
tags:
  - command
---

# /advpath

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/advpath [option] [value]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Handles plugin status and activity, this command is most handy for macro developers.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `[On | Off]` | Turns advpath on and off |
| `[pull | nopull]` | Turns pulling mode on and off |
| `[flee | noflee]` | Turns "flee" mode (reverse direction) on and off |
| `customsearch <value>` | Sets the value of customsearch, used for filtering CustomPaths and /play listcustom |
| `intercept` | Toggles if you want it to intercept the EverQuest "/follow" command or not |
| `save` | Saves the changes to customsearch to the advpath.ini settings file |
| `help` | displays a rather sparse help menu |