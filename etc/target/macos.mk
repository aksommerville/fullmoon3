# macos.mk

macos_MIDDIR:=mid/macos
macos_OUTDIR:=out/macos

macos_OPT_ENABLE:=serial fs time hw inmgr cheapsynth macos

macos_CCDEF:=$(addprefix -DFMN_USE_,$(macos_OPT_ENABLE))
macos_CCWARN:=-Werror -Wimplicit -Wno-parentheses -Wno-pointer-sign -Wno-comment
macos_CCINC:=-Isrc -I$(macos_MIDDIR)
macos_CC:=gcc -c -MMD -O3 $(macos_CCINC) $(macos_CCWARN) $(macos_CCDEF)
macos_OBJC:=gcc -c -MMD -O3 -xobjective-c $(macos_CCINC) $(macos_CCWARN) $(macos_CCDEF)
macos_AS:=gcc -xassembler-with-cpp -c -MMD -O3 $(macos_CCINC) $(macos_CCWARN) $(macos_CCDEF)
macos_LD:=gcc
macos_LDPOST:=-lm -lz -framework Cocoa -framework IOKit -framework Quartz

macos_SRCFILES:= \
  $(filter-out src/opt/% src/tool/% src/test/%,$(FMN_SRCFILES)) \
  $(filter $(addprefix src/opt/,$(addsuffix /%,$(macos_OPT_ENABLE))),$(FMN_SRCFILES))

macos_SRCFILES_DATA:=$(filter src/data/%,$(macos_SRCFILES))
$(foreach F,$(filter src/data/%.adjust,$(macos_SRCFILES_DATA)),$(eval $(patsubst src/%.adjust,$(macos_MIDDIR)/%.mid.c,$F):$F))
$(foreach F,$(filter src/data/%.tileprops,$(macos_SRCFILES_DATA)),$(eval $(patsubst src/%.tileprops,$(macos_MIDDIR)/%.png.c,$F):$F))
macos_SRCFILES_DATA:=$(filter-out src/data/song/%.adjust,$(macos_SRCFILES_DATA))
macos_SRCFILES_DATA:=$(filter-out src/data/image/%.tileprops,$(macos_SRCFILES_DATA))
macos_MIDFILES_DATA:=$(patsubst src/%,$(macos_MIDDIR)/%.c,$(macos_SRCFILES_DATA))
#TODO Image format. Eventually, macos should get images in every format, figure out how that's going to work.
$(macos_MIDDIR)/data/%.png.c:src/data/%.png $(tool_EXE_imgcvt);$(PRECMD) $(tool_EXE_imgcvt) -o$@ -i$< --fmt=Y2 --tileprops=src/data/$*.tileprops
$(macos_MIDDIR)/data/waves.txt.c:src/data/waves.txt $(tool_EXE_waves);$(PRECMD) $(tool_EXE_waves) -o$@ -i$< --name=fmnr_waves
$(macos_MIDDIR)/data/%.mid.c:src/data/%.mid $(tool_EXE_songcvt);$(PRECMD) $(tool_EXE_songcvt) -o$@ -i$< --adjust=src/data/$*.adjust
$(macos_MIDDIR)/data/map/%.c:src/data/map/% $(tool_EXE_mapcvt);$(PRECMD) $(tool_EXE_mapcvt) -o$@ -i$<
$(macos_MIDDIR)/data/%.c:src/data/% $(tool_EXE_rawdata);$(PRECMD) $(tool_EXE_rawdata) -o$@ -i$<

macos_CFILES:=$(filter %.c %.m,$(macos_SRCFILES) $(macos_MIDFILES_DATA))
macos_OFILES:=$(patsubst src/%,$(macos_MIDDIR)/%,$(addsuffix .o,$(basename $(macos_CFILES))))
-include $(macos_OFILES:.o=.d)
$(macos_MIDDIR)/%.o:            src/%.c;$(PRECMD) $(macos_CC) -o$@ $<
$(macos_MIDDIR)/%.o:$(macos_MIDDIR)/%.c;$(PRECMD) $(macos_CC) -o$@ $<
$(macos_MIDDIR)/%.o:            src/%.m;$(PRECMD) $(macos_OBJC) -o$@ $<
$(macos_MIDDIR)/%.o:$(macos_MIDDIR)/%.m;$(PRECMD) $(macos_OBJC) -o$@ $<

macos_BUNDLE:=$(macos_OUTDIR)/FullMoon.app
macos_PLIST:=$(macos_BUNDLE)/Contents/Info.plist
macos_NIB:=$(macos_BUNDLE)/Contents/Resources/Main.nib
macos_EXE:=$(macos_BUNDLE)/Contents/MacOS/fullmoon
macos_ICON:=$(macos_BUNDLE)/Contents/Resources/appicon.icns

$(macos_PLIST):src/opt/macos/Info.plist;$(PRECMD) cp $< $@
$(macos_NIB):src/opt/macos/Main.xib;$(PRECMD) ibtool --compile $@ $<
macos_INPUT_ICONS:=$(wildcard src/opt/macos/appicon.iconset/*)
$(macos_ICON):$(macos_INPUT_ICONS);$(PRECMD) iconutil -c icns -o $@ src/opt/macos/appicon.iconset

$(macos_EXE):$(macos_PLIST) $(macos_NIB) $(macos_ICON)
all:$(macos_EXE)
$(macos_EXE):$(macos_OFILES);$(PRECMD) $(macos_LD) -o$@ $(macos_OFILES) $(macos_LDPOST)

macos-run:$(macos_EXE);open -W $(macos_BUNDLE) --args --reopen-tty=$$(tty) --chdir=$$(pwd) --fbfmt=Y2 --tilesize=8
