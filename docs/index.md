---
tags:
  - plugin
resource_link: "https://www.redguides.com/community/resources/mq2advpath.95/"
support_link: "https://www.redguides.com/community/threads/mq2advpath.66802/"
repository: "https://github.com/RedGuides/MQ2AdvPath"
config: "MQ2AdvPath.ini"
authors: "A_Enchanter_00, alynel, brainiac, ctaylor22, eqmule, Knightly, Sym"
tagline: "This plugin allows you to record and playback player movement, and follow more precisely."
---

# MQ2AdvPath
<!--desc-start-->
This plugin allows you to record and playback player movement, as well as follow more precisely.
<!--desc-end-->

## Commands

<a href="cmd-advpath/">
{% 
  include-markdown "projects/mq2advpath/cmd-advpath.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2advpath/cmd-advpath.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2advpath/cmd-advpath.md') }}

<a href="cmd-afollow/">
{% 
  include-markdown "projects/mq2advpath/cmd-afollow.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2advpath/cmd-afollow.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2advpath/cmd-afollow.md') }}

<a href="cmd-play/">
{% 
  include-markdown "projects/mq2advpath/cmd-play.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2advpath/cmd-play.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2advpath/cmd-play.md') }}

<a href="cmd-record/">
{% 
  include-markdown "projects/mq2advpath/cmd-record.md" 
  start="<!--cmd-syntax-start-->" 
  end="<!--cmd-syntax-end-->" 
%}
</a>
:    {% include-markdown "projects/mq2advpath/cmd-record.md" 
        start="<!--cmd-desc-start-->" 
        end="<!--cmd-desc-end-->" 
        trailing-newlines=false 
     %} {{ readMore('projects/mq2advpath/cmd-record.md') }}

## Settings

| Setting | Description |
|---------|-------------|
| `AutoStopFollow=0/1/2` | Automatically stop following on certain conditions: yes/pause/off |
| `AutoStopPath=0/1/2`  | Automatically stop pathing on certain conditions: yes/pause/off |
| `UseStuckLogic= 0/1` | Use stuck logic: off/on                  |
| `InterceptFollow = 0/1` | Replace Default EQ `/follow` with `/afollow` |

Example MQ2AdvPath.ini,
```ini
[settings.server.ToonName]
AutoStopFollow=0
AutoStopPath=0
UseStuckLogic=1
InterceptFollow = FALSE
```

## Advanced Tutorial

From ctaylor22 on the [RedGuides Forums](https://www.redguides.com/community/threads/mq2advpath.66802/post-271280),

```text
Written by: ctaylor22

MQ2AdvPath will check checkpoints to see if it is a command that can be executed. A normal path checkpoint 
looks like the following:

   [apath1]
   1=766.86 514.00 -157.15 
   
A path checkpoint with a named checkpoint looks like the following:
   
   [apath1]
   1=766.86 514.00 -157.15 Checkpoint1
   
A checkpoint used for executing a command looks like this:

   [apath1]
   1=766.86 514.00 -157.15 /docommand somecommand
   
   or
   
   [apath1]
   1=766.86 514.00 -157.15 /echo anything you want

   or

   [apath1]
   1=766.86 514.00 -157.15 /if (${AdvPath.Pulling}) ${${If[!${AdvPath.Fleeing} && ${AdvPath.Direction.Equal[n]} 
   && ${AdvPath.Flag1.Equal[y]},/play stop apath2 normal,/echo Continuing]}}
 
 
The first position of the checkpoint has to contain "/" for the plugin to execute it. So with that said, lets 
move on to how you would pull this all together. I will try and keep this simple, so we will work with 3 paths: 
apath1, apath2, and apath3.

apath1 will be the main path and will branch out to apath2 and apath3. 
 
                  apath2
                    |                                            
                    |                                            
                    |                                            
                    |                                            
                    |                                            
                    |                                            
                    |                                            
 B                  |                                            
 e                  |                                            
 g  -----------------------------------------------------------------------------------------------------------  
 i  apath1
 n                                                                           |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                             |
                                                                           apath3  
   
What we want to do is to be able to /play normal apath1 and start playing apath1 to apath2, and then run 
apath2 in a loop(beginning to end, end to beginning). That is what the pulling mode(${AdvPath.Pulling}) does. 
When we get back to the beginning of apath2 we want to continue playing apath1 towards apath3. Then when 
we reach apath3 we want to do the same as apath2. When we get back to the beginning of apath3 we will want 
to continue running apath1.

Here is a simple path file that contains all the checkpoints that will do what I just described above.

[apath1]
1=766.86 514.00 -157.15 
2=770.61 626.22 -156.25 
3=769.67 716.44 -156.25 /docommand ${If[${AdvPath.Pulling} ${AdvPath.Direction.Equal[n]} && 
${AdvPath.Flag1.Equal[y]},/play stop apath2 normal,/echo Continuing]}
4=760.46 811.97 -157.25 
5=763.25 945.13 -157.25 
6=764.66 1057.63 -156.25 /docommand ${If[${AdvPath.Pulling} && ${AdvPath.Direction.Equal[n]} && 
${AdvPath.Flag2.Equal[y]},/play stop apath3 normal,/echo Continuing]}
7=791.19 1143.38 -156.25 
8=792.85 1148.35 -156.25 
9=824.54 1227.75 -157.25 

[apath2]
1=728.23 714.80 -148.59 /docommand ${If[${AdvPath.Pulling},/play stop setflag1 n apath1 smart normal,/play 
off]}
2=718.46 714.41 -143.69 
3=639.49 720.02 -116.25 
4=613.05 718.25 -116.25 
5=465.88 716.07 -125.25 
6=460.63 716.01 -125.25 
7=397.28 711.42 -125.25 

[apath3]
1=759.69 1068.18 -156.25 /docommand ${If[${AdvPath.Pulling},/play stop setflag2 n apath1 smart normal,/play 
off]}
2=665.82 1064.70 -117.35 
3=607.60 1067.44 -116.25 
4=562.37 1065.06 -124.25 
5=557.40 1064.35 -124.25 
6=553.60 1060.95 -124.25 
7=556.04 988.76 -124.25 
8=550.15 900.34 -124.25 
   
The above does not take into consideration pulling mobs and having aggro. Part of it is being handeled 
above in apath2 and apath3, but only the part when you run the path and return with no aggro. See below:

1=728.23 714.80 -148.59 /docommand ${If[${AdvPath.Pulling},/play stop setflag1 n apath1 smart normal,/play 
off]}
1=759.69 1068.18 -156.25 /docommand ${If[${AdvPath.Pulling},/play stop setflag2 n apath1 smart normal,/play 
off]}

what shuts down the path from being run next time around is the "setflag1 n" this sets the internal advpath 
flag1=n. Now notice apath1 checkpoint #3's. checkpoint command:

/docommand ${If[${AdvPath.Pulling} ${AdvPath.Direction.Equal[n]} && ${AdvPath.Flag1.Equal[y]},/play stop apath2 
normal,/echo Continuing]}

There is a check for advpath flag1 ${AdvPath.Flag1.Equal[y]} by setting Flag1=n apath2 will be skipped until 
Flag1 gets set back to "y".

Same thing goes for apath3 just using a different flag. Flag2 is used for apath3.
 
OK. Now all that's fine and dandy, but how do we get this to work when we are actually pulling mobs, and we 
want to run the path back to camp and not away from camp? All the above paths do not take any of that into 
consideration. This is what the flee|noflee was created for. This sets an internal flag in advpath letting 
the plugin know not only are we running in a reverse direction, but we are doing so with a purpose.  

This is what would need to be changed in the pathing file paths to take into consideration when we are fleeing.

[apath1]
1=766.86 514.00 -157.15 
2=770.61 626.22 -156.25 
3=769.67 716.44 -156.25 /docommand ${If[${AdvPath.Pulling} && !${AdvPath.Fleeing} && 
${AdvPath.Direction.Equal[n]} && ${AdvPath.Flag1.Equal[y]},/play stop apath2 normal,/echo Continuing]}
4=760.46 811.97 -157.25 
5=763.25 945.13 -157.25 
6=764.66 1057.63 -156.25 /docommand ${If[${AdvPath.Pulling} && !${AdvPath.Fleeing} && 
${AdvPath.Direction.Equal[n]} && ${AdvPath.Flag2.Equal[y]},/play stop apath3 normal,/echo Continuing]}
7=791.19 1143.38 -156.25 
8=792.85 1148.35 -156.25 
9=824.54 1227.75 -157.25 

[apath2]
1=728.23 714.80 -148.59 /docommand ${If[${AdvPath.Pulling} && ${AdvPath.Fleeing},/play stop apath1 smart,/play 
stop setflag1 n apath1 smart normal]}
2=718.46 714.41 -143.69 
3=639.49 720.02 -116.25 
4=613.05 718.25 -116.25 
5=465.88 716.07 -125.25 
6=460.63 716.01 -125.25 
7=397.28 711.42 -125.25 

[apath3]
1=759.69 1068.18 -156.25 /docommand ${If[${AdvPath.Pulling} && ${AdvPath.Fleeing},/play stop apath1 smart,/play 
stop setflag2 n apath1 smart normal]}
2=665.82 1064.70 -117.35 
3=607.60 1067.44 -116.25 
4=562.37 1065.06 -124.25 
5=557.40 1064.35 -124.25 
6=553.60 1060.95 -124.25 
7=556.04 988.76 -124.25 
8=550.15 900.34 -124.25 
 
In your macro you just have to write the pulling logic and start the pull with /play apath1 normal fast 
noflee. Somewhere along the path you have to stop to pull a mob (/play pause) and when you get agro /play flee 
unpause, or /play flee when your not paused. 

An easy way to open up apath2 and apath3 would be to check when returning from apath1 and check if             
!{AdvPath.Fleeing}, this would only be true if I ran the full length of apath1 and never gained any aggro from 
a mob. /play resetflags, or /play setflag1 y setflag2 y, would then open up apath2 and apath3 for use again.
 
/play reverse and /play flee function slightly differently. /play reverse will start running in reverse after 
it reaches the current checkpoint. /play flee sets you in reverse, but it gets the previous checkpoint and sets 
you in that checkpoints direction.

With these changes, most of the pathing logic can be managed in the pathing file. All you have to do is set 
it playing the path to start with, and this allows others to customize the pathing file as they see fit.
```

## See also
- [Nav](../mq2nav/index.md)

## Top-Level Objects

## [AdvPath](tlo-advpath.md)
{% include-markdown "projects/mq2advpath/tlo-advpath.md" start="<!--tlo-desc-start-->" end="<!--tlo-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2advpath/tlo-advpath.md') }}

## DataTypes

## [AdvPath](datatype-advpath.md)
{% include-markdown "projects/mq2advpath/datatype-advpath.md" start="<!--dt-desc-start-->" end="<!--dt-desc-end-->" trailing-newlines=false %} {{ readMore('projects/mq2advpath/datatype-advpath.md') }}

<h2>Members</h2>
{% include-markdown "projects/mq2advpath/datatype-advpath.md" start="<!--dt-members-start-->" end="<!--dt-members-end-->" %}
{% include-markdown "projects/mq2advpath/datatype-advpath.md" start="<!--dt-linkrefs-start-->" end="<!--dt-linkrefs-end-->" %}
