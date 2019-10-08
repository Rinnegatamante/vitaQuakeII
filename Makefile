STATIC_LINKING := 0
AR             := ar

ifneq ($(V),1)
   Q := @
endif

ifneq ($(SANITIZER),)
   CFLAGS   := -fsanitize=$(SANITIZER) $(CFLAGS)
   CXXFLAGS := -fsanitize=$(SANITIZER) $(CXXFLAGS)
   LDFLAGS  := -fsanitize=$(SANITIZER) $(LDFLAGS)
endif

ifeq ($(platform),)
platform = unix
ifeq ($(shell uname -a),)
   platform = win
else ifneq ($(findstring MINGW,$(shell uname -a)),)
   platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
   platform = osx
else ifneq ($(findstring win,$(shell uname -a)),)
   platform = win
endif
endif

# system platform
system_platform = unix
ifeq ($(shell uname -a),)
	EXE_EXT = .exe
	system_platform = win
else ifneq ($(findstring Darwin,$(shell uname -a)),)
	system_platform = osx
	arch = intel
ifeq ($(shell uname -p),powerpc)
	arch = ppc
endif
else ifneq ($(findstring MINGW,$(shell uname -a)),)
	system_platform = win
endif

CORE_DIR    += .
TARGET_NAME := vitaQuakeII
LIBM		    = -lm

ifeq ($(ARCHFLAGS),)
ifeq ($(archs),ppc)
   ARCHFLAGS = -arch ppc -arch ppc64
else
   ARCHFLAGS = -arch i386 -arch x86_64
endif
endif

ifeq ($(platform), osx)
ifndef ($(NOUNIVERSAL))
   CXXFLAGS += $(ARCHFLAGS)
   LFLAGS += $(ARCHFLAGS)
endif
endif

ifeq ($(STATIC_LINKING), 1)
EXT := a
endif

ifeq ($(platform), unix)
	EXT ?= so
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), linux-portable)
   TARGET := $(TARGET_NAME)_libretro.$(EXT)
   fpic := -fPIC -nostdlib
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T
	LIBM :=
else ifneq (,$(findstring osx,$(platform)))
   TARGET := $(TARGET_NAME)_libretro.dylib
   fpic := -fPIC
   SHARED := -dynamiclib
else ifneq (,$(findstring ios,$(platform)))
   TARGET := $(TARGET_NAME)_libretro_ios.dylib
	fpic := -fPIC
	SHARED := -dynamiclib

ifeq ($(IOSSDK),)
   IOSSDK := $(shell xcodebuild -version -sdk iphoneos Path)
endif

	DEFINES := -DIOS
	CC = cc -arch armv7 -isysroot $(IOSSDK)
ifeq ($(platform),ios9)
CC     += -miphoneos-version-min=8.0
CXXFLAGS += -miphoneos-version-min=8.0
else
CC     += -miphoneos-version-min=5.0
CXXFLAGS += -miphoneos-version-min=5.0
endif
else ifneq (,$(findstring qnx,$(platform)))
	TARGET := $(TARGET_NAME)_libretro_qnx.so
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), emscripten)
   TARGET := $(TARGET_NAME)_libretro_emscripten.bc
   fpic := -fPIC
   SHARED := -shared -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
else ifeq ($(platform), vita)
   TARGET := $(TARGET_NAME)_vita.a
   CC = arm-vita-eabi-gcc
   AR = arm-vita-eabi-ar
   CXXFLAGS += -Wl,-q -Wall -O3
	STATIC_LINKING = 1
else
   CC = gcc
   TARGET := $(TARGET_NAME)_libretro.dll
   SHARED := -shared -static-libgcc -static-libstdc++ -s -Wl,--version-script=$(CORE_DIR)/link.T -Wl,--no-undefined
endif

LDFLAGS += $(LIBM)

ifeq ($(DEBUG), 1)
   CFLAGS += -O0 -g -DDEBUG
   CXXFLAGS += -O0 -g -DDEBUG
else
   CFLAGS += -O3
   CXXFLAGS += -O3
endif

include Makefile.common

SYSTEM = 	libretro/libretro.o \
			libretro-common/file/retro_dirent.o \
			libretro-common/encodings/encoding_utf.o \
			libretro-common/string/stdstring.o \
			libretro-common/file/file_path.o \
			libretro-common/compat/fopen_utf8.o \
			libretro-common/compat/compat_strl.o \
			libretro-common/compat/compat_posix_string.o \
			libretro-common/compat/compat_strcasestr.o \
			libretro-common/compat/compat_snprintf.o \
			libretro-common/features/features_cpu.o \
			libretro-common/streams/file_stream.o \
			libretro-common/vfs/vfs_implementation.o

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

SOURCES	:= jpeg-8c
CFILES := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))

OBJECTS := $(CLIENT) $(QCOMMON) $(SERVER) $(GAME) $(SYSTEM) $(REFGL) $(CFILES:.c=.o)

CFLAGS   += -Wall -D__LIBRETRO__ $(fpic) -DREF_HARD_LINKED -DRELEASE -DGAME_HARD_LINKED -DOSTYPE=\"$(OSTYPE)\" -DARCH=\"$(ARCH)\" -I$(CORE_DIR)/libretro-common/include -std=c99
CXXFLAGS += -Wall -D__LIBRETRO__ $(fpic) -fpermissive

all: $(TARGET)

$(TARGET): $(OBJECTS)
ifeq ($(STATIC_LINKING), 1)
	$(AR) rcs $@ $(OBJECTS)
else
	@$(if $(Q), $(shell echo echo LD $@),)
	$(Q)$(CC) $(fpic) $(SHARED) $(INCLUDES) -o $@ $(OBJECTS) $(LDFLAGS)
endif

%.o: %.c
	@$(if $(Q), $(shell echo echo CC $<),)
	$(Q)$(CC) $(CFLAGS) $(fpic) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: clean

print-%:
	@echo '$*=$($*)'
