SOURCE=src
TARGET=ch_drv
OUTPUT=out

SOURCES=$(wildcard $(SOURCE)/*.c)
PWD = $(shell pwd)

##
## BUILD
##

all: $(subst $(SOURCE),$(OUTPUT),$(SOURCES)) $(OUTPUT)/Makefile
	make -C /lib/modules/$(shell uname -r)/build M="$(PWD)/$(OUTPUT)" modules

# Create a symlink from src to out
$(OUTPUT)/%: $(SOURCE)/%
	[ -d $(OUTPUT) ] || mkdir $(OUTPUT)
	ln -s ../$< $@

# Generate a Makefile with the needed obj-m and *-objs set
$(OUTPUT)/Makefile:
	echo "obj-m += $(TARGET).o" > $@
	echo "$(TARGET)-objs := $(subst $(TARGET).o,, $(subst .c,.o,$(subst $(SOURCE)/,,$(SOURCES))))" >> $@

clean:
	rm -Rf $(OUTPUT)
	mkdir -p $(OUTPUT)


##
## INSTALL
##

clear-buffer:
	@sh -x -c "sudo dmesg --clear"

load: $(OUTPUT)/$(TARGET).ko
	@if lsmod | grep -q $(TARGET); then \
		echo "Module '$(TARGET)' is already loaded."; \
		echo "Please use 'make unload' or 'make reload'."; \
	else \
		sh -x -c "sudo insmod $(OUTPUT)/$(TARGET).ko"; \
	fi

unload:
	@if lsmod | grep -q $(TARGET); then \
		sh -x -c "sudo rmmod $(OUTPUT)/$(TARGET).ko"; \
	else \
		echo "Module $(TARGET) already unloaded."; \
	fi

reload: unload clear-buffer load

.PHONY: all clean clear-buffer load unload reload
