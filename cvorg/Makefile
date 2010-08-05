SUBDIRS := ad9516 lib test driver scripts

# build for both platforms when no CPU is provided or CPU=default
FILTERED_CPU := $(filter-out default, $(CPU))
ifeq ($(FILTERED_CPU),)
  CPU_LIST := L865 ppc4
else
  CPU_LIST := $(FILTERED_CPU)
endif

all clean:
	@for platform in $(CPU_LIST); \
	do \
		for dir in $(SUBDIRS); \
		do \
			$(MAKE) $(filter-out all, $@) -C $$dir CPU=$$platform; \
		done \
	done

.PHONY: all clean
