/* VideoOut.js
 * Manages the <canvas> and incoming framebuffers.
 */
 
export class VideoOut {
  constructor() {
    this.element = null;
    this.context = null;
    this.imageData = null;
  }
  
  /* Create my <canvas> element and attach to (parent).
   */
  setup(parent) {
    this.element = document.createElement("CANVAS");
    const fbw = 96, fbh = 64; //TODO?
    this.element.width = fbw;
    this.element.height = fbh;
    const scale = 6;
    this.element.style.width = `${fbw * scale}px`;
    this.element.style.height = `${fbh * scale}px`;
    this.element.style.imageRendering = 'crisp-edges';
    parent.appendChild(this.element);
    this.context = this.element.getContext("2d");
    this.imageData = this.context.createImageData(fbw, fbh);
  }
  
  /* Put the new image on screen.
   * (src) must be a Uint8Array of length FBW*FBH*4, straight off the Wasm app.
   */
  render(src) {
    this.imageData.data.set(src);
    this.context.putImageData(this.imageData, 0, 0);
  }
  
}
