; install script for regedit (%0% = language)

; install presets

; Just for the case this environment might some day support more than
; one language: this line
;
;    INSTALL PRESET="eiffel/presets/eiffel.gadgets"
;
; initially looked like this:
;
;    INSTALL PRESET="eiffel/presets/%0%/eiffel.gadgets"
;
; (and so on for all the other files)

INSTALL PRESET="eiffel/presets/eiffel.gadgets"
INSTALL PRESET="eiffel/presets/eiffel.context"
INSTALL PRESET="eiffel/presets/eiffel.mouse"
INSTALL PRESET="eiffel/presets/eiffel.syntax"
INSTALL PRESET="eiffel/presets/eiffel.tabs"
INSTALL PRESET="eiffel/presets/eiffel.templates"

INSTALL PRESET="eiffel/presets/eiffel_short.gadgets"
INSTALL PRESET="eiffel/presets/eiffel_short.syntax"

; install new filetypes

FILETYPE NAME="Eiffel"         PRI=127 ADD="#?.e"              PRESETS "eiffel.gadgets"       "eiffel.syntax"       "eiffel.context" "eiffel.templates" "eiffel.mouse" "eiffel.tabs"
FILETYPE NAME="Eiffel (Short)" PRI=127 ADD="#?sofa/short/#?.e" PRESETS "eiffel_short.gadgets" "eiffel_short.syntax" "eiffel.context"                    "eiffel.mouse" "eiffel.tabs"

:uninstall

UNINSTALL FILETYPE="Eiffel [(]Short[)]"
UNINSTALL FILETYPE="Eiffel"
UNINSTALL BASEDIR="golded:add-ons/eiffel"
UNINSTALL BASEDIR="golded:etc/images/toolbar/eiffel"
UNINSTALL BASEDIR="golded:etc/updates/envSOF"
