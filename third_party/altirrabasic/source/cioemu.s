; Altirra BASIC - READ/INPUT data module
; Copyright (C) 2023 David Given, All Rights Reserved.
;
; Copying and distribution of this file, with or without modification,
; are permitted in any medium without royalty provided the copyright
; notice and this notice are preserved.  This file is offered as-is,
; without any warranty.

FCB_DIRTY = FCB__SIZE + 0
FCB_BUFFER = FCB__SIZE + 1
FCB_EXTRA__SIZE = FCB_BUFFER + 0x80

; icax1:   direction
; icax3/4: record number
; icax5:   byte offset into record
; icax6/7: ptr to FCB/buffer

.proc initcio
    ldy #0xff
    sty brkkey
    iny
    sty ichid
    lda #<(console_putchar-1)
    sta icptl
    lda #>(console_putchar-1)
    sta icpth
    
    lda #0x10
?loop:
    tax
    lda #0xff
    sta ichid, x
    lda #0
    sta icax6, x
    sta icax7, x
    txa
    add #0x10
    bpl ?loop
    
    rts
.endp

.proc ciov
    lda ichid, x
    beq ciov_console

    lda iccmd, x
    cmp #CIOCmdOpen
    jeq file_open
    cmp #CIOCmdPutChars
    jeq file_putchars
    cmp #CIOCmdClose
    jeq file_close
    rts
.endp

.proc ciov_console
    lda iccmd, x
    #if .byte @ == #CIOCmdPutChars
        mwa icbal,x ztemp3
        mwa icbll,x ztemp1
        ?loop:
            lda ztemp1+0
            ora ztemp1+1
            beq ?endloop
            
            ldy #0
            lda (ztemp3), y
            jsr console_putchar

            inc ztemp3+0
            sne:inc ztemp3+1

            lda ztemp1+0
            sub #1
            sta ztemp1+0
            scs:dec ztemp1+1
        
            jmp ?loop
        ?endloop:
        ldy #0
        rts
    #end
    #if .byte @ == #CIOCmdGetRecord
        jmp console_getrecord
    #end
    #if .byte @ == #CIOCmdClose
        ; Ignore attempts to close the console.
        ldy #0
        rts
    #end
    brk
    rts
.endp

.proc console_putchar
    #if .byte @ == #0x9b
        lda #13
        jsr console_putchar
        lda #10
    #end
    #if .byte @ >= #0x80
        rts
    #end
    ldy #BDOS_CONSOLE_OUTPUT
    jmp BDOS
.endp

.proc console_getrecord
    mwa icbal,x ztemp3
    ldy #0
    lda #0xff
    sta (ztemp3), y
    
    txa
    pha

    lda ztemp3+0
    ldx ztemp3+1
    ldy #BDOS_READ_LINE
    jsr BDOS

    ; Rewrite the buffer into the format which atbasic expects.

    ldy #1
    lda (ztemp3), y          ; get line length
    sta ztemp1+0
    ldy #0
?loop:
    iny
    iny
    lda (ztemp3), y
    dey
    dey
    sta (ztemp3), y
    iny
    cpy ztemp1+0
    bne ?loop
    
    ldy ztemp1+0
    lda #0x9b
    sta (ztemp3), y

    pla
    tax
    ldy ztemp1+0
    iny
    tya
    sta icbll, x
    lda #0
    sta icblh, x

    ; Print a newline (CP/M doesn't).
    
    lda #0x9b
    jsr console_putchar

    ldy #1
    rts
.endp

; Ensures that an FCB structure is in icax6 and icax7. Copy it to a0. Also
; copies C to a2+0.

.proc claim_fcb
    stx a2+0
    lda icax6, x
    sta a0+0
    lda icax7, x
    sta a0+1
    ora a0+0
    bne ?exit

    ; We need to allocate some memory.

    sbw memtop #FCB_EXTRA__SIZE
    lda memtop+0
    sta a0+0
    sta icax6, x
    lda memtop+1
    sta a0+1
    sta icax7, x

?exit:
    rts
.endp

; On entry:
;   icba points at the filename, terminated with 0x9b

.proc file_open
    jsr claim_fcb

    ; Zero-terminate the string.

    mwa icbal,x a1
    ldy #0xff
?terminate_loop:
    iny
    lda (a1), y
    cmp #0x9b
    bne ?terminate_loop
    lda #0
    sta (a1), y

    ; Populate and initialise the FCB.

    lda a0+0
    ldx a0+1
    ldy #BDOS_SET_DMA_ADDRESS
    jsr BDOS

    lda a1+0
    ldx a1+1
    ldy #BDOS_PARSEFILENAME
    jsr BDOS
    ldx a2+0                ; restore X

    ; Restore the 9b terminator.

    ldy #0xff
?unterminate_loop:
    iny
    lda (a1), y
    bne ?unterminate_loop
    lda #0x9b
    sta (a1), y

    ; Clear the flags.

    lda #0
    ldy #FCB_DIRTY
?fcb_clear_loop
    sta (a0), y
    iny
    cpy #FCB_EXTRA__SIZE
    bne ?fcb_clear_loop

    ; Initialise special fields in the IOCB.

    lda #1
    sta ichid, x
    lda #<(file_putchar-1)
    sta icptl, x
    lda #>(file_putchar-1)
    sta icpth, x
    lda #0
    sta icax3, x
    sta icax4, x
    sta icax5, x

    ; The buffer is unpopulated.

    lda #0xff
    ldy #FCB_R0+0
    sta (a0), y
    iny
    sta (a0), y

    ; icax1 contains the direction: 0x04 for read, 0x08 for write, 0x0c for
    ; bidirectional. The only option we care about is writing, which creates a
    ; new file.

    ldx a2+0
    lda icax1, x
    #if .byte @ == #0x08
        ; Open for output. First, delete the old file.

        lda a0+0
        ldx a0+1
        ldy #BDOS_DELETE_FILE
        jsr BDOS

        ; Then try to create a new one.

        lda a0+0
        ldx a0+1
        ldy #BDOS_CREATE_FILE
        jsr BDOS
        
        jmp ?done_open
    #end

    ; Otherwise, just open it.

    lda a0+0
    ldx a0+1
    ldy #BDOS_OPEN_FILE
?done_open:
    
    ; A is the error code. We can return this directly.

    tay
    rts
.endp

.proc file_putchars
    jsr claim_fcb
        
    mwa icbal,x a1
    ?loop:
        lda icbll,x
        ora icblh, x
        beq ?endloop
        
        ldy #0
        lda (a1), y
        jsr file_putchar

        inw a1

        dew icbll,x
        jmp ?loop
    ?endloop:
    ldy #0
    rts
.endp

; Don't use a1 here.
.proc file_putchar
    pha
    jsr claim_fcb

    jsr change_buffers

    ; Do the write.
    
    lda #FCB_BUFFER
    clc
    adc icax5, x
    tay
    pla
    sta (a0), y

    ; The buffer is now dirty.

    lda #0x80
    ldy #FCB_DIRTY
    sta (a0), y

    ; Increment pointers.

    inc icax5, x
    lda icax5, x
    #if .byte @ == 0x80
        ; We've reached the end of this record; change the seek position to the next one.

        lda #0
        sta icax5, x

        inw icax3,x
    #end

    ldy #1
    rts
.endp

; Seeks to the position in the IOCB, flushing and loading the buffer as
; required.

.proc change_buffers
    ldy #FCB_R0+0
    lda (a0), y
    cmp icax3, x
    bne ?seek_required
    iny
    lda (a0), y
    cmp icax4, x
    bne ?seek_required
    rts

?seek_required:
    jsr flush_buffer

    ; Set up for reading the new record.

    ldy #FCB_R0
    lda icax3, x
    sta (a0), y
    iny
    lda icax4, x
    sta (a0), y

    ; Do it.

    lda icax1, x
    #if .byte @ == #0x08
        ; If we're output only, we don't actually need to read the next record.
        ; We just clear it.

        ldy #FCB_BUFFER
        lda #0
    ?clear_loop:
        sta (a0), y
        iny
        cpy #FCB_BUFFER + 0x80
        bne ?clear_loop
    #else
        ; For all other modes, we need to read the next record.

        lda a0+0
        add #FCB_BUFFER
        ldx a0+1
        scc:inx
        ldy #BDOS_SET_DMA_ADDRESS
        jsr BDOS

        lda a0+0
        ldx a0+1
        ldy #BDOS_READ_RANDOM
        jsr BDOS
    
        ldx a2+0
    #end
.endp

.proc flush_buffer
    ; If the buffer isn't dirty, it doesn't need flushing.

    ldy #FCB_DIRTY
    lda (a0), y
    smi:rts
    
    ; Do the write.

    lda a0+0
    add #FCB_BUFFER
    ldx a0+1
    scc:inx
    ldy #BDOS_SET_DMA_ADDRESS
    jsr BDOS

    lda a0+0
    ldx a0+1
    ldy #BDOS_WRITE_RANDOM
    jsr BDOS
    ldx a2+0

    ldy #FCB_DIRTY
    lda #0
    sta (a0), y
    rts
.endp

.proc file_close
    lda ichid, x
    ldy #1
    cmp #0xff
    sne:rts

    ; Flush the file.

    jsr claim_fcb
    jsr flush_buffer

    ; Close it.

    lda a0+0
    ldx a0+1
    ldy #BDOS_CLOSE_FILE
    jsr BDOS
    ldx a2+0
    pha

    ; Mark the IOCB as closed.

    ldy #0xff
    sta ichid, x

    pla
    tay
    rts
.endp