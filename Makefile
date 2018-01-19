TARGET		:= vitaQuakeII
TITLE		:= QUAKE0002
SOURCES		:= source
INCLUDES	:= include

LIBS = -lvitaGL -lvorbisfile -lvorbis -logg -lspeexdsp -lmpg123 \
	-lSceLibKernel_stub -lSceAppMgr_stub -lSceSysmodule_stub \
	-lSceCtrl_stub -lSceTouch_stub -lm -lSceNet_stub -lSceNetCtl_stub \
	-lSceAppUtil_stub -lc -lScePower_stub -lSceCommonDialog_stub \
	-lSceAudio_stub -lSceGxm_stub -lSceDisplay_stub -lSceNet_stub -lSceNetCtl_stub

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

GAME = 		game/m_tank.o \
			game/p_client.o \
			game/p_hud.o \
			game/p_trail.o \
			game/p_view.o \
			game/p_weapon.o \
			game/q_shared.o \
			game/g_ai.o \
			game/g_chase.o \
			game/g_cmds.o \
			game/g_combat.o \
			game/g_func.o \
			game/g_items.o \
			game/g_main.o \
			game/g_misc.o \
			game/g_monster.o \
			game/g_phys.o \
			game/g_save.o \
			game/g_spawn.o \
			game/g_svcmds.o \
			game/g_target.o \
			game/g_trigger.o \
			game/g_turret.o \
			game/g_utils.o \
			game/g_weapon.o \
			game/m_actor.o \
			game/m_berserk.o \
			game/m_boss2.o \
			game/m_boss3.o \
			game/m_boss31.o \
			game/m_boss32.o \
			game/m_brain.o \
			game/m_chick.o \
			game/m_flash.o \
			game/m_flipper.o \
			game/m_float.o \
			game/m_flyer.o \
			game/m_gladiator.o \
			game/m_gunner.o \
			game/m_hover.o \
			game/m_infantry.o \
			game/m_insane.o \
			game/m_medic.o \
			game/m_move.o \
			game/m_mutant.o \
			game/m_parasite.o \
			game/m_soldier.o \
			game/m_supertank.o


CPPSOURCES	:= audiodec

CFILES		:=	$(CLIENT) $(QCOMMON) $(SERVER) $(GAME) $(SYSTEM) $(REFGL)
CPPFILES   := $(foreach dir,$(CPPSOURCES), $(wildcard $(dir)/*.cpp))
BINFILES := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))
OBJS     := $(addsuffix .o,$(BINFILES)) $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) 

PREFIX  = arm-vita-eabi
CC      = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS  = -fno-lto -w -g -Wl,-q -O2 -DREF_HARD_LINKED -DGAME_HARD_LINKED -DPSP2 \
		-DHAVE_OGGVORBIS -DHAVE_MPG123 -DHAVE_LIBSPEEXDSP -DUSE_AUDIO_RESAMPLER \
		-DRELEASE

CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=gnu++11 -fpermissive
ASFLAGS = $(CFLAGS)

all: $(TARGET).vpk

$(TARGET).vpk: $(TARGET).velf
	vita-make-fself -s $< build\eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLE) "$(TARGET)" param.sfo
	cp -f param.sfo build/sce_sys/param.sfo
	
	#------------ Comment this if you don't have 7zip ------------------
	7z a -tzip $(TARGET).vpk -r .\build\sce_sys\* .\build\eboot.bin 
	#-------------------------------------------------------------------

%.velf: %.elf
	cp $< $<.unstripped.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@
	vita-make-fself -s $@ eboot.bin

$(TARGET).elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)