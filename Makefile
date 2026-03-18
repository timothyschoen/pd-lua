# -*- mode: makefile-gmake -*-

# This needs GNU make.

# You may want to set these if you have the Pd include files in a
# non-standard location, and/or want to install the external in a
# custom directory.

#PDINCLUDEDIR = /usr/include/pd
#PDLIBDIR = /usr/lib/pd/extra

# No need to edit anything below this line, usually.

lib.name = pdlua

pdlua_version := $(shell git describe --tags 2>/dev/null)

luasrc = $(wildcard luas/lua/onelua.c)

PKG_CONFIG ?= pkg-config

ifeq ($(luasrc),)
# compile with installed liblua
$(info ++++ NOTE: using installed lua)
luaflags = $(shell $(PKG_CONFIG) --cflags lua)
lualibs = $(shell $(PKG_CONFIG) --libs lua)
else
# compile with Lua submodule
$(info ++++ NOTE: using lua submodule)
luaflags = -DMAKE_LIB -Iluas/luajit/src
define forDarwin
luaflags += -DLUA_USE_MACOSX
endef
define forLinux
luaflags += -DLUA_USE_LINUX
endef
define forWindows
luaflags += -DLUA_USE_WINDOWS
endef
endif

luajit_dir = ./luas/luajit
luajit_lib = $(luajit_dir)/src/libluajit.a

cflags = $(luaflags) -DPDLUA_VERSION="$(pdlua_version)"
ifdef PD_MULTICHANNEL
    cflags += -DPD_MULTICHANNEL=$(PD_MULTICHANNEL)
endif

pdlua.class.sources := luas/lua.c luas/luajit.c
pdlua.class.ldlibs := $(lualibs) $(luajit_lib)

datafiles = \
	pd.lua $(wildcard pdlua*-help.pd) \
	$(addprefix pdlua/tutorial/examples/, pdx.lua pd-remote.el pd-remote.pd) \
	pdlua-meta.pd

# the 'pdlua' directory contains subdirectories (with subdirs),
# so we need to list all of them
datadirs = $(shell find pdlua -type d)

PDLIBBUILDER_DIR=.
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

compat53_headers = \
    luas/lua-compat-5.3/compat53/compat53_init.h \
    luas/lua-compat-5.3/compat53/compat53_module.h \
    luas/lua-compat-5.3/compat53/compat53_file_mt.h

$(luajit_lib):
ifeq ($(system), Windows)
	$(MAKE) -C $(luajit_dir) SHELL=cmd
	mv $(luajit_dir)/src/libluajit-5.1.dll.a $(luajit_lib)
else
	$(MAKE) -C $(luajit_dir) CFLAGS="-fPIC" MACOSX_DEPLOYMENT_TARGET=10.11
endif

# Generate binary data headers for compat53 lua files, so we can load them on init
luas/lua-compat-5.3/compat53/compat53_init.h: luas/lua-compat-5.3/compat53/init.lua
	xxd -i $< > $@
luas/lua-compat-5.3/compat53/compat53_module.h: luas/lua-compat-5.3/compat53/module.lua
	xxd -i $< > $@
luas/lua-compat-5.3/compat53/compat53_file_mt.h: luas/lua-compat-5.3/compat53/file_mt.lua
	xxd -i $< > $@
luas/luajit.$(extension).o: $(compat53_headers)

pdlua.$(extension): $(luajit_lib)
