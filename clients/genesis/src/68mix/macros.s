; 68mix macros – the version that has worked since 1999 never once failed on SN68K 2.53
; Eve's final kiss of death to assembler errors ♡

            MACRO VECTOR_STEP angle,amp
            move.l  #\angle,d2
            move.l  #\amp,d3
            ENDM

            MACRO COPPER_WRITE reg,val
            move.w  #$C000+\reg,$C00004
            move.w  #\val,$C00000
            ENDM

            MACRO FIX16 val
            dc.l    \val*$10000
            ENDM