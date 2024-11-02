EXEC = audio_analyzer

CLIB = -I./libs/portaudio/include /libs/portaudio/lib/.libs/libportaudio.a \
 -lrt -lasound -ljack -pthread -I./lib/fftw-3.3.10/api -lfftw3

$(EXEC): main.cpp
	g++ -o $@ $^

install-deps: install-portaudio install-fftw
.PHONY: install-deps

uninstall-deps: uninstall-portaudio uninstall-fftw
.PHONY: uninstall-deps

install-portaudio:
	mkdir -p libs
	curl https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz | tar -zx -C libs
	cd libs/portaudio && ./configure && $(MAKE) -j
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