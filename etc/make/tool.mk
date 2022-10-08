# tool.mk

tool_MIDDIR:=mid/tool
tool_OUTDIR:=out/tool

tool_OPT_ENABLE:=fs serial

tool_CCDEF:=$(addprefix -DFMN_USE_,$(tool_OPT_ENABLE))
tool_CCWARN:=-Werror -Wimplicit
tool_CCINC:=-Isrc -I$(tool_MIDDIR)
tool_CC:=gcc -c -MMD -O3 $(tool_CCINC) $(tool_CCWARN) $(tool_CCDEF)
tool_AS:=gcc -xassembler-with-cpp -c -MMD -O3 $(tool_CCINC) $(tool_CCWARN) $(tool_CCDEF)
tool_LD:=gcc
tool_LDPOST:=-lm -lz

tool_SRCFILES_COMMON:=$(filter %.c, \
  $(filter-out src/game/% src/opt/% src/tool/% src/data/% src/test/%,$(FMN_SRCFILES)) \
  $(filter $(addprefix src/opt/,$(addsuffix /%,$(tool_OPT_ENABLE))),$(FMN_SRCFILES)) \
  $(filter src/tool/common/%,$(FMN_SRCFILES)) \
)
  
$(tool_MIDDIR)/%.o:src/%.c;$(PRECMD) $(tool_CC) -o $@ $<
  
tool_NAMES:=$(filter-out common,$(notdir $(wildcard src/tool/*)))

define tool_RULES
  tool_$1_SRCFILES:=$(tool_SRCFILES_COMMON) $(filter src/tool/$1/%.c,$(FMN_SRCFILES))
  tool_$1_OFILES:=$$(patsubst src/%,$(tool_MIDDIR)/%.o,$$(basename $$(tool_$1_SRCFILES)))
  -include $$(tool_$1_OFILES:.o=.d)
  tool_EXE_$1:=$(tool_OUTDIR)/$1
  all:$$(tool_EXE_$1)
  $$(tool_EXE_$1):$$(tool_$1_OFILES);$$(PRECMD) $(tool_LD) -o $$@ $$^ $(tool_LDPOST)
endef
$(foreach T,$(tool_NAMES),$(eval $(call tool_RULES,$T)))

