TEMPLATE=app
CONFIG-=qt
SOURCES=main.cpp
kernel:DEFINES+=__KERNEL__
preprocessor.commands=$$QMAKE_CXX $$QMAKE_CXXFLAGS -E $$PWD/main.cpp -o $$OUT_PWD/preprocessor.out
QMAKE_EXTRA_TARGETS+=preprocessor
