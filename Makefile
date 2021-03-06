SOURCE=src
TARGET=ch_drv
OUTPUT=out

SOURCES=$(wildcard $(SOURCE)/*.c)
PWD = $(shell pwd)

KERNEL_VERSION=$(shell uname -r)
MODULES_DIR=/lib/modules/$(shell uname -r)

##
## BUILD
##

all: $(OUTPUT)/$(TARGET).ko

$(OUTPUT)/$(TARGET).ko: $(subst $(SOURCE),$(OUTPUT),$(SOURCES)) $(OUTPUT)/Makefile
	make -C "$(MODULES_DIR)/build" M="$(PWD)/$(OUTPUT)" modules

# Create a symlink from src to out
$(OUTPUT)/%: $(SOURCE)/% $(OUTPUT)
	ln -sf ../$< $@

# Generate a Makefile with the needed obj-m and *-objs set
$(OUTPUT)/Makefile: $(OUTPUT)
	echo "obj-m += $(TARGET).o" > $@
	echo "$(TARGET)-objs := $(subst $(TARGET).o,, $(subst .c,.o,$(subst $(SOURCE)/,,$(SOURCES))))" >> $@

$(OUTPUT):
	[ -d $(OUTPUT) ] || mkdir $(OUTPUT)

clean:
	rm -Rf $(OUTPUT)


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


##
## TEST
##

check:
	@echo "Checking is not implemented yet."
	@exit 1


.PHONY: all clean clear-buffer load unload reload check
