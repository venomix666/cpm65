ACIA2_init:

ACIA2portset:
	lda   #$1E               ; 9600/8/1
    sta   ACIA2ctl           ; control reg 
    lda   #$0B               ; N parity/echo off/rx int off/ dtr active low
    sta   ACIA2cmd           ; command reg 
    rts                      ; done
;
; input chr from ACIA2 (waiting)
;
ACIA2_Input:	
    lda   ACIA2sta           ; Serial port status             
    and   #$08               ; is recvr full
	beq  ACIA2_Input
    lda   ACIA2dat           ; get chr
    rts                      
;
; non-waiting get character routine 
;
ACIA2_Scan:	
	clc
    lda   ACIA2sta           ; Serial port status
    and   #$08               ; mask rcvr full bit
    beq   ACIA2_scan2
    lda   ACIA2dat           ; get chr
	sec
	rts
ACIA2_scan2:
	lda #$00
	rts
;
; output to port
;
ACIA2_Output:
	pha                      ; save registers
ACIA2_Out1:
	lda   ACIA2sta           ; serial port status
	jsr   DELAY_6551
	pla
    sta   ACIA2dat           ; put character to Port
    rts                      ; done

DELAY_6551:
	pha
	tya
	pha
	txa
	pha
DELAY_6551_LOOP:
	ldy #10			;Clock rate in MHz
MINIDLY_6551:
	ldx #$F0		;Set X-reg
DELAY1_6551:
	dex			;Decrease X counter
	bne DELAY1_6551		;Loop until done
	dey			;Decrease Y counter
	bne MINIDLY_6551	;Loop until done

	pla
	tax
	pla
	tay
	pla
DELAY_DONE_6551:
    rts
	
