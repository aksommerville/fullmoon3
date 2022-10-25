/* ResService.js
 */
 
import { Comm } from "/js/core.js";
 
export class ResService {
  static getDependencies() {
    return [Comm, Window];
  }
  constructor(comm, Window) {
    this.comm = comm;
    this.window = window;
    
    this._listeners = []; // {id,k,cb}
    this._nextListenerId = 1;
    this._refreshInProgress = null;
    
    this.toc = []; // {type,name,path,(base64),(text),(desc),(object)}
    
    this._pendingChanges = []; // {type,name,generateText}
    this._pendingChangesTimeout = null;
    this.pendingChangesDebounceTime = 1000;
    
    this.refresh();
  }
  
  /* Get content.
   **********************************************************/
   
  getTocEntrySync(type, name) {
    const r = this.toc.find(r => r.type === type && r.name === name);
    if (!r) throw new Error(`Resource ${type}:${name} not found`);
    return r;
  }
  
  getTocEntry(type, name) {
    return this.requireToc().then(() => this.getTocEntrySync(type, name));
  }
  
  requireToc() {
    if (this._refreshInProgress) return this._refreshInProgress;
    if (this.toc.length) return Promise.resolve(this.toc);
    return this.refresh();
  }
  
  getImageSync(name) {
    const r = this.getTocEntrySync("image", name);
    if (!r.object) throw new Error(`image:${name} exists but not loaded`);
    return r.object;
  }
  
  getImage(name) {
    return this.getTocEntry("image", name)
      .then(r => {
        if (r.object) return r.object;
        return new Promise((resolve, reject) => {
          const image = new Image();
          image.addEventListener("load", () => {
            r.object = image;
            this.broadcast(`image:${name}`, image);
            resolve(image);
          });
          image.addEventListener("error", () => {
            reject(error);
          });
          image.src = r.path;
        });
      });
  }
  
  getNameForObject(object) {
    if (!object) return Promise.reject(`invalid object`);
    return this.requireToc().then(() => {
      for (const r of this.toc) {
        if (r.object === object) return r.name;
      }
      throw new Error(`object not found`);
    });
  }
  
  /* Modify resources.
   **************************************************************/
  
  generateUnusedName(type) {
    return this.requireToc().then(() => {
      const subtoc = this.toc.filter(r => r.type === type);
      let suffix = Date.now();
      for (;; suffix++) {
        const name = type + suffix;
        if (!subtoc.find(r => r.name === name)) return name;
      }
    });
  }
  
  addResource(type, name, path, text) {
    if (!type || !name) return Promise.reject(new Error(`Invalid resource type or name`));
    return this.requireToc()
      .then(() => {
        if (this.toc.find(r => r.type === type && r.name === name)) {
          throw new Error(`Resource ${type}:${name} already exists`);
        }
      })
      .then(() => this.comm.fetch("PUT", "/" + path, null, null, text))
      .then(() => {
        const r = { type, name, path, text };
        this.toc.push(r);
        this._sortToc();
        this.broadcast("toc", this.toc);
        this.broadcast(`${type}:${name}`, r);
      });
  }
  
  dirty(type, name, generateText) {
    if (!type || !name) return Promise.reject(new Error(`Invalid resource type or name`));
    return this.requireToc()
      .then(() => {
        const r = this.toc.find(r => r.type === type && r.name === name);
        if (!r) throw new Error(`Resource ${type}:${name} not found`);
        this._addPendingChange(type, name, generateText);
      });
  }
  
  // In case you have an object reference but not type or name.
  dirtyObject(object, generateText) {
    if (!object) return;
    return this.requireToc().then(() => {
      for (const r of this.toc) {
        if (r.object === object) {
          this._addPendingChange(r.type, r.name, generateText);
          return;
        }
      }
    });
  }
  
  /* Listen for changes.
   * (k) is a string:
   *  - "toc" for all changes to the set of known resources. Payload is (this.toc).
   *  - "TYPE:NAME" for changes to content of one resource. Payload is TOC entry, or null if it gets deleted.
   **********************************************************/
   
  listen(k, cb) {
    if (!cb) throw new Error(`ResService.listen: Callback required`);
    const id = this._nextListenerId++;
    this._listeners.push({id, k, cb});
    return id;
  }
  
  unlisten(id) {
    const p = this._listeners.findIndex(l => l.id === id);
    if (p >= 0) this._listeners.splice(p, 1);
  }
  
  broadcast(k, payload) {
    this._listeners.filter(l => l.k === k).forEach(l => l.cb(payload));
  }
  
  /* Refresh.
   ****************************************************************/
   
  refresh() {
    this.flushPendingChanges();
    if (this._refreshInProgress) return this._refreshInProgress;
    if (this.toc.length) {
      this.toc = [];
      this.broadcast("toc", this.toc);
      for (const {k, cb} of this._listeners) {
        if (k.indexOf(":") >= 0) {
          cb(null);
        }
      }
    }
    return this._refreshInProgress = this.comm.fetchJson("GET", "/api/resall")
      .then(toc => {
        this._refreshInProgress = null;
        this.toc = toc;
        this._sortToc();
        this.broadcast("toc", this.toc);
        for (const res of this.toc) {
          this.broadcast(`${res.type}:${res.name}`, res);
        }
        return this.toc;
      })
      .catch(e => {
        this._refreshInProgress = null;
        this.window.console.error(`Failed to load resource TOC`, e);
        return this.toc;
      });
  }
  
  _sortToc() {
    this.toc.sort((a, b) => {
      if (a.type < b.type) return -1;
      if (a.type > b.type) return 1;
      if (a.name < b.name) return -1;
      if (a.name > b.name) return 1;
      return 0;
    });
  }
  
  /* Pending changes.
   ***********************************************************/
   
  flushPendingChanges() {
    if (!this._pendingChangesTimeout) return;
    this.window.clearTimeout(this._pendingChangesTimeout);
    for (const { type, name, generateText } of this._pendingChanges) {
    
      // Appendix types.
      if (type === "tileprops") {
        this.comm.fetch("PUT", `/src/data/image/${name}.tileprops`, null, null, generateText());
        continue;
      }
      if (type === "adjust") {
        this.comm.fetch("PUT", `/src/data/song/${name}.adjust`, null, null, generateText());
        continue;
      }
    
      // Regular types.
      const r = this.toc.find(r => r.type === type && r.name === name);
      if (!r) continue;
      r.text = generateText();
      this.comm.fetch("PUT", "/" + r.path, null, null, r.text);
      this.broadcast(`${r.type}:${r.name}`, r);
    }
    this._pendingChanges = [];
    this._pendingChangesTimeout = null;
  }
  
  _addPendingChange(type, name, generateText) {
    const p = this._pendingChanges.findIndex(c => c.type === type && c.name === name);
    if (p >= 0) {
      this._pendingChanges[p] = { type, name, generateText };
    } else {
      this._pendingChanges.push({ type, name, generateText });
    }
    if (this._pendingChangesTimeout) this.window.clearTimeout(this._pendingChangesTimeout);
    this._pendingChangesTimeout = this.window.setTimeout(() => {
      this.flushPendingChanges();
    }, this.pendingChangesDebounceTime);
  }
}

ResService.singleton = true;
