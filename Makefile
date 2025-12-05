UNAME_S := $(shell uname -s)
DISTRO := $(shell lsb_release -si 2>/dev/null)

ifeq ($(UNAME_S),Darwin)
$(info Using Makefile.OSX)
include Makefile.OSX
else ifeq ($(DISTRO),Ubuntu)
$(info Using Makefile.Mate)
include Makefile.Mate
else ifeq ($(DISTRO),Pop)
$(info Using Makefile.Linux)
include Makefile.Linux
else
$(info Using Makefile.Linux)
include Makefile.Linux
endif