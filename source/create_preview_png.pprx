/*
 * create_preview_png.pprx - Convert Preview image to PNG using PPaint.
 *
 * $VER: create_preview_png.pprx 1.0 (31.8.99)
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */
directory = 'prog:envSOFxx/'

/* Come up with PPaint*/
PPPORT = 'PPAINT'
IF ~SHOW('P', PPPORT) THEN DO
   IF EXISTS('PPaint:PPaint') THEN DO
      ADDRESS COMMAND 'Run >NIL: PPaint:PPaint'
      DO 30 WHILE ~SHOW('P',PPPORT)
          ADDRESS COMMAND 'Wait >NIL: 1 SEC'
      END
   END
   ELSE DO
      SAY "Personal Paint could not be loaded."
      EXIT 10
   END
END

IF ~SHOW('P', PPPORT) THEN DO
   SAY 'Personal Paint Rexx port could not be opened'
   EXIT 10
END

ADDRESS VALUE PPPORT
OPTIONS RESULTS

if 1 then do
   'LoadImage File ' || directory || 'Preview Force'
   'SaveImage File ' || directory || 'preview.png Format=PNG'
   /* Find something to resize */
   'SaveImage File ' || directory || 'preview_small.png Format=PNG'
end

