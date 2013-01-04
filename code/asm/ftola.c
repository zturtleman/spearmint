/*
===========================================================================
Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>

This file is part of Spearmint Source Code.

Spearmint Source Code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 3 of the License,
or (at your option) any later version.

Spearmint Source Code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, Spearmint Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following
the terms and conditions of the GNU General Public License.  If not, please
request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional
terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include "qasm-inline.h"

static const unsigned short fpucw = 0x0C7F;

/*
 * GNU inline asm ftol conversion functions using SSE or FPU
 */

long qftolsse(float f)
{
  long retval;
  
  __asm__ volatile
  (
    "cvttss2si %1, %0\n"
    : "=r" (retval)
    : "x" (f)
  );
  
  return retval;
}

int qvmftolsse(void)
{
  int retval;
  
  __asm__ volatile
  (
    "movss (" EDI ", " EBX ", 4), %%xmm0\n"
    "cvttss2si %%xmm0, %0\n"
    : "=r" (retval)
    :
    : "%xmm0"
  );
  
  return retval;
}

long qftolx87(float f)
{
  long retval;
  unsigned short oldcw;

  __asm__ volatile
  (
    "fnstcw %2\n"
    "fldcw %3\n"
    "flds %1\n"
    "fistpl %1\n"
    "fldcw %2\n"
    "mov %1, %0\n"
    : "=r" (retval)
    : "m" (f), "m" (oldcw), "m" (fpucw)
  );
  
  return retval;
}

int qvmftolx87(void)
{
  int retval;
  unsigned short oldcw;

  __asm__ volatile
  (
    "fnstcw %1\n"
    "fldcw %2\n"
    "flds (" EDI ", " EBX ", 4)\n"
    "fistpl (" EDI ", " EBX ", 4)\n"
    "fldcw %1\n"
    "mov (" EDI ", " EBX ", 4), %0\n"
    : "=r" (retval)
    : "m" (oldcw), "m" (fpucw)
  );
  
  return retval;
}
