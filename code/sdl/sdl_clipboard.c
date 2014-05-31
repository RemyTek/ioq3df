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

#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#else
#	include <SDL.h>
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
#	ifdef USE_LOCAL_HEADERS
#		include "SDL_clipboard.h"
#	else
#		include <SDL_clipboard.h>
#	endif
#else
#	ifdef _WIN32
#		include <windows.h>
#	endif
#endif

void Sys_CopyTextToClipboard( const char *text )
{
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetClipboardText( text );
#else
#	ifdef _WIN32
	const size_t len = strlen(text) + 1;
	HGLOBAL hMem =  GlobalAlloc(GMEM_MOVEABLE, len);
	memcpy(GlobalLock(hMem), text, len);
	GlobalUnlock(hMem);
	OpenClipboard(0);
	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);
	CloseClipboard();
#	endif
#endif
}
