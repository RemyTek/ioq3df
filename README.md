ioq3df
======

ioq3df is an engine for the Quake III Arena modification Defrag. It is a fork of ioquake3 to support Defrag specific features.

Auto download and auto load of single pk3 files for each map.  
You can move all your map pk3 files to the autoload/maps/ folder.  
The auto download will download map dependency files to autoload/mapdeps/\*.json and pk3 files to autoload/maps/\*.pk3.  
cl_mapAutoDownload 		[0/1]  
cl_mapAutoDownload\_source  
fs_autoload 			[0/1]  
New commands:  
downloadMap <filename>  
downloadPk3 <filename>


Chat console:  
To open the chat console use ctrl+tab or bind a key to one of the new commands:  
togglechatconsole  
chatclear  
chatdump
+chat		to use it like the scoreboard bind a key with \bind p +chat


r_defaultImage to replace the default (missing texture) images  
i.e. \seta r_defaultImage "textures\liquids\bubbles.tga"

iodfe features by runaos:  
snap hud:

iodfe_hud\_snap_draw 1	- snapping hud, shows zones of possible acceleration (for 8 ms frametime)  
iodfe_hud\_snap_auto	- auto-shifting angle of the hud for different strafe styles  
iodfe_hud\_snap_def	- offset with no keys pressed or with scr_hud_snap_auto 0  
iodfe_hud\_snap_speed	- calculate zones for the stated speed instead  
iodfe_hud\_snap_rgba1	- hud 1 color  
iodfe_hud\_snap_rgba2	- hud 2 color  
iodfe_hud\_snap_y	- y position  
iodfe_hud\_snap_h	- height

iodfe_hud\_pitch		- angle marks, setting it to "-15 70" for example will put two marks at -15 and 70 degrees of pitch   
iodfe_hud\_pitch_rgba	- color  
iodfe_hud\_pitch_thickness  
iodfe_hud\_pitch_width  
iodfe_hud\_pitch_x	- x position

con_timestamp [0-1]	- adds a timestamp at each message in console  
con_timedisplay [0-3]	- displays time at input line (1), at right bottom console corner (2) or at both places (3)  
con_drawversion		- toggles version at right bottom console corner

in_numpadbug [0-1] 	- fixes non-working numpad on Windows

r_xpos, r_ypos		- game window position

dfengine features:
con_useshader [0-1]  
con_rgb  
con_opacity [0.0-1.0]


ioquake3 does use the folder %appdata%\Quake3 on windows to store config and such files.  
If you want to use the original Quake install path add these start parameters:  
+set fs_homepath "C:\pathTo\Quake III Arena"
