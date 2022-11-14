/* GameView.js
 * Canvas element to display our framebuffer.
 */
 
export class GameView {
  static getDependencies() {
    return [HTMLCanvasElement];
  }
  constructor(element) {
    this.element = element;
    
    this.context = null;
    this.imageData = null;
  }
  
  _requireContext(w, h) {
    if (this.imageData && (w === this.element.width) && (h === this.element.height)) {
      return;
    }
    this.element.width = w;
    this.element.height = h;
    this.context = this.element.getContext("2d");
    this.imageData = this.context.createImageData(w, h);
  }
  
  refresh(fb, w, h) {
    if (fb.length !== w * h *4) {
      throw new Error(`Receiving framebuffer: w=${w} * h=${h} * 4 = ${w * h * 4}, fb.length=${fb.length}`);
    }
    this._requireContext(w, h);
    this.imageData.data.set(fb);
    this.context.putImageData(this.imageData, 0, 0);
  }
}
