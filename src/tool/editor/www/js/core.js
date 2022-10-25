/* core.js
 * Basic application features.
 * Nothing specific to the Full Moon Editor in here.
 */
 
/* Injector.
 * Responsible for instantiating objects, in general.
 * Maintains a list of singletons.
 **************************************************************************/
 
export class Injector {
  static getDependencies() {
    // This won't be called, I'm just setting a good example.
    return [Window];
  }
  constructor(window) {
    this.window = window;
    
    this._singletons = {
      Window: this.window,
      Document: this.window.document,
      Injector: this,
    };
    this._instantiationInProgress = [];
    this._nextDiscriminator = 1;
  }
  
  getInstance(clazz, overrides) {
  
    if (clazz === "discriminator") {
      return this._nextDiscriminator++;
    }
  
    if (overrides) {
      for (const override of overrides) {
        if ((override.constructor === clazz) || clazz.isPrototypeOf(override.constructor)) return override;
      }
    }
  
    const name = clazz.name;
    const singleton = this._singletons[name];
    if (singleton) return singleton;
    
    if (this._instantiationInProgress.indexOf(name) >= 0) {
      throw new Error(`Dependency loop involving these classes: ${JSON.stringify(this._instantiationInProgress)}`);
    }
    this._instantiationInProgress.push(name);
    
    const dependencyClasses = clazz.getDependencies?.() || [];
    const dependencies = [];
    for (const dependencyClass of dependencyClasses) {
      dependencies.push(this.getInstance(dependencyClass, overrides));
    }
    const instance = new clazz(...dependencies);
    
    const p = this._instantiationInProgress.indexOf(name);
    if (p >= 0) this._instantiationInProgress.splice(p, 1);
    
    if (clazz.singleton) {
      this._singletons[name] = instance;
    }
    
    return instance;
  }
}

Injector.singleton = true;

/* Dom.
 * Manipulates DOM elements and manages our attachment of controllers to elements.
 * Also responsible for the modal stack, though that's not really the kind of thing this is for.
 **************************************************************************/
 
export class Dom {
  static getDependencies() {
    return [Window, Document, Injector];
  }
  constructor(window, document, injector) {
    this.window = window;
    this.document = document;
    this.injector = injector;
    
    this.mutationObserver = new this.window.MutationObserver((events) => this.onMutations(events));
    this.mutationObserver.observe(this.document.body, { childList: true, subtree: true });
    
    this.window.addEventListener("keydown", (event) => {
      if (event.code === "Escape") {
        this.dismissModals();
        event.preventDefault();
        event.stopPropagation();
      }
    });
  }
  
  onMutations(records) {
    for (const record of records) {
      for (const element of record.removedNodes || []) {
        if (element._fmn_controller) {
          element._fmn_controller?.onRemoveFromDom?.();
          delete element._fmn_controller;
        }
      }
    }
  }
  
  /* (args) may contain:
   *  - string => innerText
   *  - array => CSS class names
   *  - {
   *      "on-*" => event listener
   *      "*" => attribute
   *    }
   * Anything else is an error.
   */
  spawn(parent, tagName, ...args) {
    const element = this.document.createElement(tagName);
    for (const arg of args) {
      switch (typeof(arg)) {
        case "string": element.innerText = arg; break;
        case "object": {
            if (arg instanceof Array) {
              for (const cls of arg) {
                element.classList.add(cls);
              }
            } else for (const k of Object.keys(arg)) {
              if (k.startsWith("on-")) {
                element.addEventListener(k.substr(3), arg[k]);
              } else {
                element.setAttribute(k, arg[k]);
              }
            }
          } break;
        default: throw new Error(`Unexpected argument ${arg}`);
      }
    }
    parent.appendChild(element);
    return element;
  }
  
  spawnController(parent, clazz, overrides) {
    const element = this.spawn(parent, this.tagNameForControllerClass(clazz), [clazz.name]);
    const controller = this.injector.getInstance(clazz, [...(overrides || []), element]);
    element._fmn_controller = controller;
    return controller;
  }
  
  spawnModal(clazz, overrides) {
    const frame = this._spawnModalFrame();
    const controller = this.spawnController(frame, clazz, overrides);
    return controller;
  }
  
  getTopModalController() {
    const frames = Array.from(this.document.body.querySelectorAll(".modalFrame"));
    if (!frames.length) return null;
    const frame = frames[frames.length - 1];
    const element = frame.children[0];
    return element._fmn_controller;
  }
  
  tagNameForControllerClass(clazz) {
    for (const dcls of clazz.getDependencies?.() || []) {
      const match = dcls.name?.match(/^HTML(.*)Element$/);
      if (match) switch (match[1]) {
        // Unfortunately, the names of HTMLElement subclasses are not all verbatim tag names.
        case "": return "DIV";
        case "Div": return "DIV";
        case "Canvas": return "CANVAS";
        default: {
            console.log(`TODO: Unexpected HTMLElement subclass name '${match[1]}', returning 'DIV'`);
            return "DIV";
          }
      }
    }
    return "DIV";
  }
  
  dismissModals() {
    this.document.body.querySelector(".modalBlotter")?.remove();
    this.document.body.querySelector(".modalStack")?.remove();
  }
  
  popModal(controller) {
    for (const frame of this.document.body.querySelectorAll(".modalFrame")) {
      if (Array.from(frame.children || []).find(e => e._fmn_controller === controller)) {
        frame.remove();
        if (!this.document.body.querySelector(".modalFrame")) {
          this.dismissModals();
        }
        return;
      }
    }
    console.log(`failed to pop modal`, controller);
  }
  
  _spawnModalFrame() {
    const stack = this._requireModalStack();
    return this.spawn(stack, "DIV", ["modalFrame"]);
  }
  
  _requireModalStack() {
    let blotter = this.document.body.querySelector(".modalBlotter");
    if (!blotter) {
      blotter = this.spawn(this.document.body, "DIV", ["modalBlotter"]);
    }
    let stack = this.document.body.querySelector(".modalStack");
    if (!stack) {
      stack = this.spawn(this.document.body, "DIV", ["modalStack"], { "on-click": (event) => {
        if (event.target === stack) {
          this.dismissModals();
        }
      }});
    }
    return stack;
  }
}

Dom.singleton = true;

/* Comm.
 * Wrapper around Fetch API.
 ***********************************************************************/
 
export class Comm {
  static getDependencies() {
    return [Window];
  }
  constructor(window) {
    this.window = window;
  }
  
  fetchText(method, path, query, headers, body) {
    return this.fetch(method, path, query, headers, body, { returnType: "text" });
  }
  
  fetchBinary(method, path, query, headers, body) {
    return this.fetch(method, path, query, headers, body, { returnType: "arrayBuffer" });
  }
  
  fetchJson(method, path, query, headers, body) {
    return this.fetch(method, path, query, headers, body, { returnType: "json" });
  }
  
  fetch(method, path, query, headers, body, options) {
    const url = this._composeUrl(path, query);
    return this.window.fetch(url, this._composeFetchOptions(method, headers, body)).then((rsp) => {
      if (!rsp.ok) throw new Error(`${rsp.status} ${rsp.statusText}`);
      switch (options?.returnType || "text") {
        case "text": return rsp.text();
        case "arrayBuffer": return rsp.arrayBuffer();
        case "json": return rsp.json();
        default: return rsp;
      }
    });
  }
  
  _composeUrl(path, query) {
    if (!path) path = "/";
    if (!path.startsWith("/")) throw new Error(`Invalid fetch path '${path}', must start with '/'`);
    let url = this.window.location.protocol + "//" + this.window.location.host + path;
    if (query) url = `${url}?${this._composeQuery(query)}`;
    return url;
  }
  
  _composeQuery(query) {
    return Object.keys(query).map(k => encodeURIComponent(k) + '=' + encodeURIComponent(query[k])).join('&');
  }
  
  _composeFetchOptions(method, headers, body) {
    const options = { method };
    if (headers) options.headers = headers;
    if (body) options.body = body;
    return options;
  }
}

Comm.singleton = true;
