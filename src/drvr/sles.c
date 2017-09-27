#include <stdio.h>
#include <string.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define BUFSIZE 1024
#define CHECK_SL(a) if((a) != SL_RESULT_SUCCESS) return -1;

typedef struct {
  long l, r;
} frame_t;

static frame_t out_buf[BUFSIZE];

static m_bool sles_ini(VM* vm, DriverInfo* di) {
  // create engine
  SLObjectItf engineObject;
  CHECK_SL(slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL))
  CHECK_SL((*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE))
  SLEngineItf engineEngine;
  CHECK_SL(*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &engineEngine))

  // create output mix
  SLObjectItf outputmixObject;
  CHECK_SL((*engineEngine)->CreateOutputMix(engineEngine, &outputmixObject, 0, NULL, NULL))
  CHECK_SL(*outputmixObject)->Realize(outputmixObject, SL_BOOLEAN_FALSE))

  // create audio player
  SLDataSource audiosrc;
  SLDataSink audiosnk;
  SLDataFormat_PCM pcm;
  SLDataLocator_OutputMix locator_outputmix;
  SLDataLocator_BufferQueue locator_bufferqueue;
  locator_bufferqueue.locatorType = DATALOCATOR_BUFFERQUEUE;
  locator_bufferqueue.numBuffers = BUFSIZE; //255;
  locator_outputmix.locatorType = SL_DATALOCATOR_OUTPUTMIX;
  locator_outputmix.outputMix = outputmixObject;
  pcm.formatType = SL_DATAFORMAT_PCM;
  pcm.numChannels = 2;
  pcm.samplesPerSec = SL_SAMPLINGRATE_48;
  pcm.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_32;
  pcm.containerSize = 32;
  pcm.channelMask = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
  pcm.endianness = SL_BYTEORDER_LITTLEENDIAN;
  audiosrc.pLocator = &locator_bufferqueue;
  audiosrc.pFormat = &pcm;
  audiosnk.pLocator = &locator_outputmix;
  audiosnk.pFormat = NULL;
  SLObjectItf playerObject;
  SLInterfaceID ids[1] = {IID_BUFFERQUEUE};
  SLboolean flags[1] = {SL_BOOLEAN_TRUE};
  CHECK_SL((*engineEngine)->CreateAudioPlayer(engineEngine, &playerObject, &audiosrc, &audiosnk,
          1, ids, flags);
  CHECK_SL((*playerObject)->Realize(playerObject, SL_BOOLEAN_FALSE))
  SLPlayItf playerPlay;
  CHECK_SL(*playerObject)->GetInterface(playerObject, SL_IID_PLAY, &playerPlay))
  BufferQueueItf playerBufferqueue;
  CHECK_SL(*playerObject)->GetInterface(playerObject, IID_BUFFERQUEUE, &playerBufferqueue))
  return 1;
}

static void sles_run(VM* vm, DriverInfo* di) {
// play
  CHECK_SL((*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING))
// loop
 while(vm->is_running) {
  // get input
    m_uint i;
    for(i = 0; i < BUFSIZE; i++) {
      di->run(vm)
      out_buf[i].l = vm->sp->out[0];
      out_buf[i].r = vm->sp->out[1];
  }
  // fill buffer
 }
  // stop (is this necessary?)
  CHECK_SL((*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_STOPPED))
}

static void sels_del() {
  if(bqPlayerObject)
    (*bqPlayerObject)->Destroy(bqPlayerObject);
  if(engineEngine)
    (*engineEngine)->Destroy(engineEngine);
  if(engineObject)
    (*engineObject)->Destroy(engineObject);
}

void pulse_driver(Driver* d, VM* vm) {
  d->ini = sles_ini;
  d->run = sles_run;
  d->del = sles_del;
//  vm->wakeup = no_wakeup;
}

