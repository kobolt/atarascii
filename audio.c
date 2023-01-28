#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>

#define AUDIO_SAMPLE_RATE 44100
#define AUDIO_BASE_FREQUENCY 31399.5 /* NTSC */
#define AUDIO_VOLUME 64 /* 0 -> 127 */

typedef struct audio_channel_s {
  uint8_t volume;
  uint8_t divider;
  uint8_t control;
  uint32_t shift;
  int shift_count;
  int shift_skip;
  int shift_skip_countdown;
} audio_channel_t;



static audio_channel_t audio_channel[2];



void audio_exit(void)
{
  SDL_PauseAudio(1);
  SDL_CloseAudio();
}



void audio_set_volume(int channel, uint8_t volume)
{
  audio_channel[channel].volume = volume * 16;
}



void audio_set_frequency(int channel, uint8_t frequency)
{
  /* Original frequency divider is based on ~30KHz rate,
     but convert here since actual sample rate of SDL may be different. */
  uint8_t divider;
  double scale = (AUDIO_SAMPLE_RATE / (double)AUDIO_BASE_FREQUENCY);
  divider = (int)(double)(frequency * scale) + 1;
  audio_channel[channel].divider = divider;
}



void audio_set_control(int channel, uint8_t control)
{
  audio_channel[channel].control = control;

  /* Setup shift register: */
  switch (control) {
  case 0x0:
  case 0x4:
  case 0x5:
  case 0xB:
  case 0xC:
  case 0xD:
    audio_channel[channel].shift = 0x1;
    break;

  case 0x1:
  case 0x2:
    audio_channel[channel].shift = 0xF;
    break;

  case 0x6:
  case 0xA:
  case 0xE:
    audio_channel[channel].shift = 0x3FFFF;
    audio_channel[channel].shift_count = 0;
    break;

  case 0x8:
    audio_channel[channel].shift = 0x1FF;
    break;

  case 0x3:
  case 0x7:
  case 0x9:
  case 0xF:
    audio_channel[channel].shift = 0x1F;
    break;

  default:
    audio_channel[channel].shift = 0;
    break;
  }

  /* Setup skip countdown: */
  switch (control) {
  case 0x2:
    audio_channel[channel].shift_skip = 15;
    audio_channel[channel].shift_skip_countdown =
      audio_channel[channel].shift_skip;
    break;

  case 0xC:
  case 0xD:
  case 0xE:
  case 0xF:
    audio_channel[channel].shift_skip = 2;
    audio_channel[channel].shift_skip_countdown =
      audio_channel[channel].shift_skip;
    break;

  default:
    audio_channel[channel].shift_skip = 0;
    break;
  }
}



static bool audio_shift_execute(int channel)
{
  /* Return LSB of the shift register: */
  bool bit = audio_channel[channel].shift & 1;

  /* Do not shift if countdown is active: */
  if (audio_channel[channel].shift_skip > 0) {
    if (audio_channel[channel].shift_skip_countdown > 0) {
      audio_channel[channel].shift_skip_countdown--;
      return bit;
    } else {
      audio_channel[channel].shift_skip_countdown =
        audio_channel[channel].shift_skip;
    }
  }

  /* Shift: */
  switch (audio_channel[channel].control) {
  case 0x0: /* "set to 1" */
  case 0xB: /* "set last 4 bits to 1" */
    bit = 1;
    break;

  case 0x1: /* "4 bit poly" */
  case 0x2: /* "div 15 -> 4 bit poly" but faked as "4 bit poly" */
    audio_channel[channel].shift +=
      (((audio_channel[channel].shift & 1) ^
       ((audio_channel[channel].shift >> 1) & 1)) << 4);
    audio_channel[channel].shift >>= 1;
    break;

  case 0x4: /* "div 2 : pure tone" */
  case 0x5: /* "div 2 : pure tone" */
  case 0xC: /* "div 6 : pure tone" */
  case 0xD: /* "div 6 : pure tone" */
    audio_channel[channel].shift = !bit;
    break;

  case 0x6: /* "div 31 : pure tone" */
  case 0xA: /* "div 31 : pure tone" */
  case 0xE: /* "div 93 : pure tone" */
    audio_channel[channel].shift >>= 1;
    audio_channel[channel].shift_count++;
    if (audio_channel[channel].shift_count == 31) {
      audio_channel[channel].shift = 0x3FFFF;
      audio_channel[channel].shift_count = 0;
    }
    break;

  case 0x8: /* "9 bit poly (white noise)" */
    audio_channel[channel].shift +=
      (((audio_channel[channel].shift & 1) ^
       ((audio_channel[channel].shift >> 4) & 1)) << 9);
    audio_channel[channel].shift >>= 1;
    break;

  case 0x3: /* "5 bit poly -> 4 bit poly" but faked as "5 bit poly" */
  case 0x7: /* "5 bit poly -> div 2" */
  case 0x9: /* "5 bit poly" */
  case 0xF: /* "5 bit poly div 6" */
    audio_channel[channel].shift +=
      (((audio_channel[channel].shift & 1) ^
       ((audio_channel[channel].shift >> 2) & 1)) << 5);
    audio_channel[channel].shift >>= 1;
    break;

  default:
    break;
  }

  return bit;
}



static void audio_callback(void *userdata, Uint8 *stream, int len)
{
  (void)userdata;
  static uint32_t sample_no = 0;
  static bool bit[2];
  double sample;
  int i;

  for (i = 0; i < len; i++) {
    sample = 0;

    /* Channel 0 */
    if (sample_no % (audio_channel[0].divider) == 0) {
      bit[0] = audio_shift_execute(0);
    }
    sample += ((bit[0] ? 1.0 : -1.0) / 256) * audio_channel[0].volume;

    /* Channel 1 */
    if (sample_no % (audio_channel[1].divider) == 0) {
      bit[1] = audio_shift_execute(1);
    }
    sample += ((bit[1] ? 1.0 : -1.0) / 256) * audio_channel[1].volume;

    sample /= 2;
    stream[i] = (Uint8)(127 + (sample * AUDIO_VOLUME));
    sample_no++;
  }
}



int audio_init(void)
{
  SDL_AudioSpec desired, obtained;

  audio_set_volume(0, 0);
  audio_set_volume(1, 0);
  audio_set_frequency(0, 0);
  audio_set_frequency(1, 0);
  audio_set_control(0, 0);
  audio_set_control(1, 0);

  desired.freq     = AUDIO_SAMPLE_RATE;
  desired.format   = AUDIO_U8;
  desired.channels = 1;
  desired.samples  = 2048; /* Buffer size */
  desired.userdata = 0;
  desired.callback = audio_callback;

  if (SDL_OpenAudio(&desired, &obtained) != 0) {
    fprintf(stderr, "SDL_OpenAudio() failed: %s\n", SDL_GetError());
    return -1;
  }

  if (obtained.format != AUDIO_U8) {
    fprintf(stderr, "Did not get unsigned 8-bit audio format!\n");
    SDL_CloseAudio();
    return -1;
  }

  SDL_PauseAudio(0);
  return 0;
}



