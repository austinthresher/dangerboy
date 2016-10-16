; This test is designed to verify information found at:
; http://gameboy.mongenel.com/dmg/istat98.txt

; The specific condition being tested is:
;  Past  VBLANK           following  OAM         (at 00)  is ignored
; If this test passess, the document is incorrect.

.incdir "common"
.include "common.s"

   di
   ; Enable STAT VBLANK interrupt
   ld a, %00000010
   ldh (<IE), a
   ld a, %00010000
   ldh (<STAT), a
   xor a
   ldh (<IF), a
   ld d, a
   ei

wait:
   nop
   jp wait ; Wait for interrupt to fire

test_finish:
   di
   ldh a, (<LY)
   save_results
   assert_a 0
   assert_d 1
   jp process_results

stat_callback:
   xor a
   or d
   jr nz, test_finish

   ; The next time we hit this interrupt, we'll take the other branch
   inc d 
   ; Enable OAM interrupt
   ld a, $20 ; OAM 
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

   ld d, 0 ; Register D is a flag to indicate if we caught LYC (0) or HBLANK (1)

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
