---
tags:
  - datatype
---
# `AdvPath`

<!--dt-desc-start-->
Displays that status of various settings in MQ2AdvPath
<!--dt-desc-end-->

## Members
<!--dt-members-start-->
### {{ renderMember(type='bool', name='Active') }}

:   Plugin Loaded and ready

### {{ renderMember(type='string', name='CheckPoint') }}

:   Checkpoint name

### {{ renderMember(type='int', name='CustomPaths') }}

:   Gives a count filtered by CustomSearch

### {{ renderMember(type='string', name='CustomSearch') }}

:   contains value used for filtering CustomPaths and /play listcustom

### {{ renderMember(type='string', name='Direction') }}

:   N/R, Normal or Reverse

### {{ renderMember(type='string', name='Flag#') }}

:   Can access flags 1 through 9

### {{ renderMember(type='bool', name='Fleeing') }}

:   

### {{ renderMember(type='bool', name='Following') }}

:   Following spawn

### {{ renderMember(type='int', name='Idle') }}

:   Idle time when following and not moving

### {{ renderMember(type='float', name='Length') }}

:   Estimated length off the follow path

### {{ renderMember(type='spawn', name='Monitor') }}

:   spawn you are following

### {{ renderMember(type='int', name='NextWaypoint') }}

:   Number of NextWayPoint

### {{ renderMember(type='string', name='Path') }}

:   Returns closest path

### {{ renderMember(type='bool', name='Paused') }}

:   

### {{ renderMember(type='bool', name='Playing') }}

:   

### {{ renderMember(type='bool', name='Pulling') }}

:   

### {{ renderMember(type='bool', name='Recording') }}

:   

### {{ renderMember(type='int', name='State') }}

:   FollowState, 0 = off, 1 = Following, 2 = Playing, 3 = Recording

### {{ renderMember(type='int', name='Status') }}

:   Status 0 = off , 1 = on , 2 = paused

### {{ renderMember(type='bool', name='WaitingWarp') }}

:   

### {{ renderMember(type='int', name='Waypoints') }}

:   Total Number of Waypoints

### {{ renderMember(type='string', name='X[name/#]') }}

:   Checkpoint name or Waypoint number, returns LOC or checkpoint name

### {{ renderMember(type='string', name='Y[name/#]') }}

:   

### {{ renderMember(type='string', name='Z[name/#]') }}

:   

<!--dt-members-end-->

<!--dt-linkrefs-start-->
[bool]: ../macroquest/reference/data-types/datatype-bool.md
[float]: ../macroquest/reference/data-types/datatype-float.md
[int]: ../macroquest/reference/data-types/datatype-int.md
[spawn]: ../macroquest/reference/data-types/datatype-spawn.md
[string]: ../macroquest/reference/data-types/datatype-string.md
<!--dt-linkrefs-end-->
