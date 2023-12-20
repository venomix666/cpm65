;	.setcpu "65C02"
;	.segment "CODE"

#include "daimx.inc"
ch376_init:
	pha
	lda #$00
	sta ld_cnt
	jsr ACIA2_init		; UART init
	jsr ch376_reset
	jsr ch376_usb_mode
	jsr ch376_connect
	jsr ch376_mount

	lda #$00
	cmp ld_cnt
	bne ch376_init_failed
	
	jmp ch376_init_cont
ch376_init_failed:
;	lda #<init_nok_string
;	sta stringpointer_l
;	lda #>init_nok_string
;	sta stringpointer_h
;	jsr print_string
	
ch376_init_cont:	
	lda #$00
;	sta loadflag
;	sta saveflag
;	sta bufpos_a
;	sta bufpos_b
;	sta buf_empt
	sta ld_cnt
	
	pla
	rts



ch376_open_persistent:
	jsr ch376_set_filename_block
	jsr ch376_openfile
	rts

ch376_load_block:
	jsr ch376_set_filename_block
	jsr ch376_openfile
	jsr ch376_load_bin_block
	jsr ch376_closefile
	rts

ch376_set_filename_block:
	;; Get filename pointer from input data
	stx stringpointer_l
	sty stringpointer_h

	;; Set first part of string to /
	ldx #$00
	lda #'/'
	sta cmdbuffer,X
	inx

	;; Copy filename string to cmdbuffer
	ldy #$00
ch376_set_filename_block_loop:
	lda (stringpointer_l),Y
	sta cmdbuffer,X
	beq ch376_set_filename_block_set
	inx
	iny
	jmp ch376_set_filename_block_loop
	
ch376_set_filename_block_set:
	;; Set filename
	lda #CH376_SET_FILENAME
	jsr ch376_send_cmd
	ldx #$00
ch376_set_filename_block_setname:
	lda cmdbuffer,x
	jsr ch376_send_data

	lda cmdbuffer,x
	inx
	cmp #$00
	bne ch376_set_filename_block_setname

	rts

	
ch376_reset:	
	pha
	;; Reset CH376
	lda #CH376_RESET_ALL
	jsr ch376_send_cmd

	jsr delay_ch376
	jsr delay_ch376
	jsr delay_ch376
	jsr delay_ch376
	jsr delay_ch376
	jsr delay_ch376

;	lda #<reset_string
;	sta stringpointer_l
;	lda #>reset_string
;	sta stringpointer_h
;	jsr print_string
	
	pla
	rts

ch376_usb_mode:
	pha
	;; Set USB mode

	lda #CH376_SET_USB_MODE
	jsr ch376_send_cmd

	lda #USB_MODE_HOST_SOF
	jsr ch376_send_data	

	jsr ch376_read_data
	cmp #CMD_RET_SUCCESS
	bne ch376_usb_mode_fail

	jsr ch376_read_data
	cmp #USB_INT_CONNECT
	bne ch376_usb_mode_fail

	jmp ch376_usb_mode_done
;	lda #<usbmode_string
; 	sta stringpointer_l
;	lda #>usbmode_string
;	sta stringpointer_h
;	jsr print_string
ch376_usb_mode_fail:
	lda #$01
	sta ld_cnt
ch376_usb_mode_done:	
	pla

	rts

ch376_connect:	
	pha
	;;  Connect drive
	lda #CH376_DISK_CONNECT
	jsr ch376_send_cmd

	;jsr delay_ch376

	jsr ch376_read_status

	;; Print status
	cmp #USB_INT_SUCCESS

	bne ch376_connect_failed

;	lda #<connect_ok
;	sta stringpointer_l
;	lda #>connect_ok
;	sta stringpointer_h
;	jsr print_string
	jmp ch376_connect_done
ch376_connect_failed:
;	lda #<connect_nok
;	sta stringpointer_l
;	lda #>connect_nok
;	sta stringpointer_h
;	jsr print_string
	lda #$01
	sta ld_cnt
ch376_connect_done:
	pla
	rts
	
ch376_mount:
	pha
	;; Mount drive
	lda #CH376_DISK_MOUNT
	jsr ch376_send_cmd

	;jsr delay_ch376
	jsr ch376_read_status

	;jsr delay_ch376

	cmp #USB_INT_SUCCESS
	bne ch376_mount_failed
	
	jmp ch376_mount_done
ch376_mount_failed:
	lda #$01
	sta ld_cnt
ch376_mount_done:
	pla
	rts



ch376_openfile:
	pha
	;; Read directory
	lda #CH376_FILE_OPEN
	jsr ch376_send_cmd

	jsr ch376_read_status
	
	cmp #USB_INT_DISK_READ
	bne ch376_open_notdir

	jmp ch376_openfile_done
ch376_open_notdir:
	cmp #USB_INT_SUCCESS
	bne ch376_open_failed

	;lda #<open_ok
	;sta stringpointer_l
	;lda #>open_ok
	;sta stringpointer_h
	;jsr print_string

	jmp ch376_openfile_done
ch376_open_failed:
	lda #<open_nok
	sta stringpointer_l
	lda #>open_nok
	sta stringpointer_h
	jsr print_string
ch376_openfile_done:
	pla
	rts
	
ch376_closefile:
	pha
	lda #CH376_FILE_CLOSE
	jsr ch376_send_cmd
	lda #$01 		; Update
	jsr ch376_send_data
	jsr ch376_read_status
	cmp #USB_INT_SUCCESS

ch376_closedone:	
	pla
	rts


ch376_load_bin_block:
	;; Get start address from block
	lda #$00
	sta stringpointer_l
	lda mem_base+0
	sta stringpointer_h

	lda #CH376_BYTE_READ
	jsr ch376_send_cmd
	lda #$FF
	jsr ch376_send_data
	lda #$FF
	jsr ch376_send_data

	jmp ch376_load_bin_block_loop

ch376_load_bin_block_cont:
	lda #CH376_BYTE_READ_GO
	jsr ch376_send_cmd
	
ch376_load_bin_block_loop:
	jsr ch376_read_status
	cmp #USB_INT_DISK_READ
	bne ch376_load_bin_block_done
	lda #CH376_RD_USB_DATA0
	jsr ch376_send_cmd
	jsr ch376_read_data
	tax

ch376_load_bin_block_read:
	tya
    pha
    jsr ch376_read_data
    ldy #0
	sta (stringpointer_l),Y
    pla
    tay
	inc stringpointer_l
	bne ch376_load_bin_block_next
	inc stringpointer_h
ch376_load_bin_block_next:
	dex
	bne ch376_load_bin_block_read
	jmp ch376_load_bin_block_cont

ch376_load_bin_block_done:
	rts


ch376_read_sector:
	pha
	txa
	pha
	tya
	pha

;	lda #<read_string
;	sta stringpointer_l
;	lda #>read_string
;	sta stringpointer_h
;	jsr print_string

	;; Setup read of 128 byte sector at current position
	lda #CH376_BYTE_READ
	jsr ch376_send_cmd
	lda #$80
	jsr ch376_send_data
	lda #$00
	jsr ch376_send_data

	ldy #$00

	jmp ch376_read_sector_loop

ch376_read_sector_cont:
	lda #CH376_BYTE_READ_GO
	jsr ch376_send_cmd
	
ch376_read_sector_loop:
	jsr ch376_read_status
	cmp #USB_INT_DISK_READ
	bne ch376_read_sector_done
	lda #CH376_RD_USB_DATA0
	jsr ch376_send_cmd
	jsr ch376_read_data

	tax

ch376_read_sector_read:
	jsr ch376_read_data
	sta (dma),y
	;jsr jmp_out
	iny
	dex
	bne ch376_read_sector_read

	jmp ch376_read_sector_cont

ch376_read_sector_done:
;	tya
;	jsr Jmp_byte

;	ldy #$00
;ch376_read_sector_debugprint:
;	lda (dma),Y
;	jsr jmp_out
;	iny
;	cpy #$80
;	bne ch376_read_sector_debugprint

	pla
	tay
	pla
	tax
	pla
	rts


ch376_write_sector:
	pha

	lda #CH376_BYTE_WRITE
	jsr ch376_send_cmd
	lda #$80
	jsr ch376_send_data
	lda #$00
	jsr ch376_send_data

	ldy #$00

ch376_write_sector_loop_outer:
	jsr ch376_read_status
	cmp #USB_INT_DISK_WRITE
	bne ch376_write_sector_done

	lda #CH376_WR_REQ_DATA
	jsr ch376_send_cmd
	jsr ch376_read_data

	tax  ; Number of bytes to read in X
ch376_write_sector_loop_inner:
	lda (dma),y
	jsr ch376_send_data
	iny
	dex
	bne ch376_write_sector_loop_inner

	lda #CH376_BYTE_WRITE_GO
	jsr ch376_send_cmd

	jmp ch376_write_sector_loop_outer
ch376_write_sector_done:	

	pla
	rts


ch376_locate:
	pha

;	lda #<seek_string
;	sta stringpointer_l
;	lda #>seek_string
; 	sta stringpointer_h
;	jsr print_string

	lda #CH376_BYTE_LOCATE
	jsr ch376_send_cmd

	lda fs_offset+0
	jsr ch376_send_data
;	jsr Jmp_byte

	lda fs_offset+1
	jsr ch376_send_data	
;	jsr Jmp_byte

	lda fs_offset+2
	jsr ch376_send_data	
;	jsr Jmp_byte

	lda fs_offset+3
	jsr ch376_send_data	
;	jsr Jmp_byte

	jsr ch376_read_status
	cmp #USB_INT_SUCCESS
	beq ch376_locate_done

	lda #<seek_nok
	sta stringpointer_l
	lda #>seek_nok
	sta stringpointer_h
	jsr print_string

ch376_locate_done:

	pla
	rts

ch376_send_cmd:
	pha
	;; Send synch bytes
	lda #$57
	jsr ACIA2_Output
	lda #$AB
	jsr ACIA2_Output

	;; Send command
	pla 
	jsr ACIA2_Output

	rts

ch376_send_data:
	jsr ACIA2_Output
	rts	

ch376_read_status:	
ch376_read_data:
	jsr ACIA2_Input
	rts
	
print_string:
	pha

	ldy #$0
print_string_loop:	
	lda (stringpointer_l),y
	beq print_string_done
	jsr jmp_out
	iny
	jmp print_string_loop
print_string_done:
	jsr Jmp_CR
	pla
	rts



delay_ch376:
	pha
	tya
	pha
	txa
	pha
	
delay_ch376_loop:
	ldy #$ff
delay_ch376_min:
	ldx #$ff
delay_ch376_in:
	dex
	bne delay_ch376_in
	dey
	bne delay_ch376_min
	pla
	tax
	pla
	tay
	pla
	rts


;filename_root:
;	.byte "/*"
;	.byte $00

;filename_input:
;	.byte "Input filename: "
;	.byte $00

;init_ok_string:
;	.byte "USB init OK"
;	.byte $00

;init_nok_string:
;	.byte "USB init failed"
;	.byte $00
	
;write_string:
;	.byte "Writing line"
;	.byte $00
	
;open_ok:			
;	.ascii "File open OK"
;	.byte $00

open_nok:
	.ascii "File open failed"
	.byte $00

seek_string:
	.ascii "Seeking"
	.byte $00

seek_nok:
	.ascii "Seek failed"
	.byte $00

read_string:
	.ascii "Reading sector"
	.byte $00

;file_close_ok:
;	.byte "File closed OK"
;	.byte $00

;file_create_ok:
;	.byte "File create OK"
;	.byte $00

;file_create_nok:
;	.byte "File create failed"
;	.byte $00

;file_erase_ok:
;	.byte "File erase OK"
;  	.byte $00

;file_erase_nok:
;	.byte "File erase failed"
;	.byte $00			

