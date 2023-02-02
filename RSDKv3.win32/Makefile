#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET      := SonicCD
BUILD       := build
SOURCES     :=  ../RSDKv3 \
				../dependencies/all/theoraplay

INCLUDES    :=	../RSDKv3 \
				../dependencies/all/theoraplay

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS  := -O3 -m32 -static -DGLEW_STATIC -D__WIN32__

CXXFLAGS := $(CFLAGS) -fno-rtti -std=c++17

LIBS    :=  -ltinyxml2 -lvorbisfile -lvorbis -logg -ltheora -lmingw32 -lSDL2 -lSDL2main -lglfw3 -lglew32 -lopengl32 \
			-lgdi32 -lwinmm -limm32 -lsetupapi -lversion -lole32 -loleaut32

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))

CFILES	  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES  := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

OFILES  :=  $(addprefix $(BUILD)/, $(CFILES:.c=.o)) \
			$(addprefix $(BUILD)/, $(CPPFILES:.cpp=.o))

INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir))

.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD) $(TARGET).exe

$(BUILD):
	@mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean...
	@rm -fr $(BUILD)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(TARGET).exe: $(OFILES)
	@echo linking $(notdir $@)
	@ $(CXX) $(CXXFLAGS) -o $@ $(OFILES) $(LIBS) -Xlinker -Map=$(BUILD)/$(notdir $@.map)

$(BUILD)/%.o: %.c
	@echo $(notdir $<)
	@ $(CC) $(CFLAGS) -c $< -o $@ $(INCLUDE)

$(BUILD)/%.o: %.cpp
	@echo $(notdir $<)
	@ $(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDE)
