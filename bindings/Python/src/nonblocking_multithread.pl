#!/usr/bin/perl
#
# I *think* we can do this with swigs %feature()
# support. http://www.swig.org/Doc1.3/Customization.html#features
# Documentation isn't great. I'm trying it out, so Florent can see the results.
# I disabled the call to this file in the Makefile for now. Nick Piper, April 2005
#
# post-process pisock_wrap.c to make the C calls non-blocking when running a multithreaded
# Python interpreter. I didn't find a way to do that properly in SWIG, because we need to
# bracket the C call to the wrapped function with SaveThread/RestoreThread to unlock the
# global interpreter lock. SWIG would only let me insert the code at beginning and end
# of the function, which is not good because there is access to python objects inside
# each wrapped function and these need to be properly guarded.
#
# Florent Pillet, April 2005
#
while (<>) {
	$thread_guard = 0;
	if (/^\s+result = \(PI_ERR\).+\(.*\);/ || /^\s+result = \(int\)pi_file.+\(.*\);/) {
		$thread_guard = 1;
	}
	print "\t{\n\t\tPyThreadState *__save = PyEval_SaveThread();\n\t" if $thread_guard;
	print $_;
	print "\t\tPyEval_RestoreThread(__save);\n\t}\n" if $thread_guard;
}

