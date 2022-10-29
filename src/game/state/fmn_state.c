#include "fmn_state.h"
#include <string.h>

/* Items.
 ****************************************************************/
 
static uint8_t fmn_state_item_selected;
static uint16_t fmn_state_item_possessed;
static uint8_t fmn_state_item_count[16];

void fmn_state_reset_items() {
  fmn_state_item_selected=0;
  fmn_state_item_possessed=0;

  memset(fmn_state_item_count,0xff,sizeof(fmn_state_item_count));
  fmn_state_item_count[FMN_ITEM_pitcher]=0;
  fmn_state_item_count[FMN_ITEM_coin]=0;
  fmn_state_item_count[FMN_ITEM_match]=0;
  fmn_state_item_count[FMN_ITEM_corn]=0;
  
  //XXX temp
  fmn_state_item_selected=FMN_ITEM_violin;
  fmn_state_item_possessed=0xffff;
  fmn_state_item_count[FMN_ITEM_pitcher]=1;
  fmn_state_item_count[FMN_ITEM_coin]=9;
  fmn_state_item_count[FMN_ITEM_match]=10;
  fmn_state_item_count[FMN_ITEM_corn]=254;
}

uint8_t fmn_state_get_selected_item() {
  return fmn_state_item_selected;
}

void fmn_state_set_selected_item(uint8_t itemid) {
  fmn_state_item_selected=itemid&15;
}

uint16_t fmn_state_get_possessed_items() {
  return fmn_state_item_possessed;
}

void fmn_state_add_possessed_item(uint8_t itemid) {
  fmn_state_item_possessed|=1<<itemid;
}

uint8_t fmn_state_get_item_count(uint8_t itemid) {
  if (itemid>=16) return 0;
  return fmn_state_item_count[itemid];
}

uint8_t fmn_state_adjust_item_count(uint8_t itemid,int8_t d) {
  if (itemid>=16) return 0;
  if (!d) return 0;
  if (fmn_state_item_count[itemid]==0xff) return 0;
  int16_t nc=((int16_t)(fmn_state_item_count[itemid]))+d;
  if (nc<0) nc=0;
  else if (nc>0xfe) nc=0xfe;
  if (nc==fmn_state_item_count[itemid]) return 0;
  fmn_state_item_count[itemid]=nc;
  return 1;
}

uint8_t fmn_state_get_selected_item_if_possessed() {
  if (!(fmn_state_item_possessed&(1<<fmn_state_item_selected))) return 0xff;
  return fmn_state_item_selected;
}
