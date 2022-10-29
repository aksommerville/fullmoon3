#include "fmn_cheapsynth_internal.h"

/* Update song, main entry point.
 */
 
int32_t fmn_cheapsynth_update_song() {
  while (fmn_cheapsynth.song) {
  
    // Got a delay or pause? We're done here.
    if (fmn_cheapsynth.songdelay) return fmn_cheapsynth.songdelay;
    if (fmn_cheapsynth.songpause) return INT32_MAX;
    
    // End of song? Loop and return 1, so we can't loop forever.
    if (
      (fmn_cheapsynth.songp>=fmn_cheapsynth.songc)||
      !fmn_cheapsynth.song[fmn_cheapsynth.songp]
    ) {
     _end_of_song_:;
      fmn_cheapsynth.songp=fmn_cheapsynth.songloopp;
      return 1;
    }
    
    // Pop the lead byte, check for delay.
    uint8_t lead=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
    if (!(lead&0x80)) {
      return fmn_cheapsynth.songdelay=lead*fmn_cheapsynth.songrate_frames;
    }
    
    // One-off Note.
    if ((lead&0xe0)==0x80) {
      if (fmn_cheapsynth.songp>fmn_cheapsynth.songc-2) goto _end_of_song_;
      uint8_t a=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
      uint8_t b=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
      uint8_t velocity=(a&0x1f)<<2; velocity|=velocity>>7;
      uint8_t noteid=0x20+(a>>2);
      uint8_t durticks=((a&0x03)<<4)|(b>>4);
      uint8_t chid=b&0x0f;
      fmn_cheapsynth_note(chid,noteid,velocity,durticks*fmn_cheapsynth.songrate);
      continue;
    }
    
    // Config, Note On, and Note Off are all flags by the top 4 bits.
    switch (lead&0xf0) {
      case 0xa0: { // Config
          if (fmn_cheapsynth.songp>fmn_cheapsynth.songc-2) goto _end_of_song_;
          uint8_t a=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
          uint8_t b=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
          fmn_cheapsynth_config(lead&0x0f,a,b);
        } continue;
      case 0xb0: { // Note On
          if (fmn_cheapsynth.songp>fmn_cheapsynth.songc-2) goto _end_of_song_;
          uint8_t a=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
          uint8_t b=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
          fmn_cheapsynth_note_on(lead&0x0f,a,b);
        } continue;
      case 0xc0: { // Note Off
          if (fmn_cheapsynth.songp>fmn_cheapsynth.songc-1) goto _end_of_song_;
          uint8_t a=fmn_cheapsynth.song[fmn_cheapsynth.songp++];
          fmn_cheapsynth_note_off(lead&0x0f,a);
        } continue;
    }
    
    // Unknown command!
    fprintf(stderr,"cheapsynth: Abort song due to unknown command 0x%02x at %d/%d\n",lead,fmn_cheapsynth.songp-1,fmn_cheapsynth.songc);
    fmn_cheapsynth_play_song(0,0);
    goto _end_of_song_;
  }
  return 0x7fff;
}

/* Advance song clock.
 */
 
void fmn_cheapsynth_advance_song(int32_t framec) {
  if (framec<1) return;
  if (framec>fmn_cheapsynth.songdelay) fmn_cheapsynth.songdelay=0;
  else fmn_cheapsynth.songdelay-=framec;
}
