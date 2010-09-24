SUBDIRS := doc driver lib test

# build for L865 when no CPU is provided or CPU=default
FILTERED_CPU := $(filter-out default, $(CPU))
ifeq ($(FILTERED_CPU),)
  MYCPU := L865
else
  MYCPU := $(FILTERED_CPU)
endif

# do not compile this for lynx
MYCPU := $(filter-out ppc4, $(MYCPU))

all:
	@if [ "$(MYCPU)" != "" ]; then \
                for dir in $(SUBDIRS); \
                do \
                        (cd $$dir && $(MAKE) CPU=$(MYCPU)); \
                done \
        fi

.PHONY: all clean

clean:
	@if [ "$(MYCPU)" != "" ]; then \
                for dir in $(SUBDIRS); \
                do \
                        $(MAKE) clean -C $$dir CPU=$(MYCPU); \
                done \
        fi
