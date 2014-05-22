/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// chat console.c

#include "client.h"


int g_chatconsole_field_width = 78;


#define	NUM_CHATCON_TIMES 4

#define		CHATCON_TEXTSIZE	32768
typedef struct {
	qboolean	initialized;

	short	text[CHATCON_TEXTSIZE];
	int		current;		// line where next message will be printed
	int		x;				// offset in current line for next print
	int		display;		// bottom of console displays this line

	int 	linewidth;		// characters across screen
	int		totallines;		// total lines in console scrollback

	float	xadjust;		// for wide aspect screens

	float	displayFrac;	// aproaches finalFrac at scr_conspeed
	float	finalFrac;		// 0.0 to 1.0 lines of console to display

	int		vislines;		// in scanlines

	int		times[NUM_CHATCON_TIMES];	// cls.realtime time the line was generated
								// for transparent notify lines
	vec4_t	color;
} chatconsole_t;

chatconsole_t	chatcon;

//iodfe
cvar_t		*cl_chat_timestamp;
cvar_t		*cl_chat_timedisplay;
cvar_t		*cl_chat_drawversion;

cvar_t		*cl_chat_conspeed;

#define	DEFAULT_CHATCONSOLE_WIDTH	78


/*
================
ChatCon_ToggleConsole_f
================
*/
void ChatCon_ToggleConsole_f (void) {
	// Can't toggle the console when it's the only thing available
	if ( clc.state == CA_DISCONNECTED && Key_GetCatcher( ) == KEYCATCH_CONSOLE ) {
		return;
	}

	Field_Clear( &g_chatconsoleField );
	g_chatconsoleField.widthInChars = g_chatconsole_field_width - (cl_chat_timedisplay->integer ? 9 : 0);

	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CONSOLE );
	Key_SetCatcher( Key_GetCatcher( ) ^ KEYCATCH_CHATCONSOLE );
}

/*
===================
ChatCon_ToggleMenu_f
===================
*/
void ChatCon_ToggleMenu_f( void ) {
	CL_KeyEvent( K_ESCAPE, qtrue, Sys_Milliseconds() );
	CL_KeyEvent( K_ESCAPE, qfalse, Sys_Milliseconds() );
}

/*
================
ChatCon_Clear_f
================
*/
void ChatCon_Clear_f (void) {
	int		i;

	for ( i = 0 ; i < CHATCON_TEXTSIZE ; i++ ) {
		chatcon.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}

	ChatCon_Bottom();		// go to end
}

						
/*
================
ChatCon_Dump_f

Save the console contents out to a file
================
*/
void ChatCon_Dump_f (void)
{
	int		l, x, i;
	short	*line;
	fileHandle_t	f;
	int		bufferlen;
	char	*buffer;
	char	filename[MAX_QPATH];

	if (Cmd_Argc() != 2)
	{
		Com_Printf ("usage: chatdump <filename>\n");
		return;
	}

	Q_strncpyz( filename, Cmd_Argv( 1 ), sizeof( filename ) );
	COM_DefaultExtension( filename, sizeof( filename ), ".txt" );

	f = FS_FOpenFileWrite( filename );
	if (!f)
	{
		Com_Printf ("ERROR: couldn't open %s.\n", filename);
		return;
	}

	Com_Printf ("Dumped console text to %s.\n", filename );

	// skip empty lines
	for (l = chatcon.current - chatcon.totallines + 1 ; l <= chatcon.current ; l++)
	{
		line = chatcon.text + (l%chatcon.totallines)*chatcon.linewidth;
		for (x=0 ; x<chatcon.linewidth ; x++)
			if ((line[x] & 0xff) != ' ')
				break;
		if (x != chatcon.linewidth)
			break;
	}

#ifdef _WIN32
	bufferlen = chatcon.linewidth + 3 * sizeof ( char );
#else
	bufferlen = chatcon.linewidth + 2 * sizeof ( char );
#endif

	buffer = Hunk_AllocateTempMemory( bufferlen );

	// write the remaining lines
	buffer[bufferlen-1] = 0;
	for ( ; l <= chatcon.current ; l++)
	{
		line = chatcon.text + (l%chatcon.totallines)*chatcon.linewidth;
		for(i=0; i<chatcon.linewidth; i++)
			buffer[i] = line[i] & 0xff;
		for (x=chatcon.linewidth-1 ; x>=0 ; x--)
		{
			if (buffer[x] == ' ')
				buffer[x] = 0;
			else
				break;
		}
#ifdef _WIN32
		Q_strcat(buffer, bufferlen, "\r\n");
#else
		Q_strcat(buffer, bufferlen, "\n");
#endif
		FS_Write(buffer, strlen(buffer), f);
	}

	Hunk_FreeTempMemory( buffer );
	FS_FCloseFile( f );
}
						

/*
================
ChatCon_CheckResize

If the line width has changed, reformat the buffer.
================
*/
void ChatCon_CheckResize (void)
{
	int		i, j, width, oldwidth, oldtotallines, numlines, numchars;
	short	tbuf[CHATCON_TEXTSIZE];

	width = (SCREEN_WIDTH / SMALLCHAR_WIDTH) - 2;

	if (width == chatcon.linewidth)
		return;

	if (width < 1)			// video hasn't been initialized yet
	{
		width = DEFAULT_CHATCONSOLE_WIDTH;
		chatcon.linewidth = width;
		chatcon.totallines = CHATCON_TEXTSIZE / chatcon.linewidth;
		for(i=0; i<CHATCON_TEXTSIZE; i++)

			chatcon.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
	}
	else
	{
		oldwidth = chatcon.linewidth;
		chatcon.linewidth = width;
		oldtotallines = chatcon.totallines;
		chatcon.totallines = CHATCON_TEXTSIZE / chatcon.linewidth;
		numlines = oldtotallines;

		if (chatcon.totallines < numlines)
			numlines = chatcon.totallines;

		numchars = oldwidth;
	
		if (chatcon.linewidth < numchars)
			numchars = chatcon.linewidth;

		Com_Memcpy (tbuf, chatcon.text, CHATCON_TEXTSIZE * sizeof(short));
		for(i=0; i<CHATCON_TEXTSIZE; i++)

			chatcon.text[i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';


		for (i=0 ; i<numlines ; i++)
		{
			for (j=0 ; j<numchars ; j++)
			{
				chatcon.text[(chatcon.totallines - 1 - i) * chatcon.linewidth + j] =
						tbuf[((chatcon.current - i + oldtotallines) %
							  oldtotallines) * oldwidth + j];
			}
		}

	}

	chatcon.current = chatcon.totallines - 1;
	chatcon.display = chatcon.current;
}


/*
================
ChatCon_Init
================
*/
void ChatCon_Init (void) {
	int		i;

	//iodfe
	cl_chat_timestamp = Cvar_Get ("cl_chat_timestamp", "1", CVAR_ARCHIVE);
	cl_chat_timedisplay = Cvar_Get ("cl_chat_timedisplay", "3", CVAR_ARCHIVE);
	cl_chat_drawversion = Cvar_Get ("cl_chat_drawversion", "1", CVAR_ARCHIVE);

	cl_chat_conspeed = Cvar_Get ("cl_chat_conspeed", "3", 0);

	Field_Clear( &g_chatconsoleField );
	g_chatconsoleField.widthInChars = g_chatconsole_field_width;
/*	for ( i = 0 ; i < COMMAND_HISTORY ; i++ ) {
		Field_Clear( &historyEditLines[i] );
		historyEditLines[i].widthInChars = g_chatconsole_field_width;
	}
	CL_LoadConsoleHistory( );
*/
	Cmd_AddCommand ("togglechatconsole", ChatCon_ToggleConsole_f);
	Cmd_AddCommand ("togglechatmenu", ChatCon_ToggleMenu_f);
	Cmd_AddCommand ("chatclear", ChatCon_Clear_f);
	Cmd_AddCommand ("chatdump", ChatCon_Dump_f);
	Cmd_SetCommandCompletionFunc( "chatdump", Cmd_CompleteTxtName );

	//initialising chat console
	if (!chatcon.initialized) {
		chatcon.color[0] = 
		chatcon.color[1] = 
		chatcon.color[2] =
		chatcon.color[3] = 1.0f;
		chatcon.linewidth = -1;
		ChatCon_CheckResize ();
		chatcon.initialized = qtrue;
	}

}

/*
================
ChatCon_Shutdown
================
*/
void ChatCon_Shutdown(void)
{
	Cmd_RemoveCommand("togglechatconsole");
	Cmd_RemoveCommand("togglechatmenu");
	Cmd_RemoveCommand("chatclear");
	Cmd_RemoveCommand("chatdump");
}

/*
===============
ChatCon_Linefeed
===============
*/
void ChatCon_Linefeed (qboolean skipnotify)
{
	int		i;

	// mark time for transparent overlay
	if (chatcon.current >= 0)
	{
    if (skipnotify)
		  chatcon.times[chatcon.current % NUM_CHATCON_TIMES] = 0;
    else
		  chatcon.times[chatcon.current % NUM_CHATCON_TIMES] = cls.realtime;
	}

	chatcon.x = 0;
	if (chatcon.display == chatcon.current)
		chatcon.display++;
	chatcon.current++;
	for(i=0; i<chatcon.linewidth; i++)
		chatcon.text[(chatcon.current%chatcon.totallines)*chatcon.linewidth+i] = (ColorIndex(COLOR_WHITE)<<8) | ' ';
}

/*
================
CL_ChatConsolePrint

Handles cursor positioning, line wrapping, etc
All console printing must go through this in order to be logged to disk
If no console is visible, the text will appear at the top of the game window
================
*/
void CL_ChatConsolePrint( char *txt ) {
	int		y, l;
	unsigned char	c;
	unsigned short	color;
	qboolean skipnotify = qfalse;		// NERVE - SMF
	int prev;							// NERVE - SMF

	//iodfe
	if (chatcon.x==0 && cl_chat_timestamp && cl_chat_timestamp->integer) {
		char txtt[MAXPRINTMSG];
		qtime_t	now;
		Com_RealTime( &now );
//		if( cl_chat_timestamp->integer == 2 )
//			Com_sprintf(txtt,sizeof(txtt),"^9%02d:%02d ^7%s",now.tm_hour,now.tm_min,txt);
//		else
			Com_sprintf(txtt,sizeof(txtt),"^9%02d:%02d:%02d ^7%s",now.tm_hour,now.tm_min,now.tm_sec,txt);
		strcpy(txt,txtt);
	}

	// TTimo - prefix for text that shows up in console but not in notify
	// backported from RTCW
	if ( !Q_strncmp( txt, "[skipnotify]", 12 ) ) {
		skipnotify = qtrue;
		txt += 12;
	}
	
	// for some demos we don't want to ever show anything on the console
	if ( cl_noprint && cl_noprint->integer ) {
		return;
	}
	
	if (!chatcon.initialized) {
		chatcon.color[0] = 
		chatcon.color[1] = 
		chatcon.color[2] =
		chatcon.color[3] = 1.0f;
		chatcon.linewidth = -1;
		ChatCon_CheckResize ();
		chatcon.initialized = qtrue;
	}

	color = ColorIndex(COLOR_WHITE);

	while ( (c = *((unsigned char *) txt)) != 0 ) {
		if ( Q_IsColorString( txt ) ) {
			color = ColorIndex( *(txt+1) );
			txt += 2;
			continue;
		}

		// count word length
		for (l=0 ; l< chatcon.linewidth ; l++) {
			if ( txt[l] <= ' ') {
				break;
			}

		}

		// word wrap
		if (l != chatcon.linewidth && (chatcon.x + l >= chatcon.linewidth) ) {
			ChatCon_Linefeed(skipnotify);

		}

		txt++;

		switch (c)
		{
		case '\n':
			ChatCon_Linefeed (skipnotify);
			break;
		case '\r':
			chatcon.x = 0;
			break;
		default:	// display character and advance
			y = chatcon.current % chatcon.totallines;
			chatcon.text[y*chatcon.linewidth+chatcon.x] = (color << 8) | c;
			chatcon.x++;
			if(chatcon.x >= chatcon.linewidth)
				ChatCon_Linefeed(skipnotify);
			break;
		}
	}


	// mark time for transparent overlay
	if (chatcon.current >= 0) {
		// NERVE - SMF
		if ( skipnotify ) {
			prev = chatcon.current % NUM_CHATCON_TIMES - 1;
			if ( prev < 0 )
				prev = NUM_CHATCON_TIMES - 1;
			chatcon.times[prev] = 0;
		}
		else
		// -NERVE - SMF
			chatcon.times[chatcon.current % NUM_CHATCON_TIMES] = cls.realtime;
	}
}


/*
==============================================================================

DRAWING

==============================================================================
*/


/*
================
ChatCon_DrawInput

Draw the editline after a ] prompt
================
*/
void ChatCon_DrawInput (void) {
	int		y;
	int		x = 0;
	vec4_t	color;

	if ( clc.state != CA_DISCONNECTED && !(Key_GetCatcher( ) & KEYCATCH_CHATCONSOLE ) ) {
		return;
	}

	y = chatcon.vislines - ( SMALLCHAR_HEIGHT * 2 );

	if (cl_chat_timedisplay->integer & 1) {
		char ts[9];
		int i;
		
		qtime_t	now;
		Com_RealTime( &now );
		Com_sprintf(ts,sizeof(ts),"%02d:%02d:%02d",now.tm_hour,now.tm_min,now.tm_sec);
		
		re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
		for (i = 0 ; i<8 ; i++) {
			SCR_DrawSmallChar( chatcon.xadjust + (i+1) * SMALLCHAR_WIDTH, y, ts[i] );
		}
		x = 9;
	}

/*	black background
	color[0] = 0.0f;;
	color[1] = 0.0f;;
	color[2] = 0.0f;;
	color[3] = 0.5f;

	SCR_FillRect( x * SMALLCHAR_WIDTH, y, ( g_chatconsole_field_width - x ) * SMALLCHAR_WIDTH, 
		SMALLCHAR_HEIGHT, color );
*/
	re.SetColor( chatcon.color );

	SCR_DrawSmallChar( chatcon.xadjust + (x+1) * SMALLCHAR_WIDTH, y, ']' );

	Field_Draw( &g_chatconsoleField, chatcon.xadjust + (x+2) * SMALLCHAR_WIDTH, y,
		SCREEN_WIDTH - 3 * SMALLCHAR_WIDTH, qtrue, qtrue );


}


/*
================
ChatCon_DrawSolidConsole

Draws the console with the solid background
================
*/
void ChatCon_DrawSolidConsole( float frac ) {
	int				i, x, y;
	int				rows;
	short			*text;
	int				row;
	int				lines;
//	qhandle_t		conShader;
	int				currentColor;
	vec4_t			color;

	lines = cls.glconfig.vidHeight * frac;
	if (lines <= 0)
		return;

	if (lines > cls.glconfig.vidHeight )
		lines = cls.glconfig.vidHeight;

	// on wide screens, we will center the text
	chatcon.xadjust = 0;
	SCR_AdjustFrom640( &chatcon.xadjust, NULL, NULL, NULL );

	// draw the background
	y = frac * SCREEN_HEIGHT;
	if ( y < 1 ) {
		y = 0;
	}
	else {
		SCR_DrawPic( 0, 0, SCREEN_WIDTH, y, cls.consoleShader );
	}

	color[0] = 1;
	color[1] = 0;
	color[2] = 0;
	color[3] = 1;
	SCR_FillRect( 0, y, SCREEN_WIDTH, 2, color );

/*	black background
	color[0] = 0.0f;;
	color[1] = 0.0f;;
	color[2] = 0.0f;;
	color[3] = 0.5f;

	SCR_FillRect( SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT, ( g_chatconsole_field_width - 0 ) * SMALLCHAR_WIDTH, 
		lines - ( SMALLCHAR_HEIGHT * 2 ), color );
*/

	// draw the version number

	re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );

	if (cl_chat_drawversion->integer) {
		i = strlen( Q3_VERSION );

		for (x=0 ; x<i ; x++) {
			SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x + 1 ) * SMALLCHAR_WIDTH, 
				SMALLCHAR_HEIGHT * 1, Q3_VERSION[x] );
//				(lines-(SMALLCHAR_HEIGHT+SMALLCHAR_HEIGHT/2)), Q3_VERSION[x] );
		}
	}

	if (cl_chat_timedisplay->integer & 2) {
		char ts[30];
		qtime_t	now;

		Com_RealTime( &now );
		Com_sprintf(ts,sizeof(ts),"%02d:%02d:%02d %04d-%02d-%02d",now.tm_hour,now.tm_min,now.tm_sec,1900 + now.tm_year,1 + now.tm_mon,now.tm_mday);
		i = strlen( ts );

		for (x = 0 ; x<i ; x++) {
			SCR_DrawSmallChar( cls.glconfig.vidWidth - ( i - x ) * SMALLCHAR_WIDTH, 
				SMALLCHAR_HEIGHT * 2, ts[x]);
//				lines - (SMALLCHAR_HEIGHT * (cl_chat_drawversion->integer ? 2 : 1) + SMALLCHAR_HEIGHT/2), ts[x]);
		}
	}

	// draw the text
	chatcon.vislines = lines;
	rows = (lines-SMALLCHAR_WIDTH)/SMALLCHAR_WIDTH;		// rows of text to draw

	y = lines - (SMALLCHAR_HEIGHT*3);

	// draw from the bottom up
	if (chatcon.display != chatcon.current)
	{
	// draw arrows to show the buffer is backscrolled
		re.SetColor( g_color_table[ColorIndex(COLOR_RED)] );
		for (x=0 ; x<chatcon.linewidth ; x+=4)
			SCR_DrawSmallChar( chatcon.xadjust + (x+1)*SMALLCHAR_WIDTH, y, '^' );
		y -= SMALLCHAR_HEIGHT;
		rows--;
	}
	
	row = chatcon.display;

	if ( chatcon.x == 0 ) {
		row--;
	}

	currentColor = 7;
	re.SetColor( g_color_table[currentColor] );

	for (i=0 ; i<rows ; i++, y -= SMALLCHAR_HEIGHT, row--)
	{
		if (row < 0)
			break;
		if (chatcon.current - row >= chatcon.totallines) {
			// past scrollback wrap point
			continue;	
		}

		text = chatcon.text + (row % chatcon.totallines)*chatcon.linewidth;

		for (x=0 ; x<chatcon.linewidth ; x++) {
			if ( ( text[x] & 0xff ) == ' ' ) {
				continue;
			}

			if ( ( (text[x]>>8)&7 ) != currentColor ) {
				currentColor = (text[x]>>8)&7;
				re.SetColor( g_color_table[currentColor] );
			}
			SCR_DrawSmallChar(  chatcon.xadjust + (x+1)*SMALLCHAR_WIDTH, y, text[x] & 0xff );
		}
	}

	// draw the input prompt, user text, and cursor if desired
	ChatCon_DrawInput ();

	re.SetColor( NULL );
}



/*
==================
ChatCon_DrawConsole
==================
*/
void ChatCon_DrawConsole( void ) {
	// check for console width changes from a vid mode change
	ChatCon_CheckResize ();

	// if disconnected, render console full screen
	if ( clc.state == CA_DISCONNECTED ) {
		if ( !( Key_GetCatcher( ) & (KEYCATCH_UI | KEYCATCH_CGAME)) ) {
			ChatCon_DrawSolidConsole( 1.0 );
			return;
		}
	}

	if ( chatcon.displayFrac ) {
		ChatCon_DrawSolidConsole( chatcon.displayFrac );
	} else {

	}
}

//================================================================

/*
==================
ChatCon_RunConsole

Scroll it up or down
==================
*/
void ChatCon_RunConsole (void) {
	// decide on the destination height of the console
	if ( Key_GetCatcher( ) & KEYCATCH_CHATCONSOLE )
		chatcon.finalFrac = 0.5;		// half screen
	else
		chatcon.finalFrac = 0;				// none visible
	
	// scroll towards the destination height
	if (chatcon.finalFrac < chatcon.displayFrac)
	{
		chatcon.displayFrac -= cl_chat_conspeed->value*cls.realFrametime*0.001;
		if (chatcon.finalFrac > chatcon.displayFrac)
			chatcon.displayFrac = chatcon.finalFrac;

	}
	else if (chatcon.finalFrac > chatcon.displayFrac)
	{
		chatcon.displayFrac += cl_chat_conspeed->value*cls.realFrametime*0.001;
		if (chatcon.finalFrac < chatcon.displayFrac)
			chatcon.displayFrac = chatcon.finalFrac;
	}

}


void ChatCon_PageUp( void ) {
	chatcon.display -= 2;
	if ( chatcon.current - chatcon.display >= chatcon.totallines ) {
		chatcon.display = chatcon.current - chatcon.totallines + 1;
	}
}

void ChatCon_PageDown( void ) {
	chatcon.display += 2;
	if (chatcon.display > chatcon.current) {
		chatcon.display = chatcon.current;
	}
}

void ChatCon_Top( void ) {
	chatcon.display = chatcon.totallines;
	if ( chatcon.current - chatcon.display >= chatcon.totallines ) {
		chatcon.display = chatcon.current - chatcon.totallines + 1;
	}
}

void ChatCon_Bottom( void ) {
	chatcon.display = chatcon.current;
}


void ChatCon_Close( void ) {
	if ( !com_cl_running->integer ) {
		return;
	}
	Field_Clear( &g_chatconsoleField );

	Key_SetCatcher( Key_GetCatcher( ) & ~KEYCATCH_CHATCONSOLE );
	chatcon.finalFrac = 0;				// none visible
	chatcon.displayFrac = 0;
}
