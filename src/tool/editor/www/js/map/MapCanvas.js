/* MapCanvas.js
 * A single <canvas> element display the focussed map and its neighbors.
 * This is the main business part of MapController.
 * We interact with the service layer, because we render not just the main map, but also eight neighbors.
 */
 
import { Dom } from "/js/core.js";
import { ResService } from "/js/ResService.js";
import { MapService } from "./MapService.js";
import { FullmoonMap } from "./FullmoonMap.js";
import { MapPaintService } from "./MapPaintService.js";

export class MapCanvas {
  static getDependencies() {
    return [HTMLCanvasElement, Dom, Window, ResService, MapService, MapPaintService];
  }
  constructor(element, dom, window, resService, mapService, mapPaintService) {
    this.element = element;
    this.dom = dom;
    this.window = window;
    this.resService = resService;
    this.mapService = mapService;
    this.mapPaintService = mapPaintService;
    
    // Parent should replace:
    this.onNavigate = (name) => {};
    this.onCreateNeighbor = (dir, name) => {};
    this.onDirty = () => {};
    
    this.name = "";
    this.map = null; // FullmoonMap
    this.tilesheet = null; // Image
    this.neighborw = null; // FullmoonMap
    this.neighbore = null; // FullmoonMap
    this.neighborn = null; // FullmoonMap
    this.neighbors = null; // FullmoonMap
    this.neighbornw = null; // FullmoonMap | string (error message)
    this.neighborne = null; // ''
    this.neighborsw = null; // ''
    this.neighborse = null; // ''
    this.xlines = [0, 0, 0, 0]; // Position in canvas of horz edges (W left, mid left, mid right, E right)
    this.ylines = [0, 0, 0, 0]; // '' vert
    this._badges = []; // {x,y,w,h,tileid,command,map?} bounds in pixels
    this._mustRepositionBadges = false; // during mouse action
    
    // Presence in this array enables a decoration for the main map.
    this.decorations = ["gridLines", "badges"];
    
    this._renderTimeout = null;
    this.resizeObserver = new this.window.ResizeObserver(event => {
      this._recalculateBoundaries(this.element.offsetWidth, this.element.offsetHeight);
      this._recalculateBadgePositions();
      this._renderSoon();
    });
    this.resizeObserver.observe(this.element);
    
    this.element.addEventListener("mousedown", event => this.onMouseDown(event));
    this.element.addEventListener("contextmenu", event => event.preventDefault());
    this._mouseMoveListener = null;
    this._mouseUpListener = null;
    this._mouseCell = [0, 0];
    
    this.badgeIcons = new Image();
    this.badgeIcons.addEventListener("load", () => this._renderSoon());
    this.badgeIcons.src = "/img/badges.png";
  }
  
  onRemoveFromDom() {
    this.resizeObserver.unobserve(this.element);
    this._dropMouseListeners();
  }
  
  setMap(map, name) {
    this.map = map;
    this.name = name;
    this._loadAuxResources().then(() => this._renderSoon());
    this._rebuildBadges();
  }
  
  /* Load tilesheet, neighbors, etc.
   ****************************************************************/
   
  _loadAuxResources() {
    const promises = [];
    const map = this.map;
    
    const tilesheetName = map.getTilesheetName();
    if (tilesheetName) {
      promises.push(
        this.resService.getImage(tilesheetName).then((image) => {
          this.tilesheet = image;
        })
        .catch(() => {})
      );
    }
    
    promises.push(
      Promise.all(["w", "e", "n", "s"]
        .map(dir => [dir, map.getNeighborName(dir)])
        .filter(([dir, name]) => name)
        .map(([dir, name]) => this.mapService.getMapByName(name)
          .then(neighbor => { this["neighbor" + dir] = neighbor; })
        )
      ).then(() => Promise.all(["nw", "ne", "sw", "se"]
        .map(dir => this._loadDiagonalNeighbor(dir))
      ))
    );
    
    return Promise.all(promises);
  }
  
  _loadDiagonalNeighbor(dir) {
    const dira = dir[0], dirb = dir[1];
    const namea = this["neighbor" + dira]?.getNeighborName(dirb);
    const nameb = this["neighbor" + dirb]?.getNeighborName(dira);
    
    if (!namea && !nameb) {
      this["neighbor" + dir] = null;
    
    } else if (!namea || !nameb) {
      // One is missing. This is normal if one of the cardinal neighbors is missing too.
      // But if both cardinal neighbors are present, it's a real mismatch.
      if (this["neighbor" + dira] && this["neighbor" + dirb]) {
        this["neighbor" + dir] = "Ambiguous!";
      } else {
        return this.mapService.getMapByName(namea || nameb)
          .then(neighbor => { this["neighbor" + dir] = neighbor; });
      }
        
    } else if (namea === nameb) {
      // Normal case. Both cardinal neighbors are present, and agree about the diagonal neighbor.
      return this.mapService.getMapByName(namea)
        .then(neighbor => { this["neighbor" + dir] = neighbor; });
        
    } else {
      // Very ambiguous. Both cardinal neighbors are present, and each has a definite and different opinion about the diagonal neighbor.
      this["neighbor" + dir] = "Ambiguous!";
        
    }
    return Promise.resolve();
  }
  
  /* Rebuild the badge list.
   * This is crazy expensive, we have to look at every other map to find entry points.
   * Expensive but also valuable, that's the kind of thing this editor exists for.
   ****************************************************************/
   
  _rebuildBadges() {
    this._badges = [];
    for (const command of this.map.commands) {
      this._generateBadgeForCommandIfNeeded(command);
    }
    this.mapService.getAllMaps().then((toc) => {
      for (const r of toc) {
        for (const command of r.object.commands) {
          if ((command[0] === "door") && (command[3] === this.name)) {
            this._addBadge(command[4], command[5], 0x00, command, r);
          }
        }
      }
      this._recalculateBadgePositions();
      this._renderSoon();
    });
  }
  
  _generateBadgeForCommandIfNeeded(command) {
    switch (command[0]) {
      case "hero": this._addBadge(command[1], command[2], 0x01, command); break;
      case "cellif": this._addBadge(command[2], command[3], 0x02, command); break;
      case "door": this._addBadge(command[1], command[2], 0x03, command); break;
      case "sprite": this._addBadge(command[1], command[2], 0x04, command); break; //TODO might want more detail for display
      case "compass": this._addBadge(command[1], command[2], 0x05, command); break;
    }
  }
  
  _addBadge(col, row, tileid, command, map) {
    this._badges.push({
      x: 0, y: 0, // position doesn't matter; we'll sweep them all in just a sec
      w: 16, h: 16,
      tileid, command, map
    });
  }
  
  _recalculateBadgePositions() {
    const tilesize = (this.xlines[2] - this.xlines[1]) / FullmoonMap.COLC;
    
    // Drop any badges whose command is no longer present.
    for (let i=this._badges.length; i-->0; ) {
      const badge = this._badges[i];
      if (
        (badge.map && (badge.map.object.commands.indexOf(badge.command) < 0)) ||
        (!badge.map && (this.map.commands.indexOf(badge.command) < 0))
      ) {
        this._badges.splice(i, 1);
      }
    }
    
    // First put everything in the upper-left corner of its target cell.
    for (const badge of this._badges) {
      const position = FullmoonMap.getCommandPosition(badge.command, badge.map);
      if (position) {
        badge.x = Math.floor(this.xlines[1] + position[0] * tilesize);
        badge.y = Math.floor(this.ylines[1] + position[1] * tilesize);
      }
    }
    
    // Now sweep again, and where there's overlap, bump them right or down.
    // Two badges on one cell is perfectly normal (think doors), but any more is rare.
    for (let ai=1; ai<this._badges.length; ai++) {
      for (;;) {
        let conflict = false;
        const a = this._badges[ai];
        for (let bi=0; bi<ai; bi++) {
          const b = this._badges[bi];
          if (a.x >= b.x + b.w) continue;
          if (a.y >= b.y + b.h) continue;
          if (a.x + a.w <= b.x) continue;
          if (a.y + a.h <= b.y) continue;
          conflict = true;
          break;
        }
        if (!conflict) break;
        const xbase = Math.floor((a.x - this.xlines[1] + 1) / tilesize) * tilesize + this.xlines[1];
        const limit = xbase + tilesize;
        a.x += a.w;
        if (a.x + a.w >= limit) {
          a.x = xbase;
          a.y += a.h;
        }
      }
    }
  }
  
  /* Render.
   ****************************************************************/
   
  _renderSoon() {
    if (this._renderTimeout) return;
    const timeout = 0; // Zero for snappy, expensive rendering as soon as possible.
    this._renderTimeout = this.window.setTimeout(() => {
      this._renderTimeout = null;
      this._renderNow();
    }, timeout);
  }
  
  _renderNow() {
    
    /* Acquire the graphics context.
     * If there's no canvas, that's cool, just get out.
     */
    const canvas = this.element;
    if (!canvas) return;
    const fullw = this.element.offsetWidth;
    const fullh = this.element.offsetHeight;
    canvas.width = fullw;
    canvas.height = fullh;
    const context = canvas.getContext("2d");
    context.imageSmoothingEnabled = false;
    
    // Blackout. Usually ends up overwritten, but if not this is the letterbox/pillarbox blotter.
    context.fillStyle = "#000";
    context.fillRect(0, 0, fullw, fullh);
    
    // Render each of the up-to-nine maps.
    for (let dx=0; dx<3; dx++) {
      for (let dy=0; dy<3; dy++) {
        const x = this.xlines[dx];
        const y = this.ylines[dy];
        const w = this.xlines[dx + 1] - x;
        const h = this.ylines[dy + 1] - y;
        switch (dy * 3 + dx) {
          case 0: this._renderMap(context, x, y, w, h, this.neighbornw, false); break;
          case 1: this._renderMap(context, x, y, w, h, this.neighborn, false); break;
          case 2: this._renderMap(context, x, y, w, h, this.neighborne, false); break;
          case 3: this._renderMap(context, x, y, w, h, this.neighborw, false); break;
          case 4: this._renderMap(context, x, y, w, h, this.map, true); break;
          case 5: this._renderMap(context, x, y, w, h, this.neighbore, false); break;
          case 6: this._renderMap(context, x, y, w, h, this.neighborsw, false); break;
          case 7: this._renderMap(context, x, y, w, h, this.neighbors, false); break;
          case 8: this._renderMap(context, x, y, w, h, this.neighborse, false); break;
        }
      }
    }
    
    // Highlight the major region boundaries.
    context.beginPath();
    for (let i=0; i<4; i++) {
      context.moveTo(this.xlines[i], this.ylines[0]);
      context.lineTo(this.xlines[i], this.ylines[3]);
      context.moveTo(this.xlines[0], this.ylines[i]);
      context.lineTo(this.xlines[3], this.ylines[i]);
    }
    context.strokeStyle = "#08f";
    context.stroke();
  }
  
  _renderMap(context, x, y, w, h, map, withDecorations) {
    if (!map) {
      context.fillStyle = "#444";
      context.fillRect(x, y, w, h);
    } else if (typeof(map) === "string") {
      context.fillStyle = "#f00";
      context.fillRect(x, y, w, h);
    } else {
      const tilesheetName = map.getTilesheetName();
      if (tilesheetName) {
        let image = null;
        try {
          image = this.resService.getImageSync(tilesheetName);
        } catch (e) {}
        if (image) {
          this._renderLoadedMap(context, x, y, w, h, map, image, withDecorations);
        } else {
          this._renderLoadedMap(context, x, y, w, h, map, null, withDecorations);
          this.resService.getImage(tilesheetName)
            .then(() => this._renderSoon())
            .catch(() => {});
        }
      } else {
        this._renderLoadedMap(context, x, y, w, h, map, null, withDecorations);
      }
    }
    // If we're not decorating it, draw a mild blotter.
    if (!withDecorations) {
      context.globalAlpha = 0.6;
      context.fillStyle = "#888";
      context.fillRect(x, y, w, h);
      context.globalAlpha = 1.0;
    }
  }
  
  _renderLoadedMap(context, x, y, w, h, map, tilesheet, withDecorations) {
    const srctilesize = tilesheet ? (tilesheet.naturalWidth >> 4) : 0;
    
    // Precalculate quantized posititions for cell boundaries.
    const xv = [], yv = [];
    for (let col=0; col<=FullmoonMap.COLC; col++) xv.push(x + Math.floor((col * w) / FullmoonMap.COLC));
    for (let row=0; row<=FullmoonMap.ROWC; row++) yv.push(y + Math.floor((row * h) / FullmoonMap.ROWC));
    
    // Draw tiles. Normally copying from tilesheet, but use arbitrary colors if we don't have it.
    for (let row=0, p=0; row<FullmoonMap.ROWC; row++) {
      const cellh = yv[row + 1] - yv[row];
      for (let col=0; col<FullmoonMap.COLC; col++, p++) {
        const cellw = xv[col + 1] - xv[col];
        if (tilesheet) {
          const srcx = (map.cellv[p] & 15) * srctilesize;
          const srcy = (map.cellv[p] >> 4) * srctilesize;
          context.drawImage(tilesheet, srcx, srcy, srctilesize, srctilesize, xv[col], yv[row], cellw, cellh);
        } else {
          context.fillStyle = this._colorForUnknownCell(map.cellv[p]);
          context.fillRect(xv[col], yv[row], cellw, cellh);
        }
      }
    }
    
    // Decorations if requested.
    if (withDecorations) {
      if (this.decorations.includes("gridLines")) this._renderGridLines(context, xv, yv);
      if (this.decorations.includes("badges")) this._renderBadges(context);
    }
  }
  
  _renderGridLines(context, xv, yv) {
    if ((xv.length < 1) || (yv.length < 1)) return;
    const xz = xv[xv.length - 1];
    const yz = yv[yv.length - 1];
    context.beginPath();
    for (let x=0; x<xv.length; x++) {
      context.moveTo(xv[x], yv[0]);
      context.lineTo(xv[x], yz);
    }
    for (let y=0; y<xv.length; y++) {
      context.moveTo(xv[0], yv[y]);
      context.lineTo(xz, yv[y]);
    }
    context.strokeStyle = "#080";
    context.stroke();
  }
  
  _renderBadges(context) {
    for (const badge of this._badges) {
      const srcx = badge.tileid * 16;
      const srcy = 0;
      context.drawImage(this.badgeIcons, srcx, srcy, 16, 16, badge.x, badge.y, badge.w, badge.h);
    }
  }
  
  _colorForUnknownCell(tileid) {
    // We need 12 bits RGB from the 8 bit tileid.
    // 3 Red, 3 Green, 2 Blue.
    // Don't overthink it; this is only for a fallback when images fail to load.
    let r = tileid >> 5; r <<= 1; r |= r >> 3;
    let g = (tileid >> 2) & 7; g <<= 1; g |= g >> 3;
    let b = tileid & 3; b |= b << 2;
    r = "0123456789abcdef"[r];
    g = "0123456789abcdef"[g];
    b = "0123456789abcdef"[b];
    return "#" + r + g + b;
  }
  
  _recalculateBoundaries(fullw, fullh) {
    const minFullSize = 100;
    if ((fullw < minFullSize) || (fullh < minFullSize)) {
      this.xlines = [0, 0, 0, 0];
      this.ylines = [0, 0, 0, 0];
      return false;
    }
    const centralPortionLimit = 0.80; // neighbors get minimum 1/8 of middle
    let midw, midh;
    {
      const availw = fullw * centralPortionLimit;
      const availh = fullh * centralPortionLimit;
      const wforh = (FullmoonMap.COLC * availh) / FullmoonMap.ROWC;
      if (wforh <= availw) {
        midw = Math.floor(wforh);
        midh = Math.floor(availh);
      } else {
        midw = Math.floor(availw);
        midh = Math.floor((availw * FullmoonMap.ROWC) / FullmoonMap.COLC);
      }
    }
    this.xlines = [
      (fullw >> 1) - ((midw * 3) >> 1),
      (fullw >> 1) - (midw >> 1),
      (fullw >> 1) + (midw >> 1),
      (fullw >> 1) + ((midw * 3) >> 1),
    ];
    this.ylines = [
      (fullh >> 1) - ((midh * 3) >> 1),
      (fullh >> 1) - (midh >> 1),
      (fullh >> 1) + (midh >> 1),
      (fullh >> 1) + ((midh * 3) >> 1),
    ];
    return true;
  }
  
  /* Mouse events.
   ******************************************************************************/
   
  _dropMouseListeners() {
    if (this._mouseUpListener) {
      this.window.removeEventListener("mouseup", this._mouseUpListener);
      this._mouseUpListener = null;
    }
    if (this._mouseMoveListener) {
      this.window.removeEventListener("mousemove", this._mouseMoveListener);
      this._mouseMoveListener = null;
    }
    this.mapPaintService.cancel();
  }
  
  onMouseUp(event) {
    this.mapPaintService.mouseUp();
    this._dropMouseListeners();
  }
    
  onMouseMove(event) {
    const resolved = this._resolveMousePosition(event.x, event.y);
    if (!resolved) return;
    if (resolved.neighbor) return;
    if ((resolved.col === this._mouseCell[0]) && (resolved.row === this._mouseCell[1])) return;
    this._mouseCell = [resolved.col, resolved.row];
    this.mapPaintService.mouseMove(resolved.col, resolved.row);
    if (this._mustRepositionBadges) {
      this._recalculateBadgePositions();
      this._renderSoon();
    }
  }
   
  // null | { neighbor: "n"|"s"|"e"|"w"|"ne"|"nw"|"se"|"sw" } | { col, row, badge? }
  _resolveMousePosition(x, y) {
  
    // Don't have maps geometry? Dead space.
    if (this.xlines[3] <= this.xlines[0]) return null;
    if (this.ylines[3] <= this.ylines[0]) return null;
    
    const bounds = this.element.getBoundingClientRect();
    x -= bounds.x;
    y -= bounds.y;
    
    // Outside the 9 maps? Dead space.
    if (x < this.xlines[0]) return null;
    if (y < this.ylines[0]) return null;
    if (x >= this.xlines[3]) return null;
    if (y >= this.ylines[3]) return null;
    
    // Check for neighbors via (xlines,ylines)
    if (x < this.xlines[1]) { // NW,W,SW
      if (y < this.ylines[1]) return { neighbor: "nw" };
      if (y < this.ylines[2]) return { neighbor: "w" };
      return { neighbor: "sw" };
    } else if (x >= this.xlines[2]) { // NE,E,SE
      if (y < this.ylines[1]) return { neighbor: "ne" };
      if (y < this.ylines[2]) return { neighbor: "e" };
      return { neighbor: "se" };
    } else { // N,S,main
      if (y < this.ylines[1]) return { neighbor: "n" };
      if (y >= this.ylines[2]) return { neighbor: "s" };
    }
    
    // Inside the main, calculate based on floating-point tile size.
    // This isn't exactly what we do when rendering, but should always produce the right answer.
    const mapw = this.xlines[2] - this.xlines[1];
    const maph = this.ylines[2] - this.ylines[1];
    const colw = mapw / FullmoonMap.COLC;
    const rowh = maph / FullmoonMap.ROWC;
    let col = Math.floor((x - this.xlines[1]) / colw);
    let row = Math.floor((y - this.ylines[1]) / rowh);
    if (col < 0) col = 0; else if (col >= FullmoonMap.COLC) col = FullmoonMap.COLC - 1;
    if (row < 0) row = 0; else if (row >= FullmoonMap.ROWC) row = FullmoonMap.ROWC - 1;
    
    // Check badges.
    // When these match, we still report everything as with regular cell motion.
    // So the caller can decide whether she cares about badges.
    for (const badge of this._badges) {
      if (x < badge.x) continue;
      if (y < badge.y) continue;
      if (x >= badge.x + badge.w) continue;
      if (y >= badge.y + badge.h) continue;
      return { col, row, badge };
    }
    
    return { col, row };
  }
   
  onMouseDown(event) {
    event.stopPropagation();
    event.preventDefault();
    this._dropMouseListeners();
    const position = this._resolveMousePosition(event.x, event.y);
    if (!position) return;
    if (position.neighbor) {
      this._clickNeighbor(position.neighbor);
      return;
    }
    const modifiers = this.mapPaintService.modifiersFromEvent(event);
    if (position.badge) {
      if (!this.mapPaintService.mouseDownInBadge(position.col, position.row, position.badge, modifiers)) {
        // Common enough for one-shot add/remove/etc commands to modify the badge set.
        this._recalculateBadgePositions();
        this._renderSoon();
        return;
      }
      this._mustRepositionBadges = true;
    } else {
      if (!this.mapPaintService.mouseDown(position.col, position.row, modifiers)) return;
      this._mustRepositionBadges = false;
    }
    this._mouseCell = [position.col, position.row];
    this._mouseUpListener = (event) => this.onMouseUp(event);
    this._mouseMoveListener = (event) => this.onMouseMove(event);
    this.window.addEventListener("mouseup", this._mouseUpListener);
    this.window.addEventListener("mousemove", this._mouseMoveListener);
  }
  
  _clickNeighbor(dir) {
    const neighbor = this["neighbor" + dir];
    if (!neighbor) {
      this._promptAndCreateNeighbor(dir);

    } else if (typeof(neighbor) === "string") {
      console.log(`TODO Resolve ambiguous neighbor ${dir}: ${neighbor}`);
      //TODO This arises when our two cardinal neighbors disagree about their common neighbor, our diagonal.
      // Figure out how we want to resolve it. (interactively, of course)

    } else {
      const name = this.getNeighborName(dir);
      if (name) {
        this.onNavigate(name);
      }
    }
  }
  
  getNeighborName(dir) {
    // The map knows its cardinal neighbor names. Try that first.
    let name = this.map.getNeighborName(dir);
    if (name) return name;
    // Failed? OK assume it's diagonal. The two characters of (dir) are candidate cardinal neighbors.
    if (dir.length !== 2) return "";
    const dira = dir[0], dirb = dir[1];
    const neighbora = this["neighbor" + dira];
    const neighborb = this["neighbor" + dirb];
    if (neighbora instanceof FullmoonMap) {
      if (name = neighbora.getNeighborName(dirb)) return name;
    }
    if (neighborb instanceof FullmoonMap) {
      if (name = neighborb.getNeighborName(dira)) return name;
    }
    return "";
  }
  
  _promptAndCreateNeighbor(dir) {
    this.resService.generateUnusedName("map").then((name) => {
      if (!name) return;
      if (!(name = this.window.prompt(`Name for new map, ${dir} of this one:`, name))) return;
      this.onCreateNeighbor(dir, name);
    });
  }
}
