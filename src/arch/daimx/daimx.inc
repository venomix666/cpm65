Start_OS = $E800         ; easy access to monitor program
Jmp_CR = $E803
Jmp_1sp = $E806		; jump table for usable monitor
Jmp_2sp = $E809			; routines
Jmp_xsp = $E80C			; This will not change in future
Jmp_nib = $E80F			; releases, only be added to
Jmp_byte = $E812   ;Print1Byte
jmp_wrd = $E815   ;Print2Byte
jmp_bell = $E818   ;Bell
jmp_delay = $E81B   ;Delay
jmp_scan = $E81E  ;Scan_input
jmp_inp = $E821  ;Input_chr
jmp_out = $E824   ;Output
jmp_input = $E827  ;Input
jmp_input1 = $E82A  ;Input1
jmp_basic_cold = $A003
stringpointer_l = $EE
stringpointer_h = $EF
cmdbuffer = $0500
fs_offset = $05f3
bin_addr_l = $05f7
bin_addr_h = $05f8
ld_cnt = $05f9

ACIA2dat = $8100
ACIA2sta = $8101
ACIA2cmd = $8102
ACIA2ctl = $8103

VRAM_base = $9000
VRAM_row_two = VRAM_base + $40
VRAM_lastrow = VRAM_base + $77F
VRAM_nextlastrow = VRAM_base + $73F
vidcursor_x = $0410
vidcursor_y = $0411
vid_textmode = $0414
vid_curchar = $0415
vid_style = $0416
vid_curvis = $0417
vidpointer = $00EA

#define CH376_RESET_ALL		$05
#define CH376_EXISTS 		$06
#define CH376_SET_USB_MODE 	$15
#define CH376_GET_STATUS	$22
#define CH376_SET_FILENAME 	$2F
#define CH376_DISK_CONNECT 	$30
#define CH376_DISK_MOUNT 	$31
#define CH376_FILE_OPEN 	$32
#define CH376_FILE_CLOSE 	$36
#define CH376_FILE_CREATE 	$34
#define CH376_FILE_ERASE 	$35
#define CH376_RD_USB_DATA0	$27
#define CH376_WR_REQ_DATA	$2D
#define CH376_FILE_ENUM_GO 	$33
#define CH376_BYTE_READ		$3A
#define CH376_BYTE_READ_GO	$3B
#define CH376_BYTE_LOCATE 	$39
#define CH376_BYTE_WRITE 	$3C
#define CH376_BYTE_WRITE_GO 	$3D
#define USB_MODE_HOST_RESET 	$07
#define USB_MODE_HOST_SOF 	$06
#define USB_INT_SUCCESS  	$14
#define USB_INT_DISK_READ	$1D
#define USB_INT_CONNECT 	$15
#define USB_INT_DISK_WRITE 	$1E
#define ERR_MISS_FILE 		$42
#define CMD_RET_SUCCESS 	$51
