#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <cmath>

#include <portaudio.h>
#include <fftw3.h>
#include <curses.h>

using namespace std;

#define SAMPLE_RATE 44100.0
#define FRAMES_PER_BUFFER 1024

#define SPECTRO_FREQ_START 20
#define SPECTRO_FREQ_END 20000

#define WIN_WIDTH 60
#define MARGIN 2

#define VOL_WIN_HEIGHT 2
#define VOL_INIT_X 0
#define VOL_INIT_Y 0

#define FREQ_WIN_HEIGHT 20
#define FREQ_INIT_X 0
#define FREQ_INIT_Y (VOL_INIT_Y + VOL_WIN_HEIGHT + MARGIN)

WINDOW* VOL_WIN;
WINDOW* FREQ_WIN;
std::unordered_map<int, double> current_max;

void init_vol_win() {
  VOL_WIN = newwin(VOL_WIN_HEIGHT, WIN_WIDTH, 0, 0);
  waddstr(VOL_WIN, "Volume:\n");
}

void init_freq_win() {
  FREQ_WIN = newwin(FREQ_WIN_HEIGHT, WIN_WIDTH, VOL_WIN_HEIGHT + MARGIN, 0);
  waddstr(FREQ_WIN, "Frequencies:\n");
}

void init_current_max() {
  for (int freq = 0; freq < WIN_WIDTH; freq++) {
    current_max[freq] = 0.0;
  }
}

void init_screen() {
  init_current_max();
  initscr();
  init_vol_win();
  init_freq_win();
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

  float volL = 0;
  float volR = 0;

  for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2) {
    volL = max(volL, abs(in[i]));
    volR = max(volR, abs(in[i+1]));
  }

  int initial_x;
  int initial_y;
  getyx(VOL_WIN, initial_y, initial_x);

  for (int i = 0; i < WIN_WIDTH; i++) {
    float barProportion = i/(float)WIN_WIDTH;
    if (barProportion <= volL && barProportion <= volR) {
      waddch(VOL_WIN, '=');
    } else if (barProportion <= volL) {
      waddch(VOL_WIN, '-');
    } else if (barProportion <= volR) {
      waddch(VOL_WIN, '_');
    } else {
      waddch(VOL_WIN, ' ');
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
        * callbackData->spectroSize)];

    if (abs(proportion) > current_max[i]) {
      current_max[i] = min(abs(proportion), 1.0);
    }

    for (int j = 1; j < FREQ_WIN_HEIGHT; j++) {
      double desired_level = (double)((FREQ_WIN_HEIGHT) - j) / (double)FREQ_WIN_HEIGHT;
      wmove(FREQ_WIN, j, i + 1);

      if (proportion >= desired_level) {
        waddch(FREQ_WIN, '*');
      } else {
        waddch(FREQ_WIN, ' ');
      }
    }
  }

  display_current_max();

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
  init_screen();

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