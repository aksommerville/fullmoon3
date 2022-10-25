/* ImageController.js
 * Top-level UI controller for "image" resources, which is both image and tileprops.
 */
 
import { Dom } from "/js/core.js";
import { ResService } from "/js/ResService.js";
import { Tileprops } from "./Tileprops.js";
import { TileModal } from "./TileModal.js";

export class ImageController {
  static getDependencies() {
    return [HTMLElement, Dom, "discriminator", ResService, Window];
  }
  constructor(element, dom, discriminator, resService, window) {
    this.element = element;
    this.dom = dom;
    this.discriminator = discriminator;
    this.resService = resService;
    this.window = window;
    
    this.res = null;
    this.image = null;
    this.tileprops = null;
    
    this.neighborsImage = null;
    const preloadNeighborsImage = new Image();
    preloadNeighborsImage.addEventListener("load", () => {
      this.neighborsImage = preloadNeighborsImage;
      this._render();
    });
    preloadNeighborsImage.src = "/img/neighbors.png";
    
    this._resizeObserver = new this.window.ResizeObserver((event) => {
      this._render();
    });
    this._resizeObserver.observe(this.element);
    
    this._buildUi();
  }
  
  onRemoveFromDom() {
    this._resizeObserver.unobserve(this.element);
  }
  
  setResource(res) {
    this.res = res;
    this.image = null;
    this.tileprops = null;
    this.resService.getImage(this.res.name).then((image) => {
      this.image = image;
      const tpres = this.resService.getTocEntrySync("tileprops", this.res.name);
      if (tpres) {
        if (!tpres.object) tpres.object = new Tileprops(tpres.text);
        this.tileprops = tpres.object;
      }
      this._render();
    }).catch(e => {
      console.log(`ImageController failed to load image:${res.name}`, e);
    });
  }
  
  _buildUi() {
    this.element.innerHTML = "";
    
    const controls = this.dom.spawn(this.element, "DIV", ["controls"], { "on-change": () => this._render() });
    this._spawnVisibilityToggle(controls, "image");
    this._spawnVisibilityToggle(controls, "gridLines");
    this._spawnVisibilityToggle(controls, "physics");
    this._spawnVisibilityToggle(controls, "neighbors");
    this._spawnVisibilityToggle(controls, "family");
    this._spawnVisibilityToggle(controls, "probability");
    
    this.dom.spawn(this.element, "CANVAS", { "on-click": e => this.onClick(e) });
    
    this._render();
  }
  
  _spawnVisibilityToggle(parent, name) {
    const id = `ImageController-${this.discriminator}-visibility-${name}`;
    const input = this.dom.spawn(parent, "INPUT", ["hidden", "visibilityToggle"], { type: "checkbox", id, name, "checked": "checked" });
    const label = this.dom.spawn(parent, "LABEL", ["visibilityToggle"], { for: id }, name);
  }
  
  _getVisibilityFeatures() {
    return Array.from(this.element.querySelectorAll("input.visibilityToggle:checked")).map(e => e.name);
  }
  
  /* Render.
   ************************************************************************/
  
  _render() {
    const canvas = this.element.querySelector("canvas");
    if (!canvas) return;
    canvas.width = canvas.offsetWidth;
    canvas.height = canvas.offsetHeight;
    const context = canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    
    context.fillStyle = "#222";
    context.fillRect(0, 0, canvas.width, canvas.height);
    
    if (!this.image) return;
    const features = this._getVisibilityFeatures();
    const bounds = this._calculateImageBounds(canvas.width, canvas.height, this.image.naturalWidth, this.image.naturalHeight);
    
    if (features.includes("image")) {
      context.drawImage(this.image, 0, 0, this.image.naturalWidth, this.image.naturalHeight, bounds.x, bounds.y, bounds.w, bounds.h);
    }
    if (this.tileprops) {
      if (features.includes("physics")) {
        this._renderPhysics(context, bounds, this.tileprops.physics);
      }
      if (features.includes("neighbors")) {
        this._renderNeighbors(context, bounds, this.tileprops.neighbors);
      }
      if (features.includes("family")) {
        this._renderFamily(context, bounds, this.tileprops.family);
      }
      if (features.includes("probability")) {
        this._renderProbability(context, bounds, this.tileprops.probability);
      }
    }
    if (features.includes("gridLines")) {
      this._renderGridLines(context, bounds);
    }
  }
  
  _renderGridLines(context, bounds) {
    context.beginPath();
    for (let col=0; col<=16; col++) {
      const x = bounds.x + (bounds.w * col) / 16;
      context.moveTo(x, bounds.y);
      context.lineTo(x, bounds.y + bounds.h);
    }
    for (let row=0; row<=16; row++) {
      const y = bounds.y + (bounds.h * row) / 16;
      context.moveTo(bounds.x, y);
      context.lineTo(bounds.x + bounds.w, y);
    }
    context.strokeStyle = "#0ff";
    context.stroke();
  }
  
  /* Physics, neighbors, and family are all designed such that zero is a sane default and need not render anything.
   * Physics makes a colored badge in the upper-left corner of the cell.
   * Neighbors draws an icon in the upper-right corner.
   * Family is printed in decimal from the bottom-left.
   */
  _renderPhysics(context, bounds, src) {
    const badgeSize = Math.ceil(bounds.w / 64); // about 1/4 cell
    for (let row=0, p=0; row<16; row++) {
      for (let col=0; col<16; col++, p++) {
        switch (src[p]) {
          case 1: context.fillStyle = "#0f0"; break; // solid
          case 2: context.fillStyle = "#00f"; break; // hole
          case 3: context.fillStyle = "#f00"; break; // reserved
          default: continue; // vacant or illegal
        }
        const x = Math.floor(bounds.x + (col * bounds.w) / 16);
        const y = Math.floor(bounds.y + (row * bounds.h) / 16);
        context.fillRect(x, y, badgeSize, badgeSize);
      }
    }
  }
  _renderNeighbors(context, bounds, src) {
    if (this.neighborsImage) {
      const iconSize = this.neighborsImage.naturalWidth / 3;
      for (let row=0, p=0; row<16; row++) {
        for (let col=0; col<16; col++, p++) {
          if (!src[p]) continue;
          const x = Math.floor(bounds.x + ((col + 1) * bounds.w) / 16) - this.neighborsImage.naturalWidth;
          const y = Math.floor(bounds.y + (row * bounds.h) / 16);
          for (let suby=0, mask=0x80; suby<3; suby++) {
            for (let subx=0; subx<3; subx++) {
              if ((subx === 1) && (suby === 1)) continue;
              if (src[p] & mask) {
                context.drawImage(
                  this.neighborsImage,
                  subx * iconSize,
                  suby * iconSize,
                  iconSize, iconSize,
                  x + subx * iconSize,
                  y + suby * iconSize, 
                  iconSize, iconSize
                );
              }
              mask >>= 1;
            }
          }
        }
      }
    }
  }
  _renderFamily(context, bounds, src) {
    context.font = "20px sans-serif";
    for (let row=0, p=0; row<16; row++) {
      for (let col=0; col<16; col++, p++) {
        if (!src[p]) continue;
        const text = `${src[p]}`;
        const x = Math.floor(bounds.x + (col * bounds.w) / 16);
        const y = bounds.y + Math.floor(((row + 1) * bounds.h) / 16);
        context.fillStyle = "#000";
        context.strokeStyle = "#ff0";
        context.strokeText(text, x, y);
        context.fillText(text, x, y);
      }
    }
  }
  _renderProbability(context, bounds, src) {
    const badgeSize = Math.ceil(bounds.w / 64); // about 1/4 cell
    for (let row=0, p=0; row<16; row++) {
      for (let col=0; col<16; col++, p++) {
        if (!src[p]) continue; // default, most likely, do nothing.
        if (src[p] === 0xff) { // 255 is special, it means "by appointment only"
          context.fillStyle = "#f00";
        } else { // otherwise shades of blue, brighter is more probable.
          let b = 0xff - src[p];
          b = b.toString(16).padStart(2, '0');
          context.fillStyle = `#0000${b}`;
        }
        const x = Math.floor(bounds.x + ((col + 1) * bounds.w) / 16) - badgeSize;
        const y = Math.floor(bounds.y + ((row + 1) * bounds.h) / 16) - badgeSize;
        context.fillRect(x, y, badgeSize, badgeSize);
      }
    }
  }
  
  _calculateImageBounds(fullw, fullh, imgw, imgh) {
    if ((fullw < 1) || (fullh < 1) || (imgw < 1) || (imgh < 1)) return { x: 0, y: 0, w: 0, h: 0 };
    let w = (imgw * fullh) / imgh, h;
    if (w <= fullw) h = fullh;
    else {
      w = fullw;
      h = (fullw * imgh) / imgw;
    }
    w = Math.floor(w);
    h = Math.floor(h);
    const x = (fullw >> 1) - (w >> 1);
    const y = (fullh >> 1) - (h >> 1);
    return { x, y, w, h };
  }
  
  /* Events.
   *********************************************************************/
   
  onClick(event) {
    if (!this.tileprops) return;
    const canvas = this.element.querySelector("canvas");
    if (!canvas) return;
    const elementBounds = canvas.getBoundingClientRect();
    const x = event.clientX - elementBounds.x;
    const y = event.clientY - elementBounds.y;
    const bounds = this._calculateImageBounds(canvas.width, canvas.height, this.image.naturalWidth, this.image.naturalHeight);
    const tileSize = bounds.w >> 4;
    const col = Math.floor((x - bounds.x) / tileSize);
    const row = Math.floor((y - bounds.y) / tileSize);
    if ((col < 0) || (col >= 16)) return;
    if ((row < 0) || (row >= 16)) return;
    const tileid = (row << 4) | col;
    const controller = this.dom.spawnModal(TileModal);
    controller.setup(this.image, this.tileprops, tileid);
    controller.onDirty = () => {
      this.resService.dirty("tileprops", this.res.name, () => this.tileprops.encode());
      this._render();
    };
  }
}
