/****** console.rexx *******************************************************
 * NAME
 *    console.rexx -- Output console to execute CLI commands.
 * INPUTS
 *    Port/K
 *       Portname to create and to which commands can be sent. If ommited,
 *       "CONSOLE.1" is used.
 *    Debug/S
 *       Enable debugging messages.
 * FUNCTION
 *    Start the console and waits for commands.
 *
 *    (This script is automatically invoked by execute.rexx, if the
 *    requested port does not yet exist.)
 *
 *    Commands can be executed by sending them as ARexx messages to the port
 *    Preferable this should be done calling the script execute.rexx.
 *
 *    All command are assumed to be normal CLI commands like "list", "echo"
 *    or "type". However, there are a view exceptions:
 *
 *    QUIT
 *       Quits the console and removes its message port. For compatibility,
 *       ENDCLI and ENDSHELL work the same.
 *    CD [`directory']
 *       The normal CD command does not work because the previous current
 *       directory is restored once ADDRESS COMMAND returns. Therefore,
 *       this command has to be emulated by the shell.
 *
 *       The emulation works pretty much the same. However, relativ paths
 *       are not relative to the invoking shell, but to the current
 *       directory of the console:
 *
 *          CD "ram:"
 *             Change current directory to "ram:"
 *          CD hugo/sepp
 *             Change to "hugo/sepp" relative to the current directory of
 *             the console.
 *          CD
 *             Display current directory in console. Works like PWD under
 *             other systems. This, by the way, simulates the behavior of
 *             the original c:CD.
 *    CD-INTERNAL `directory'
 *       Changes the current of the console to the one specified, but only
 *       if its not already there anyway. If no CD is necessary, no command
 *       is executed and nothing is displayed in the console.
 *
 *       See NOTES below for more details.
 * EXAMPLES
 *    In most cases, you will like to redirect the output of the shell to
 *    one window reserved for it. Furthermore, you will want to it to run
 *    as a task in the background:
 *
 *       run <>nil: rx console.rexx
 *                  >con:////Console/AUTO/CLOSE/WAIT/INACTIVE
 *
 *    Because of AUTO, you can close the window when it is not needed. Once
 *    new commands arrive, it automatically reopens. Because of INACTIVE,
 *    the window will not change to the current window once it opens. This
 *    avoids that you are distract from your editor or other developer
 *    tools.
 *
 *    An even more useful variant is:
 *
 *       run <>nil: rx rexx:console.rexx
 *                  >con:0/4096/512/96/Console/
 *                  AUTO/CLOSE/WAIT/INACTIVE/
 *                  ALT0/4096/512/400/SCREENGOLDED.1
 *
 *    ALT is used to specify the window size when the zoom-gadget is
 *    clicked. Here the normal size is very small so the window does not
 *    waste much space on the screen. But in case of something happening
 *    (e.g. loads of compiler errors), only one mouse-click results in a
 *    nice, big window. Finally, SCREENGOLDED.1 causes the window to pop-up
 *    on a public screen named GOLDED.1, where usually GoldEd is running.
 *
 *    If you just want to shell to share your current CLI without running
 *    in the background, simply use
 *
 *       rx console.rexx
 *
 *    In that case however you still need a second CLI to send command to
 *    the shell because it does not read from the console but from its
 *    ARexx port.
 * NOTES
 *    The execute.rexx scripts *always* executes a CD-INTERNAL with the
 *    current directory of the invoking CLI. This avoids a lot of problems
 *    if you stick to using this script.
 * COPYRIGHT
 *    Copyright (C) 1999 Thomas Aglassinger <agi@sbox.tu-graz.ac.at>
 *    Freeware. Use at your own risk.
 * SEE ALSO
 *    execute.rexx
 **************************************************************************/
version_info = "$VER: console.rexx 1.2 (24.1.99)"

Options FailAt 21

Signal On Failure
Signal On NoValue

template = 'Port/K,Auto/S,Debug/S'
port = 'CONSOLE.1'
auto = 0
debug = 0

LF = d2c(10)
CR = d2c(13)
ESC = d2c(27)
BS = d2c(8)

Call add_library('rexxsupport.library', 0)
Call add_library('rexxdossupport.library', 0)

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

if OpenPort(port) then do

   command_counter = 0
   show_prompt = 0

   if ~auto then do
      Call say_prompt()
   end

   quit = 0
   do until quit
      /* Wait for command */
      Call WaitPkt(port)
      packet = GetPkt(port)

      /* Reset return value */
      rv = 0

      real_message = packet ~= null()
      if real_message then do

         /* Obtain command */
         command = GetArg(packet)
         command = Strip(command,'L')

         /* Strip a possible surrounding pair of quotes */
         if (Right(command,1) = '"') & (Left(command,1) = '"') then do
            command = SubStr(command, 2, Length(command) - 2)
         end

         quit = (Find('QUIT ENDSHELL ENDCLI', Upper(command)) > 0)
         if ~quit then do
            first_word = Upper(Word(command, 1))
            if (first_word = 'CD') | (first_word = 'CD-INTERNAL') then do
               /* Execute "CD" command */
               if Words(command) > 1 then do
                  directory = SubStr(command, WordIndex(command,2))
               end
               else do
                  directory = ''
               end
               directory = Strip(Strip(directory, 'B'), 'B', '"')
               current_directory = Pragma('Directory')
               if directory ~= '' then do
                  /* Change current directory */
                  Call say_debug( first_word || ' from "' || current_directory || '" to "' || directory || '"')
                  if (first_word ~= 'CD-INTERNAL') ,
                     | (Upper(directory) ~= Upper(current_directory)) ,
                  then do
                     Call say_command()
                     Call Pragma('Directory', directory)
                  end
               end
               else do
                  /* "CD" without parameters shows current directory
                   * (like "pwd" under Unix) */
                  Call say_command()
                  Say current_directory
               end
            end
            else do
               /* Execute normal command */
               Call say_command()
               Address Command command
               rv = RC
            end
         end
         else do
            Call say_status(command)
         end
         Call Reply(packet, rv)

         if rv > 0 then do
            Call Say_status('Command returned ' || rv)
         end

         if ~quit then do
            if show_prompt then do
               Call say_prompt()
            end
         end
      end
   end

   Say LF || 'Console ended'

   ClosePort(port)
end
else do
   Say 'The port "' || port || '" already exists.'
   exit 10
end

exit 0

/* Show status message in bold face */
say_status:
   Parse Arg message
   Say ESC || '[1m' || message || ESC || '[0m'
   Return

/* Show command about to execute */
say_command:
   /*
   if ~show_prompt then do
      Call say_prompt()
   end
   */
   command_counter = command_counter + 1
   show_prompt = 1
   Call say_status command
   Return

/* Show debug message */
say_debug:
   if debug then do
      Parse Arg message
      Say message
   end
   Return

say_prompt:
   Call WriteCh('STDOUT', ESC || '[1m> ' || ESC || '[0m')
   show_prompt = 0
   Return

/* Signal handler for CLI failures */
Failure:
   Call say_status(LF || 'Command failed')
   Say 'Console ended'
   Exit RC

/* Signal handler for accessing unitialized variables */
NoValue:
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

