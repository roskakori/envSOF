/*
 * create_images.pprx - Create toolbar images by extracting brushes from the
 *                      from toolbar.ilbm using PPaint 7.x
 *
 * $VER: create_images.pprx 1.1 (28.8.99)
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 */

directory = 'prog:envSOFxx/source/images'

images = ''

/* Names of images to extract */
images = images || 'view_as_class         view_as_short                   * '
images = images || 'view_as_class_ghosted view_as_short_ghosted           * '
images = images || 'open_class            open_short                      * '
images = images || 'new_class             reformat               features * '
images = images || 'make                  help                            * '

old_directory = Pragma('D', directory)

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
   'LoadImage toolbar.ilbm Force'
end

line = 0
column = 0
quit = 0

do while ~quit
   image_name = Word(images, 1)
   if image_name = '' then do
      quit = 1
   end
   else if image_name = '*' then do
      line = line + 1
      column = 0
   end
   else do
      Say line column image_name
      x0 = 24 * column + 25
      y0 = 20 * line
      x1 = x0 + 23
      y1 = y0 + 19
      'DefineBrush x0=' || x0 || ' y0=' || y0 || ' x1=' || x1 || ' y1=' || y1
      'SaveBrush File="goldEd:etc/images/toolbar/eiffel/' || image_name || '"'
      column = column + 1
   end
   images = DelWord(Strip(images, 'L'), 1,1)
end

Pragma('D', old_directory)

