EXEC = audio_analyzer

CLIB = -I./libs/portaudio/include ./libs/portaudio/lib/.libs/libportaudio.a \
-I./libs/fftw-3.3.10/api -lfftw3 -lncurses -Wno-c++11-extensions

PLATFORM := $(shell uname -s)

ifeq ($(PLATFORM), Linux)
	CXX = g++
	ARGS := $(shell sudo apt install libncurses-dev)

else ifeq ($(PLATFORM),Darwin)
	CXX = clang++
	ARGS = -framework CoreAudio -framework AudioUnit -framework AudioToolbox \
                    -framework Carbon

else
    $(error Unsupported platform: $(PLATFORM))
endif

$(EXEC): main.cpp
	$(CXX) $(ARGS) $(CLIB) -o $@ $^

all: install-deps $(EXEC)

install-deps: install-portaudio install-fftw
.PHONY: install-deps

uninstall-deps: uninstall-portaudio uninstall-fftw
.PHONY: uninstall-deps

install-portaudio:
	mkdir -p libs
	curl https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz | tar -zx -C libs
	CFLAGS="-Wno-unused-variable"
	cd libs/portaudio && ./configure --disable-mac-universal && $(MAKE) -j
.PHONY: install-portaudio

uninstall-portaudio:
	cd libs/portaudio && $(MAKE) uninstall
	rm -rf libs/portaudio
.PHONY: uninstall-portaudio

install-fftw:
	mkdir -p libs
	curl https://www.fftw.org/fftw-3.3.10.tar.gz | tar -zx -C libs
	cd libs/fftw-3.3.10 && ./configure && $(MAKE) -j && sudo $(MAKE) install
.PHONY: install-fftw

uninstall-fftw:
	cd libs/portaudio && $(MAKE) uninstall
	rm -rf libs/portaudio
.PHONY: uninstall-fftw

clean:
	rm -f $(EXEC)
.PHONY: clean