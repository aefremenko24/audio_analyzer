#include <stdlib.h>
#include <iostream>
#include <cstring>
#include <math.h>

#include <portaudio.h>
#include <fftw3.h>
#include <curses.h>

using namespace std;

#define DISP_SIZE 60
#define SAMPLE_RATE 44100.0
#define FRAMES_PER_BUFFER 512

#define SPECTRO_FREQ_START 20
#define SPECTRO_FREQ_END 20000

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

static int streamCallBackVolume(
    const void* inputBuffer, unsigned long framesPerBuffer
    ) {
  float* in = (float*)inputBuffer;

  float volL = 0;
  float volR = 0;

  for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2) {
    volL = max(volL, abs(in[i]));
    volR = max(volR, abs(in[i+1]));
  }

  for (int i = 0; i < DISP_SIZE; i++) {
    float barProportion = i/(float)DISP_SIZE;
    if (barProportion <= volL && barProportion <= volR) {
      waddch(stdscr, '=');
    } else if (barProportion <= volL) {
      waddch(stdscr, '-');
    } else if (barProportion <= volR) {
      waddch(stdscr, '_');
    } else {
      waddch(stdscr, ' ');
    }
  }

  return 0;
}

static int streamCallBackFrequencies(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    void* userData
) {
  float* in = (float*)inputBuffer;
  (void)outputBuffer;
  streamCallbackData* callbackData = (streamCallbackData*)userData;

  cout << "\r" << flush;

  for (unsigned long i = 0; i < framesPerBuffer; i++) {
    callbackData->in[i] = in[i * 2];
  }

  fftw_execute(callbackData->p);

  for (int i = 0; i < DISP_SIZE; i++) {
    double proportion = i / ((double)DISP_SIZE*3);
    double freq = callbackData->out[(int)(callbackData->startIndex + proportion
        * callbackData->spectroSize)];

    if (freq < 0.125) {
      waddch(stdscr, ',');
    } else if (freq < 0.25) {
      waddch(stdscr, '.');
    } else if (freq < 0.375) {
      waddch(stdscr, ';');
    } else if (freq < 0.5) {
      waddch(stdscr, ':');
    } else if (freq < 0.625) {
      waddch(stdscr, '\'');
    } else if (freq < 0.75) {
      waddch(stdscr, '\"');
    } else if (freq < 0.875) {
      waddch(stdscr, 'o');
    } else {
      waddch(stdscr, '0');
    }
  }

  return 0;
}

static int streamCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
) {
  wrefresh(stdscr);

  waddstr(stdscr, "Volume:\n");
  streamCallBackVolume(inputBuffer, framesPerBuffer);

  waddch(stdscr, '\n');

  waddstr(stdscr, "Frequencies:\n");
  streamCallBackFrequencies(inputBuffer, outputBuffer, framesPerBuffer, userData);

  wrefresh(stdscr);
  clear();

  return 0;
}

int main() {
  PaError err;
  err = Pa_Initialize();
  checkErr(err);

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

  initscr();

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

  Pa_Sleep(30 * 1000);

  err = Pa_CloseStream(stream);
  checkErr(err);

  err = Pa_Terminate();
  checkErr(err);

  fftw_destroy_plan(spectroData->p);
  fftw_free(spectroData->in);
  fftw_free(spectroData->out);
  fftw_free(spectroData);

  delwin(stdscr);

  cout << endl;

  return EXIT_SUCCESS;
}