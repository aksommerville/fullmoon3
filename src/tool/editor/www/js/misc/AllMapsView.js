/* AllMapsView.js
 */
 
import { Dom } from "/js/core.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
import { MapService } from "/js/map/MapService.js";
import { ResService } from "/js/ResService.js";

export class AllMapsView {
  static getDependencies() {
    return [HTMLElement, Dom, MapService, ResService, Window];
  }
  constructor(element, dom ,mapService, resService, window) {
    this.element = element;
    this.dom = dom;
    this.mapService = mapService;
    this.resService = resService;
    this.window = window;
    
    this.name = "";
    this.layout = null;
    
    this.mapService.getAllMaps().then(toc => this._buildUi(toc));
  }
  
  setup(name) {
    this.name = name;
  }
  
  _buildUi(toc) {
    this.element.innerHTML = "";
    this.layout = this._layoutMaps(toc);
    
    const menu = this.dom.spawn(this.element, "SELECT", ["alternate"], { "on-change": () => this.onAlternateChosen() });
    this.dom.spawn(menu, "OPTION", { value: "", selected: "selected", disabled: "disabled" }, "Other sets");
    
    const canvas = this.dom.spawn(this.element, "CANVAS", ["mainView"], { "on-click": (event) => this.onClickCanvas(event) });
    this._render(canvas, this.layout);
    
    while (toc.length) {
      const name = toc[0].name;
      this._layoutMaps(toc, name);
      this.dom.spawn(menu, "OPTION", { value: name }, name);
    }
  }
  
  onClickCanvas(event) {
    if (!this.layout) return;
    const bounds = event.target.getBoundingClientRect();
    const x = event.clientX - bounds.x;
    const y = event.clientY - bounds.y;
    if ((x < 0) || (y < 0)) return;
    const tilesize = 8;
    const screenw = FullmoonMap.COLC * tilesize;
    const screenh = FullmoonMap.ROWC * tilesize;
    let col = Math.floor(x / screenw);
    let row = Math.floor(y / screenh);
    if (col >= this.layout.w) return;
    if (row >= this.layout.h) return;
    col += this.layout.x;
    row += this.layout.y;
    const map = this.layout.v[row * this.layout.stride + col];
    if (!map) return;
    this.resService.getNameForObject(map).then(name => {
      this.window.location = `${this.window.location.pathname}#/res/map/${name}`;
    }).catch(() => {});
  }
  
  onAlternateChosen() {
    const name = this.element.querySelector("select.alternate").value;
    if (!name) return;
    this.window.location = `${this.window.location.pathname}#/allmaps/${name}`;
  }
  
  /* Render.
   *********************************************************************/
  
  _render(canvas, layout) {
    const tilesize = 8;
    const screenw = tilesize * FullmoonMap.COLC;
    const screenh = tilesize * FullmoonMap.ROWC;
    canvas.width = screenw * layout.w;
    canvas.height = screenh * layout.h;
    const context = canvas.getContext("2d");
    for (let sy=0; sy<layout.h; sy++) {
      for (let sx=0; sx<layout.w; sx++) {
        const map = layout.v[(layout.y + sy) * layout.stride + layout.x + sx];
        this._renderMap(context, sx * screenw, sy * screenh, tilesize, map);
      }
    }
  }
  
  _renderMap(context, dstx, dsty, tilesize, map) {
    if (!map) {
      context.fillStyle = "#000";
      context.fillRect(dstx, dsty, tilesize * FullmoonMap.COLC, tilesize * FullmoonMap.ROWC);
      return;
    }
    const tsname = map.getTilesheetName();
    if (!tsname) {
      context.fillStyle = "#000";
      context.fillRect(dstx, dsty, tilesize * FullmoonMap.COLC, tilesize * FullmoonMap.ROWC);
      return;
    }
    this.resService.getImage(tsname).then((src) => {
      this._renderMapWithTilesheet(context, dstx, dsty, tilesize, map, src);
    }).catch((e) => {
      console.log(e);
      context.fillStyle = "#000";
      context.fillRect(dstx, dsty, tilesize * FullmoonMap.COLC, tilesize * FullmoonMap.ROWC);
    });
  }
  
  _renderMapWithTilesheet(context, dstx, dsty, tilesize, map, src) {
    for (let row=0, p=0; row<FullmoonMap.ROWC; row++) {
      for (let col=0; col<FullmoonMap.COLC; col++, p++) {
        const tileid = map.cellv[p];
        const srcx = (tileid & 15) * tilesize;
        const srcy = (tileid >> 4) * tilesize;
        context.drawImage(src, srcx, srcy, tilesize, tilesize, dstx + col * tilesize, dsty + row * tilesize, tilesize, tilesize);
      }
    }
  }
  
  /* Generate layout.
   ************************************************************************/
  
  /* Put all the maps in a grid ready for rendering.
   * Returns {
   *   v: (FullmoonMap | null)[],
   *   x,y: Top left corner in (v).
   *   w,h: Bounds in (v) actually used, from (x,y).
   *   stride: Distance row-to-row in (v).
   * }
   * We pop entries from (toc) as we use them. Anything remaining was not connected to the first entry.
   * (which is normal. We only navigate by edge neighbors, not through doors).
   */
  _layoutMaps(toc, name) {
    if (!name) name = this.name;
    const maxw = 50;
    const maxh = 50;
    const layout = {
      x: maxw >> 1,
      y: maxh >> 1,
      w: 0,
      h: 0,
      stride: maxw,
      maxh,
      v: [],
    };
    for (let i=maxw*maxh; i-->0; ) layout.v.push(null);
    
    if (toc.length > 0) {
      let p = name ? toc.findIndex(r => r.name === name) : 0;
      if (p < 0) p = 0;
      layout.v[layout.y * layout.stride + layout.x] = toc[p].object;
      layout.w = 1;
      layout.h = 1;
      toc.splice(p, 1);
      this._expandLayout(layout, toc, layout.x, layout.y);
    }
    
    return layout;
  }
  
  /* Given a populated cell (x,y) in (layout), recursively populate and reenter its neighbors until we can't.
   * Anything we add to the layout, we remove from (toc).
   * We will not exceed (layout)'s existing physical bounds, but will extend its logical bounds as needed.
   * Once a layout cell is populated, it will not be touched again.
   * (x,y) are absolute, from (layout)'s physical bounds.
   */
  _expandLayout(layout, toc, x, y) {
    const p = y * layout.stride + x;
    if ((p < 0) || (p >= layout.v.length)) return;
    const map = layout.v[p];
    if (!map) return;
    
    // Assume that we won't grow by more than one cell in any direction, because we won't.
    if (x < layout.x) { layout.w++; layout.x--; }
    if (y < layout.y) { layout.h++; layout.y--; }
    if (x >= layout.x + layout.w) layout.w++;
    if (y >= layout.y + layout.h) layout.h++;
    
    const tryNeighbor = (dx, dy, dir) => {
      const nx = x + dx, ny = y + dy;
      if ((nx < 0) || (nx >= layout.stride)) return;
      if ((ny < 0) || (ny >= layout.maxh)) return;
      const name = map.getNeighborName(dir);
      if (!name) return;
      const p = toc.findIndex(r => r.name === name);
      if (p < 0) return;
      const neighbor = toc[p].object;
      toc.splice(p, 1);
      layout.v[ny * layout.stride + nx] = neighbor;
      this._expandLayout(layout, toc, nx, ny);
    };
    
    tryNeighbor(-1, 0, "w");
    tryNeighbor( 1, 0, "e");
    tryNeighbor( 0,-1, "n");
    tryNeighbor( 0, 1, "s");
  }
}
