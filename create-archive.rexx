/* create-archive.rexx -- Create GedEiffel archive */

FailAt 5
Parse Arg arguments

Address Command

tmp_file = 't:GedEiffel.tmp'

'GuideCheck golded:tools/eiffel/Manual.guide'
Say
Say

old_directory = Pragma('D', 'prog:GedEiffel')

/* Obtain version number of distribution */
'version >' || tmp_file || ' golded:tools/eiffel/Manual.guide'
if Open('version_file', tmp_file, 'read') then do
   version_line = ReadLn('version_file')
   Call Close('version_file')
   'delete quiet ' || tmp_file
   Parse Var version_line . ' ' version
   Say 'version = ' || version
end
else do
   Say 'Cannot open "' || tmp_file || '"'
   Exit 10
end

call cleanup_archive
'delete quiet t:GedEiffel.#?'

/* Copy current golded settings to archive */
Say 'copy GoldEd settings'
Call copy('golded:registry/presets/eiffel.(gadgets|mouse|syntax|templates)', 'presets/')
Call copy('golded:syntax/eiffel.parser'   , 'syntax/')
Call copy('golded:scanner/eiffel'         , 'scanner/')
Call copy('golded:toolbar/eiffel/#?'      , 'toolbar/eiffel')
Call copy('golded:tools/eiffel/arexx/#?'  , 'tools/eiffel/arexx/')

/* Copy Readme and non-golded dependant ARexx scripts */
Say 'copy other material'
Call copy('README'                           , 't:GedEiffel.readme')
Call copy('golded:tools/eiffel/Manual.guide' , '')

/* Creat LHA archive */
Say
Say 'create LHA archive'
Say
old_directory = Pragma('D', 'prog:')

list_command = 'list >' || tmp_file || ' all lformat=%p%n GedEiffel/README#? GedEiffel/Manual#? GedEiffel/Install#? GedEiffel/(presets|scanner|syntax|toolbar|tools) GedEiffel/Screenshot GedEiffel/Screenshot.info GedEiffel.info'
list_command
'lha -xadern a t:GedEiffel.lha @' || tmp_file

/* Create ZIP archive containing LHA and .readme */
Say
Say 'create ZIP archive'
Say
Call Pragma('D', 't:')
'zip -0 -R GedEiffel.zip GedEiffel GedEiffel.lha GedEiffel.readme'

Say
'delete quiet ' || tmp_file
'list NoHead t:GedEiffel.#?'

/* Create stuff in release: */
'copy t:GedEiffel.(lha|readme) release:'
'copy t:GedEiffel.lha release:backup/GedEiffel-' || version || '.lha'
'copy t:GedEiffel.readme release:backup/GedEiffel-' || version || '.readme'

if Upper(arguments) ~= "NOCLEANUP" then do
   call cleanup_archive
end

Call Pragma('D', old_directory)

exit 0


copy: procedure
   Parse Arg source, target
   'copy quiet clone "' || source || '" TO "' || target || '"'
   Return

cleanup_archive:
   Say 'cleanup archive'
   'delete quiet Manual.guide rexx/#? presets/#? scanner/#? syntax/#? tools/eiffel/arexx/#? toolbar/eiffel/#?'
   Return

