---
tags:
  - command
---

# /record

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/record [save|checkpoint] <name>
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
handles path recording, including optional checkpoints. Alias: /arecord
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `(no options)` | begins recording a path with default options |
| `save <PathName> ##` | Records a path and saves it to the specified name. ## is the distance between checkpoints. The lower the number, the more checkpoints. e.g. <br> /record save pathname 30 |
| `Checkpoint <checkPointName>` | records checkpoint to the specified name |