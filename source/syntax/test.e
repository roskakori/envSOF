indexing
	description:
		"Stress testing class for syntax highlighting; %
		%Because of several deliberate errors, it won't compile. %
		%%N%
		%Also note that the eiffel.parser does not treat everything %
		%you find in here appropriate due some minor bugs.";


class TEST

creation {ANY} 
	make

feature {ANY} 
	
	make is 
			-- Simple header comment for a simple routine.
		do  
			print("Hello %
					%World.%N");
		end -- make

	Multi_line: STRING is "%
		%a string going %
		%over several %
		%lines.";

	Simple_letter: CHARACTER is 'x';

	Form_feed: CHARACTER is '%F';

	Whatever_number: INTEGER is Unique;

	_NamingViolation: BOOLEAN;

	no_violation: A_CLASS;

	no_real_violation: INCOMPLETE_;
		-- strictly, this is a violation; but it is so common during
		-- typing that I didn't see any point in warning about it

	indeed_a_violation: A___CLASS_With_SOME_lOWERCASE;

	another_violation: A__;

	A_tyypo: STRING is "a tyypo";
		-- a tyypo in various elements

	-- Comments with a 'character'without a blank in between are also typos.

	-- at the end of this comment, there is a tyypo

	-- T'ain'

	-- T'ain' no

	-- T'ain' no doubt this is no proper English

	Under_construction: STRING is "work in progress

	Unterminated: STRING is "

	Unterminated_in_next_line is "%

	Same_after_some_text is "some text %

	Good_escapes: STRING is "%N*%/123/*%%*%%%N*%"*%%%%"

	Broken_escapes: STRING is "%X*%/abc/*%/123456789/"

	Unterminated_multi_line: STRING is "just stop %

	Unterminated_multi_line2: STRING is "just stop again, %
													%but only after one full line %

	Contains_empty_line: STRING is "%
		%still feeling good.%
		%%
		%even now!%
		-- Emptiness is a warm gun.
		%but this is not inside the string anymore";

	Too_long_character: CHARACTER is 'abc';

	Multi_line_character: CHARACTER is '%
		%x'; -- not sure if this is Eiffel at all

	-- Comment with "string" and 'character' and something else.

	-- Comment ending with a 'character'


	-- Comment referring to an `identifier' in the code.

	-- Comment with `more' than `one' such `reference'.

	--`reference' at the beginning.

	-- Unterminated `reference

	--`unterminated

	--`

	--`' (empty reference)

	-- a ``double_quote'' doesn't make sense

	-- This line doesn't have a reference, but a single quote (')

	-- This is not an `Eiffel-Reference; it contains $@\:§'; but anyway

	These § characters ~ are ` not Ö allowed % in £ the ÿ code. 

	These are: !#$()*+,-./:;<=>?@[\]^{|}

end -- class TEST
