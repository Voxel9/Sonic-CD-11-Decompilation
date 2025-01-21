#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
TARGET		:= SonicCD
BUILD		:= build

SRC_DIRS	:=	../RSDKv3 \
				../RSDKv3/platform/3DSSim \
				../dependencies/all/theoraplay \
				../dependencies/all/tinyxml2

CFILES		:=	theoraplay.c

CPPFILES	:=	Animation.cpp \
				Audio.cpp \
				Collision.cpp \
				Debug.cpp \
				Drawing.cpp \
				Ini.cpp \
				Input.cpp \
				main.cpp \
				Math.cpp \
				ModAPI.cpp \
				Object.cpp \
				Palette.cpp \
				Player.cpp \
				Reader.cpp \
				RetroEngine.cpp \
				Scene.cpp \
				Scene3D.cpp \
				Script.cpp \
				Sprite.cpp \
				String.cpp \
				Text.cpp \
				Userdata.cpp \
				Video.cpp \
				\
				Graphics.cpp \
				Timing.cpp \
				\
				tinyxml2.cpp

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS		:= -g -O3 -m32 -static -D_WIN32

CXXFLAGS	:=	$(shell pkg-config --cflags --static vorbisfile vorbis theora glew glfw3) \
				$(CFLAGS) -fno-rtti -fno-exceptions -std=c++17

LIBS		:=	$(shell pkg-config --libs --static vorbisfile vorbis theora glew glfw3) \
				-lmingw32 -lopengl32 -lWinmm

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
VPATH   := $(foreach dir,$(SRC_DIRS),$(CURDIR)/$(dir))

OFILES  :=  $(addprefix $(BUILD)/, $(CFILES:.c=.o)) \
			$(addprefix $(BUILD)/, $(CPPFILES:.cpp=.o))

INCLUDE := $(foreach dir,$(SRC_DIRS),-I$(CURDIR)/$(dir))

.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD) $(TARGET).exe

$(BUILD):
	@mkdir -p $@

#---------------------------------------------------------------------------------
clean:
	@echo clean...
	@rm -fr $(BUILD) $(TARGET).exe

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
