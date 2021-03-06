/*
 * This file is part of FreeHAL 2012.
 *
 * Copyright(c) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Tobias Schulz and contributors.
 * http://www.freehal.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#ifndef HAL2012
#define HAL2012 1

#include "hal2012-version.h"

// C or C++? Windows or POSIX?
#if defined (USE_CXX) || defined(__cplusplus)
#   define __cplusplus 1
#   define USE_CXX 1
#   define CXX 1
#endif
#if defined (__MINGW) || defined(__MINGW32__)
#   define WINDOWS 1
#endif

#ifdef CXX
#   define EXTERN_C extern "C"
#   define BEGIN_EXTERN_C extern "C" {
#   define END_EXTERN_C }
#else
#   define EXTERN_C
#   define BEGIN_EXTERN_C
#   define END_EXTERN_C
#endif

// include Freehal headers
#ifdef CXX
#   include "hal2012-cxx.h"
    extern "C" {
#       include "hal2012-c.h"
    }
#else
#   include "hal2012-c.h"
#endif

#include "hal2012-as.h"

#ifdef WINDOWS
#   include <winsock2.h>
#   include <windows.h>
#   define sleep(x) Sleep(1000*(x))
#   define usleep(x) Sleep(x)
#endif

#endif /* HAL2012 */
