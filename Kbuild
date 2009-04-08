# Include generic definitions
include $(M)/../makefiles/Kbuild.include

# Extra CC flags && needed pathes
ccflags-y += \
	-Wno-strict-prototypes \
	-I$(TOPDIR)/utils

# CDCM files
cdcm-y := $(OBJFILES)

# Shipped CDCM objects
shipped-cdcm := $(addsuffix _shipped, $(addprefix $(OBJDIR)/,$(cdcm-y)))

# That's what we'll do
always := outdir $(cdcm-y) $(shipped-cdcm)

# All compiled files goes here
$(obj)/outdir:
	@mkdir -p $(obj)/$(OBJDIR)

# Shipping CDCM objects
quiet_cmd_ship_cdcm = SHIPPING $<
      cmd_ship_cdcm = mv $< $@

# Tell Makefile HOWTO ship them
$(addprefix $(obj)/,$(shipped-cdcm)): $(obj)/$(OBJDIR)/%.o_shipped: $(obj)/%.o
	$(call cmd,ship_cdcm)