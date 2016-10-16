; This test counts the number of STAT interrupts after
; each line when LYC, OAM and HBLANK interrupts are enabled.
; After the desired starting line, it constantly updates LYC
; to equal LY. It counts the number of interrupts fired after
; starting this from each line and prints the results.

.incdir "common"
.include "common.s"

   di

   ; Clear 144 bytes starting at C400
   ld h, $C4
   ld l, $00
   ld b, $99
   xor a
-  ld (hl+), a
   dec b
   jr nz, -

   ld b, $FF 
   
   ; Enable VBLANK interrupt
   xor a
   ldh (<IF), a
   ld a, %00000001
   ldh (<IE), a
   ei

wait:
   nop
   jp wait ; Wait for interrupt to fire

test_finish:
   di
   print_results test_callback
   halt_execution

test_callback:
   ; Loop through to print each memory value
   ld d, $91
   ld e, 10 
print_loop:
   push hl
   dec d
   ld h, $C4
   ld l, d
   ld a, (hl)
   pop hl
   call print_a
   dec e
   jp z, newline
   xor a
   or d
   jp nz, print_loop 

   call print_newline
   print_string_literal "DONE. PASS IF 0,2x2,"
   call print_newline
   print_string_literal "3x2,4x2, ... 49x2,4A"
   ret

newline:
   call print_newline
   jp print_loop - 2

stat_callback:
   ; Add 1 to $C400 + LYC
   ld h, $C4
   ld l, b
   inc (hl)

   ldh a, (<LY)
   inc a
   ldh (<LYC), a

   ld a, $68 ; OAM + HBLANK + LYC
   ldh (<STAT), a
  
   ; Clear raiased interrupt flags
   xor a
   ldh (<IF), a
   reti

vblank_callback:

   ; Increment LYC
   inc b ; This will overflow to 0 on our first loop
   ld a, b
   ldh (<LYC), a
   ld a, $40 ; Enable LYC interrupt
   ldh (<STAT), a

   ld d, 0 ; Register D is a flag to indicate if we caught LYC (0) or OAM (1)

   ; Enable STAT interrupt and VBLANK interrupt
   ld a, 3
   ldh (<IE), a

   add sp, +2 ; We're jumping out, not returning, so lets not overflow
   
   ld a, b
   cp $90
   jp z, test_finish

   ; Clear raised interrupts
   xor a
   ldh (<IF), a
   ei
   jp wait

.org INTR_VEC_STAT
   jp stat_callback

.org INTR_VEC_VBLANK
   jp vblank_callback
