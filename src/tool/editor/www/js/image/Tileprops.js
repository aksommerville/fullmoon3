/* Tileprops.js
 * Optional appendix to image.
 */
 
export class Tileprops {
  constructor(src) {
    this.physics = new Uint8Array(256);
    this.neighbors = new Uint8Array(256);
    this.family = new Uint8Array(256);
    this.probability = new Uint8Array(256);
    if (src) this.decode(src);
  }
  
  decode(src) {
    let page = 0, row = 0, lineno = 0, srcp = 0;
    try {
      while (srcp < src.length) {
        lineno++;
        let nlp = src.indexOf("\n", srcp);
        if (nlp < 0) nlp = src.length;
        const line = src.substring(srcp, nlp).trim();
        srcp = nlp + 1;
        if (!line) continue;
      
        if (row >= 16) {
          row = 0;
          page++;
        }
        switch (page) {
          case 0: this._decodeRow(this.physics, row * 16, line); break;
          case 1: this._decodeRow(this.neighbors, row * 16, line); break;
          case 2: this._decodeRow(this.family, row * 16, line); break;
          case 3: this._decodeRow(this.probability, row * 16, line); break;
          // More pages may be added in the future. No worries, ignore them if we don't recognize.
        }
        row++;
      }
    } catch (e) {
      throw new Error(`${lineno}: ${e.message}`);
    }
  }
  
  _decodeRow(dst, dstp, src) {
    src = src.split(/\s+/).map(s => +s);
    if (src.length !== 16) throw new Error(`Expected 16 integers`);
    for (let i=0; i<16; i++) dst[dstp++] = src[i];
  }
  
  encode() {
    let dst = "";
    dst += this._encodeTable(this.physics);
    dst += this._encodeTable(this.neighbors);
    dst += this._encodeTable(this.family);
    dst += this._encodeTable(this.probability);
    return dst;
  }
  
  _encodeTable(src) {
    let dst = "";
    for (let row=0, p=0; row<16; row++) {
      for (let col=0; col<16; col++, p++) {
        dst += src[p] + ' ';
      }
      dst += "\n";
    }
    dst += "\n";
    return dst;
  }
}

Tileprops.PRESETS = [{
  name: "fat5x3",
  w: 5,
  h: 3,
  masks: [
    0x0b, 0x1f, 0x16, 0xfe, 0xfb,
    0x6b, 0xff, 0xd6, 0xdf, 0x7f,
    0x68, 0xf8, 0xd0, 0x00, 0x00,
  ],
}, {
  name: "skinny4x4",
  w: 4,
  h: 4,
  masks: [
    0x0a, 0x1a, 0x12, 0x02,
    0x4a, 0x5a, 0x52, 0x42,
    0x48, 0x58, 0x50, 0x40,
    0x08, 0x18, 0x10, 0x00,
  ],
}];
