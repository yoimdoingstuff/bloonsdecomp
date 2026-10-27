# Included by Rockbox apps/plugins/plugins.make after the installer registers it.
BLOONSDIR := $(APPSDIR)/plugins/bloons
BLOONSBUILD := $(BUILDDIR)/apps/plugins/bloons
ROCKS += $(BLOONSBUILD)/bloons.rock

# Rockbox's standard rockboxdev.sh builds C only. Supply a matching C++17
# front-end and newlib libm, e.g. a compatible arm-none-eabi-g++ multilib.
BLOONS_CXX ?= $(patsubst %gcc,%g++,$(CC))
BLOONS_CXXFLAGS = $(filter-out -std=% -Wstrict-prototypes -Wmissing-prototypes -Wold-style-definition,$(PLUGINFLAGS)) \
    -std=gnu++17 -DBLOONS_EMBEDDED -DBLOONS_ROCKBOX -fno-exceptions -fno-rtti -fno-threadsafe-statics \
    -fno-use-cxa-atexit -ffunction-sections -fdata-sections -fno-builtin \
    -I$(BLOONSDIR)/src -I$(BLOONSDIR)
BLOONS_CFLAGS = $(PLUGINFLAGS) -I$(BLOONSDIR) -fno-builtin
BLOONS_LIBM ?= $(shell $(BLOONS_CXX) $(filter -mcpu=% -march=% -mthumb -mfloat-abi=%,$(PLUGINFLAGS)) -print-file-name=libm.a)
BLOONSOBJ := $(addprefix $(BLOONSBUILD)/,rockbox.o render.o bridge.o src/game.o src/levels.o)
OTHER_SRC += $(BLOONSDIR)/rockbox.c $(BLOONSDIR)/render.c

$(BLOONSBUILD)/%.o: $(BLOONSDIR)/%.c $(BLOONSDIR)/bloons.make
	$(SILENT)mkdir -p $(dir $@)
	$(CC) $(BLOONS_CFLAGS) -c $< -o $@

$(BLOONSBUILD)/%.o: $(BLOONSDIR)/%.cpp $(wildcard $(BLOONSDIR)/src/*.hpp) $(BLOONSDIR)/bridge.h
	$(SILENT)mkdir -p $(dir $@)
	$(BLOONS_CXX) $(BLOONS_CXXFLAGS) -c $< -o $@

$(BLOONSBUILD)/bloons.rock: $(BLOONSOBJ) $(PLUGIN_LIBS) $(PLUGIN_CRT0)
	$(CC) $(PLUGINFLAGS) -o $(BLOONSBUILD)/bloons.elf $(BLOONSOBJ) \
	    $(PLUGIN_CRT0) $(PLUGIN_LIBS) $(BLOONS_LIBM) -lgcc $(PLUGINLDFLAGS)
	$(call objcopy_plugin,$(BLOONSBUILD)/bloons.elf,$@)
