/****** execute.rexx *******************************************************
 * NAME
 *    execute.rexx -- Execute command in console.
 * USAGE
 *    rx execute.rexx command
 * FUNCTION
 *    Execute `command' in console opened by console.rexx
 *
 *    Before the actual command is executed, the console is made to change
 *    change its internal current directory to the same where this script was
 *    started from. If this directories are not equal upon invocation, the
 *    EiffelShell also displays this CD command.
 * RESULT
 *    The return code of the command executed by the console or 10 if
 *    the console is not running. In the later case, you also get a
 *    descriptive error message.
 * EXAMPLES
 *    A simple invocation can be:
 *
 *       rx execute.rexx echo sepp
 *
 *    If you want to do sophisticated things like redirection, you have
 *    to quote the whole command, e.g.
 *
 *       rx execute.rexx "echo sepp >t:sepp.text"
 *
 *    if you want to use quotes and redirection, you have to escape them
 *    as usually
 *
 *       rx execute.rexx "echo *"sepp was here*" >t:sepp.text"
 *
 *    results in the command
 *
 *       echo "sepp was here" >t:sepp.text
 *
 *    to be executed.
 * COPYRIGHT
 *    Copyright (C) 1999 Thomas Aglassinger <agi@sbox.tu-graz.ac.at>
 *    Freeware. Use at your own risk.
 * SEE ALSO
 *    console.rexx
 ***************************************************************************/
version_info = "$VER: execute.rexx 1.2 (12.3.99)"

Options Results

Signal On Failure
Signal On NoValue

template = 'Port/K,Command/A/F'
port = 'CONSOLE.1'

rexx_path = 'GoldEd:add-ons/eiffel/rexx/'
console_script = rexx_path || 'console.rexx'

Call add_library('rexxdossupport.library', 2, '')

/* Check Arguments */
Parse Arg arguments

If Strip(arguments, 'B', ' "') = '?' then do
   Call WriteCh('STDOUT', template || ': ')
   Pull arguments
end

if ~ReadArgs(arguments, template) then do
   Say "Error in arguments: " || Fault(RC)
   Exit 10
end

if ~Show('Ports', port) then do
   console_specification = GetVar('Console/' || port)
   if RC ~= 0 then do
      console_specification = ,
         'CON:0/9999/512/84/' || port || ,
         '/AUTO/CLOSE/INACTIVE/ALT0/9999/512/400/SCREEN**'
   end

   console_command = 'run <>nil: rx >"' || console_specification || '" ' || ,
                     console_script || ' Port="' || port || '"'

   Address Command console_command
   if RC = 0 then do
      Address Command 'WaitForPort ' || port
   end
   else do
      Say 'Attempt to start new console at port "' || port || '" failed.'
      Say '(The console script is expected to be in "' || console_script || '".)'
   end
end

/* Do it */
if Show('Ports', port) then do
   Address Value port
   'cd-internal "' || Pragma('Directory') || '"'
   command
   Exit RC
end
else do
   Say 'Can not find output console at port "' || port || '"'
   Say 'Start console.rexx before using this script'
   Exit 10
end

/* Signal handler for CLI failures */
Failure:
   Say 'Command failed return code ' || RC
   Exit RC

/* Signal handler for accessing unitialized variables */
NoValue:
   LF = d2c(10)
   message = LF || SIGL SourceLine(SIGL) || LF || ,
             'uninitialized value in line ' || SIGL
   Say message
   Exit 20

/****** ugly/add_library **************************************************
 * NAME
 *   add_library -- Add library or abort.
 * FUNCTION
 *   If the requested library exists in "libs:", it is opened AddLib().
 *
 *   Otherwise, `message', prefixed by "Library ... not found.", is
 *   displayed and the program exts with code 20.
 * INPUTS
 *   library - Name of the library to open
 *   version - Minimum library version (0 for any)
 * RESULT
 *   If this function returns, everything was ok.
 **************************************************************************/
add_library: procedure
    Parse Arg library, version, error_message

    if ~Show('L', library) then do
        added = 0
        if Exists('libs:' || library) then do
           added = AddLib(library, 0, -30, version)
        end
        if ~added then do
            message = "Library '" || library "'"
            if (version > 0) then do
                message = message || " version " || version
            end
            message = message || 'not found. ' || error_message
            Say message
            Exit 20
        end
    end

    return

