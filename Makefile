TARGET		:= vitaQuakeII
TITLE		:= QUAKE0002
SOURCES		:= source
SHADERS     := shaders
INCLUDES	:= include

LIBS = -lvitaGL -lvorbisfile -lvorbis -logg -lspeexdsp -lmpg123 -lmathneon \
	-lSceLibKernel_stub -lSceAppMgr_stub -lSceSysmodule_stub -ljpeg \
	-lSceCtrl_stub -lSceTouch_stub -lm -lSceNet_stub -lSceNetCtl_stub \
	-lSceAppUtil_stub -lc -lScePower_stub -lSceMotion_stub -lSceCommonDialog_stub \
	-lSceAudio_stub -lSceGxm_stub -lSceDisplay_stub -lSceNet_stub -lSceNetCtl_stub \
	-lvitashark -lSceShaccCg_stub

SYSTEM = 	psp2/vid_psp2.o \
			psp2/snddma_psp2.o \
			psp2/sys_psp2.o \
			psp2/in_psp2.o \
			psp2/net_psp2.o \
			psp2/glimp_psp2.o \
			psp2/glob.o

CLIENT =	client/cl_input.o \
			client/cl_inv.o \
			client/cl_main.o \
			client/cl_cin.o \
			client/cl_ents.o \
			client/cl_fx.o \
			client/cl_parse.o \
			client/cl_pred.o \
			client/cl_scrn.o \
			client/cl_tent.o \
			client/cl_view.o \
			client/menu.o \
			client/console.o \
			client/keys.o \
			client/snd_dma.o \
			client/snd_mem.o \
			client/snd_mix.o \
			client/qmenu.o \
			client/cl_newfx.o 

QCOMMON = 	qcommon/cmd.o \
			qcommon/cmodel.o \
			qcommon/common.o \
			qcommon/crc.o \
			qcommon/cvar.o \
			qcommon/files.o \
			qcommon/md4.o \
			qcommon/net_chan.o \
			qcommon/pmove.o 

SERVER = 	server/sv_ccmds.o \
			server/sv_ents.o \
			server/sv_game.o \
			server/sv_init.o \
			server/sv_main.o \
			server/sv_send.o \
			server/sv_user.o \
			server/sv_world.o

REFSOFT = 	ref_soft/r_alias.o \
			ref_soft/r_main.o \
			ref_soft/r_light.o \
			ref_soft/r_misc.o \
			ref_soft/r_model.o \
			ref_soft/r_part.o \
			ref_soft/r_poly.o \
			ref_soft/r_polyse.o \
			ref_soft/r_rast.o \
			ref_soft/r_scan.o \
			ref_soft/r_sprite.o \
			ref_soft/r_surf.o \
			ref_soft/r_aclip.o \
			ref_soft/r_bsp.o \
			ref_soft/r_draw.o \
			ref_soft/r_edge.o \
			ref_soft/r_image.o

REFGL = 	ref_gl/gl_draw.o \
			ref_gl/gl_image.o \
			ref_gl/gl_light.o \
			ref_gl/gl_mesh.o \
			ref_gl/gl_model.o \
			ref_gl/gl_rmain.o \
			ref_gl/gl_rmisc.o \
			ref_gl/gl_rsurf.o \
			ref_gl/gl_warp.o

BASEQ2_DIRS = game game/savegame game/player game/monster/berserker game/monster/boss2 game/monster/boss3 game/monster/brain \
	game/monster/chick game/monster/flipper game/monster/float game/monster/flyer game/monster/gladiator game/monster/gunner \
	game/monster/hover game/monster/infantry game/monster/insane game/monster/medic game/monster/misc game/monster/mutant \
	game/monster/parasite game/monster/soldier game/monster/supertank game/monster/tank game/shared
BASEQ2 := $(foreach dir,$(BASEQ2_DIRS), $(wildcard $(dir)/*.c))
			
ROGUE_DIRS = rogue rogue/shared rogue/savegame rogue/player rogue/dm rogue/monster/widow rogue/monster/turret rogue/monster/tank \
	rogue/monster/supertank rogue/monster/stalker rogue/monster/soldier rogue/monster/parasite rogue/monster/mutant rogue/monster/misc \
	rogue/monster/medic rogue/monster/insane rogue/monster/infantry rogue/monster/hover rogue/monster/gunner rogue/monster/gladiator \
	rogue/monster/flyer rogue/monster/float rogue/monster/flipper rogue/monster/chick rogue/monster/carrier rogue/monster/brain \
	rogue/monster/boss3 rogue/monster/boss2 rogue/monster/berserker
ROGUE := $(foreach dir,$(ROGUE_DIRS), $(wildcard $(dir)/*.c))

XATRIX_DIRS = xatrix xatrix/shared xatrix/savegame xatrix/player xatrix/monster/tank xatrix/monster/supertank xatrix/monster/soldier \
	xatrix/monster/parasite xatrix/monster/mutant xatrix/monster/misc xatrix/monster/medic xatrix/monster/insane xatrix/monster/infantry xatrix/monster/hover xatrix/monster/gunner \
	xatrix/monster/gladiator xatrix/monster/gekk xatrix/monster/flyer xatrix/monster/float xatrix/monster/flipper xatrix/monster/fixbot xatrix/monster/chick \
	xatrix/monster/brain xatrix/monster/boss5 xatrix/monster/boss3 xatrix/monster/boss2 xatrix/monster/berserker
XATRIX := $(foreach dir,$(XATRIX_DIRS), $(wildcard $(dir)/*.c))

ZAERO_DIRS = zaero zaero/zaero zaero/shared zaero/savegame zaero/player zaero/monster/tank zaero/monster/supertank \
	zaero/monster/soldier zaero/monster/sentien zaero/monster/parasite zaero/monster/mutant zaero/monster/misc zaero/monster/medic \
	zaero/monster/insane zaero/monster/infantry zaero/monster/hover zaero/monster/hound zaero/monster/handler zaero/monster/gunner \
	zaero/monster/gladiator zaero/monster/flyer zaero/monster/float zaero/monster/flipper zaero/monster/chick zaero/monster/brain \
	zaero/monster/boss3 zaero/monster/boss2 zaero/monster/boss zaero/monster/berserker zaero/monster/actor
ZAERO := $(foreach dir,$(ZAERO_DIRS), $(wildcard $(dir)/*.c))

CPPSOURCES	:= audiodec

CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
CGFILES  := $(foreach dir,$(SHADERS), $(wildcard $(dir)/*.cg))
CGSHADERS  := $(CGFILES:.cg=.h)
OBJS     := $(CLIENT) $(QCOMMON) $(SERVER) $(SYSTEM) $(REFGL) $(CPPFILES:.cpp=.o) $(BASEQ2:.c=.o)
OBJS_ROGUE := $(CLIENT) $(QCOMMON) $(SERVER) $(SYSTEM) $(REFGL) $(CPPFILES:.cpp=.o) $(ROGUE:.c=.o)
OBJS_XATRIX := $(CLIENT) $(QCOMMON) $(SERVER) $(SYSTEM) $(REFGL) $(CPPFILES:.cpp=.o) $(XATRIX:.c=.o)
OBJS_ZAERO := $(CLIENT) $(QCOMMON) $(SERVER) $(SYSTEM) $(REFGL) $(CPPFILES:.cpp=.o) $(ZAERO:.c=.o)

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -ffast-math -mtune=cortex-a9 -mfpu=neon -fsigned-char -g -Wl,-q -O3 \
		-DREF_HARD_LINKED -DHAVE_OGGVORBIS -DHAVE_MPG123 -Wall -Wno-missing-braces\
		-DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER -DRELEASE -DGAME_HARD_LINKED -DPSP2
CFLAGS += -DOSTYPE=\"$(OSTYPE)\" -DARCH=\"$(ARCH)\"
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

rogue: rogue.bin
rogue: CFLAGS += -DROGUE

xatrix: xatrix.bin
xatrix: CFLAGS += -DXATRIX

zaero: zaero.bin
zaero: CFLAGS += -DZAERO

baseq2: $(TARGET).velf
	vita-make-fself -c -s $< build\eboot.bin
	
rogue.bin: rogue.velf
	vita-make-fself -c -s $< build\rogue.bin
	
xatrix.bin: xatrix.velf
	vita-make-fself -c -s $< build\xatrix.bin
	
zaero.bin: zaero.velf
	vita-make-fself -c -s $< build\zaero.bin

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -c -s $< build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) -d ATTRIBUTE2=12 "$(TARGET)" param.sfo
	cp -f param.sfo build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip $(TARGET).vpk -r .\build\sce_sys\* .\build\eboot.bin  .\build\shaders\* .\build\rogue.bin .\build\xatrix.bin .\build\zaero.bin
	#-------------------------------------------------------------------

%_f.h:
	psp2cgc -profile sce_fp_psp2 $(@:_f.h=_f.cg) -Wperf -O2 -o build/$(@:_f.h=_f.gxp)
	
%_v.h:
	psp2cgc -profile sce_vp_psp2 $(@:_v.h=_v.cg) -Wperf -O2 -o build/$(@:_v.h=_v.gxp)

shaders: $(CGSHADERS)

%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@
	
rogue.elf: $(OBJS_ROGUE)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@
	
xatrix.elf: $(OBJS_XATRIX)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@
	
zaero.elf: $(OBJS_ZAERO)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS) $(OBJS_ROGUE) $(OBJS_XATRIX) $(OBJS_ZAERO)
