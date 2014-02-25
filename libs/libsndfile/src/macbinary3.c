/*
** Copyright (C) 2003-2012 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include	"sfconfig.h"

#include	<stdlib.h>
#include	<string.h>

#include	"sndfile.h"
#include	"sfendian.h"
#include	"common.h"

#if (OSX_DARWIN_VERSION >= 1)

int
macbinary3_open (SF_PRIVATE * UNUSED (psf))
{
	return 0 ;
} /* macbinary3_open */

#else

int
macbinary3_open (SF_PRIVATE * UNUSED (psf))
{
	return 0 ;
} /* macbinary3_open */

#endif /* OSX_DARWIN_VERSION */

