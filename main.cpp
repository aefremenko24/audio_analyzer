#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <cmath>

#include <portaudio.h>
#include <fftw3.h>
#include <curses.h>

using namespace std;

/*
NOTE: Coordinates used in this project assume (0, 0) is the top left corner of the terminal.
 Increasing y moves the cursor down; increasing x moves the cursor to the right.
 */

// Rate at which the audio is sampled, in Hz
#define SAMPLE_RATE 44100.0

// Number of data points collected in a single buffer
#define FRAMES_PER_BUFFER 1024

// Frequency display range minimum and maximum (normal human hearing range here)
#define SPECTRO_FREQ_START 20
#define SPECTRO_FREQ_END 20000

// Width of the console display window in number of characters
#define WIN_WIDTH 100

// Number of lines to be skipped between different view windows
#define MARGIN 2

// x- and y-coordinates of the top left corner of the volume view window
#define VOL_INIT_X 0
#define VOL_INIT_Y 0

// The height of the frequency view window in number of lines
#define FREQ_WIN_HEIGHT 20

// y-coordinate of the top left corner of the frequency view window
#define FREQ_INIT_X 0

// Data structures representing the volume and frequency view windows
WINDOW* VOL_WIN;
WINDOW* FREQ_WIN;

// Map representing the recent maximum measurement of the amplitude at each frequency.
// Each value in the map will be decremented over time until new max is set.
std::unordered_map<int, double> current_max;

// Number of input channels for the input source the program is working with
int num_channels;

void init_vol_win(int num_channels = 1) {
  VOL_WIN = newwin(num_channels + 1, WIN_WIDTH, 0, 0);
  waddstr(VOL_WIN, "Volume:\n");
}

void init_freq_win(int num_channels = 1) {
  FREQ_WIN = newwin(FREQ_WIN_HEIGHT, WIN_WIDTH, num_channels + 1 + MARGIN, 0);
  waddstr(FREQ_WIN, "Frequencies:\n");
}

void init_current_max() {
  for (int freq = 0; freq < WIN_WIDTH; freq++) {
    current_max[freq] = 0.0;
  }
}

void decrement_current_max() {
  double decrement = 0.0003 * (double)FREQ_WIN_HEIGHT;
  for (int freq = 0; freq < WIN_WIDTH; freq++) {
    if (current_max[freq] * (double)FREQ_WIN_HEIGHT > decrement) {
      current_max[freq] -= decrement;
    }
  }
}

void init_screen(int num_channels = 1) {
  init_current_max();
  initscr();
  init_vol_win(num_channels);
  init_freq_win(num_channels);
}

void refresh_screen() {
  wrefresh(VOL_WIN);
  wrefresh(FREQ_WIN);
}

void del_screen() {
  delwin(VOL_WIN);
  delwin(FREQ_WIN);
}

typedef struct {
  double* in;
  double* out;
  fftw_plan p;
  int startIndex;
  int spectroSize;
} streamCallbackData;

static streamCallbackData* spectroData;

static void checkErr(PaError err) {
  if (err != paNoError) {
    std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    exit(EXIT_FAILURE);
  }
}

void streamCallBackVolume(
    const void* inputBuffer, unsigned long framesPerBuffer
    ) {
  float* in = (float*)inputBuffer;

  const int NUM_CHANNELS = num_channels;
  float channelVolumes[NUM_CHANNELS];

  for (unsigned long channelNum = 0; channelNum < NUM_CHANNELS; channelNum++) {
    channelVolumes[channelNum] = 0.0;
  }

  for (unsigned long i = 0; i < framesPerBuffer * NUM_CHANNELS; i += NUM_CHANNELS) {
    for (unsigned long channelNum = 0; channelNum < NUM_CHANNELS; channelNum++) {
      channelVolumes[channelNum] = max(channelVolumes[channelNum], abs(in[i + channelNum]));
    }
  }

  int initial_x;
  int initial_y;
  getyx(VOL_WIN, initial_y, initial_x);

  for (unsigned long channelNum = 0; channelNum < NUM_CHANNELS; channelNum++) {
    wmove(VOL_WIN, VOL_INIT_Y + channelNum + 1, VOL_INIT_X);
    for (int i = 0; i < WIN_WIDTH; i++) {
      float barProportion = i/((float)WIN_WIDTH);
      if (barProportion <= channelVolumes[channelNum]) {
        waddch(VOL_WIN, '=');
      } else {
        waddch(VOL_WIN, ' ');
      }
    }
  }

  wmove(VOL_WIN, initial_y, initial_x);
}

void display_current_max() {
  int initial_x;
  int initial_y;
  getyx(FREQ_WIN, initial_y, initial_x);

  for (const std::pair<int, double>& n : current_max) {
    int y_pos = 1 + FREQ_WIN_HEIGHT - (int)((double)FREQ_WIN_HEIGHT * n.second);
    int x_pos = n.first;

    wmove(FREQ_WIN, y_pos, x_pos);
    waddch(FREQ_WIN, '_');
  }

  wmove(FREQ_WIN, initial_y, initial_x);
}

void streamCallBackFrequencies(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    void* userData
) {
  float* in = (float*)inputBuffer;
  (void)outputBuffer;
  streamCallbackData* callbackData = (streamCallbackData*)userData;

  for (unsigned long i = 0; i < framesPerBuffer; i++) {
    callbackData->in[i] = in[i * 2];
  }

  int initial_x;
  int initial_y;
  getyx(FREQ_WIN, initial_y, initial_x);

  fftw_execute(callbackData->p);

  for (int i = 0; i < WIN_WIDTH; i++) {
    double freq = pow(i / ((double)WIN_WIDTH), 2);
    double proportion = callbackData->out[(int)(callbackData->startIndex + freq
        * callbackData->spectroSize)]/5;

    if (abs(proportion) > current_max[i]) {
      current_max[i] = min(abs(proportion), 1.0);
    }

    for (int j = 1; j < FREQ_WIN_HEIGHT; j++) {
      double desired_level = (double)((FREQ_WIN_HEIGHT) - j) / (double)FREQ_WIN_HEIGHT;
      wmove(FREQ_WIN, j, i + 1);

      if (proportion >= desired_level) {
        waddch(FREQ_WIN, 'o');
      } else {
        waddch(FREQ_WIN, ' ');
      }
    }
  }

  display_current_max();
  decrement_current_max();

  wmove(FREQ_WIN, initial_y, initial_x);
}

static int streamCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {

  streamCallBackVolume(inputBuffer, framesPerBuffer);

  streamCallBackFrequencies(inputBuffer, outputBuffer, framesPerBuffer, userData);

  refresh_screen();

  return 0;
}

int prompt_device() {
  int numDevices = Pa_GetDeviceCount();
  if (numDevices < 0) {
    cout << "Encountered error while getting the number of devices." << endl;
    exit(EXIT_FAILURE);
  } else if (numDevices == 0){
    cout << "No devices found on this machine." << endl;
    exit(EXIT_SUCCESS);
  }
  cout << "Number of devices:" << numDevices << endl;

  const PaDeviceInfo* deviceInfo;

  for (int device_index = 0; device_index < numDevices; device_index++) {
    deviceInfo = Pa_GetDeviceInfo(device_index);
    cout << "Device #" << device_index << endl;
    cout << "\tName: " << deviceInfo->name << endl;
    cout << "\tMax Input Channels: " << deviceInfo->maxInputChannels << endl;
    cout << "\tMax Output Channels: " << deviceInfo->maxOutputChannels << endl;
    cout << "\tDefault Sample Rate: " << deviceInfo->defaultSampleRate << endl;
  }

  int deviceSelection;
  cout << "\nWhich device would you like to use? Enter a number from 0 to " << numDevices - 1 << " below:" << endl;
  cin >> deviceSelection;

  while (cin.fail() || deviceSelection < 0 || deviceSelection > numDevices - 1) {
    cout << "Device index entered is invalid. Try again:" << endl;
    cin >> deviceSelection;
  }

  return deviceSelection;
}

streamCallbackData* init_spectro_data() {
  spectroData = (streamCallbackData*)malloc(sizeof(streamCallbackData));
  spectroData->in = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
  spectroData->out = (double*)malloc(sizeof(double) * FRAMES_PER_BUFFER);
  if (spectroData->in == nullptr || spectroData->out == nullptr) {
    cout << "Could not allocate spectro data." << endl;
    exit(EXIT_FAILURE);
  }
  spectroData->p = fftw_plan_r2r_1d(FRAMES_PER_BUFFER, spectroData->in, spectroData->out,
                                    FFTW_R2HC, FFTW_ESTIMATE);

  double sampleRatio = FRAMES_PER_BUFFER / SAMPLE_RATE;
  spectroData->startIndex = std::ceil(sampleRatio * SPECTRO_FREQ_START);
  spectroData->spectroSize = min(std::ceil(sampleRatio * SPECTRO_FREQ_END),
                                 FRAMES_PER_BUFFER / 2.0)
                             - spectroData->startIndex;

  return spectroData;
}

void close_stream(PaStream* stream, streamCallbackData* spectroData, PaError err) {
  err = Pa_CloseStream(stream);
  checkErr(err);

  err = Pa_Terminate();
  checkErr(err);

  fftw_destroy_plan(spectroData->p);
  fftw_free(spectroData->in);
  fftw_free(spectroData->out);
  fftw_free(spectroData);

  del_screen();
}

void init_stream(PaError* err) {
  *err = Pa_Initialize();
  checkErr(*err);
}

void process_stream(int deviceSelection, streamCallbackData* spectroData, PaError err) {

  PaStreamParameters inputParameters;
  PaStreamParameters outputParameters;

  memset(&inputParameters, 0, sizeof(inputParameters));
  inputParameters.channelCount = Pa_GetDeviceInfo(deviceSelection)->maxInputChannels;
  inputParameters.device = deviceSelection;
  inputParameters.hostApiSpecificStreamInfo = nullptr;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceSelection)->defaultLowInputLatency;

  memset(&outputParameters, 0, sizeof(outputParameters));
  outputParameters.channelCount = Pa_GetDeviceInfo(deviceSelection)->maxOutputChannels;
  outputParameters.device = deviceSelection;
  outputParameters.hostApiSpecificStreamInfo = nullptr;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceSelection)->defaultLowInputLatency;

  num_channels = inputParameters.channelCount;
  init_screen(num_channels);

  PaStream* stream;
  err = Pa_OpenStream(
      &stream,
      &inputParameters,
      nullptr,
      SAMPLE_RATE,
      FRAMES_PER_BUFFER,
      paNoFlag,
      streamCallBack,
      spectroData
  );
  checkErr(err);

  err = Pa_StartStream(stream);
  checkErr(err);

  char input;
  while(input != ' ' && input != 'r') {
    input = tolower(getch());
    if (input == 'r') {
      close_stream(stream, spectroData, err);
      init_stream(&err);
      spectroData = init_spectro_data();
      return process_stream(deviceSelection, spectroData, err);
    }
  }

  close_stream(stream, spectroData, err);
}

int main() {
  PaError err;

  init_stream(&err);
  streamCallbackData* spectroData = init_spectro_data();
  int deviceSelection = prompt_device();
  process_stream(deviceSelection, spectroData, err);

  return EXIT_SUCCESS;
}