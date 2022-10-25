/* MapPaintService.js
 * Manages editing a map with the mouse.
 */
 
import { FullmoonMap } from "./FullmoonMap.js";
 
export class MapPaintService {
  static getDependencies() {
    return [];
  }
  constructor() {
  
    // Owner should set:
    this.tileprops = null;
    this.onDirty = (map/*optional*/) => {};
    this.onSelectedTileChanged = (tileid) => {};
    this.onNavigate = (nameOrObject) => {};
    this.onEditCommand = (command, map) => {};
    this.onAddCommand = (x, y) => {};
  
    this.toolNames = [
      "rainbow",  // Modify cell and join with neighbors.
      "pencil",   // Modify cell strictly.
      "pickup",   // Grab cell value as current selection.
      "navigate", // Enter doors.
      "add",      // Create command at cell.
      "delete",   // Remove command.
      "drag",     // Move command to another cell.
      "edit",     // Open command modal.
      "mural",    // Copy from tilesheet according to position re anchor.
    ];
  
    // null or tool name:
    this.toolLeft = "rainbow";
    this.toolRight = "pickup";
    this._inProgress = null;
    
    this.selectedTile = 0x00;
    
    this._map = null;
    this._badge = null; // present when badge op in progress
    this._anchor = [0, 0]; // mural
  }
  
  setMap(map) {
    this._map = map;
  }
  
  setToolLeft(name) {
    if (name === this.toolLeft) return;
    this.toolLeft = name;
  }
  
  setSelectedTile(tileid) {
    if (tileid === this.selectedTile) return;
    this.selectedTile = tileid;
    this.onSelectedTileChanged(tileid);
  }
  
  modifiersFromEvent(event) {
    let modifiers = 0;
    if (event.altKey) modifiers |= MapPaintService.MODIFIER_ALT;
    if (event.shiftKey) modifiers |= MapPaintService.MODIFIER_SHIFT;
    if (event.ctrlKey) modifiers |= MapPaintService.MODIFIER_CTRL;
    if (event.button > 0) modifiers |= MapPaintService.MODIFIER_RIGHT;
    return modifiers;
  }
  
  modifyTool(toolName, modifiers) {
  
    // Without (tileprops), the Rainbow Pencil is just a Pencil.
    if (!this.tileprops && (toolName === "rainbow")) toolName = "pencil";
  
    if (modifiers & MapPaintService.MODIFIER_CTRL) return "pickup";
    if (modifiers & MapPaintService.MODIFIER_SHIFT) return "navigate";
    return toolName;
  }
  
  /* Events from owner.
   * You must convert mouse coordinates to map cells.
   *****************************************************************/
   
  // true to begin tracking: We'll expect mouseMove() and eventually mouseUp().
  mouseDown(x, y, modifiers) {
    if (!this._map) return false;
    if ((x < 0) || (x >= FullmoonMap.COLC)) return false;
    if ((y < 0) || (y >= FullmoonMap.ROWC)) return false;
    this.cancel();
    let tool = null;
    this._badge = null;
    if (modifiers & MapPaintService.MODIFIER_RIGHT) {
      tool = this.modifyTool(this.toolRight, modifiers);
    } else {
      tool = this.modifyTool(this.toolLeft, modifiers);
    }
    if (!tool) return false;
    if (!this._begin(tool, x, y)) return false;
    this._toolInProgress = tool;
    return true;
  }
  
  // Start here if a badge is clicked on. We may or may not use it, you don't need to care.
  mouseDownInBadge(x, y, badge, modifiers) {
    if (!this._map) return false;
    if (!badge) return false;
    if ((x < 0) || (x >= FullmoonMap.COLC)) return false;
    if ((y < 0) || (y >= FullmoonMap.ROWC)) return false;
    this.cancel();
    let tool = null;
    this._badge = badge;
    if (modifiers & MapPaintService.MODIFIER_RIGHT) {
      tool = this.modifyTool(this.toolRight, modifiers);
    } else {
      tool = this.modifyTool(this.toolLeft, modifiers);
    }
    if (!tool) return false;
    if (!this._begin(tool, x, y)) return false;
    this._toolInProgress = tool;
    return true;
  }
  
  mouseMove(x, y) {
    if (!this._toolInProgress) return;
    if ((x < 0) || (x >= FullmoonMap.COLC)) return;
    if ((y < 0) || (y >= FullmoonMap.ROWC)) return;
    this._move(this._toolInProgress, x, y);
  }
  
  mouseUp() {
    if (!this._toolInProgress) return;
    this._end(this._toolInProgress);
    this._toolInProgress = null;
  }
  
  cancel() {
    if (!this._toolInProgress) return;
    this._end(this._toolInProgress);
    this._toolInProgress = null;
  }
  
  /* Mouse events, with tool resolved.
   *******************************************************************/
   
  _begin(tool, x, y) {
    switch (tool) {
      case "rainbow": this._rainbow(x, y); return true;
      case "pencil": this._pencil(x, y); return true;
      case "pickup": this._pickup(x, y); return true;
      case "navigate": this._navigate(x, y); return false;
      case "add": this._add(x, y); return false;
      case "delete": this._delete(x, y); return false;
      case "drag": this._drag(x, y); return true;
      case "edit": this._edit(x, y); return false;
      case "mural": this._muralBegin(x, y); return true;
    }
    return false;
  }
  
  _move(tool, x, y) {
    switch (tool) {
      case "rainbow": this._rainbow(x, y); break;
      case "pencil": this._pencil(x, y); break;
      case "pickup": this._pickup(x, y); break;
      case "drag": this._drag(x, y); break;
      case "mural": this._muralContinue(x, y); break;
    }
  }
  
  _end(tool) {
    switch (tool) {
    }
  }
  
  /* Paint tools.
   ********************************************************************/
   
  _rainbow(x, y) {
    const p = y * FullmoonMap.COLC + x;
    this._map.cellv[p] = this.selectedTile;
    for (let dy=-1; dy<=1; dy++) {
      for (let dx=-1; dx<=1; dx++) {
        this._joinNeighbors(x + dx, y + dy, !dx && !dy);
      }
    }
    this.onDirty();
  }
  
  _pencil(x, y) {
    const p = y * FullmoonMap.COLC + x;
    if (this._map.cellv[p] === this.selectedTile) return;
    this._map.cellv[p] = this.selectedTile;
    this.onDirty();
  }
  
  _pickup(x, y) {
    const p = y * FullmoonMap.COLC + x;
    if (this.selectedTile === this._map.cellv[p]) return;
    this.selectedTile = this._map.cellv[p];
    this.onSelectedTileChanged(this.selectedTile);
  }
  
  _muralBegin(x, y) {
    this._anchor = [x, y];
    const p = y * FullmoonMap.COLC + x;
    this._map.cellv[p] = this.selectedTile;
    this.onDirty();
  }
  
  _muralContinue(x, y) {
    const dx = x - this._anchor[0];
    const dy = y - this._anchor[1];
    const col = (this.selectedTile & 15) + dx;
    const row = (this.selectedTile >> 4) + dy;
    if ((col < 0) || (col >= 16)) return;
    if ((row < 0) || (row >= 16)) return;
    this._map.cellv[y * FullmoonMap.COLC + x] = (row << 4) | col;
    this.onDirty();
  }
  
  /* Command tools.
   ***************************************************************/
   
  _navigate(x, y) {
    if (!this._badge) return;
    if (this._badge.map) {
      this.onNavigate(this._badge.map);
    } else {
      const name = FullmoonMap.getCommandRemoteName(this._badge.command);
      if (name) this.onNavigate(name);
    }
  }
   
  _add(x, y) {
    this.onAddCommand(x, y);
  }
   
  _delete(x, y) {
    if (!this._badge) return;
    const map = this._badge.map || this._map;
    const p = map.commands.indexOf(this._badge.command);
    if (p < 0) return;
    map.commands.splice(p, 1);
    this.onDirty(this._badge.map);
  }
   
  _drag(x, y) {
    if (!this._badge) return;
    if (!FullmoonMap.setCommandPosition(this._badge.command, x, y, this._badge.map && this._badge.map !== this._map)) return;
    this.onDirty(this._badge.map);
  }
   
  _edit(x, y) {
    if (!this._badge) return;
    this.onEditCommand(this._badge.command, this._badge.map || this._map);
  }
  
  /* Neighbor joining (rainbow pencil)
   ********************************************************************/
   
  _joinNeighbors(x, y, randomize) {
  
    // Ensure this cell is in bounds and determine its family.
    // Family zero never adjusts, so get out early in that case.
    if ((x < 0) || (x >= FullmoonMap.COLC)) return;
    if ((y < 0) || (y >= FullmoonMap.ROWC)) return;
    const p = y * FullmoonMap.COLC + x;
    const family = this.tileprops.family[this._map.cellv[p]];
    if (!family) return;
    
    // Generate an 8-bit mask of my cardinal and diagonal neighbors.
    const neighbors = this._identifyNeighbors(x, y);
    
    // Find all possible tileid in my family.
    // Discard any that require bits not in my neighbor mask.
    // Gather a parallel array of bit counts from each mask.
    const bitCounts = [];
    let candidates = this._getTileidsInFamily(family).filter(ntileid => {
      if (this.tileprops.neighbors[ntileid] & ~neighbors) return false;
      bitCounts.push(this._popcnt8(this.tileprops.neighbors[ntileid]));
      return true;
    });
    if (candidates.length < 1) return;
    
    // Eliminate any candidate whose count is not the maximum.
    // (this can't eliminate all of them).
    const maxCount = Math.max(...bitCounts);
    for (let i=candidates.length; i-->0; ) {
      if (bitCounts[i] !== maxCount) candidates.splice(i, 1);
    }
    
    // If we have just one candidate, use it and we're done.
    if (candidates.length === 1) {
      this._map.cellv[p] = candidates[0];
      return;
    }
    
    // If the current value of this cell is among my candidates and we're not randomizing, keep it.
    if (!randomize && candidates.includes(this._map.cellv[p])) return;
    
    // Collect the probability for each candidate.
    // If they are all "appointment only", pretend they are all equally likely -- it means we fucked up in designing the tilesheet.
    // Otherwise, eliminate any "appointment only".
    let probabilities = candidates.map(tileid => this.tileprops.probability[tileid]);
    if (Math.min(...probabilities) === 255) {
      probabilities = probabilities.map(() => 0);
    } else {
      for (let i=probabilities.length; i-->0; ) {
        if (probabilities[i] === 255) {
          probabilities.splice(i, 1);
          candidates.splice(i, 1);
        }
      }
      if (candidates.length === 1) {
        this._map.cellv[p] = candidates[0];
        return;
      }
    }
    
    // Find the highest probability (ie the least likely, sorry, poor choice of name).
    // Reverse everything according to that. A "0" is eleven times more likely than a "10".
    const range = Math.max(...probabilities) + 1;
    probabilities = probabilities.map(p => range - p);
    let v = Math.floor(Math.random() * probabilities.reduce((a, v) => a + v, 0));
    for (let i=0; i<candidates.length; i++) {
      v -= probabilities[i];
      if (v <= 0) {
        this._map.cellv[p] = candidates[i];
        return;
      }
    }
  }
  
  _popcnt8(v) {
    return (
      ((v & 0x80) ? 1 : 0) +
      ((v & 0x40) ? 1 : 0) +
      ((v & 0x20) ? 1 : 0) +
      ((v & 0x10) ? 1 : 0) +
      ((v & 0x08) ? 1 : 0) +
      ((v & 0x04) ? 1 : 0) +
      ((v & 0x02) ? 1 : 0) +
      ((v & 0x01) ? 1 : 0)
    );
  }
  
  _identifyNeighbors(x, y) {
    const family = this.tileprops.family[this._map.cellv[y * FullmoonMap.COLC + x]];
    let neighbors = 0;
    for (let dy=-1, mask=0x80; dy<=1; dy++) {
      for (let dx=-1; dx<=1; dx++) {
        if (!dx && !dy) continue;
        // Clamp (nx,ny) instead of filtering: OOB cells we pretend are their nearest valid neighbor.
        let nx = x + dx;
        if (nx < 0) nx = 0;
        else if (nx >= FullmoonMap.COLC) nx = FullmoonMap.COLC - 1;
        let ny = y + dy;
        if (ny < 0) ny = 0;
        else if (ny >= FullmoonMap.ROWC) ny = FullmoonMap.ROWC - 1;
        if (this.tileprops.family[this._map.cellv[ny * FullmoonMap.COLC + nx]] === family) {
          neighbors |= mask;
        }
        mask >>= 1;
      }
    }
    return neighbors;
  }
  
  _getTileidsInFamily(family) {
    const tileids = [];
    for (let i=0; i<256; i++) {
      if (this.tileprops.family[i] === family) tileids.push(i);
    }
    return tileids;
  }
}

MapPaintService.singleton = true;

MapPaintService.MODIFIER_ALT = 1;
MapPaintService.MODIFIER_SHIFT = 2;
MapPaintService.MODIFIER_CTRL = 4;
MapPaintService.MODIFIER_RIGHT = 8;
