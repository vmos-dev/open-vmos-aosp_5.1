This directory contains a set of tests for Android's Bionic Standard C++ library.

You must define the BIONIC_TESTS environment variable to build these
test programs. For example, do:

    cd system/extras/tests/bionic/libstdc++
    mm BIONIC_TESTS=1

Preferably, to build and run you can use this:

    runtest_py libstdcpp

All test programs should exit with a status code of 0 in case of success, and 1
in case of failure.

The directory layout is currently flat because there is one Bionic test. If you
want to add GNU STDC++ or benchmark tests, look in tests/bionic/libc as an
example how to structure your files.

