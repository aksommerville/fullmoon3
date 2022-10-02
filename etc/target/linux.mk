# linux.mk

linux_MIDDIR:=mid/linux
linux_OUTDIR:=out/linux

linux_OPT_ENABLE:=serial fs time genioc hw alsa evdev glx render_rgba inmgr

linux_CCDEF:=$(addprefix -DFMN_USE_,$(linux_OPT_ENABLE))
linux_CCWARN:=-Werror -Wimplicit
linux_CCINC:=-Isrc -I$(linux_MIDDIR) -I/usr/include/libdrm
linux_CC:=gcc -c -MMD -O3 $(linux_CCINC) $(linux_CCWARN) $(linux_CCDEF)
linux_AS:=gcc -xassembler-with-cpp -c -MMD -O3 $(linux_CCINC) $(linux_CCWARN) $(linux_CCDEF)
linux_LD:=gcc
linux_LDPOST:=-lX11 -lXinerama -lGL -ldrm -lasound -lpthread -lm -lz

linux_SRCFILES:= \
  $(filter-out src/opt/% src/tool/% src/test/%,$(FMN_SRCFILES)) \
  $(filter $(addprefix src/opt/,$(addsuffix /%,$(linux_OPT_ENABLE))),$(FMN_SRCFILES))

linux_SRCFILES_DATA:=$(filter src/data/%,$(linux_SRCFILES))
linux_MIDFILES_DATA:=$(patsubst src/%,$(linux_MIDDIR)/%.c,$(linux_SRCFILES_DATA))
#TODO Image format. Eventually, linux should get images in every format, figure out how that's going to work.
$(linux_MIDDIR)/data/%.png.c:src/data/%.png $(tool_EXE_imgcvt);$(PRECMD) $(tool_EXE_imgcvt) -o$@ -i$< --fmt=Y2
$(linux_MIDDIR)/data/%.c:src/data/% $(tool_EXE_rawdata);$(PRECMD) $(tool_EXE_rawdata) -o$@ -i$<

linux_CFILES:=$(filter %.c,$(linux_SRCFILES) $(linux_MIDFILES_DATA))
linux_OFILES:=$(patsubst src/%,$(linux_MIDDIR)/%,$(addsuffix .o,$(basename $(linux_CFILES))))
-include $(linux_OFILES:.o=.d)
$(linux_MIDDIR)/%.o:            src/%.c;$(PRECMD) $(linux_CC) -o$@ $<
$(linux_MIDDIR)/%.o:$(linux_MIDDIR)/%.c;$(PRECMD) $(linux_CC) -o$@ $<

linux_EXE:=$(linux_OUTDIR)/fullmoon
all:$(linux_EXE)
$(linux_EXE):$(linux_OFILES);$(PRECMD) $(linux_LD) -o$@ $^ $(linux_LDPOST)

# We do support a color framebuffer of course, and it's the default.
# But during development, I am only making the 8x8 y2 graphics in the first pass.
linux-run:$(linux_EXE);$(linux_EXE) --fbfmt=Y2 --tilesize=8
