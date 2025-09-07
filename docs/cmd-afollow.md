---
tags:
  - command
---

# /afollow

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/afollow [spawn <#>] [Option]...
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Controls what and how you're following.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `[on|off]` | On will follow, off will clear all follow orders |
| `[pause|unpause]` | Will pause or unpause following |
| `spawn #(id)` | Will follow the spawn ID given |
| `[door|nodoor]` | afollow will automatically open doors (default) unless *nodoor* is specified. |
| `help` | will display a rather sparse help text |
| `intercept` | Toggle mq2advpath's interception of eq's [/follow](../everquest/commands/cmd-follow.md) command. Turn it off to separate /follow and /afollow. Issuing a '[/advpath save](cmd-advpath.md)' after this would then save that setting for future usage. |

## Examples

- Change the interception of [/follow](../everquest/commands/cmd-follow.md) and save the setting
  ```eqcommand
  /afollow intercept
  /advpath save
  ```