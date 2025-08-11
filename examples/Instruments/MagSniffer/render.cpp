#include <cmath>

#include "LDSP.h"
#include "libraries/OscSender/OscSender.h"

const float MAX_MAGNITUDE = 300;
const float MIN_MAGNITUDE = 30;

const float MAX_RATE = 4;
const float MIN_RATE = 1;

float time_until_next_vibrate = 0;

OscSender oscSender;

bool setup(LDSPcontext *context, void *userData) {
  oscSender.setup(2737, "192.168.51.82");
  return true;
}

void render(LDSPcontext *context, void *userData) {
  float mag_x = sensorRead(context, chn_sens_magX);
  float mag_y = sensorRead(context, chn_sens_magY);
  float mag_z = sensorRead(context, chn_sens_magZ);

  printf("%f %f %f \n", mag_x, mag_y, mag_z);

  float magnitude = sqrt(mag_x * mag_x + mag_y * mag_y + mag_z * mag_z);

  // printf("%f \n", magnitude);

  /*
   * Without any external magnets present, values are around 35
   * With a strong magnet ~3in away, values reach 200
   * (Briefly!) bringing the magnet to the back of the device yields values
   * exceeding 2000
   */

  float relative_magnitude =
      (magnitude - MIN_MAGNITUDE) / (MAX_MAGNITUDE - MIN_MAGNITUDE);

  float vibration_rate = relative_magnitude * (MAX_RATE - MIN_RATE) + MIN_RATE;
  float pulse_period = 1 / vibration_rate;
  float pulse_width = relative_magnitude / vibration_rate;

  if (time_until_next_vibrate <= 0) {
    printf("vibrate %f %f \n", pulse_period, pulse_width);
    ctrlOutputWrite(context, chn_cout_vibration, pulse_width * 1000);
    time_until_next_vibrate = pulse_period;
  }

  time_until_next_vibrate -=
      1.0 / (context->audioSampleRate / context->audioFrames);

  for (int n = 0; n < context->audioFrames; n++) {
    for (int chn = 0; chn < context->audioOutChannels; chn++) {
      audioWrite(context, n, chn, 0);
    }
  }

  oscSender.newMessage("/mag").add(mag_x).add(mag_y).add(mag_z).sendNow();
}

void cleanup(LDSPcontext *context, void *userData) {}
