; install new presets

INSTALL PRESET="presets/eiffel.gadgets"
INSTALL PRESET="presets/eiffel.mouse"
INSTALL PRESET="presets/eiffel.syntax"
INSTALL PRESET="presets/eiffel.templates"

; install new filetype

FILETYPE ADD="(#?.e)" PRI=127 PRESETS "eiffel.gadgets" "eiffel.mouse" "eiffel.syntax" "eiffel.templates"

; Below message is shown in the installer window anyway
; INFO MSG="Installation complete, please RESTART GoldED."
