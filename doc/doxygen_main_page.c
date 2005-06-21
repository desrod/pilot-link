/** \mainpage Pilot-link design and developer's reference

This is the developer and design manual for the pilot-link.
It should always be up to date since it is generated
directly from the source files using Doxygen.

\section hacking Hacking on this documentation

There is the beginning of a style guide for documenting under
\ref tipshints.

Feel free to start documenting or playing with doxygen configuration. 
This main page can be found in doc/doxygen_main_page.c .

Each doxygen section must be within a single comment block although 
large comment blocks can be split into separate pages:
\ref stylediscussion.

This main page is just an introduction to doxygen markup, see the
Doxygen manual for the full command set.

- \ref tipshints Tips and hints for using doxygen
- \ref stylediscussion Long comments, pages, editors
- \ref reference Links to the Doxygen manual

*/
/**
\page tipshints Useful tips for doxygen in C files

 - \ref index Introduction
 - \ref stylediscussion Long comments, pages, editors
 - \ref reference The Doxygen manual
 
\section tips An introduction to doxygen markup
 
\subsection Locations What to document

All declarations for:

-# typedef
-# struct
-# enum
-# functions

This will enable doxygen to link all parameter types to the declarations
every time the type is used in a function - very helpful to new developers.

\subsection Files Private files

If your declarations are in separate files, like private header files, 
a simple block can still be linked into doxygen as long as the file is
identified to doxygen using a '\\file' section:

** \\file filename.h\n
	\\brief one-liner summary of the file purpose\n
	\\author the usual copyright statement

\subsection Methods How to document

Every doxygen comment block starts with an adapted comment marker. 
You can use an extra slash /// or an extra asterisk ** . Blocks end
in the usual way. Doxygen accepts commands using a backslash.

To put a description with each function or structure, use '\\brief' 
End the brief description with a blank line. The rest of the documentation will
then be shown in the body of the doxygen page.

Commands may begin with \\ or @

\subsection Presentation Extras

	-# Start a line with a hyphen to start a list - the indent determines the
nesting of the list:
		- To create a numbered list, use -# e.g. for a sublist:
			-# start a numbered list
		- revert to previous list

	End the list with a blank line.
Use :: at the start of a function or structure to link to the page 
for that function in the doxygen documentation. 

Use the param command to describe function parameters in the text.

Use the 'back reference' to document enumerator values:\n
enum testenum {\n
	enum_one **< less than marker tells doxygen to use this line
		to document enum_one.

\subsection config Editing Doxygen configuration

To edit the doxygen configuration, you can use:
*
cd src/doc
*
doxywizard doxygen.cfg &

*/

/** \page stylediscussion Style discussion

- \ref index Introduction
- \ref tipshints Tips and hints for using doxygen
- \ref reference Links to the Doxygen manual

[codehelp 2004-07-25] This page can be handled more easily by splitting it
into repeated comments, split into pages. I've worked on doxygen files in
Kate, KWrite and XCode (MacOSX) and the comment higlighting works fine.
If you do have problems, particularly when you start a new line within an
existing comment, enter a character at the end of the last highlighted line
to refresh the highlighting. Some editors have a specific refresh option.

*/

/** \page reference Doxygen reference documentation

- \ref index Introduction
- \ref tipshints Tips and hints for using doxygen
- \ref stylediscussion Long comments, pages, editors

The Doxygen web site (http://www.stack.nl/~dimitri/doxygen/) has a
complete user manual.  For the impatient, here are the most
interesting sections:

- How to write grouped documentation for files, functions, variables,
etc.: http://www.stack.nl/~dimitri/doxygen/grouping.html .  Do not
forget to add a file documentation block (\@file) at the top of your
file. Otherwise, all documentation in that file will <i>not</i> appear
in the html output.

- List of the special commands you can use within your documentation
blocks: http://www.stack.nl/~dimitri/doxygen/commands.html

*/
