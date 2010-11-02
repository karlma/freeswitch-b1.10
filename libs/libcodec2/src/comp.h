/*---------------------------------------------------------------------------*\
                                                                             
  FILE........: comp.h
  AUTHOR......: David Rowe                                                          
  DATE CREATED: 24/08/09
                                                                             
  Complex number definition.
                                                                             
\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2009 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __COMP__
#define __COMP__

/* Complex number */

typedef struct {
  float real;
  float imag;
} COMP;

#endif
