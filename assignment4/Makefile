OBJDIR=objs
SRCDIR=src
HARNESSDIR=$(SRCDIR)/asst4harness
DEPDIR=deps
LOGDIR=logs.*

# all should come first in the file, so it is the default target!
.PHONY: all run clean cleanlogs
all : worker master

run: run.sh worker master | $(LOGDIR)
	./run.sh 1 tests/hello418.txt

SRCS=
DEPS=

WTF=

# Define our wonderful make functions.
include functions.mk

# Include all sub directories.
$(eval $(call define_program,worker,     \
        $(HARNESSDIR)/worker/main.cpp        \
        $(HARNESSDIR)/worker/work_engine.cpp \
        $(SRCDIR)/myserver/worker.cpp      \
))

$(eval $(call define_program,master,    \
        $(HARNESSDIR)/master/main.cpp       \
        $(HARNESSDIR)/master/main_loop.cpp  \
        $(SRCDIR)/myserver/master.cpp   \
))

$(eval $(call define_library,comm,      \
        $(HARNESSDIR)/comm/comm.cpp         \
        $(HARNESSDIR)/comm/connect.cpp      \
))

$(eval $(call define_library,types,     \
        $(HARNESSDIR)/types/types.cpp       \
        $(HARNESSDIR)/types/messages.cpp    \
))

$(OBJDIR)/libcomm.a: $(OBJDIR)/libtypes.a

worker master: $(OBJDIR)/libcomm.a $(OBJDIR)/libtypes.a


# I don't want to have to learn csh syntax.
SHELL := /bin/bash

LIBS=libglog libevent libgflags
PKGCONFIG=PKG_CONFIG_PATH=external_lib/pkgconfig:$$PKG_CONFIG_PATH pkg-config

# Fail quickly if we can't find one of the libraries.
# Ok, this is really the point I should have switched to autoconf.
LIBS_NOTFOUND:=$(foreach lib,$(LIBS),$(shell $(PKGCONFIG) --exists $(lib); if [ $$? -ne 0 ]; then echo $(lib); fi))
ifneq ($(strip $(LIBS_NOTFOUND)),)
  $(error "Libraries not found: '$(LIBS_NOTFOUND)'")
endif

CXX=g++
CXXFLAGS+=-Wall -Wextra -O2 -std=c++11
CPPFLAGS+=-I$(CURDIR)/src/asst4harness -I$(CURDIR)/src/asst4include $(foreach lib,$(LIBS), $(shell $(PKGCONFIG) --cflags $(lib)))
LDFLAGS+=-lpthread $(foreach lib,$(LIBS), $(shell $(PKGCONFIG) --libs $(lib))) -Xlinker -rpath -Xlinker external_lib

$(LOGDIR):
	mkdir -p $@

local_server: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -c -o $@

$(DEPDIR)/%.d: $(SRCDIR)/%.cpp Makefile
	@set -e; rm -f $@; \
        $(CXX) $(CXXFLAGS) -M $(CPPFLAGS) $< > $@.$$$$; \
        sed 's,$(notdir $*)\.o[ :]*,$(OBJDIR)/$*.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


-include $(DEPS)

clean:
	rm -rf $(OBJDIR) $(DEPDIR) master worker *.pyc

cleanlogs:
	rm -rf $(LOGDIR) latedays.qsub.*

handin: all
	tar -cf handin.tar writeup.pdf -C src myserver

