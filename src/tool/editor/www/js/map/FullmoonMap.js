/* FullmoonMap.js
 * One "map" resource, the live object.
 * Javascript has a standard class called "Map" so that's out.
 */
 
export class FullmoonMap {
  constructor() {
    this.cellv = new Uint8Array(FullmoonMap.COLC * FullmoonMap.ROWC);
    this.commands = []; // Each is an array of strings, 1:1 with tokens in the text file.
  }
  
  decode(src) {
    if (src instanceof ArrayBuffer) {
      src = new TextDecoder("utf8").decode(src);
    }
    // First 8 lines must be the cell content as a strictly-formatted hex dump.
    // After that it's a bit looser, each non-empty line is a command.
    this.commands = [];
    let srcp = 0, lineno = 0;
    while (srcp < src.length) {
      lineno++;
      let nlp = src.indexOf("\n", srcp);
      if (nlp < 0) nlp = src.length;
      const line = src.substring(srcp, nlp);
      srcp = nlp + 1;
      if (lineno <= FullmoonMap.ROWC) {
        this._setRowFromText(lineno - 1, line);
      } else {
        const words = line.split(/\s+/).filter(w => w);
        if (words.length) this.commands.push(words);
      }
    }
  }
  
  encode() {
    let dst = "";
    for (let y=FullmoonMap.ROWC, p=0; y-->0; ) {
      for (let x=FullmoonMap.COLC; x-->0; p++) {
        dst += (this.cellv[p] || 0).toString(16).padStart(2, '0');
      }
      dst += "\n";
    }
    dst += "\n";
    for (const command of this.commands) {
      dst += command.join(" ") + "\n";
    }
    return dst;
  }
  
  _setRowFromText(y, src) {
    if (src.length !== FullmoonMap.COLC * 2) {
      throw new Error(`Invalid length ${src.length} for map row ${y}`);
    }
    let cellp = y * FullmoonMap.COLC;
    let srcp = 0;
    for (; srcp < src.length; cellp++, srcp+=2) {
      const v = parseInt(src.substr(srcp, 2), 16);
      if (isNaN(v)) throw new Error(`Malformed hex byte at col ${srcp}, row ${y}`);
      this.cellv[cellp] = v;
    }
  }
  
  /* Returns an array of string ([0] === keyword).
   * The returned array is live; if you modify it, we are modified.
   */
  getCommand(keyword, index = 0) {
    for (const command of this.commands) {
      if (command[0] !== keyword) continue;
      if (!index--) return command;
    }
    return null;
  }
  
  /* Friendly command accessors.
   ******************************************************************/
   
  getSingleResource(keyword) {
    const command = this.getCommand(keyword);
    return command?.[1] || "";
  }
  
  setSingleResource(keyword, name) {
    if (name) {
      const command = this.getCommand(keyword);
      if (command) {
        command[1] = name;
      } else {
        this.commands.push([keyword, name]);
      }
    } else {
      for (;;) {
        const p = this.commands.findIndex(c => c[0] === keyword);
        if (p < 0) break;
        this.commands.splice(p, 1);
      }
    }
  }
   
  getTilesheetName() { return this.getSingleResource("tilesheet"); }
  setTilesheetName(name) { this.setSingleResource("tilesheet", name); }
  
  getSongName() { return this.getSingleResource("song"); }
  setSongName(name) { this.setSingleResource("song", name); }
  
  // ("n","s","w","e")
  getNeighborName(dir) { return this.getSingleResource("neighbor" + dir); }
  setNeighborName(dir, name) { this.setSingleResource("neighbor" + dir, name); }
  
  getHomeName() { return this.getSingleResource("home"); }
  setHomeName(name) { this.setSingleResource("home", name); }
  
  // => [col,row] with default
  getHero() {
    const command = this.getCommand("hero");
    if (!command) return [FullmoonMap.COLC >> 1, FullmoonMap.ROWC >> 1];
    return [~~command[1], ~~command[2]];
  }
  
  setHero(col, row) {
    const command = this.getCommand("hero");
    if (command) {
      command[1] = col;
      command[2] = row;
    } else {
      this.commands.push(["hero", col, row]);
    }
  }
  
  // [x,y] or null. (remote) true if this command comes from some other map (eg doors, show me the Exit side)
  static getCommandPosition(command, remote) {
    switch (command[0]) {
      case "door": {
          if (remote) return [+command[4], +command[5]];
          return [+command[1], +command[2]];
        }
      case "hero": return [+command[1], +command[2]];
      case "cellif": return [+command[2], +command[3]];
      case "sprite": return [+command[1], +command[2]];
    }
    return null;
  }
  
  // True if changed.
  static setCommandPosition(command, x, y, remote) {
    if ((x < 0) || (x >= FullmoonMap.COLC)) return false;
    if ((y < 0) || (y >= FullmoonMap.ROWC)) return false;
    x = x.toString();
    y = y.toString();
    switch (command[0]) {
      case "door": {
          if (remote) {
            command[4] = x;
            command[5] = y;
          } else {
            command[1] = x;
            command[2] = y;
          }
          return true;
        }
      case "hero": command[1] = x; command[2] = y; return true;
      case "cellif": command[2] = x; command[3] = y; return true;
      case "sprite": command[1] = x; command[2] = y; return true;
    }
    return false;
  }
  
  static getCommandRemoteName(command) {
    switch (command[0]) {
      case "door": return command[3];
      case "home": return command[1];
    }
    return null;
  }
  
}

FullmoonMap.COLC = 12;
FullmoonMap.ROWC = 8;

FullmoonMap.COMMANDS_SCHEMA = [{
  keyword: "tilesheet",
  args: ["image"],
}, {
  keyword: "song",
  args: ["song"],
}, {
  keyword: "hero",
  args: ["x", "y"],
}, {
  keyword: "neighborw",
  args: ["map"],
}, {
  keyword: "neighbore",
  args: ["map"],
}, {
  keyword: "neighborn",
  args: ["map"],
}, {
  keyword: "neighbors",
  args: ["map"],
}, {
  keyword: "home",
  args: ["map"],
}, {
  keyword: "event",
  args: ["event", "hook", "args"],//TODO loose data
}, {
  keyword: "cellif",
  args: ["flag", "x", "y", "tile"],
}, {
  keyword: "door",
  args: ["x", "y", "map", "dstx", "dsty"],
}, {
  keyword: "sprite",
  args: ["x", "y", "controller", "args"],//TODO loose data
}];
