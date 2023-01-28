#ifndef _AUDIO_H
#define _AUDIO_H

#include <stdint.h>

void audio_exit(void);
void audio_set_volume(int channel, uint8_t volume);
void audio_set_frequency(int channel, uint8_t frequency);
void audio_set_control(int channel, uint8_t control);
int audio_init(void);

#endif /* _AUDIO_H */
