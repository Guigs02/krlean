;-----------------------------------------------------------------------------
; floor.asm - floating point floor
;-----------------------------------------------------------------------------

                SECTION .text

                global  floor
                global  _floor
                
floor:
_floor:
                push    ebp
                mov     ebp,esp
                sub     esp,4                   ; Allocate temporary space
                fld     qword [ebp+8]           ; Load real from stack
                fstcw   [ebp-2]                 ; Save control word
                fclex                           ; Clear exceptions
                mov     word [ebp-4],0763h      ; Rounding control word
                fldcw   [ebp-4]                 ; Set new rounding control
                frndint                         ; Round to integer
                fclex                           ; Clear exceptions
                fldcw   [ebp-2]                 ; Restore control word
                mov     esp,ebp
                pop     ebp
                ret
