ROOTDIR  =../../..
PKGNAME  =touchvg
PROJNAME =export

INCLUDES += -I$(ROOTDIR)/core/include \
            -I$(ROOTDIR)/core/include/canvas \
            -I$(ROOTDIR)/core/include/export

SOURCES   =$(wildcard *.cpp)

include $(ROOTDIR)/core/mk/config.mk
include $(ROOTDIR)/core/mk/Makefile.swig
