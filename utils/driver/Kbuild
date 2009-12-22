# Include generic definitions
include $(ROOTDIR)/makefiles/Kbuild.include

# Driver utility files
utils-y := $(OBJFILES)

# Shipped driver utils objects
shipped-utils := $(addsuffix _shipped, $(addprefix $(CPU)/$(KVER)/,$(utils-y)))

# That's what we'll do
always := outdir $(utils-y) $(shipped-utils)

# All compiled files goes here
$(obj)/outdir:
	@mkdir -p $(obj)/$(CPU)/$(KVER)

# Shipping driver utility objects
quiet_cmd_ship_utils = SHIPPING $<
      cmd_ship_utils = mv $< $@

# Tell Makefile HOWTO ship them
$(addprefix $(obj)/,$(shipped-utils)): $(obj)/$(CPU)/$(KVER)/%.o_shipped: $(obj)/%.o
	$(call cmd,ship_utils)