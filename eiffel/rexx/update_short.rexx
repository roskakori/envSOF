/*
 * update_short.rexx -- Create short form of a class.
 *
 * The short form is written to a file with the same name, but to the
 * directory "sofa/short/" relative to the original class directory.
 * For example "toy:source/hugo.e" is written to
 * "toy:source/sofa/short/hugo.e". If the short form already exists,
 * it is only written if the class file is newer than the short form.
 *
 * Copyright (C) 1999 Thomas Aglassiner <agi@sbox.tu-graz.ac.at>
 * Freeware. Use at your own risk.
 */
version_info = "$VER: update_short.rexx 1.1 (26.8.99)"

/* update_guide.rexx -- Update SmallEiffel class short form */
Call AddLib('rexxsupport.library', 0, -30, 0)
Call AddLib('rexxdossupport.library', 0, -30, 0)

/* Set default options */
template = 'File/A,Force/S,Quiet/S,Debug/S,Port/K,ShortOptions/F'
file = ''
port = Address()
ShortOptions = ''
force = 0
quiet = 0
debug = 0
host  = ''

/* Some constants */
linefeed = '0a'x
tab = '9'x

/* Check Arguments */
Parse Arg arguments

if Strip(arguments, 'B', ' "') = '?' then do
   Call WriteCh('STDOUT', template || ': ')
   Pull arguments
end

if ~ReadArgs(arguments, template) then do
   Say "Error in arguments: " || Fault(RC)
   Exit 10
end

/* Send status message commands to `port' */
Address Value port

separator_index = Max(LastPos(':', file), LastPos('/', file))

class_file = file
sofa_directory = Left(file, separator_index) || 'sofa'
short_directory = sofa_directory || '/short'
short_file = short_directory || '/' || SubStr(file, separator_index + 1)

Call debug_message('class = "' || class_file || '"')
Call debug_message('short = "' || short_file || '"')

/* Create short directory if necessary */
if ~Exists(sofa_directory) then do
   Call debug_message('create "' || sofa_directory || '"')
   MakeDir(sofa_directory)
end
if ~Exists(short_directory) then do
   Call debug_message('create "' || short_directory || '"')
   MakeDir(short_directory)
end

/* Compare datestamp of `class_file' and `short_file' */
if ~force then do
   class_time = file_time(class_file)
   if class_time = 0 then do
      Say 'Cannot find "' || class_file || '"'
      Exit 10
   end
   short_time = file_time(short_file)
   if class_time > short_time then do
      force = 1
   end
end

if force then do
   if exists(short_file) then do
      Call set_file_protection(short_file, 'rwed')
   end
   short_command = 'short '|| ShortOptions || ' "' || class_file || '" >"' || short_file || '"'
   Call progress_message('Updating short form...')
   Call debug_message(short_command)
   Address Command short_command
   Call set_file_protection(short_file, 'r') /* Make cached short form read only */
   Call progress_message('')
end
else do
   Call status_message('"' || short_file || '" is up to date')
end

Exit 0

/* Date and time of file in seconds till 1-Jan-1978 */
file_time: procedure
   Parse Arg filename

   info = StateF(filename)
   if info ~= '' then do
      Parse Var info . . . . days minutes ticks
      time = days * 24 * 60 * 60 + minutes * 60 + ticks / 50
   end
   else do
      time = 0 /* Assume 1-Jan-1978 if file does not exist */
   end

   Return time

/* Display progress message. Depending on current host, this can be in the console or
   the status line of the application. */
progress_message:
   Parse Arg message

   if Left(port, 6) = "GOLDED" then do
      'REQUEST STATUS="' || message || '"'
   end
   else if message ~= '' then do
      Say message
   end

   Return

/* Display status message if not `quiet' */
status_message:
   if ~quiet then do
      Parse Arg message
      Say message
   end

   Return

set_file_protection: procedure
   Parse Arg filename, mask
   Address Command 'protect "' || filename || '" ' || mask
   if RC ~= 0 then do
      Say 'Cannot change protection of "' || filename || '" to "' || mask || '"'
      Exit 10
   end
   Return

/* Display debug message if `debug' */
debug_message:
   if debug then do
      Parse Arg message
      Say message
   end

   Return

