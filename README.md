ioq3df
======

ioq3df is an engine for the Quake III Arena modification Defrag. It is a fork of ioquake3 to support Defrag specific features.


iodfe features by runaos:

snap hud:

iodfe_hud_snap_draw 1	- snapping hud, shows zones of possible acceleration (for 8 ms frametime)
iodfe_hud_snap_auto	- auto-shifting angle of the hud for different strafe styles
iodfe_hud_snap_def	- offset with no keys pressed or with scr_hud_snap_auto 0
iodfe_hud_snap_speed	- calculate zones for the stated speed instead
iodfe_hud_snap_rgba1	- hud 1 color
iodfe_hud_snap_rgba2	- hud 2 color
iodfe_hud_snap_y	- y position
iodfe_hud_snap_h	- height

iodfe_hud_pitch		- angle marks, setting it to "-15 70" for example will put two marks at -15 and 70 degrees of pitch 
iodfe_hud_pitch_rgba	- color
iodfe_hud_pitch_thickness
iodfe_hud_pitch_width
iodfe_hud_pitch_x	- x position

con_timestamp [0-1]	- adds a timestamp at each message in console
con_timedisplay [0-3]	- displays time at input line (1), at right bottom console corner (2) or at both places (3)
con_drawversion		- toggles version at right bottom console corner

in_numpadbug [0-1] 	- fixes non-working numpad on Windows

r_xpos, r_ypos		- game window position
