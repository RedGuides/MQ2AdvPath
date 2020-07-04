# MQ2AdvPath

This plugin allows you to record and playback player movement, and follow more precisely

## afollow

```txt
========= Advance Pathing Help =========
/afollow [on|off] [pause|unpause] [slow|fast]
/afollow spawn # [slow|fast] - default=fast
/afollow help
=======================================================
```

## record

```txt
========= Advance Pathing Help =========
/record
/record save <PathName> ##
/record Checkpoint <checkPointName>
/record Help## is the distance between checkpoints to force checkpoints to be writen to the path file
=======================================================
```

## play

```txt
========= Advance Pathing Help =========
/play [pathName|off] [stop] [pause|unpause] [loop|noloop] [smart|nosmart] [flee|noflee] [door|nodoor] [fast|slow] [eval|noeval]

[listcustom] [show] [help] [setflag1]-[setflag9] [y/n] [resetflags]
The /play command will execute each command on the line. The commands that should be used single, or at the end of a line:
[off] plist] [listcustom] [show] [help]

setflag1-setflag9 can be used anywhere in the line and uses the following format:
/play setflag1 n pathname or /play pathname setflag n (n can be any single alpha character)
AdvPath flags can be accessed using ${AdvPath.Flag1} - ${AdvPath.Flag9}
/play resetflags resets all flags(1-9) to 'y'
=======================================================
```

## Misc

```txt
*Eval -- If the current location has a checkpoint that starts with slash (/) Evalute it like mq2melee does.
Loop -- continues play back
Pause -- stops the play back at the current point.
unPause -- if paused continue from where left off.
Normal -- play path forward.
Reverse -- play path backward
Slow -- Turns on walk and tights up path. Use it for tight spaces.
Door -- Autoopen doors
Smart -- Start play back from nearest way point
*Zone -- If you zone and new zone has path play it. Example: "/play zone pok innothule guk plspot" could take you from plane of knowledge to your pl spot in guk.
```

## TLO

```txt
${AdvPath.Active} - Plugin Loaded and ready
${AdvPath.State} - FollowState, 0 = off, 1 = Following, 2 = Playing, 3 = Recording
${AdvPath.Waypoints} - Total Number of Waypoints
${AdvPath.NextWaypoint} - Number of NextWaypoint
${AdvPath.Y[Check Point Name OR Waypoint number]} LOC
${AdvPath.X[Check Point Name OR Waypoint number]} LOC
${AdvPath.Z[Check Point Name OR Waypoint number]} LOC
${AdvPath.Monitor} - Spawn your following
${AdvPath.Idle} - Idle time when following and not moving
${AdvPath.Length} - Estimated length off the follow path
${AdvPath.Following} - BOOL Following spawn
${AdvPath.Playing} - BOOL Playing
${AdvPath.Recording} - BOOL Recording
${AdvPath.Status} - INT Status 0 = off , 1 = on , 2 = paused
${AdvPath.Paused} - BOOL Paused
```

## MQ2AdvPath.ini

```ini
[settings.server.ToonName]
AutoStopFollow=0
AutoStopPath=0
UseStuckLogic=1
```

```txt
AutoStopFollow=0/1/2 - how it works now/pause/turns off
AutoStopPath=0/1/2 - how it works now/pause/turns off
UseStuckLogic= 0/1 = off/on
```