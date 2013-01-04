; ===========================================================================
; Copyright (C) 2011 Thilo Schulz <thilo@tjps.eu>
; 
; This file is part of Spearmint Source Code.
; 
; Spearmint Source Code is free software; you can redistribute it
; and/or modify it under the terms of the GNU General Public License as
; published by the Free Software Foundation; either version 3 of the License,
; or (at your option) any later version.
; 
; Spearmint Source Code is distributed in the hope that it will be
; useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
; 
; You should have received a copy of the GNU General Public License
; along with Spearmint Source Code.  If not, see <http://www.gnu.org/licenses/>.
:
; In addition, Spearmint Source Code is also subject to certain additional terms.
; You should have received a copy of these additional terms immediately following
; the terms and conditions of the GNU General Public License.  If not, please
; request a copy in writing from id Software at the address below.
:
; If you have questions concerning this license or the applicable additional
; terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc.,
; Suite 120, Rockville, Maryland 20850 USA.
; ===========================================================================

; MASM ftol conversion functions using SSE or FPU
; assume __cdecl calling convention is being used for x86, __fastcall for x64

IFNDEF idx64
.model flat, c
ENDIF

.data

ifndef idx64
  fpucw WORD 0F7Fh
endif

.code

IFDEF idx64
; qftol using SSE

  qftolsse PROC
    cvttss2si eax, xmm0
	ret
  qftolsse ENDP

  qvmftolsse PROC
    movss xmm0, dword ptr [rdi + rbx * 4]
	cvttss2si eax, xmm0
	ret
  qvmftolsse ENDP

ELSE
; qftol using FPU

  qftolx87m macro src
    sub esp, 2
    fnstcw word ptr [esp]
    fldcw fpucw
    fld dword ptr src
	fistp dword ptr src
	fldcw [esp]
	mov eax, src
	add esp, 2
	ret
  endm
  
  qftolx87 PROC
    qftolx87m [esp + 6]
  qftolx87 ENDP

  qvmftolx87 PROC
    qftolx87m [edi + ebx * 4]
  qvmftolx87 ENDP

; qftol using SSE
  qftolsse PROC
    movss xmm0, dword ptr [esp + 4]
    cvttss2si eax, xmm0
	ret
  qftolsse ENDP

  qvmftolsse PROC
    movss xmm0, dword ptr [edi + ebx * 4]
	cvttss2si eax, xmm0
	ret
  qvmftolsse ENDP
ENDIF

end
