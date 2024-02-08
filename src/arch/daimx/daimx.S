; CP/M-65 Copyright © 2022 David Given
; This file is licensed under the terms of the 2-clause BSD license. Please
; see the COPYING file in the root project directory for the full text.

#include "zif.inc"
#include "daimx.inc"
#include "cpm65.inc"
#include "driver.inc"
#include "jumptables.inc" 	
ZEROPAGE

.global ptr
.global ptr1
ptr: .word 0
ptr1: .word 0
dma: .fill 2
current_bank: .byte 0
; --- Initialisation code ---------------------------------------------------

; Called once on startup and then never again.

zproc _start
    ; Print banner.

    ldy #banner_end - banner
    zrepeat
        lda banner - 1, y
        jsr jmp_out
        dey
    zuntil_eq

    ; Initialize USB-disk interface

    ;jsr ch376_init
	jsr jmp_ch376_init

    ; Load the BDOS image.
    lda mem_base+1
    ldx #<bdos_filename
    ldy #>bdos_filename
    ;jsr ch376_load_block
    jsr jmp_load_block
    
    ; Relocate it.

    lda mem_base+1
    ldx zp_base
    jsr bios_RELOCATE

    ; Open CPMFS	
    ldx #<cpmfs_filename	
	ldy #>cpmfs_filename
	;jsr ch376_open_persistent
    jsr jmp_open_persistent
    
    ; Initialize drivers
    jsr initdrivers

    ; Avoid junk character in buffer at startup
    lda #$00
    sta pending_key

    ; Compute the entry address and jump.

    lda mem_base+1
    pha
    lda #COMHDR_ENTRY-1 ; rts addresses are one before the target
    pha

    lda #<biosentry
    ldx #>biosentry
    rts                 ; indirect jump
zendproc
    
bdos_filename:
    .ascii "BDOS"
    .byte 00

banner: ; reversed!
    .byte 13, 10
    .ascii "------------------- X-MIAD rof 56-M/PC -------------------"
banner_end:

cpmfs_filename:
    .ascii "CPMFS"
    .byte 00

; Stuff above of here must be 0x80 long, currently OK

; --- Drivers -------------------------------------------------------
.data
.global drvtop
drvtop: .word drv_TTY

defdriver TTY, DRVID_TTY, drvstrat_TTY, drv_SCREEN

; TTY driver strategy routine
; Y = TTY opcode

zproc drvstrat_TTY
    jmpdispatch jmptable_lo, jmptable_hi

jmptable_lo:
    jmptablo tty_const
    jmptablo tty_conin
    jmptablo tty_conout
jmptable_hi:
    jmptabhi tty_const
    jmptabhi tty_conin
    jmptabhi tty_conout
zendproc


defdriver SCREEN, DRVID_SCREEN, drvstrat_SCREEN, drv_SERIAL

; SCREEN driver strategy routine
; Y = TTY opcode

zproc drvstrat_SCREEN
    jmpdispatch screen_jmptable_lo, screen_jmptable_hi

screen_jmptable_lo:
    jmptablo screen_version
    jmptablo screen_getsize
    jmptablo screen_clear
    jmptablo screen_setcursor
    jmptablo screen_getcursor
    jmptablo screen_putchar
    jmptablo screen_putstring
    jmptablo screen_getchar
    jmptablo screen_showcursor
    jmptablo screen_scrollup
    jmptablo screen_scrolldown
    jmptablo screen_cleartoeol
    jmptablo screen_setstyle
screen_jmptable_hi:
    jmptabhi screen_version
    jmptabhi screen_getsize
    jmptabhi screen_clear
    jmptabhi screen_setcursor
    jmptabhi screen_getcursor
    jmptabhi screen_putchar
    jmptabhi screen_putstring
    jmptabhi screen_getchar
    jmptabhi screen_showcursor
    jmptabhi screen_scrollup
    jmptabhi screen_scrolldown
    jmptabhi screen_cleartoeol
    jmptabhi screen_setstyle
zendproc

; SERIAL driver strategy routine
; Y = TTY opcode

defdriver "SERIAL", DRVID_SERIAL, drvstrat_SERIAL, 0

zproc drvstrat_SERIAL
    jmpdispatch serial_jmptable_lo, serial_jmptable_hi

serial_jmptable_lo:
    jmptablo serial_inp
    jmptablo serial_out
    jmptablo serial_open
    jmptablo serial_close
    jmptablo serial_outp
    jmptablo serial_in    
serial_jmptable_hi:
    jmptabhi serial_inp
    jmptabhi serial_out
    jmptabhi serial_open
    jmptabhi serial_close
    jmptabhi serial_outp
    jmptabhi serial_in 
zendproc 

; --- SERIAL driver -------------------------------------------------------

zproc serial_inp
    jsr ACIA3_Scan
    rts     
zendproc

zproc serial_out
    jmp ACIA3_Output     
zendproc

zproc serial_open
    jmp ACIA3_init
zendproc

zproc serial_close
    rts
zendproc

zproc serial_outp
    jsr ACIA3_Output
    clc
    rts
zendproc

zproc serial_in
    jmp ACIA3_Input
zendproc

; --- SCREEN driver -------------------------------------------------------

zproc screen_version
    lda #0
    rts
zendproc

zproc screen_getsize
    ; Screen size is 64x30
    lda #63
    ldx #29
    rts
zendproc

zproc screen_clear
    jmp jmp_cls
    ;; Reset pointer
    ;lda #<VRAM_base
    ;sta vidpointer+0
    ;lda #>VRAM_base
    ;sta vidpointer+1


    ;ldy #$00
;y_loop:
    ;ldx #$00
;x_loop:
    ;lda #$20 ; fill with space
    ;tya
    ;pha
    ;lda #$20
    ;ldy #$00
    ;sta (vidpointer),Y
    ;pla
    ;tay
    ;lda #$00
    ;inc vidpointer+0
    ;cmp vidpointer+0
    ;bne cont
    ;inc vidpointer+1
;cont:
    ;inx
    ;cpx #64
    ;bne x_loop

    ;iny
    ;cpy #30
    ;bne y_loop

    ;; Reset pointer
    ;lda #<VRAM_base
    ;sta vidpointer+0
    ;lda #>VRAM_base
    ;sta vidpointer+1

    ;; Reset cursor position
    ;lda #$00
    ;sta vidcursor_x
    ;sta vidcursor_y
    
    ;rts
zendproc

zproc screen_setcursor
    jmp jmp_setcursor
    ; Update cursor location
    ;sta vidcursor_x
    ;stx vidcursor_y
  
    ; Ensure that cursor is removed from current position
    ;ldy #0
    ;lda (vidpointer),Y
    ;cmp #0
    ;bne nopointer 
    ;lda vid_curchar
    ;sta (vidpointer),Y
;nopointer:
 
    ; Update pointer value
    ;lda #<VRAM_base
    ;sta vidpointer+0
    ;lda #>VRAM_base
    ;sta vidpointer+1
    
    ; Number of rows in x
;yp_loop:
    ;cpx #$00
    ;beq yp_done
    ;dex
    ;clc
    ;lda vidpointer+0
    ;adc #64
    ;sta vidpointer+0
    ;bcc yp_next
    ;inc vidpointer+1
;yp_next:
    ;jmp yp_loop
;yp_done:
    ;clc
    ;lda vidcursor_x
    ;adc vidpointer+0
    ;sta vidpointer+0
    ;bcc xp_next
    ;inc vidpointer+1
;xp_next:
    
    ;rts
zendproc

zproc screen_getcursor
    lda vidcursor_x
    ldx vidcursor_y
    rts
zendproc

zproc screen_putchar
    ;jsr jmp_out
    ora vid_style
    ldy #0
    sta (vidpointer),Y

    lda #63
    cmp vidcursor_x
    beq screen_putchar_done
    inc vidcursor_x
    lda vidcursor_x
    ldx vidcursor_y
    jsr screen_setcursor 
screen_putchar_done:
    rts    
zendproc

zproc screen_putstring
    sta ptr+0
    stx ptr+1
    ldy #0
putstring_loop:
    lda (ptr),y
    beq putstring_endloop
    jsr jmp_out 
    iny
    jmp putstring_loop
putstring_endloop:
    clc
    rts
zendproc

zproc screen_getchar
    jsr jmp_scan
    cmp #00
    beq nodata
    cmp #$08
    bne notbs
    lda #127
notbs:    
    clc
    rts
nodata:
    sec   
    rts
zendproc

zproc screen_showcursor
    jmp jmp_showcursor
    ;cmp #$00
    ;bne curson
    ;sta vid_curvis
    ;rts
;curson:
    ;lda #$01
    ;sta vid_curvis
    ;rts
zendproc

zproc screen_scrollup
    jmp jmp_scrollup
    ;lda #<VRAM_row_two
    ;sta ptr+0
    ;lda #>VRAM_row_two
    ;sta ptr+1
    
    ;lda #<VRAM_base
    ;sta vidpointer+0
    ;lda #>VRAM_base
    ;sta vidpointer+1

    ;ldy #$00
;scrup_loop_y:
    ;ldx #$00
;scrup_loop_x:
    ;tya
    ;pha
    ;ldy #$00
    ;lda (ptr),y
    ;sta (vidpointer),y
    ;pla
    ;tay 
    
    ;inc ptr+0
    ;lda #$00
    ;cmp ptr+0
    ;bne scrup_cont
    ;inc ptr+1
;scrup_cont:
    ;inc vidpointer+0
    ;lda #$00
    ;cmp vidpointer+0
    ;bne scrup_vcont
    ;inc vidpointer+1
;scrup_vcont:
    ;inx
    ;cpx #64
    ;bne scrup_loop_x

    ;iny
    ;cpy #30
    ;bne scrup_loop_y
   
    ;lda vidcursor_x
    ;sta ptr+0
    ;ldx vidcursor_y
    ;cpx #0
    ;beq scrup_top
    ;dex
;scrup_top:
    ;stx ptr+1
 
    ;lda #0
    ;ldx #29
    ;jsr screen_setcursor
    ;jsr screen_cleartoeol

    ;lda ptr+0
    ;ldx ptr+1
    ;jsr screen_setcursor
    
    ;rts
zendproc

zproc screen_scrolldown
    jmp jmp_scrolldown
    ;lda #<VRAM_nextlastrow
    ;sta ptr+0
    ;lda #>VRAM_nextlastrow
    ;sta ptr+1
    
    ;lda #<VRAM_lastrow
    ;sta vidpointer+0
    ;lda #>VRAM_lastrow
    ;sta vidpointer+1

    ;ldy #30
;scrdown_loop_y:
    ;ldx #63
;scrdown_loop_x:
    ;tya
    ;pha
    ;ldy #$00
    ;lda (ptr),y
    ;sta (vidpointer),y
    ;pla
    ;tay
    
    ;lda ptr+0
    ;bne scrdown_cont
    ;dec ptr+1
;scrdown_cont:
    ;dec ptr+0
    
    ;lda vidpointer+0
    ;bne scrdown_vcont
    ;dec vidpointer+1
;scrdown_vcont:
    ;dec vidpointer+0
    
    ;dex
    ;cpx #00
    ;bne scrdown_loop_x

    ;dey
    ;cpy #00
    ;bne scrdown_loop_y
   
    ;lda vidcursor_x
    ;sta ptr+0
    ;ldx vidcursor_y
    ;cpx #29
    ;beq scrdown_bot
    ;inx
;scrdown_bot:
    ;stx ptr+1
 
    ;lda #0
    ;ldx #0
    ;jsr screen_setcursor
    ;jsr screen_cleartoeol
    
    ;lda ptr+0
    ;ldx ptr+1
    ;jsr screen_setcursor
    
    ;rts
zendproc

zproc screen_cleartoeol
    jmp jmp_cleartoeol
;    ldx vidcursor_x
;    ldy #$00
;clearline_loop:
;    lda #$20
;    ora vid_style
;    sta (vidpointer),y
;    cpx #63
;    beq clearline_done
;    inx
;    inc vidpointer+0
;    lda #0
;    cmp vidpointer+0
;    bne clearline_cont
;    inc vidpointer+1
;clearline_cont:
;    jmp clearline_loop
;clearline_done:
;    lda vidcursor_x
;    ldx vidcursor_y
;    jsr screen_setcursor 
;    rts  
zendproc

zproc screen_setstyle
    cmp #$01
    bne normal_style
    lda #$80
    sta vid_style
    rts
normal_style:
    lda #$00
    sta vid_style
    rts
zendproc

; --- TTY driver -------------------------------------------------------

; Returns 0x00 if no key is pending, 0xff if one is.
; C if no key is pending, !C if key pending
zproc tty_const
    lda pending_key
    zif_eq
        jsr jmp_scan
	    sta pending_key
        zif_eq
            sec
            rts
        zendif
    zendif
    lda #$ff
    clc
    rts
zendproc

; Blocks and waits for the next keypress; returns it in A.

zproc tty_conin
    lda pending_key
    zif_eq
	    jsr jmp_inp
        rts
    zendif

    ldx #0
    stx pending_key
    rts
zendproc

zproc tty_conout
	cmp #127
	zif_eq
		lda #8
	zendif
        jsr jmp_out
        rts
zendproc

; -- Rest of the BIOS ---

; Sets the current DMA address.

zproc bios_SETDMA
    sta dma+0
    stx dma+1
    rts
zendproc

; Select a disk.
; A is the disk number.
; Returns the DPH in XA.
; Sets carry on error.

zproc bios_SELDSK
    cmp #0
    zif_ne
        sec                 ; invalid drive
        rts
    zendif

    lda #<dph
    ldx #>dph
    clc
    rts
zendproc

; Set the current absolute sector number.
; XA is a pointer to a three-byte number.

zproc bios_SETSEC
    jmp jmp_set_sector
    ;sta ptr+0
    ;stx ptr+1

	
	;ldy #2
    ;    lda (ptr),y
    ;    lsr a
    ;    sta fs_offset+3

    ;    dey
    ;    lda (ptr),y
    ;    ror a
    ;    sta fs_offset+2

    ;    dey
    ;    lda (ptr),y
    ;    ror a
    ;    sta fs_offset+1

    ;    lda #0
    ;    ror a
    ;    sta fs_offset+0

    ;    clc

	
    ;rts
zendproc

zproc bios_READ
	;jsr ch376_locate
	;jsr ch376_read_sector
	jsr jmp_locate
    lda dma;#<dma
    ldx dma+1;#>dma
    jsr jmp_read_sector
    rts
zendproc

zproc bios_WRITE
	;jsr ch376_locate
	;jsr ch376_write_sector
	jsr jmp_locate
    lda dma;#<dma
    ldx dma+1;#>dma
    jsr jmp_write_sector
    rts
zendproc

zproc bios_GETTPA
    ;ldy current_bank
    lda mem_base;, Y  
    ldx mem_end;, Y
    rts
zendproc

zproc bios_SETTPA
    ldy current_bank
    cpy #0
    bne settpa_end
    sta mem_base;, Y 
    stx mem_end;, Y
settpa_end:
    rts
zendproc

zproc bios_GETZP
    ;ldy current_bank
    lda zp_base;, Y
    ldx zp_end;, Y
    rts
zendproc

zproc bios_SETZP
    ;ldy current_bank
    sta zp_base;, Y 
    stx zp_end;, Y
    rts
zendproc

zproc bios_SETBANK
    sta current_bank
    rts
zendproc

;#include "daimx_file.inc"
;#include "daimx_ACIA2.inc"	
#include "daimx_ACIA3.inc"
.data
;current_bank:   .byte 0
zp_base: .byte __ZP0_START__;, __ZP1_START__
zp_end:  .byte __ZP0_END__;, __ZP1_END__

mem_base: .byte __TPA0_START__@mos16hi, __TPA1_START__@mos16hi
mem_end:  .byte __TPA0_END__@mos16hi, __TPA1_END__@mos16hi
	
; DPH for drive 0 (our only drive)

define_drive dph, 128*64, 2048, 128, 0
directory_buffer = _start
NOINIT

filehandle:  .byte 0    ; file handle of disk image
pending_key: .byte 0    ; pending keypress from system
sector_num:  .fill 3    ; current absolute sector number
	
osgbpb_block:           ; block used by entry_READ and entry_WRITE
    .fill $0d
osgbpb_block_end:

; vim: filetype=asm sw=4 ts=4 et

