---
tags:
  - command
---

# /play

## Syntax

<!--cmd-syntax-start-->
```eqcommand
/play [option]... [setting]
```
<!--cmd-syntax-end-->

## Description

<!--cmd-desc-start-->
Plays a previously recorded path. Includes ability to change paths and execute commands.
<!--cmd-desc-end-->

## Options

| Option | Description |
|--------|-------------|
| `[pathName | off]` | plays the path, or stops playing the path |
| `[slow | fast]` | walks or runs the path |
| `[smart|nosmart]` | Start playback from nearest way point, or start from the beginning. |
| `[loop|noloop]` | Loop = Continuous playback<br><br>noloop = single playback |
| `[flee|noflee]` | reverse playback, or don't |
| `[door|nodoor]` | automatically open doors or not |
| `[eval|noeval]` | If the current location has a checkpoint that starts with slash (/) evaluate it like mq2melee does. |
| `show` | brings up a graphical user interface |
| `[list|listcustom]` | * list=show a list of all available paths<br>* listcustom=show paths matching customsearch, a value set with /advpath |
| `help` | displays a rather sparse help text |
| `setflag{1-9} *n` | sets flags for internal playback logic. * can be any alpha character, defaults to "y" |
| `resetflags` | resets all flags(1-9) to "y" |
| `[normal|reverse]` | play path forward or backward |
| `zone` | If you zone and new zone has path play it. Example: "/play zone pok innothule guk plspot" could take you from plane of knowledge to your pl spot in guk. |

## Examples

- stops play, sets flag2 to "n" on custom path named "apath1"

```eqcommand
/play stop setflag2 n apath1 smart normal
```