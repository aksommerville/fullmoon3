/* tiny_upload.c
 * A separate program that runs on the Tiny, receives files over the serial link, and writes them to the SD card.
 * So I don't have to keep moving the SD card back and forth physically during development.
 *
 * Serial protocol.
 * Sender first sends a header. "key=value" separated by LF, and the whole thing terminated by a NUL.
 * Header fields "path" and "size" are required. eg:
 *   path=/fullmoon/fullmoon.bin
 *   size=123456
 *   [NUL]
 *   [ENCODED PAYLOAD...]
 * After the payload, another header can begin.
 * Receiver never acks, or anything else.
 * Header size is limited by our buffer, see 'intake' below.
 *
 * Payload is encoded as hexdecimal digits, two encoded digits per payload byte.
 * "size" is the pre-encoded size, so the transferred length is twice that.
 * I have to do this because I can't figure out how to prevent Linux from turning LF to CR on the way out.
 */
 
/* Include and declare only the bare essentials.
 * We only link against src/opt/tiny, not the Full Moon stuff.
 ***********************************************************************/
 
#include "game/image/fmn_image.h" /* headers only */

int8_t fmn_platform_init();
uint8_t fmn_platform_read_input();
struct fmn_image *fmn_platform_get_framebuffer();
void fmn_platform_framebuffer_ready(struct fmn_image *fb);
int usb_read_byte();
int8_t tiny_file_open_writeable(const char *path);
void tiny_file_close();
int32_t tiny_file_write(const void *src,int32_t srcc,int32_t seek);
void tiny_file_remove(const char *path);

static char intake[2048];
static uint16_t intakec=0;
static uint8_t file_open=0; // If zero, we're reading the header.

static uint8_t fatal=0; // If nonzero, we've failed irreparably.
static uint32_t file_size=0;
static uint32_t file_written=0;

/* Initialize.
 */
 
void setup() {
  if (fmn_platform_init()<0) fatal=0xe0;
}

/* Flush intake to file. Call only when the file is open.
 * We clear intake, and arrange to read headers again if warranted.
 */
 
static void flush_intake_to_file() {
  if (tiny_file_write(intake,intakec,0)<0) fatal=0x1c;
  file_written+=intakec;
  if (file_written>=file_size) {
    file_size=file_written=0;
    file_open=0;
    tiny_file_close();
  }
  intakec=0;
}

/* Call when we receive a NUL and no file is open -- (intake) contains a header.
 */
 
static void header_ready() {

  // Parse header in (intake).
  const char *path=0;
  int32_t size=0;
  uint16_t intakep=0;
  while (intakep<intakec) {
  
    char *k=intake+intakep,*v=0;
    uint16_t kc=0,vc=0;
    while ((intakep<intakec)&&(k[kc]!='=')&&(k[kc]!=0x0a)) { intakep++; kc++; }
    if ((intakep<intakec)&&(intake[intakep]=='=')) {
      intakep++;
      v=intake+intakep;
      while ((intakep<intakec)&&(v[vc]!=0x0a)) { intakep++; vc++; }
    }
    
    // mother fucker. something is inserting CRs. We can deal with it in the header, but in the payload, we're fucked.
    uint16_t i=0; for (;i<vc;i++) if (v[i]==0x0d) v[i]=0;
    while (vc&&((unsigned char)v[vc-1]<=0x20)) vc--; // careful! somehow CR's get added during the transfer. grumble grumble.
    
    // A sneaky cheat: Line should end now with LF. Replace that with NUL, so we can read (v) as terminated.
    if ((intakep>=intakec)||(intake[intakep]!=0x0a)) {
      fatal=0x02;
      return;
    }
    intake[intakep++]=0;
    
    if ((kc==4)&&!memcmp(k,"path",4)) {
      if (path) { fatal=0x01; return; }
      path=v;
      
    } else if ((kc==4)&&!memcmp(k,"size",4)) {
      if (size) { fatal=0x20; return; }
      uint16_t vp=0;
      for (;vp<vc;vp++) {
        uint8_t digit=v[vp]-'0';
        if (digit>9) { fatal=0xff; return; }
        size*=10;
        size+=digit;
      }
      
    } else {
      // Unknown header field, that's fine.
    }
  }
  if (!path&&!size) {
    // Both fields missing? Let's figure it's a false alarm.
    intakec=0;
    return;
  }
  if (!path||!size) {
    // NB (size) is not allowed to be zero.
    fatal=0xfc;
    return;
  }
  
  // Open the file.
  tiny_file_remove(path);
  if (tiny_file_open_writeable(path)<0) {
    fatal=0x1f;
    return;
  }
  file_open=1;
  file_size=size;
  file_written=0;
  intakec=0;
}

/* Render framebuffer.
 */
 
static void render(uint8_t *dst) {
  // It would be cool to give detailed feedback, if we want to enable text output...
  // For now, it's red=error, green=ready/success, yellow=incoming header, or gray with a progress bar during payload xfer.
  if (fatal) {
    memset(dst,0x03,96*32);
    memset(dst+96*32,fatal,96*32);
  } else if (file_written<file_size) {
    memset(dst,0x49,96*64);
    int16_t barw=(file_written*96)/file_size;
    if (barw>0) {
      if (barw>=96) barw=96;
      memset(dst+96*31,0xff,barw);
      memset(dst+96*32,0xff,barw);
    }
  } else if (intakec) {
    memset(dst,0x1f,96*64);
  } else {
    memset(dst,0x1c,96*64);
  }
}

/* Decode hexadecimal digit. >15 if invalid.
 */
 
static uint8_t decode_digit(uint8_t src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='f')) return src-'a'+10;
  if ((src>='A')&&(src<='F')) return src-'A'+10;
  return 0xff;
}

/* Update.
 */

void loop() {
  uint8_t input=fmn_platform_read_input();
  
  /* Read one byte at a time from serial and decide what to do with it, and do that.
   * There's a few strategic 'break' here to ensure that we do update the framebuffer now and then.
   * (I suspect usb_read_byte() will not actually return <0 until the very end of input).
   */
  int16_t hibyte=-1;
  while (!fatal) {
    int rcv=usb_read_byte();
    if (rcv<0) break;
    if (file_open) {
      if ((rcv=decode_digit(rcv))>15) { fatal=0xc3; break; }
      if (hibyte<0) {
        hibyte=rcv;
      } else { 
        rcv|=(hibyte<<4); hibyte=-1;
        intake[intakec++]=rcv;
        if (intakec>=sizeof(intake)) {
          flush_intake_to_file();
          break;
        } else if (file_written+intakec>=file_size) {
          flush_intake_to_file();
          break;
        }
      }
    } else if (rcv) {
      if (!intakec&&(rcv<=0x20)) ; // ignore leading space
      else if (intakec>=sizeof(intake)) fatal=0x92;
      else intake[intakec++]=rcv;
    } else {
      header_ready();
      break;
    }
  }
  
  struct fmn_image *fb=fmn_platform_get_framebuffer();
  render(fb->v);
  fmn_platform_framebuffer_ready(fb);
  
  if (fatal) delay(10000);
}
