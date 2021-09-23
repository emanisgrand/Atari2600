	processor 6502

	include "vcs.h"
	include "macro.h"

	seg code
	org $F000

Start:
	CLEAN_START		; macro to safely clear memory and TIA

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Start a new frame by turning on VBLANK and VSYNC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
NextFrame:
	lda #2        	; same as the value %00000010
	sta VBLANK		; turn on VBLANK
	sta VSYNC		; turn on VSYNC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Generate the three lines of VSYNC
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	sta WSYNC		; first scanline
	sta WSYNC		; second scanline
	sta WSYNC		; third scanline
	
	lda #0
	sta VSYNC		; turn off VSYNC

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Let the TIA output the recommended 37 scanlines of VBLANK
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ldx #37			; X = 37 (to count 37 scanlines)
VBlankLoop:
	sta WSYNC		; hit WSYNC and wait for the next scanline
	dex 			; X--
	bne VBlankLoop	; loop while X!=0

	sta VBLANK		; turn off VBLANK

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Draw 192 visibile scanline (kernel)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	ldx #192		; counter for 192 visible scanlines
VisibleLoop:
	stx COLUBK	    ; set the background color
	sta WSYNC		; wait for the next scanline
	dex 			; X--
	bne VisibleLoop	; loop while X != 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Output 30 more VBLANK lines (overscan) to complete our frame
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	lda #2
	sta VBLANK		; hit and turn on VBLANK again

	ldx #30			; counter for 30 scanlines
OverscanLoop:	
	sta WSYNC		; wait for the next scanline
	dex				; x--
	bne OverscanLoop; loop while x != 0

	jmp NextFrame

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Complete my ROM size to 4KB
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	org $FFFC
	.word Start
	.word Start