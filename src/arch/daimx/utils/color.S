#include "zif.inc"
#include "cpm65.inc"

CRAM_A = 0x9800
CRAM_B = 0x9900
CRAM_C = 0x9A00
CRAM_D = 0x9B00
CRAM_E = 0x9C00
CRAM_F = 0x9D00
CRAM_G = 0x9E00
CRAM_H = 0x9F00

ZEROPAGE

color: .fill 1

zproc main
	lda cpm_fcb+1

	jsr hexconv
    zif_cs
	    rts
    zendif

	sta color
    asl color
    asl color
    asl color
    asl color
    
    lda cpm_fcb+2

    jsr hexconv
    zif_cs
	    rts
    zendif

    ora color

    ldx #0
fill_a:
	sta CRAM_A,X
	inx
	bne fill_a

fill_b:
	sta CRAM_B,X
	inx
	bne fill_b

fill_c:
	sta CRAM_C,X
	inx
	bne fill_c

fill_d:
	sta CRAM_D,X
	inx
	bne fill_d

fill_e:
	sta CRAM_E,X
	inx
	bne fill_e

fill_f:
	sta CRAM_F,X
	inx
	bne fill_f

fill_g:
	sta CRAM_G,X
	inx
	bne fill_g

fill_h:
	sta CRAM_H,X
	inx
	bne fill_h

	rts
zendproc

zproc hexconv
    cmp #'0'
    bcs hexconv_a
    jmp print_usage
hexconv_a:
    cmp #'9'+1
    bcs hexconv_b
    sec
    sbc #0x30
    clc
    rts 
hexconv_b:
    cmp #'F'+1
    bcc hexconv_c
    jmp print_usage
hexconv_c:
    sec
    sbc #0x37
    clc
    rts
print_usage:
    lda #<usage_string
    ldx #>usage_string
    ldy #BDOS_WRITE_STRING
    jsr BDOS
    sec
    rts
zendproc

usage_string:
    .ascii "color - set text color on the DAIM-X computer"
    .byte 13, 10
    .ascii "Usage: color fg[0-f] bg[0-f], e.g. f0"
    .byte 13, 10, 0
