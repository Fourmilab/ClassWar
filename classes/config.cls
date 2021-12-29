
\   Class system configuration specifications

: CONFIG.CLS.VER
    "CONFIG.CLS V1.0 3/24/90 JW"
;

\ config.cls.ver type cr

\   CLASS.EDIT definition for Sun TEXTEDIT tool

80 string cledit

(
: class.edit
\   dup type cr
    "textedit " cledit s! cledit strcat cledit system drop 1
;
)

\   Unix version of CLASS.FILENAME

: class.filename
    "/tmp/" cledit s! cledit strcat ".tmp" cledit strcat cledit
;

\   If CLASS.DEBRIS-CHECK is defined, it is run to check for stack trash
\   after calling user methods.

: class.debris-check
    depth if
        ." "Derbis left on " .s cr
    then
;

\   MEMDUMP  --  Dump memory in hex for debugging

20 string estr
variable eol

: memdump                             ( length addr --  )
    0 eol !
    swap 0 ?do
        dup @ "%08X " estr strform estr type 4 +
        1 eol +! eol @ 8 >= if 0 eol ! cr then
    loop
    cr
;

\   F?  --  Utility definition to show a floating point value at a pointer

: f?
    2@ f.
;
