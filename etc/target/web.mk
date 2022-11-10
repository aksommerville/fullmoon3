# web.mk

web_MIDDIR:=mid/web
web_OUTDIR:=out/web

web_OPT_ENABLE:=web

ifeq (,$(strip $(web_WASI_SDK)))
  $(error Please define web_WASI_SDK in etc/config.mk. See https://github.com/WebAssembly/wasi-sdk)
endif

web_CCOPT:=-nostdlib
web_LDOPT:=-nostdlib -Xlinker --no-entry -Xlinker --import-undefined -Xlinker --export-all
web_CCDEF:=$(addprefix -DFMN_USE_,$(web_OPT_ENABLE))
web_CCWARN:=-Werror -Wimplicit -Wno-parentheses
web_CCINC:=-Isrc -I$(web_MIDDIR) -I/usr/include/libdrm
web_CC:=$(web_WASI_SDK)/bin/clang -c -MMD -O3 $(web_CCINC) $(web_CCWARN) $(web_CCDEF) $(web_CCOPT)
web_AS:=$(web_WASI_SDK)/bin/clang -xassembler-with-cpp -c -MMD -O3 $(web_CCINC) $(web_CCWARN) $(web_CCDEF) $(web_CCOPT)
web_LD:=$(web_WASI_SDK)/bin/clang $(web_LDOPT)
web_LDPOST:=

web_SRCFILES:= \
  $(filter-out src/opt/% src/tool/% src/test/%,$(FMN_SRCFILES)) \
  $(filter $(addprefix src/opt/,$(addsuffix /%,$(web_OPT_ENABLE))),$(FMN_SRCFILES))

web_SRCFILES_DATA:=$(filter src/data/%,$(web_SRCFILES))
$(foreach F,$(filter src/data/%.adjust,$(web_SRCFILES_DATA)),$(eval $(patsubst src/%.adjust,$(web_MIDDIR)/%.mid.c,$F):$F))
$(foreach F,$(filter src/data/%.tileprops,$(web_SRCFILES_DATA)),$(eval $(patsubst src/%.tileprops,$(web_MIDDIR)/%.png.c,$F):$F))
web_SRCFILES_DATA:=$(filter-out src/data/song/%.adjust,$(web_SRCFILES_DATA))
web_SRCFILES_DATA:=$(filter-out src/data/image/%.tileprops,$(web_SRCFILES_DATA))
web_MIDFILES_DATA:=$(patsubst src/%,$(web_MIDDIR)/%.c,$(web_SRCFILES_DATA))
#TODO Image format. Eventually, web should get images in every format, figure out how that's going to work.
$(web_MIDDIR)/data/%.png.c:src/data/%.png $(tool_EXE_imgcvt);$(PRECMD) $(tool_EXE_imgcvt) -o$@ -i$< --fmt=Y2 --tileprops=src/data/$*.tileprops
$(web_MIDDIR)/data/waves.txt.c:src/data/waves.txt $(tool_EXE_waves);$(PRECMD) $(tool_EXE_waves) -o$@ -i$< --name=fmnr_waves
$(web_MIDDIR)/data/%.mid.c:src/data/%.mid $(tool_EXE_songcvt);$(PRECMD) $(tool_EXE_songcvt) -o$@ -i$< --adjust=src/data/$*.adjust
$(web_MIDDIR)/data/map/%.c:src/data/map/% $(tool_EXE_mapcvt);$(PRECMD) $(tool_EXE_mapcvt) -o$@ -i$<
$(web_MIDDIR)/data/%.c:src/data/% $(tool_EXE_rawdata);$(PRECMD) $(tool_EXE_rawdata) -o$@ -i$<

web_CFILES:=$(filter %.c,$(web_SRCFILES) $(web_MIDFILES_DATA))
web_OFILES:=$(patsubst src/%,$(web_MIDDIR)/%,$(addsuffix .o,$(basename $(web_CFILES))))
-include $(web_OFILES:.o=.d)
$(web_MIDDIR)/%.o:            src/%.c;$(PRECMD) $(web_CC) -o$@ $<
$(web_MIDDIR)/%.o:$(web_MIDDIR)/%.c;$(PRECMD) $(web_CC) -o$@ $<

web_EXE:=$(web_OUTDIR)/fullmoon.wasm
all:$(web_EXE)
$(web_EXE):$(web_OFILES);$(PRECMD) $(web_LD) -o$@ $^ $(web_LDPOST)

web_HTDOCS_DSTDIR:=$(web_OUTDIR)/www
web_HTDOCS_SRC:=$(filter src/opt/web/www/%,$(web_SRCFILES))
web_HTDOCS:=$(patsubst src/opt/web/www/%,$(web_HTDOCS_DSTDIR)/%,$(web_HTDOCS_SRC)) $(web_HTDOCS_DSTDIR)/fullmoon.wasm
all:$(web_HTDOCS)
$(web_HTDOCS_DSTDIR)/%:src/opt/web/www/%;$(PRECMD) cp $< $@
$(web_HTDOCS_DSTDIR)/fullmoon.wasm:$(web_EXE);$(PRECMD) cp $< $@

# Regular web-run uses the web app straight off src/opt/web/www, so you can modify it on the fly conveniently.
web-run:$(web_EXE) $(tool_EXE_weblocal);$(tool_EXE_weblocal) --htdocs=src/opt/web/www --htdocs=$(web_OUTDIR)
web-run-built:$(web_HTDOCS) $(tool_EXE_weblocal);$(tool_EXE_weblocal) --htdocs=src/opt/web/www
