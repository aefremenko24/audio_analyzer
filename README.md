# audio_analyzer

Cross-platform digital sound analyzer application written in C++ using the PulseAudio sound server.
Sound input is collected from a chosen input and metrics, including volume, frequencies, and live waveform are displayed in the console.
Currently supports Linux (using ALSA) and MacOS (using CoreAudio).

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

`NCURSES` is used to display the textual user interface (TUI) of this program. The library is preinstalled on most machines. However, if you are having trouble importing the library, make sure that it is installed on your machine by running the appropriate command in your terminal:

- macOS: `brew install ncurses`
- Linux: `sudo apt install libncurses-dev`

### Making the project

To run audio_analyzer for the first time, run `make install-deps` to install the dependencies required for this project. Some of the libraries require `sudo` for their installation, so you will likely be prompted to enter your password.

Once the dependencies are installed, run `make` to compile the project.

### Running audio_analyzer

Once the project is compiled and all prerequisites are completed, run the `audio_analyzer` file populated in the repo-level directory and follow the prompts in the terminal.  

## Built With

* [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) - Sound Server used to capture sound signals
* [NCURSES](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/) - Library used to display the TUI
* [FFTW](https://www.fftw.org/) - Software used to analyze the frequencies of the signal.

## Authors

* **Arthur Efremenko** - *Initial work* - [GitHub](https://github.com/aefremenko24)