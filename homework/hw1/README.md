Author      : Joseph Blecha
Class       : CS 314, SIUE Summer
Date Created: 05/55/2026
Description: This directory contains multiple programs related to HW 1.
    Each program is the answer to problems 1-9 of this assigment with source
    files named #p.c and executables named #p where # is the respective
    problem number.
    This file also contains a makefile to compile executables. Simple type
    "make #", where # is the problem you wish to compile or "make all" to compile
    all files.

Problem 1: The inicial value given to the varible is the same in the child 
    process. However, when the value of the variable is changed in both 
    parent and child, the change only occures in the respective process.

Problem 2: Both the parent and the child are able to access the opened file.
    When both try to right at the same time, a race conditon ensues over
    whether the parent or child will write first; though both writes will
    take place.

Problem 3: In order to ensure that the child prints "hello" first, without
    using wait(), I simply delayed the parents processing time by using
    sleep(1). Which is a long enough pause to ensure that the childs print
    function executes first.

Problem 4: each form of exec() can be used in the program by uncommiting 
    the desired version and commenting out the rest. The exec() family of
    functions allow for multiple arugment types which are specified by
    the letters after exec. a leading "l" implies arguments are profied
    individually. while a leading "v" implies arguments are passed in vector
    form. Including a "p" allows you to specify a path, while 
    including an "e" allows you to specify a custom environment. "pe" allows
    you to specifiy both a path and custom environment.

Problem 5: When wait() is successfully called in the parent function, it
     returns the PID of the parent process. While when wait() is called in the
     child, it fails and returns -1.

Problem 6: waitpid() is useful when you have multiple children processes
    running, since it allows you to wait on a specific PID and not just any
    child.

Problem 7: After the child closes STDOUT it is unable to print with printf()
    since printf() writes to STDOUT. Though printf() is still usable in the
    parent process.

