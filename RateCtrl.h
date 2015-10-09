#ifndef RATE_CTRL_H
#define RATE_CTRL_H


void CBR_Set(AVCodecContext *c, long br);
void VBR_Set(AVCodecContext *c, long br, long max, long min);

#endif