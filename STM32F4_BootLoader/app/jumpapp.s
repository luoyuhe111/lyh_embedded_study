    PRESERVE8
    THUMB

    AREA    |.text|, CODE, READONLY

JumpApp         PROC
                EXPORT  JumpApp

                LDR     SP, [R0, #0]
                LDR     PC, [R0, #4]
                ENDP

    ALIGN

    END
