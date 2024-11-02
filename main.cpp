#include <stdlib.h>
#include <iostream>
#include <portaudio.h>

using namespace std;

static void checkErr(PaError err) {
  if (err != paNoError) {
    std::cout << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    exit(EXIT_FAILURE);
  }
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

  err = Pa_Terminate();
  checkErr(err);

  return EXIT_SUCCESS;
}