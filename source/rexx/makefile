#
# Makefile for GedEiffel ARexx scripts
#
#---------------------------------------------------------------------------

CPP	= cpp -P -C -pedantic-errors -I /rexx-include/
STRIP	= rx prog:StripEmptyLines/StripEmptyLines.rexx

DIR	= golded:add-ons/eiffel/rexx/

all : $(DIR)execute.rexx \
	$(DIR)console.rexx

rebuild :
	touch *.prx
	make all

$(DIR)execute.rexx : execute.prx
	$(CPP) execute.prx >$(DIR)execute.rexx
	$(STRIP) $(DIR)execute.rexx 1

$(DIR)console.rexx : console.prx
	$(CPP) console.prx >$(DIR)console.rexx
	$(STRIP) $(DIR)console.rexx 1

#
