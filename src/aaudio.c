#include <aaudio/AAudio.h>
#include "defs.h"

#define AACHECK(a) if (a != AAUDIO_OK) return -1;

static AAudioStreamBuilder *builder;
static AAudioStream        *stream;

m_bool aa_open() {
  AACHECK(AAudio_createStreamBuilder(     &builder))
  AAudioStreamBuilder_setPerformanceMode( builder, PERFORMANCE_MODE);
  AAudioStreamBuilder_setSharingMode(     builder, SHARING_MODE);
//  AAudioStreamBuilder_setDataCallback(builder, aa_cb, vm);
//  AAudioStreamBuilder_setErrorCallback(builder, er_cb, vm);
  AAudioStreamBuilder_setChannelCount(builder, 2);
  AAudioStreamBuilder_setSampleRate(builder, 48000);
  AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
//AAudioStreamBuilder_setFramesPerDataCallback(mBuilder, CALLBACK_SIZE_FRAMES);
  AAudioStreamBuilder_setBufferCapacityInFrames(builder, 48 * 8);
  AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
  AAudioStreamBuilder_openStream(builder, &stream);
  return 1;
}

static int32_t aa_frames() {
  int32_t frames  = AAudioStream_getFramesPerBurst(stream);
  while (framesPerWrite < 48) {
    frames *= 2;
  }
  return frames;
}
static void aa_run(VM* vm, DriverInfo* di) {
  int64_t timeout = 1000 * NANOS_PER_MILLISECOND;
  int32_t frames  = aa_frames();
  float data[frames * 2];

  while(vm->is_running) {
    m_uint i;
    // get adc
    for(i = 0; i < frames; i++) {
      vm_run(vm);
      memcpy(data, vm->sp->data, 2* sizeof(float));
    }
    AudioStream_write(stream, data, frames, timeout); // better check that
  }
}

static void aa_del(VM* vm) {
  if(stream) {
    AAudioStream_requestStop(stream);
    AAudioStream_close(stream);
  }
  if(builder)
    AAudioStreamBuilder_delete(builder);
}

// TODO channel count
// FORBID compiling for android with doubles
