# Include global settings
include ../Makefile.common

default:
	@echo "No default rule; specify one explicitly" >&2 && exit 1

clean:
	rm -f *.o *.lst

.PHONY: default clean

# Rule for disassembly

%.lst: ../common/%.o
	$(OBJDUMP) -rtd $< >$@

# Include dependencies
include Makefile.deps
