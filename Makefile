UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
include Makefile.Linux
else ifeq ($(UNAME_S),Darwin)
include Makefile.OSX
else
$(error Unsupported OS: $(UNAME_S))
endif