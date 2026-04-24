            include "68mix/macros.s"

            org     $00000000

            dc.l    $00FFE000,$00000400     ; Stack + Reset vector (SGDK standard)
            dc.l    EntryPoint              ; Our real start

EntryPoint:
            jsr     InitDemo
            jmp     MainLoop

InitDemo:
            ; clear screen, init VDP, etc. – SGDK will patch this later
            rts

MainLoop:
            ; === OUR DEMO CODE STARTS HERE ===
            VECTOR_STEP $2500,FIX16(80)
            COPPER_WRITE $81,$FF00          ; example: set palette entry 1 to white

            bra     MainLoop                ; infinite loop for now

            even