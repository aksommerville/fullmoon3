#include "fmn_cheapsynth_internal.h"

/* Reset all channels.
 */
 
void fmn_cheapsynth_reset_channel(struct fmn_cheapsynth_channel *chan) {
  chan->mode=FMN_CHEAPSYNTH_CHANNEL_MODE_SQUARE;
  chan->waveid=0;
  chan->atktimemslo=chan->atktimemshi=20;
  chan->dectimemslo=chan->dectimemshi=30;
  chan->rlstimemslo=chan->rlstimemshi=200;
  chan->atklevello=chan->atklevelhi=0x1000;
  chan->suslevello=chan->suslevelhi=0x0400;
}

void fmn_cheapsynth_reset_channels() {
  struct fmn_cheapsynth_channel *chan=fmn_cheapsynth.channelv;
  uint8_t i=FMN_CHEAPSYNTH_CHANNEL_COUNT;
  for (;i-->0;chan++) fmn_cheapsynth_reset_channel(chan);
}
