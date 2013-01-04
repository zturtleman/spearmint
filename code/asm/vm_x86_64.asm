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

; Call wrapper for vm_x86 when built with MSVC in 64 bit mode,
; since MSVC does not support inline x64 assembler code anymore.
;
; assumes __fastcall calling convention

.code

; Call to compiled code after setting up the register environment for the VM
; prototype:
; uint8_t qvmcall64(int *programStack, int *opStack, intptr_t *instructionPointers, byte *dataBase);

qvmcall64 PROC
  push rsi							; push non-volatile registers to stack
  push rdi
  push rbx
  ; need to save pointer in rcx so we can write back the programData value to caller
  push rcx

  ; registers r8 and r9 have correct value already thanx to __fastcall
  xor rbx, rbx						; opStackOfs starts out being 0
  mov rdi, rdx						; opStack
  mov esi, dword ptr [rcx]			; programStack
  
  call qword ptr [r8]				; instructionPointers[0] is also the entry point

  pop rcx

  mov dword ptr [rcx], esi			; write back the programStack value
  mov al, bl						; return opStack offset

  pop rbx
  pop rdi
  pop rsi
  
  ret
qvmcall64 ENDP

end
