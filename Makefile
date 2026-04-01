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

luajit_dir = ./luas/luajit/src
luajit_lib = $(luajit_dir)/libluajit.a

luajit_src = ./luas/luajit.c
lua_src = ./luas/lua.c

luaflags = -DMAKE_LIB
define forDarwin
luaflags += -DLUA_USE_MACOSX
endef
define forLinux
luaflags += -DLUA_USE_LINUX
endef
define forWindows
luaflags += -DLUA_USE_WINDOWS
endef

# stbi and nanosvg have functions we don't use
suppress-wunused=1

cflags = $(luaflags) -DPDLUA_VERSION="$(pdlua_version)" -Iluas/luajit/src
ifdef PD_MULTICHANNEL
    cflags += -DPD_MULTICHANNEL=$(PD_MULTICHANNEL)
endif

pdlua.class.sources := $(lua_src) $(luajit_src)
pdlua.class.ldlibs := $(luajit_lib)

datafiles = \
	pd.lua $(wildcard pdlua*-help.pd) \
	$(addprefix pdlua/tutorial/examples/, pdx.lua pd-remote.el pd-remote.pd) \
	pdlua-meta.pd

# the 'pdlua' directory contains subdirectories (with subdirs),
# so we need to list all of them
datadirs = $(shell /usr/bin/find pdlua -type d)

PDLIBBUILDER_DIR=.
include $(PDLIBBUILDER_DIR)/Makefile.pdlibbuilder

$(luajit_lib):
ifeq ($(system), Windows)
	$(MAKE) -C $(luajit_dir) BUILDMODE=static XCFLAGS="-DLUAJIT_ENABLE_LUA52COMPAT"
else
	$(MAKE) -C $(luajit_dir) BUILDMODE=static CFLAGS="-fPIC" MACOSX_DEPLOYMENT_TARGET=10.7 XCFLAGS="-DLUAJIT_ENABLE_LUA52COMPAT"
endif

pdlua.$(extension): $(luajit_lib)
