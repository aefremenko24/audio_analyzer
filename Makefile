EXEC = audio_analyzer

$(EXEC): main.cpp
	g++ -o $@ $^

clean:
	rm -f $(EXEC)