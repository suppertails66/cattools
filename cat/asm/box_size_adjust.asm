

;===============================================================================
; Constants
;===============================================================================

scratchByte = $0622       ; generic scratchpad space

dlgBoxBasePpu = $2150     ; base PPU address to which dialogue box text is
                          ; printed (original: 21B1)
dlgBoxCharsPerLine = $0A  ; characters printed per line before auto-wrapping
                          ; (original: 08)
dlgBoxLinesPerPrint = $03 ; number of lines printed per vblank interval
dlgBoxCurLine = $0625     ; a byte in RAM used to store current line number as
                          ; we print, so work can be distributed across multiple
                          ; frames
                          ; (must be unused in original game, or there will be
                          ; problems)
dlgBoxScratchCounter = scratchByte ; scratchpad space for line number while
                                   ; printing



;===============================================================================
; Segment: message pending handler
; The original routine is overwritten with this; modified version must fit
; within 0x74 bytes.
;===============================================================================

.SEGMENT "code1D6AD"

;===============================================================================
; End of previous routine; we need to be able to branch to this
; if certain preconditions are not met.
;===============================================================================
prevFuncRts:
  rts

;===============================================================================
; Check if dialogue box is pending, and draw it if it is and we're allowed to
; draw right now.
; Runs once per frame in VBlank handler.
;===============================================================================

lda $005F         ; get dialogue flag
and #$80          ; if bit 7 is set, dialogue is pending
beq prevFuncRts   ; if unset, we're done

lda $005F         ; get dialogue flag

  ;=====================================
  ; in the original game, the "dialogue
  ; pending" flag is cleared here; we
  ; instead leave it active until all
  ; the text has been written, allowing
  ; us to resume processing on
  ; subsequent frames if necessary
  ;=====================================
;   and #$7F
nop
nop
  
ora #$10          ; set ??? flag
                  ; I'm not sure what this does, but it may be used to signal
                  ; that a drawing event has occurred this frame, in order to
                  ; prevent too many drawing events from occuring in one vblank
sta $005F         ; write back updated flags

;=====================================
; n.b. for non-"dynamic" dialogue
; lines, this is the routine entry
; point -- it must be kept in the
; same place
;=====================================

lda $003C         ; save current bank number
pha 

  ; switch to bank 0
  lda #$00
  jsr $C818
  
  ; write message pointer table pointer to $0010
  lda $8003
  sta $0010
  lda $8004
  sta $0011

  ; get message index from $0769
  lda $0769
  asl 
  ; add to pointer table pointer
  clc 
  adc $0010
  sta $0010
  lda $0011
  adc #$00
  sta $0011
  
  ; save ?
;  tya 
;  pha 
    ; get source address
    ldy #$00
    lda ($0010),Y
    sta $0012
    iny 
    lda ($0010),Y
;    sta $0013
  ; restore ? ...
;  pla 
;  tay 
  ; ... and immediately overwrite it
  ; (moved to SetInitialPpuPos)
;  ldy #$00
  
  ; set initial PPU print address
;  lda #.HIBYTE(dlgBoxBasePpu)
;  sta $0011
;  lda #.LOBYTE(dlgBoxBasePpu)
;  sta $0010
  jsr setInitialPpuPos
  
  ; prepare line number counter
  lda #dlgBoxLinesPerPrint
  sta dlgBoxScratchCounter
  
    ;===================================
    ; print a line at a time until
    ; terminator encountered
    ;===================================
  
  lineDrawLoop:
    
    ; set PPUADDR
    lda $0011
    sta $2006
    lda $0010
    sta $2006
    ; prepare counter with number of characters per line
    ldx #dlgBoxCharsPerLine
    
    lineDrawSubLoop:
      ; get next character
      lda ($0012),Y
      
      ; adding linebreaks requires us to increment y here instead of at the
      ; end of the loop
      iny
      cmp #$00
      
      ; branch if 00 (terminator)
      beq boxDrawDone
      
      ; advance a line if top bit set
      bmi endCurrentLine
      
      sta $2007
;      iny
      dex 
      ; loop over line
      bne lineDrawSubLoop
    
    ; advance drawing position to next line
    endCurrentLine:
    clc 
    lda #$20
    adc $0010
    sta $0010
    lda #$00
    adc $0011
    sta $0011
    ; go to next line
;    jmp lineDrawLoop
    jmp handleLineEnd

; done drawing everything
boxDrawDone:
jsr finishDlgBox

; done drawing current set of lines, but more dialogue remains
lineDrawDone:
pla         ; get old bank number
jmp $C818   ; restore old bank and ret


;===============================================================================
; Segment: frame drawing routine modifications
;===============================================================================

.SEGMENT "code1D56E"

jmp extendBoxBorder

;===============================================================================
; Segments: attribute maps for indoor scenes with dialogue,
; which must accomodate the expanded window
; (ignore, came up with a more generic solution)
;===============================================================================

;.SEGMENT "attrMapHouse"
; 1A432

; original
;.byte $AA, $FA, $FA, $FA, $FA, $FA, $FA, $AA
;.byte $AA, $73, $50, $50, $00, $00, $CC, $AA
;.byte $AA, $77, $A5, $65, $00, $00, $CC, $AA
;.byte $AA, $77, $55, $55, $55, $55, $DD, $AA
;.byte $AA, $F7, $F5, $F5, $F5, $F5, $FD, $AA

; new
;.byte $AA, $3F, $0F, $0F, $0F, $0F, $CF, $AA
;.byte $AA, $73, $50, $50, $00, $00, $CC, $AA
;.byte $AA, $77, $A5, $65, $00, $00, $CC, $AA
;.byte $AA, $77, $55, $55, $55, $55, $DD, $AA
;.byte $AA, $F7, $F5, $F5, $F5, $F5, $FD, $AA


;===============================================================================
; Segment: all our extra code
;===============================================================================

.SEGMENT "freeSpace"

;=====================================
; dialogue printing
;=====================================

setInitialPpuPos:
  
  ; this wouldn't fit in the original routine due to other modifications
  sta $0013
  ldy #$00
  
  ; set initial PPU print address
  lda #.HIBYTE(dlgBoxBasePpu)
  sta $0011
  lda #.LOBYTE(dlgBoxBasePpu)
  sta $0010
  
  ; get current line number
  lda dlgBoxCurLine
  ; multiply by 16
  asl
  asl
  asl
  asl
  ; add to initial ppu address (twice -- each line is 32 bytes, but by doing it
  ; this way, we can have up to 16 lines instead of 8)
  pha
  jsr addToPpuAddr
  pla
  jsr addToPpuAddr
  
  ; multiply current line number by box width (currently 10)
  lda dlgBoxCurLine
  asl
  asl
  asl
  clc
  adc dlgBoxCurLine
  adc dlgBoxCurLine
  ; add to base message index
  clc 
  adc $0012
  sta $0012
  lda #$00
  adc $0013
  sta $0013
  
  rts

addToPpuAddr:
  clc 
  adc $0010
  sta $0010
  lda #$00
  adc $0011
  sta $0011
  rts

finishDlgBox:
  ; clear the line count
  lda #$00
  sta dlgBoxCurLine
  
  ; mark dialogue as handled
  lda $005F         ; get dialogue flag
  and #$7F          ; clear high bit
  sta $005F         ; write back updated flags
  
  rts
  
handleLineEnd:
  ; if printing was triggered manually, not during vblank, always draw all lines
  lda $005F
  and #$80
  beq @endLineDraw
  
  ; decrement lines remaining counter
  dec dlgBoxScratchCounter
  lda dlgBoxScratchCounter
  beq @endFrameDraw      ; branch if we've printed all we can for this frame
  
  @endLineDraw:
    jmp lineDrawLoop       ; draw next line
  
  @endFrameDraw:
    ; update line counter
    lda #dlgBoxLinesPerPrint
    clc
    adc dlgBoxCurLine
    sta dlgBoxCurLine
    ; terminate printing for current frame
    jmp lineDrawDone

;=====================================
; message border printing
;=====================================

boxBorderTilemapTop:
.byte $70, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $71, $72, $60
.byte $76, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $77, $78
.byte $76, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $61, $77, $63

boxBorderAttrMapTop:
.byte $AA, $3F, $0F, $0F, $0F, $0F, $CF, $AA

extendBoxBorder:

  ;=====================================
  ; we only want to expand the box for
  ; areas with actual dialogue, so we
  ; whitelist the specific areas we want
  ; to handle
  ;=====================================
  lda $062E
  cmp #$03    ; standard house
  beq @doIt
  cmp #$1F    ; classroom
  bcs @doIt
;  cmp #$20    ; church 1
;  beq @doIt
;  cmp #$21    ; church 2
;  beq @doIt
  bcc @done   ; anything else: don't extend frame
  
  @doIt:

  ;=====================================
  ; write top frame tilemap
  ;=====================================
  
  ; source address
  lda #.LOBYTE(boxBorderTilemapTop)
  sta $00FC
  lda #.HIBYTE(boxBorderTilemapTop)
  sta $00FD
  
  ; base PPUADDR
  lda #$21
  sta $00FB
  lda #$25
  sta $00FA
  
  ; number of lines
  lda #$3
  sta $00F9
  
  ; PPU line advance length
  lda #$20
  sta $0006
  
  ; width
  lda #$17
  sta scratchByte
  
  jsr drawTilemap
  
  ; fix shadow
  lda #$21
  sta $2006
  lda #$9B
  sta $2006
  lda #$63
  sta $2007

  ;=====================================
  ; update attribute map
  ;=====================================
  
  ; source address
  lda #.LOBYTE(boxBorderAttrMapTop)
  sta $00FC
  lda #.HIBYTE(boxBorderAttrMapTop)
  sta $00FD
  
  ; base PPUADDR
  lda #$23
  sta $00FB
  lda #$D0
  sta $00FA
  
  ; number of lines
;  lda #$1
;  sta $00F9
  ; optimization: we need 1 line and $00F9 is always zero at this point
  inc $00F9
  
  ; PPU line advance length
  ; (no need to set since we're only doing one line)
;  lda #$8
;  sta $0006
  
  ; width
  lda #$8
  sta scratchByte
  
  jsr drawTilemap
  
  ;=====================================
  ; make up work that was overwritten to
  ; add the call to this routine
  ;=====================================
  @done:
  pla 
  sta $00FD
  jmp $D571
  

drawTilemap:
  ; draw each line
  @lineDrawLoop:
    ldy #$00
    ; set PPUADDR
    lda $00FB
    sta $2006
    lda $00FA
    sta $2006
    ; line length
    lda scratchByte
    tax
    ; copy all tiles in line
    @lineDrawSubLoop:
      lda ($00FC),Y
      sta $2007
      iny 
      dex 
      bne @lineDrawSubLoop
    ; move PPUADDR down a line
    clc 
;    lda #$20
    lda $0006
    adc $00FA
    sta $00FA
    lda #$00
    adc $00FB
    sta $00FB
    
    ; move source data down a line
    jsr advanceTilemapSrcAddr
    
    ; decrement line counter
    dec $00F9
    bne @lineDrawLoop
  
  rts

;=====================================
; at startup, load alternate title
; graphics
;=====================================
loadAltTitle:
  ; save old bank
  lda $003C
  pha 
  
  ; switch to bank 5
  lda #$05
  jsr $C825
  
  ; source address: BC80
  lda #$BC
  sta $00FD
  lda #$80
  sta $00FC
  
  ; set PPUADDR to $1000
;  lda $2002
  lda #$10
  sta $2006
  lda #$00
  sta $2006
  
  ; do graphics decompression
  jsr $E3C1
  
  ; restore old bank
  pla 
  jmp $C825
  
  ; make up work
;  rts
;  jmp $C348  
  
;  rts

;===============================================================================
; Segment: load alternate title graphics at startup
;===============================================================================

;.segment "code1C861"

;jmp loadAltTitle

.segment "code33E2"

; load new title graphics
jmp loadAltTitle

;===============================================================================
; Segment: jump to credits screen code at startup
;===============================================================================

.segment "code29C5"

jsr showStartupCredits

;===============================================================================
; Segment: credits screen code
;===============================================================================

.segment "code3F96"

showStartupCredits:

  ; make up work
  jsr $A9AC

  ;=====================================
  ; load regular font (title graphics
  ; are loaded at this point, and we
  ; need the lower case letters)
  ;=====================================

  lda #$10
  sta $2006
  lda #$00
  sta $2006
  lda #$02
  jsr $C873

  ;=====================================
  ; blank current screen's tilemap
  ;=====================================
  
  ; set PPUADDR to $2000
  lda #$20
  sta $2006
  lda #$00
  sta $2006
  ; draw "blank" tilemap
  ldx #$AA
  ldy #$D3
  jsr $B3F0

  ;=====================================
  ; draw new tilemap
  ;=====================================
  
  ; source address
  lda #$87
  sta $00FC
  lda #$BE
  sta $00FD
  
  ; base PPUADDR
  lda $BE84
  sta $00FB
  lda $BE83
  sta $00FA
  
  ; number of lines
  lda $BE86
  sta $00F9
  
  ; PPU line advance length
  lda #$40
  sta $0006
  
  ; width
  lda $BE85
  sta scratchByte
  
  jsr drawTilemap

  ;=====================================
  ; handle display tasks
  ;=====================================
  
  ; fade in
  lda #$1D
  jsr $C003
  
  ; delay $B4 frames
  ldy #$B4
  jsr $AA92
  ; carry is set by $2A92 if start was pressed to skip the rest of the
  ; sequence
  bcc @notDone1
    ; skip rest of sequence
    jmp @finish
  @notDone1:
  ; delay $B4 frames
  ldy #$78
  jsr $AA92
  
  @finish:
  
  ; fade out
  lda #$1C
  jsr $C003
  
  ; load (new) title graphics
  jsr $B3D1

  ; delay #$1E frames
  ldy #$1E
  jmp $AA92
  
;  jmp loadAltTitle
;  rts

;===============================================================================
; Segment: status message printer
; This prints messages that have been copied to a RAM buffer.
; While we keep the buffer the same size, we alter the layout; the original box
; is 8x4 characters, but we use 10x3 (plus 2 spaces we can't use).
; The number of characters is lower, but this actually makes it easier to fit
; in longer words, so it's to our benefit.
;===============================================================================

.segment "code1C47B"

lda $005F
ora #$10
sta $005F
ldx #$00

; line 1
; set PPUADDR
lda #$20
sta $2006
lda #$6B
sta $2006
; characters per line
ldy #$0A
@loop1:
  lda $0700,X
  sta $2007
  inx 
  dey 
  bne @loop1

; line 2
; set PPUADDR
lda #$20
sta $2006
lda #$8B
sta $2006
ldy #$0A
@loop2:
  lda $0700,X
  sta $2007
  inx 
  dey 
  bne @loop2

; line 3
lda #$20
sta $2006
lda #$AB
sta $2006
ldy #$0A
@loop3:
  lda $0700,X
  sta $2007
  inx 
  dey 
  bne @loop3

; line 4
; (2 characters instead of 10)
;lda #$20
;sta $2006
;lda #$CB
;sta $2006
;ldy #$02
;@loop4:
;  lda $0700,X
;  sta $2007
;  inx 
;  dey 
;  bne @loop4

lda $0723
and #$7F
sta $0723
rts 

; OOPS I NEEDED SPACE
advanceTilemapSrcAddr:
  lda scratchByte
  adc $00FC
  sta $00FC
  lda #$00
  adc $00FD
  sta $00FD
  rts
  


;===============================================================================
; Segments: password screen prompts
;===============================================================================

.segment "code17050"

; set PPU target
; original: $208B, new: $2089
ldx #$20
ldy #$89

.segment "code17071"

; set string length
; original: #$0C, new: #$10
; I'd very much like for this to be #$12, but due to the game's inexplicable
; fondness for copying things to RAM before printing them, that's not easily
; possible and I don't care enough about this game to try to circumvent it
lda #$10

;.segment "code1C25C"
;
; ???
;and #$1F


