#include <stdlib.h>
#include <iostream>
#include <portaudio.h>

using namespace std;

#define NUMBER_OF_CHANNELS 2
#define SAMPLE_RATE 44100
#define FRAMES_PER_BUFFER 512


static void checkErr(PaError err) {
  if (err != paNoError) {
    std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    exit(EXIT_FAILURE);
  }
}

static int patestCallBack(
    const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags,
    void* userData
    ) {
  float* in = (float*)inputBuffer;
  (void)outputBuffer;

  int dispSize = 100;
  cout << "\r";

  float volL = 0;
  float volR = 0;

  for (unsigned long i = 0; i < framesPerBuffer * 2; i += 2) {
    volL = max(volL, abs(in[i]));
    volR = max(volR, abs(in[i+1]));
  }

  for (int i = 0; i < dispSize; i++) {
    float barProportion = i/(float)dispSize;
    if (barProportion <= volL && barProportion <= volR) {
      cout << "█";
    } else if (barProportion <= volL) {
      cout << "▀";
    } else if (barProportion <= volR) {
      cout << "▄";
    } else {
      cout << " ";
    }
  }

  fflush(stdout);

  return 0;
}

int main() {
  PaError err;
  err = Pa_Initialize();
  checkErr(err);

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
  inputParameters.channelCount = NUMBER_OF_CHANNELS;
  inputParameters.device = deviceSelection;
  inputParameters.hostApiSpecificStreamInfo = nullptr;
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceSelection)->defaultLowInputLatency;

  memset(&outputParameters, 0, sizeof(outputParameters));
  outputParameters.channelCount = NUMBER_OF_CHANNELS;
  outputParameters.device = deviceSelection;
  outputParameters.hostApiSpecificStreamInfo = nullptr;
  outputParameters.sampleFormat = paFloat32;
  outputParameters.suggestedLatency = Pa_GetDeviceInfo(deviceSelection)->defaultLowInputLatency;

  PaStream* stream;
  err = Pa_OpenStream(
      &stream,
      &inputParameters,
      &outputParameters,
      SAMPLE_RATE,
      FRAMES_PER_BUFFER,
      paNoFlag,
      patestCallBack,
      NULL
      );
  checkErr(err);

  err = Pa_StartStream(stream);
  checkErr(err);

  Pa_Sleep(10 * 1000);

  err = Pa_CloseStream(stream);
  checkErr(err);

  err = Pa_Terminate();
  checkErr(err);

  return EXIT_SUCCESS;
}