; ----------------- assembly instructions ---------------------------- 
;
; this is a subroutine library only
; it must be included in an executable source file
;
;
;*** I/O Locations *******************************
; define the i/o address of the ACIA1 chip
;*** 6551 CIA ************************
;ACIA2dat       =     $8100
;ACIA2sta       =     $8101
;ACIA2cmd       =     $8102
;ACIA2ctl       =     $8103
;
;***********************************************************************
; 6551 I/O Support Routines
;
ACIA2_init:
        ;lda   #<ACIA1_scan      	; setup BASIC vectors
        ;sta   VEC_IN
	;lda   #>ACIA1_scan	; BASIC's chr input
        ;sta   VEC_IN+1
        ;lda   #<ACIA1_Output	
        ;sta   VEC_OUT
	;lda   #>ACIA1_Output	; BASIC's chr output 
        ;sta   VEC_OUT+1
	;lda   #<Psave
        ;sta   VEC_SV
	;lda   #>Psave		; SAVE cmd
        ;sta   VEC_SV+1
	;lda   #<pload
        ;sta   VEC_LD
	;lda   #>pload		; LOAD cmd
        ;sta   VEC_LD+1

ACIA2portset:
;	lda   #$10               ; 115200/8/1
	lda   #$1E               ; 9600/8/1
        sta   ACIA2ctl           ; control reg 
        lda   #$0B               ; N parity/echo off/rx int off/ dtr active low
        sta   ACIA2cmd           ; command reg 
        rts                      ; done
;
; input chr from ACIA1 (waiting)
;
ACIA2_Input:	
        lda   ACIA2sta           ; Serial port status             
        and   #$08               ; is recvr full
	beq  ACIA2_Input
;        bne  ACIA1_Input_k        ; char to get
	;jmp ACIA1_Input
;KB_input:
	;lda kb_data_avail
	;and #$01

;	LDA Via1IFR
;	AND #$02
;	beq ACIA1_Input
	;lda #$00
;	jsr kb_irq
;	lda kb_data_avail
;	and #$01
;	beq ACIA1_Input
;	lda #$00
;	sta kb_data_avail
;	lda kb_data
;	jmp ACIA1_Input_done
;ACIA1_Input_k:	
        Lda   ACIA2dat           ; get chr
;ACIA1_Input_done:	
        RTS                      ;
;
; non-waiting get character routine 
;
ACIA2_Scan:	
	clc
        lda   ACIA2sta           ; Serial port status
        and   #$08               ; mask rcvr full bit
        beq   ACIA2_scan2
        Lda   ACIA2dat           ; get chr
	sec
	rts
ACIA2_scan2:
	;lda kb_data_avail
	;and #$01
;	lda Via1IFR
;	and #$02
;	beq ACIA1_scan3
;	jsr kb_irq
;	lda kb_data_avail
;	and #$01
;	beq ACIA1_scan3
;	lda #$00
;	sta kb_data_avail
;	lda kb_data
;	sec
;	rts
;ACIA1_scan3:
	lda #$00
	rts
;
; output to OutPut Port
;
ACIA2_Output:
	PHA                      ; save registers
;	sta  vid_cin		 ; Save output byte for video
ACIA2_Out1:
	lda   ACIA2sta           ; serial port status
        ;and   #$10               ; is tx buffer empty
        ;beq   ACIA1_Out1         ; no
	;; Delay loop to ensure that TX buffer is empty due to 6551 bug
;	PLA                      ; get chr
;	pha
	jsr   DELAY_6551
	pla
        sta   ACIA2dat           ; put character to Port
;	jsr   DELAY_6551	 ;
;	lda   #$01
;	cmp   vid_textmode
;	bne   notextmode
;	jsr   vid_putc
;notextmode:
;	pla
        RTS                      ; done

DELAY_6551:
	pha
	tya
	pha
	txa
	pha
	;PHY			;Save Y
	;PHX			;Save X
DELAY_6551_LOOP:
	LDY #10			;Clock rate in MHz
MINIDLY_6551:
	LDX #$F0		;Set X-reg
;	LDX #$12		;
DELAY1_6551:
	DEX			;Decrease X counter
	BNE DELAY1_6551		;Loop until done
	DEY			;Decrease Y counter
	BNE MINIDLY_6551	;Loop until done

	pla
	tax
	pla
	tay
	pla
	;PLX			;Restore X
	;PLY			;Restore Y
DELAY_DONE_6551:
	;jmp vid_putc
	RTS			;Return
	
;end of file
