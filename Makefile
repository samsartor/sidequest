########################################
## SETUP MAKEFILE
##      Set the appropriate TARGET (our
## executable name) and any OBJECT files
## we need to compile for this program.
##
## Next set the path to our local
## include/, lib/, and bin/ folders.
## (If you we are compiling in the lab,
## then you can ignore these values.
## They are only for if you are
## compiling on your personal machine.)
##
## Set if we are compiling in the lab
## environment or not.  Set to:
##    1 - if compiling in the Lab
##    0 - if compiling at home
##
## Finally, set the flags for which
## libraries are using and want to
## compile against.
########################################

TARGET = sq-tracer
INCLUDE = gl.h point.h freeCamera.h helper.h intersectionData.h shape.h plane.h ray.h sphere.h material.h rt.h
OBJECTS = main.o point.o freeCamera.o helper.o intersectionData.o plane.o ray.o sphere.o material.o rt.o

SDIR = src
ODIR = obj

DEPS = $(INCLUDES)

LOCAL_INC_PATH = /Users/jpaone/Desktop/include
LOCAL_LIB_PATH = /Users/jpaone/Desktop/lib
LOCAL_BIN_PATH = /Users/jpaone/Desktop/bin

BUILDING_IN_LAB = 1

USING_GLEW = 0
USING_GLUI = 0
USING_OPENAL = 0
USING_OPENGL = 1
USING_SOIL = 0

#########################################################################################
#########################################################################################
#########################################################################################
##
## !!!STOP!!!
## THERE IS NO NEED TO MODIFY ANYTHING BELOW THIS LINE
## IT WILL WORK FOR YOU





#############################
## COMPILING INFO
#############################

CC    = clang
CXX    = clang++
CFLAGS = -Wall -g -std=c++11

INCPATH += -I./include

LAB_INC_PATH = Z:/CSCI441GFx/include
LAB_LIB_PATH = Z:/CSCI441GFx/lib
LAB_BIN_PATH = Z:/CSCI441GFx/bin

# if we are not building in the Lab
ifeq ($(BUILDING_IN_LAB), 0)
    # and we are running Windows
    ifeq ($(OS), Windows_NT)
        # then set our lab paths to our local paths
        # so the Makefile will still work seamlessly
        LAB_INC_PATH = $(LOCAL_INC_PATH)
        LAB_LIB_PATH = $(LOCAL_LIB_PATH)
        LAB_BIN_PATH = $(LOCAL_BIN_PATH)
    endif
endif

#############################
## SETUP GLUI
##      We must link against
##  GLUI before we link against
##  GLUT.
##
##     If you want to build
##  on your own machine, you
##  need to change the
##  appropriate paths.
#############################

# if we are using GLUI in this program
ifeq ($(USING_GLUI), 1)
    # Windows Lab builds
    ifeq ($(OS), Windows_NT)
        INCPATH += -I$(LAB_INC_PATH)
        LIBPATH += -L$(LAB_LIB_PATH)

    # Non-Windows build
    else
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
    endif

    LIBS += -lglui
endif

#############################
## SETUP SOIL 
#############################

# if we are using SOIL in this program
ifeq ($(USING_SOIL), 1)
    # Windows builds
    ifeq ($(OS), Windows_NT)
        INCPATH += -I$(LAB_INC_PATH)
        LIBPATH += -L$(LAB_LIB_PATH)
    # Mac builds
    else ifeq ($(shell uname), Darwin)
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
	LIBS += -framework CoreFoundation
    # Linux
    else
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
    endif
	
    LIBS += -lSOIL
endif

#############################
## SETUP OpenGL & GLUT 
#############################

# if we are using OpenGL & GLUT in this program
ifeq ($(USING_OPENGL), 1)
    # Windows builds
    ifeq ($(OS), Windows_NT)
        INCPATH += -IC:/Strawberry/c/include/
        LIBPATH += -LC:/Strawberry/c/lib/
        LIBS += -lglut -lopengl32 -lglu32

    # Mac builds
    else ifeq ($(shell uname), Darwin)
        LIBS += -framework GLUT -framework OpenGL

    # Linux and all other builds
    else
        LIBS += -lglut -lGL -lGLU
    endif
endif

#############################
## SETUP GLEW 
#############################

# if we are using GLEW in this program
ifeq ($(USING_GLEW), 1)
    # Windows builds
    ifeq ($(OS), Windows_NT)
        INCPATH += -I$(LAB_INC_PATH)
        LIBPATH += -L$(LAB_LIB_PATH)
		LIBS += -lGLEW.dll
		WINDOWS_GLEW = 1

    # Non-Windows builds
	else
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
		LIBS += -lglew
		WINDOWS_GLEW = 0
    endif
else
	WINDOWS_GLEW = 0
endif

#############################
## SETUP OpenAL & ALUT
#############################

# if we are using OpenAL & GLUT in this program
ifeq ($(USING_OPENAL), 1)
    # Windows builds
    ifeq ($(OS), Windows_NT)
        INCPATH += -I$(LAB_INC_PATH)
        LIBPATH += -L$(LAB_LIB_PATH)
        LIBS += -lalut.dll -lOpenAL32.dll
		WINDOWS_AL = 1

    # Mac builds
    else ifeq ($(shell uname), Darwin)
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
        LIBS += -framework OpenAL
		WINDOWS_AL = 0

    # Linux and all other builds
    else
        INCPATH += -I$(LOCAL_INC_PATH)
        LIBPATH += -L$(LOCAL_LIB_PATH)
        LIBS += -lalut -lopenal
		WINDOWS_AL = 0
    endif
else
	WINDOWS_AL = 0
endif

#############################
## COMPILATION INSTRUCTIONS 
#############################

all: $(TARGET)

clean:
	rm -f $(addprefix $(ODIR)/,$(OBJECTS)) $(TARGET)

depend:
	rm -f Makefile.bak
	mv Makefile Makefile.bak
	sed '/^# DEPENDENCIES/,$$d' Makefile.bak > Makefile
	echo '# DEPENDENCIES' >> Makefile
	$(CXX) $(INCPATH) -MM *.cpp >> Makefile

$(ODIR)/%.o: $(SDIR)/%.cpp $(DEPS)
	$(CXX) $(INCPATH) -c -o $@ $< $(CFLAGS)

$(TARGET): $(addprefix $(ODIR)/,$(OBJECTS))
	$(CXX) $(CFLAGS) $(INCPATH) -o $@ $^ $(LIBPATH) $(LIBS)
	if [ $(USING_OPENAL) -eq 1 ]; \
	then \
		if [ $(WINDOWS_AL) -eq 1 ]; \
		then \
			cp $(LAB_BIN_PATH)/OpenAL32.dll .; \
			cp $(LAB_BIN_PATH)/libalut.dll .; \
		fi \
	fi    
	if [ $(USING_GLEW) -eq 1 ]; \
	then \
		if [ $(WINDOWS_GLEW) -eq 1 ]; \
		then \
			cp $(LAB_BIN_PATH)/cygGLEW-1-13.dll .; \
		fi \
	fi  

# DEPENDENCIES
main.o: main.cpp
