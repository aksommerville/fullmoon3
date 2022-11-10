all:
.SILENT:
.SECONDARY:
PRECMD=echo "  $(word 2,$(subst /, ,$@)) $(@F)" ; mkdir -p $(@D) ;

ifeq ($(MAKECMDGOALS),clean)
  clean:;rm -rf mid out
else
  etc/config.mk:|etc/config.mk.default;$(PRECMD) cp $< $@
  include etc/config.mk
  include etc/make/common.mk
  include etc/make/tool.mk
  include etc/make/test.mk
  $(foreach T,$(FMN_TARGETS),$(eval include etc/target/$T.mk))
endif
