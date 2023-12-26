ACIA3_init:

ACIA3portset:
	lda   #$1E               ; 9600/8/1
    sta   ACIA3ctl           ; control reg 
    lda   #$0B               ; N parity/echo off/rx int off/ dtr active low
    sta   ACIA3cmd           ; command reg 
    rts                      ; done
;
; input chr from ACIA3 (waiting)
;
ACIA3_Input:	
    lda   ACIA3sta           ; Serial port status             
    and   #$08               ; is recvr full
	beq  ACIA3_Input
    lda   ACIA3dat           ; get chr
    rts                      
;
; non-waiting get character routine 
;
ACIA3_Scan:	
    lda   ACIA3sta           ; Serial port status
    and   #$08               ; mask rcvr full bit
    beq   ACIA3_scan2
    lda   ACIA3dat           ; get chr
	clc
	rts
ACIA3_scan2:
	lda #$00
    sec
	rts
;
; output to port
;
ACIA3_Output:
	pha                      ; save registers
ACIA3_Out1:
	lda   ACIA3sta           ; serial port status
	jsr   DELAY_6551         ; Use same delay routine as ACIA2
	pla
    sta   ACIA3dat           ; put character to Port
    rts                      ; done

