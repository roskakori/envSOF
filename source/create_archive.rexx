/*
 * create_archive.rexx -- Create envSOFxx archive.
 *
 * Copyright 1999 Thomas Aglassinger and others, see file "forum.txt"
 *
 * NOTE: Don't be surprised if this doesn't work on your machine.
 */

Call AddLib('rexxsupport.library', 0, -30, 0)

FailAt 5
Parse Arg arguments

Address Command

location = "prog:envSOFxx"
tmp_file = 'tt:envSOFxx.tmp'

me = 'Thomas Aglassinger <agi@sbox.tu-graz.ac.at>'
lf = d2c(10)
manual_guide = 'golded:add-ons/eiffel/Manual.guide'

/* Obtain version number of distribution */
'version >' || tmp_file || '  ' || manual_guide
if Open('version_file', tmp_file, 'read') then do
   version_line = ReadLn('version_file')
   Call Close('version_file')
   'delete quiet ' || tmp_file
   Parse Var version_line . ' ' version
   Say 'version = ' || version
end
else do
   Say 'Cannot read version information from "' || tmp_file || '"'
   Exit 10
end

/* Compute base/archive/target name */
Parse Var version main_version '.' release

base_name = 'envSOF' || main_version || release
archive_name = base_name || '.lha'
target_parent = 'ram:'
target_directory = target_parent || base_name
Say 'target  = ' || target_directory
Say

old_directory = Pragma('D', location)

/* Validate manual */
Say 'check manual'
Failat 10
'GuideCheck >' tmp_file || ' ' || manual_guide
if RC ~= 0 then do
   Say 'Cannot accept manual "' || manual_guide || '"'
   Say
   'type ' || tmp_file
   Exit 10
end

/* Validate references in "install" */
Say 'check installer'
'search install envSOF >' tmp_file
if Open('search_result', tmp_file, 'read') then do
   do while ~eof('search_result')
      line = ReadLn('search_result')
      if line ~= '' then do
         Parse Var line . (base_name) rest
         if rest = '' then do
            Say 'reference in "install" must be "' || base_name || '"'
            Say line
         end
      end
   end
   Call Close('search_result')
   'delete quiet ' || tmp_file
end
else do
   Say 'Cannot read search result from "' || tmp_file || '"'
   Exit 10
end

/* Cleanup */
call cleanup_archive
if exists('t:' || base_name || '.lha') then do
   'delete quiet t:' || base_name || '.#?'
end

/* Remove test_xxx programs and object/linker files */
'list source/#? all pat=test_#?.(lnk) lformat="delete quiet %p%m" >' || tmp_file
'execute ' || tmp_file                               /* ^ */
'list source/#? all pat=#?.(o|lnk) lformat="delete quiet %p%n" >' || tmp_file
'execute ' || tmp_file

/* Create directory */
Call make_dir(target_directory)
Call copy('/envSOFxx.info', target_directory || '.info')

/* Create README */
readme = target_directory || '/README'
Say 'create ' || readme
Call copy('README.info', readme || '.info')
'echo noline "" >' || readme
text = ''
text = text || 'Short:    Eiffel/Sofa Add-On for GoldEd Studio 6' || lf
text = text || 'Uploader: ' || me || lf
text = text || 'Author:   ' || me || lf
text = text || 'Type:     text/edit' || lf
text = text || 'Kurz:     Eiffel/Sofa Erweiterung für GoldEd Studio 6' || lf
text = text || 'Version:  ' || version || lf || lf
text = text || 'TITLE' || lf || lf
text = text || '  ' || base_name || lf || lf
text = text || 'VERSION' || lf || lf
text = text || '  ' || version || lf || lf
text = text || 'AUTHOR' || lf || lf
text = text || '  ' || me || lf || lf

Call append_text_to_readme(text)
Call append_file_to_readme('README.description')

text = ''
text = text || 'AVAILABILITY' || lf || lf
text = text || '  aminet:text/edit/' || archive_name || lf || lf

Call append_text_to_readme(text)
Call append_file_to_readme('README.copyright')

Call copy(readme, 't:' || base_name || '.readme')
Call copy(readme, 'README')

/* Copy source code */
Say 'copy source code'
Call copy_all('source#?', target_directory)

/* Copy installer */
Say 'copy installer'
Call copy_all('install#?', target_directory)
Call copy('Setup#?', target_directory)

/* Copy current golded settings to archive */
Say 'copy settings'
Call make_dir(target_directory || '/eiffel')
Call make_dir(target_directory || '/eiffel/images')
Call make_dir(target_directory || '/eiffel/presets')
Call make_dir(target_directory || '/eiffel/rexx')
Call make_dir(target_directory || '/eiffel/syntax')
Call make_dir(target_directory || '/installdata')
Call make_dir(target_directory || '/regedit')
Call make_dir(target_directory || '/scanner')
Call make_dir(target_directory || '/template')

Call copy('golded:add-ons/eiffel/rexx/#?', target_directory || '/eiffel/rexx')
Call copy('golded:add-ons/eiffel/syntax/#?', target_directory || '/eiffel/syntax')

Call copy('golded:etc/registry/presets/eiffel#?', target_directory || '/eiffel/presets')

Call copy('dh0:studio/etc/registry/presets/#?.syntax', target_directory || '/eiffel/presets')

'delete quiet "' || target_directory || '/eiffel/presets/eiffel.(api|menu)"'

Say 'copy scanner & templates'
Call copy('golded:etc/scanner/eiffel#?', target_directory || '/scanner')
Call copy('golded:add-ons/eiffel/template/#?', target_directory || '/template')

Say 'copy toolbar'
Call make_dir(target_directory || '/toolbar')
Call make_dir(target_directory || '/toolbar/eiffel')
Call copy('golded:etc/images/toolbar/eiffel/#?', target_directory || '/toolbar/eiffel')

/* Copy manual */
Say 'copy manual'
Call copy('golded:add-ons/eiffel/Manual.guide', target_directory)
Call copy('golded:add-ons/eiffel/forum.txt', target_directory)
Call copy('Manual.guide.info', target_directory)
Call copy('forum.txt.info', target_directory)

/* Copy preview */
Say 'copy preview'
Call copy('Preview#?', target_directory)

/* Copy GoldEd install stuff */
Say 'copy installdata'
Call copy_all('installdata', target_directory || '/installdata')
Call copy_all('golded:add-ons/regedit', target_directory || '/regedit')

/* Check if installer really copies everything */
Say 'check installer'
Call check_installer_copies('scanner')
Call check_installer_copies('template')

/* Creat LHA archive */
Say
Say 'create LHA archive'
Say
old_directory = Pragma('D', target_parent)

list_command = 'list >' || tmp_file || ' all lformat=%p%n ' || base_name || '#?'
list_command
'lha -xadern a t:' || base_name || '.lha @' || tmp_file

/* Create TGZ archive containing LHA and .readme */
Say
Say 'create TGZ archive'
Call Pragma('D', 't:')
'tar -czf ' || base_name || '.tgz' || ' ' || base_name || '.readme ' || base_name || '.lha'

Say
'delete quiet ' || tmp_file
'list NoHead t:' || base_name || '.#?'

/* Create stuff in release: */
Call copy(base_name || '.(lha|readme)', 'release:')
Call copy(base_name || '.(lha|readme)', 'release:bak')

if Upper(arguments) ~= "NOCLEANUP" then do
   call cleanup_archive
end

Call Pragma('D', old_directory)

Say
Say 'finished.'

exit 0

/*-------------------------------------------------------------------------*/

make_dir: procedure
   Parse Arg directory
   if ~exists(directory) then do
      'MakeDir ' || directory
   end
   Return

copy: procedure
   Parse Arg source, target
   'copy quiet clone "' || source || '" TO "' || target || '"'
   Return

copy_all: procedure
   Parse Arg source, target
   'copy all quiet clone "' || source || '" TO "' || target || '"'
   Return

cleanup_archive: procedure EXPOSE target_directory
   Say 'cleanup archive'
   'delete quiet all ' || target_directory || '#?'
   Return

/* Append `text' to README file */
append_text_to_readme: procedure EXPOSE readme
   Parse Arg text
   if Open('file', readme, 'append') then do
      Call WriteCh('file', text)
      Call Close('file')
   end
   else do
      Say 'Cannot append text to "' || readme || '"'
      exit 10
   end
   Return

/* Append contents of `filename' to README file */
append_file_to_readme: procedure EXPOSE readme
   Parse Arg filename
   'type >>' || readme || ' ' || filename
   Return

/* Check if 'install' copies all files in `directory' */
check_installer_copies: procedure EXPOSE target_directory
   Parse Arg directory
   full_directory = target_directory || '/' || directory
   list = ShowDir(full_directory)
    word_count = Words(list)
   if word_count = 0 then do
      Say
      Say 'cannot check empty directory'
      Exit 10
   end
   else do
      do i = 1 for word_count
         needle  = directory || '/' || Word(list, i)
         Say '   copylib(' || needle || ')'
         'search >nil: quiet nonum from="install" search="' || needle || '"'
         if RC ~= 0 then do
            Say
            Say 'Cannot find copylib for file "' || needle || '"'
            Exit 10
         end
      end
   end
   Return


