EXEC = audio_analyzer

CLIB = -I./libs/portaudio/include /libs/portaudio/lib/.libs/libportaudio.a -lrt -lasound -ljack -pthread

$(EXEC): main.cpp
	g++ -o $@ $^

install-deps:
	mkdir -p libs
	curl https://files.portaudio.com/archives/pa_stable_v190700_20210406.tgz | tar -zx -C libs
	cd libs/portaudio && ./configure && $(MAKE) -j

.PHONY: install-deps

uninstall-deps:
	cd libs/portaudio && $(MAKE) uninstall
	rm -rf libs/portaudio
.PHONY: uninstall-deps

clean:
	rm -f $(EXEC)
.PHONY: clean