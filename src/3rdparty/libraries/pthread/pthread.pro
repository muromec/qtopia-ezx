qtopia_project(stub)
TARGET=pthread
idep(contains(LIBS,-lpthread):LIBS-=-lpthread)
idep(LIBS+=-lpthread)
