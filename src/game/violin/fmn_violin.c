#include "api/fmn_platform.h"
#include "api/fmn_common.h"
#include "game/image/fmn_image.h"
#include "game/play/fmn_play.h"
#include "game/fmn_data.h"
#include "fmn_violin.h"
#include <string.h>

/* A beat is the quantum of time for song recording.
 * This is *not* the metronome beat that the user hears, or the bars on the staff -- those are every 2 beats.
 */
#define FMN_VIOLIN_TEMPO 20 /* video frames per beat */
#define FMN_VIOLIN_TEMPO_HALF 10
#define FMN_VIOLIN_SONG_LENGTH 16 /* in beats */

/* Globals.
 */

static uint8_t fmn_violin_active=0; 
static uint8_t fmn_violin_dir=0;
static uint16_t fmn_violin_clock=0; // video frames since begin
static uint8_t fmn_violin_song[FMN_VIOLIN_SONG_LENGTH]; // circular
static uint8_t fmn_violin_songp=0; // Current entry point in fmn_violin_song. Historical record begins +1 of here.
static uint16_t fmn_violin_songp_time=0; // fmn_violin_clock at the last increment of songp

/* Begin.
 */
 
void fmn_violin_begin() {
  fmn_platform_audio_pause_song(1);
  fmn_violin_dir=0;
  fmn_violin_active=1;
  fmn_violin_clock=0;
  fmn_violin_songp=0;
  fmn_violin_songp_time=0;
  memset(fmn_violin_song,0,sizeof(fmn_violin_song));
}

/* End.
 */
 
void fmn_violin_end() {
  fmn_violin_active=0;
  fmn_violin_update(0);
  fmn_platform_audio_pause_song(0);
  
  uint8_t song[FMN_VIOLIN_SONG_LENGTH];
  uint8_t tailc=fmn_violin_songp+1;
  if (tailc>=FMN_VIOLIN_SONG_LENGTH) tailc=0;
  memcpy(song,fmn_violin_song+tailc,sizeof(fmn_violin_song)-tailc);
  memcpy(song+sizeof(fmn_violin_song)-tailc,fmn_violin_song,tailc);
  fmn_game_cast_song(song,FMN_VIOLIN_SONG_LENGTH);
}

/* Notes.
 */
 
static uint8_t fmn_violin_noteid_from_dir(uint8_t dir) {
  switch (dir) {
    case FMN_DIR_SW: return 0x3c;
    case FMN_DIR_S:  return 0x3e;
    case FMN_DIR_SE: return 0x3f;
    case FMN_DIR_W:  return 0x41;
    case FMN_DIR_E:  return 0x43;
    case FMN_DIR_NW: return 0x44;
    case FMN_DIR_N:  return 0x46;
    case FMN_DIR_NE: return 0x48;
  }
  return 0;
}

/* Update.
 */

void fmn_violin_update(uint8_t dir) {

  switch (fmn_violin_clock%FMN_VIOLIN_TEMPO) {
    case 0: if (!(fmn_violin_songp&1)) fmn_platform_audio_note(0,0x30,0x40,200); break;
    case FMN_VIOLIN_TEMPO_HALF: {
        fmn_violin_songp_time=fmn_violin_clock;
        fmn_violin_songp++;
        if (fmn_violin_songp>=FMN_VIOLIN_SONG_LENGTH) fmn_violin_songp=0;
        fmn_violin_song[fmn_violin_songp]=0;
      } break;
  }
  fmn_violin_clock++;

  if (dir==fmn_violin_dir) return;
  if (fmn_violin_dir) {
    // It would be better to track our note and release just that.
    // Probably not a big deal. We do suppress music during violin activity, by design.
    fmn_platform_audio_release_all();
  }
  if (dir) {
    uint8_t noteid=fmn_violin_noteid_from_dir(dir);
    if (noteid) {
      fmn_platform_audio_note(0,noteid,0x60,2000);
      if (!fmn_violin_song[fmn_violin_songp]) fmn_violin_song[fmn_violin_songp]=dir;
    }
  }
  fmn_violin_dir=dir;
}

/* Render UI.
 */
 
void fmn_violin_render(struct fmn_image *dst) {
  if (!fmn_violin_active) return;
  
  // Output area. We consume a horizontal ribbon of the framebuffer.
  // No horizontal margins, because our images don't have clipping except at their real edge.
  const int16_t beatw=6; // 96/16
  const int16_t lineh=4;
  const int16_t totalh=lineh*5+1;
  const int16_t bottommargin=2;
  const int16_t topy=dst->h-bottommargin-totalh;
  const int16_t songw=beatw*FMN_VIOLIN_SONG_LENGTH;
  
  // Background. Whiteout and horizontal lines.
  fmn_image_fill_rect(dst,0,topy,dst->w,totalh,3);
  fmn_image_fill_rect(dst,0,topy+1*lineh,dst->w,1,2);
  fmn_image_fill_rect(dst,0,topy+2*lineh,dst->w,1,2);
  fmn_image_fill_rect(dst,0,topy+3*lineh,dst->w,1,2);
  fmn_image_fill_rect(dst,0,topy+4*lineh,dst->w,1,2);
  
  /* (songp+1) is the beat furthest in the past.
   * Decide where it goes in output space.
   */
  int16_t subtime=fmn_violin_clock-fmn_violin_songp_time;
  int16_t x0=-(subtime*beatw)/FMN_VIOLIN_TEMPO;
  
  // Vertical lines and notes.
  uint8_t subp=0; for (;subp<=FMN_VIOLIN_SONG_LENGTH;subp++) {
    int16_t x=x0+(subp*songw)/FMN_VIOLIN_SONG_LENGTH;
    int8_t p=subp+fmn_violin_songp;
    if (p>=FMN_VIOLIN_SONG_LENGTH) p-=FMN_VIOLIN_SONG_LENGTH;
    if (!(p&1)) {
      fmn_image_fill_rect(dst,x,topy,1,totalh,2);
    }
    if (fmn_violin_song[p]) {
      int16_t dsty=topy,srcx=0;
      switch (fmn_violin_song[p]) {
        case FMN_DIR_S: srcx=5*0; dsty+=4*lineh; break;
        case FMN_DIR_W: srcx=5*1; dsty+=3*lineh; break;
        case FMN_DIR_E: srcx=5*2; dsty+=2*lineh; break;
        case FMN_DIR_N: srcx=5*3; dsty+=1*lineh; break;
      }
      fmn_image_blit(dst,x-2,dsty-2,&fmnr_image_uibits,srcx,14,5,5,0);
    }
  }
  
  // Vertical edge lines.
  fmn_image_fill_rect(dst,0,topy,dst->w,1,0);
  fmn_image_fill_rect(dst,0,topy+totalh-1,dst->w,1,0);

  #if 0 // close but not quite, and it's a mess...
  // Select output area.
  //XXX Extend to left and right screen edges, then we shouldn't need to worry about clipping.
  const int16_t margin=3;
  const int16_t height=22; // %5==2
  int16_t boundsl=margin;
  int16_t boundsr=dst->w-margin;
  int16_t boundsb=dst->h-margin;
  int16_t boundst=boundsb-height;

  // Background.
  fmn_image_fill_rect(dst,boundsl,boundst,boundsr-boundsl,boundsb-boundst,3);
  
  // Staff lines (constant).
  int16_t staffh=(height-2)/5;
  fmn_image_fill_rect(dst,boundsl,boundst+1+1*staffh,boundsr-boundsl,1,2);
  fmn_image_fill_rect(dst,boundsl,boundst+1+2*staffh,boundsr-boundsl,1,2);
  fmn_image_fill_rect(dst,boundsl,boundst+1+3*staffh,boundsr-boundsl,1,2);
  fmn_image_fill_rect(dst,boundsl,boundst+1+4*staffh,boundsr-boundsl,1,2);
  
  // Bar lines (shifting in time).
  // A beat is half a bar. So there are notes on the bars and notes halfway between them.
  // We record 16 beats. So show at least 8 bars. I mean exactly 8 bars. Right?
  // Bars are the audible beats; that's the measure of FMN_VIOLIN_TEMPO.
  // A bar should touch the right edge exactly at (fmn_violin_clock%FMN_VIOLIN_TEMPO==0).
  int16_t bartime=FMN_VIOLIN_TEMPO;
  int16_t barw=(boundsr-boundsl-2)/8;
  int16_t xr=boundsr-1-((fmn_violin_clock%FMN_VIOLIN_TEMPO)*barw)/FMN_VIOLIN_TEMPO;
  int16_t x=xr;
  for (;x>boundsl;x-=barw) {
    fmn_image_fill_rect(dst,x,boundst,1,boundsb-boundst,2);
  }
  
  uint8_t i=0; for (;i<FMN_VIOLIN_SONG_LENGTH;i++) {
    if (i==fmn_violin_songp) fprintf(stderr," (%d)",fmn_violin_song[i]);
    else fprintf(stderr," %d",fmn_violin_song[i]);
  }
  fprintf(stderr,"\n");
  
  // Notes.
  const int16_t maxtime=FMN_VIOLIN_SONG_LENGTH*(FMN_VIOLIN_TEMPO>>1);
  int16_t staffw=barw*(FMN_VIOLIN_SONG_LENGTH>>1);
  int16_t x0=xr-staffw;
  uint8_t storep=0; for (;storep<FMN_VIOLIN_SONG_LENGTH;storep++) {
    uint8_t dir=fmn_violin_song[storep];
    if (!dir) continue;
    int16_t time=storep-(fmn_violin_songp+1);
    if (time<0) time+=FMN_VIOLIN_SONG_LENGTH;
    time*=FMN_VIOLIN_TEMPO>>1;
    int16_t x=x0+(time*staffw)/maxtime;
    int16_t dsty=boundst+1;//TODO
    int16_t srcx=0*5;//TODO
    fmn_image_blit(dst,x-2,dsty,&fmnr_image_uibits,srcx,14,5,5,0);
  }
  
  #if 0 //XXX oh dear what have i done
  // Encoded song.
  int16_t halfbarw=barw>>1;
  x=boundsr-1-(((fmn_violin_clock%(FMN_VIOLIN_TEMPO>>1))*halfbarw)/(FMN_VIOLIN_TEMPO>>1));
  int8_t p=fmn_violin_songp,i=FMN_VIOLIN_SONG_LENGTH;
  for (;i-->0;x-=halfbarw,p--) {
    if (p<0) p+=FMN_VIOLIN_SONG_LENGTH;
    if (fmn_violin_song[p]) {
      int16_t dsty=boundst+1;//TODO
      int16_t srcx=0*5;//TODO
      fmn_image_blit(dst,x-2,dsty,&fmnr_image_uibits,srcx,14,5,5,0);
    }
  }
  #endif
  
  // Frame.
  fmn_image_fill_rect(dst,boundsl,boundst,boundsr-boundsl,1,0);
  fmn_image_fill_rect(dst,boundsl,boundsb-1,boundsr-boundsl,1,0);
  fmn_image_fill_rect(dst,boundsl,boundst,1,boundsb-boundst,0);
  fmn_image_fill_rect(dst,boundsr-1,boundst,1,boundsb-boundst,0);
  #endif
}
