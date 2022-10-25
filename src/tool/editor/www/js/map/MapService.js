/* MapService.js
 */
 
import { ResService } from "/js/ResService.js";
import { FullmoonMap } from "/js/map/FullmoonMap.js";
 
export class MapService {
  static getDependencies() {
    return [Window, ResService];
  }
  constructor(window, resService) {
    this.window = window;
    this.resService = resService;
  }
  
  promptForNewMap(name) {
    return new Promise((resolve, reject) => {
      if (!name) name = this.window.prompt("New map name:", `map${Date.now()}`);
      if (!name) return reject("Cancelled");
      if (!name.match(/^[0-9a-zA-Z_]+$/)) {
        this.window.alert(`Invalid map name. Please use a C identifier.`);
        return reject("Cancelled");
      }
      this.resService.requireToc().then((toc) => {
        if (toc.find(res => ((res.type === "map") && (res.name === name)))) {
          this.window.alert(`Map name '${name}' already in use.`);
          reject("Cancelled");
        } else {
          this.resService.addResource("map", name, `src/data/map/${name}`, this.generateBlankMap()).then(() => {
            resolve(name);
          }).catch(e => reject(e));
        }
      }).catch(e => reject(e));
    });
  }
  
  generateBlankMap() {
    let text = "";
    let row = "";
    for (let i=FullmoonMap.COLC; i-->0; ) row += "00";
    row += "\n";
    for (let i=FullmoonMap.ROWC; i-->0; ) text += row;
    text += "\n"
    text += "tilesheet outdoors\n";
    return text;
  }
  
  decode(text) {
    const map = new FullmoonMap();
    map.decode(text);
    return map;
  }
  
  encode(map) {
    return map ? map.encode() : this.generateBlankMap();
  }
  
  getMapByName(name) {
    return this.resService.getTocEntry("map", name).then(r => {
      if (!r.object) r.object = this.decode(r.text);
      return r.object;
    });
  }
  
  getAllMaps() {
    return this.resService.requireToc()
      .then((toc) => toc.filter(r => r.type === "map"))
      .then((toc) => {
        for (const r of toc) {
          if (!r.object) r.object = this.decode(r.text);
        }
        return toc;
      });
  }
  
  oppositeDir(dir) {
    switch (dir) {
      case "nw": return "se";
      case "n": return "s";
      case "ne": return "sw";
      case "w": return "e";
      case "e": return "w";
      case "sw": return "ne";
      case "s": return "n";
      case "se": return "se";
    }
    return dir;
  }
  
  /* Given a map resource whose object has at least one neighbor, walk around the neighborhood and find other connections.
   * Reads other map resources synchronously off ResService.
   * We're not exhaustive or anything, and might miss things in cases where the neighborhood is poorly populated.
   * The assumption is that (r) is a new map with just one neighbor.
   * (but we do promise not to do anything stupid).
   */
  discoverNeighbors(r) {
    if (!r?.object) return;
    this._discoverNeighbors1(r, "n", "e", ["s", "e"], "s", ["w", "s"], "w", ["n", "w"]);
    this._discoverNeighbors1(r, "n", "w", ["s", "w"], "s", ["e", "s"], "e", ["n", "e"]);
    this._discoverNeighbors1(r, "e", "s", ["w", "s"], "w", ["n", "w"], "n", ["e", "n"]);
    this._discoverNeighbors1(r, "e", "n", ["w", "n"], "w", ["s", "w"], "s", ["e", "s"]);
    this._discoverNeighbors1(r, "s", "e", ["n", "e"], "n", ["w", "n"], "w", ["s", "w"]);
    this._discoverNeighbors1(r, "s", "w", ["n", "w"], "n", ["e", "n"], "e", ["s", "e"]);
    this._discoverNeighbors1(r, "w", "n", ["e", "n"], "e", ["s", "e"], "s", ["w", "s"]);
    this._discoverNeighbors1(r, "w", "s", ["e", "s"], "e", ["n", "e"], "n", ["w", "n"]);
  }
  _discoverNeighbors1(r, ...path) {
    let p = r;
    for (const step of path) {
      let name = null;
      if (typeof(step) === "string") name = p.object.getNeighborName(step);
      else name = p.object.getNeighborName(step[0]);
      if (!name) return;
      let next = null;
      try { next = this.resService.getTocEntrySync("map", name); }
      catch (e) { return; }
      if (typeof(step) !== "string") {
        r.object.setNeighborName(step[1], next.name);
        if (!next.object) next.object = this.decode(next.text);
        if (!next.object.getNeighborName(this.oppositeDir(step[1]))) {
          next.object.setNeighborName(this.oppositeDir(step[1]), r.name);
          this.resService.dirty("map", next.name, () => next.object.encode());
        }
      }
      p = next;
    }
  }
}

MapService.singleton = true;
