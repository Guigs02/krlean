;
; Protected mode BIOS calls
;

        SECTION .text

        global  prot2real
        global  _prot2real
        global  real2prot
        global  _real2prot
        global  bios_print_string
        global  _bios_print_string
        global  bios_get_drive_params
        global  _bios_get_drive_params
        global  bios_read_disk
        global  _bios_read_disk
        global vesa_get_info
        global _vesa_get_info
        global vesa_get_mode_info
        global _vesa_get_mode_info
        global vesa_set_mode
        global _vesa_set_mode

; OS Loader base address
OSLDRSEG    equ 0x9000
OSLDRBASE   equ (OSLDRSEG * 16)

; Segment selectors for data and code in real and protected mode
PROT_CSEG   equ 0x08
PROT_DSEG   equ 0x10
REAL_CSEG   equ 0x18
REAL_DSEG   equ 0x20

; Location of real mode stack
REAL_STACK  equ (0x2000 - 0x10)

; Convert linear address to osldr segment-relative address
%define LOCAL(addr) (addr - OSLDRBASE)

;
; Switch from protected to real mode
;

prot2real:
_prot2real:
        BITS    32
        ; No interrupts while switching mode
        cli

        ; Save protected mode stack and base pointers
        mov     eax, esp
        mov     [prot_esp], eax
        mov     eax, ebp
        mov     [prot_ebp], eax
        
        ; Save interrupt descriptors
        sidt    [prot_idt]
        sgdt    [prot_gdt]

        ; Store return address in real mode stack
        mov     eax,  [esp]
        mov     [REAL_STACK], eax

        ; Set up real mode stack
        mov     eax, REAL_STACK
        mov     esp, eax
        mov     ebp, eax

        ; Setup segment registers
        mov     ax, REAL_DSEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        mov     ss, ax

        ; Transition to 16 bit segment
        jmp     REAL_CSEG:(LOCAL(start16))

start16:
        BITS    16

        ; Exit protected mode by clearing the PE bit of CR0
        mov     eax, cr0
        and     eax, ~1
        mov     cr0, eax

        ; Reload code segment register and clear prefetch
        jmp     dword OSLDRSEG:(LOCAL(realmode))

realmode:

        ; Setup real mode segment registers
        mov     ax, OSLDRSEG
        mov     ds, ax
        mov     es, ax
        mov     fs, ax
        mov     gs, ax
        xor     ax, ax
        mov     ss, ax
        
        ; Load real mode IDT
        a32 lidt [LOCAL(real_idt)]

        ; Return on the real mode stack
        sti
        ret

;
; Switch from real to protected mode
;

real2prot:
_real2prot:
        BITS    16
        ; Disable interrupts
        cli
        
        ; Restore protected mode descriptors
        mov     ax, OSLDRSEG
        mov     ds, ax
        a32 lidt    [LOCAL(prot_idt)]
        a32 lgdt    [LOCAL(prot_gdt)]

        ; Enable protected mode
        mov     eax, cr0
        or      eax, 1
        mov     cr0, eax

        ; Set code segment and clear prefetch
        jmp     dword PROT_CSEG:protmode

protmode:
        BITS    32
        ; Setup rest of segment registers for protected mode
        mov     ax, PROT_DSEG
        mov     ds, ax
        mov     es, ax
        mov     ss, ax
        mov     fs, ax
        mov     gs, ax

        ; Store return address in real mode stack
        mov     eax, [esp]
        mov     [REAL_STACK], eax

        ; Restore protected mode stack
        mov     eax, [prot_esp]
        mov     esp, eax
        mov     eax, [prot_ebp]
        mov     ebp, eax

        ; Put return address onto protected mode stack
        mov     eax, [REAL_STACK]
        mov     [esp], eax

        ; Return on protected mode stack
        xor     eax, eax
        ret
;
; Print string on screen
;
; void bios_print_string(char *str);
;

bios_print_string:
_bios_print_string:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get string address
        mov     esi, [ebp + 8]

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Output all characters in string
nextchar:
        mov     al, [si]
        cmp     al, 0
        je      printdone

        mov     ah, 0x0e
        int     0x10
        
        cmp     al, 10
        jne     notnl

        ; Output cr/nl for newline
        mov     al, 13
        mov     ah, 0x0e
        int     0x10
        
notnl:

        inc     si
        jmp     nextchar
printdone:
                
        ; Return to protected mode
        call    _real2prot
        BITS    32

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop    ebp
        ret

;
; Get drive parameters
;
; int bios_get_drive_params(int drive, unsigned int *cyls, unsigned int *heads, unsigned int *sects);
;

bios_get_drive_params:
_bios_get_drive_params:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get drive number
        mov     edx, [ebp + 8]

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Call BIOS int13h/ah=08h to get drive parameters
        mov     ah, 0x08
        xor     di, di
        mov     es, di
        int     0x13
        mov     bl, ah
                
        ; Return to protected mode
        call    _real2prot
        BITS    32
        
        ; Compute number of cylinders
        xor     eax, eax
        mov     al, ch
        mov     ah, cl
        shr     ah, 6
        inc     ax
        mov     edi, [ebp + 12]
        mov     [edi], eax

        ; Compute number of heads
        xor     eax, eax
        mov     al, dh
        inc     ax
        mov     edi, [ebp + 16]
        mov     [edi], eax

        ; Compute number of sectors
        xor     eax, eax
        mov     al, cl
        and     al, 0x3f
        mov     edi, [ebp + 20]
        mov     [edi], eax

        ; Return status
        xor     eax, eax
        mov     al, bl

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop     ebp
        ret

;
; Read from disk
;
; int bios_read_disk(int drive, int cyl, int head, int sect, int nsect, void *buffer);
;

bios_read_disk:
_bios_read_disk:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get cylinder, head, and sector, number of sectors, and buffer
        mov     al, [ebp + 24]          ; number of sectors
        mov     ch, [ebp + 12]          ; cylinders (lo)
        mov     cl, [ebp + 13]          ; cylinders (hi)
        shl     cl, 6
        or      cl, [ebp + 20]          ; sector number
        mov     dh, [ebp + 16]          ; head number
        mov     dl, [ebp + 8]           ; drive number
        mov     ebx, [ebp + 28]         ; data buffer
        sub     ebx, OSLDRBASE

        ; Save ax which is not preserved in prot->real transition
        mov     si, ax

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Call BIOS int13h/ah=02h to read disk
        mov     ax, si
        mov     ah, 0x02
        int     0x13
        mov     bl, ah

        ; Return to protected mode
        call    _real2prot
        BITS    32

        ; Return status
        xor     eax, eax
        mov     al, bl

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop    ebp

        ret

;
; Get VESA information
;
; int vesa_get_info(struct vesa_info *info);
;

vesa_get_info:
_vesa_get_info:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get info buffer address
        mov     edi, [ebp + 8]
        sub     edi, OSLDRBASE

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Call VESA
        mov     ax, OSLDRSEG
        mov     es, ax
        mov     ax, 0x4f00
        int     0x10
        mov     bx, ax

        ; Return to protected mode
        call    _real2prot
        BITS    32

        ; Return status
        xor     eax, eax
        mov     ax, bx

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop    ebp

        ret

;
; Get VESA mode information
;
; int vesa_get_mode_info(int mode, struct vesa_mode_info *info);
;

vesa_get_mode_info:
_vesa_get_mode_info:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get mode and info buffer address
        mov     ecx, [ebp + 8]
        mov     edi, [ebp + 12]
        sub     edi, OSLDRBASE

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Call VESA
        mov     ax, OSLDRSEG
        mov     es, ax
        mov     ax, 0x4f01
        int     0x10
        mov     bx, ax

        ; Return to protected mode
        call    _real2prot
        BITS    32

        ; Return status
        xor     eax, eax
        mov     ax, bx

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop    ebp

        ret

;
; Set VESA graphics mode
;
; int vesa_set_mode(int mode);
;

vesa_set_mode:
_vesa_set_mode:
        push ebp
        mov  ebp, esp

        ; Save register
        push    ebx
        push    esi
        push    edi

        ; Get mode
        mov     ebx, [ebp + 8]

        ; Enter real mode
        call    _prot2real
        BITS     16

        ; Call VESA
        mov     ax, 0x4f02
        int     0x10
        mov     bx, ax

        ; Return to protected mode
        call    _real2prot
        BITS    32

        ; Return status
        xor     eax, eax
        mov     ax, bx

        ; Restore registers
        pop     edi
        pop     esi
        pop     ebx

        ; Return
        pop    ebp

        ret

; Storage for protected model stack pointer, and descriptor tables
prot_esp:   dd 0
prot_ebp:   dd 0
prot_idt:   dw 0, 0, 0
prot_gdt:   dw 0, 0, 0

; Real mode IDT for BIOS
real_idt:   dw 0x3ff   ; 256 entries, 4b each = 1K
            dd 0       ; Real mode IVT @ 0x0000
