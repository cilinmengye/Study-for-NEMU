LD = $(CXX)

### Run an application with $(ISA)=native

env:
	$(MAKE) -C $(NAVY_HOME)/libs/libos ISA=native

run: app env
	@LD_PRELOAD=$(NAVY_HOME)/libs/libos/build/native.so $(APP) $(mainargs)
#	@LD_PRELOAD=/usr/lib/gcc/x86_64-linux-gnu/11/libasan.so $(NAVY_HOME)/libs/libos/build/native.so $(APP) $(mainargs)

gdb: app env
	@gdb -ex "set environment LD_PRELOAD $(NAVY_HOME)/libs/libos/build/native.so" --args $(APP) $(mainargs)

.PHONY: env run gdb
