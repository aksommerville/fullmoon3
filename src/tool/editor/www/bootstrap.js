import { Injector, Dom } from "/js/core.js";
import { RootController } from "/js/root/RootController.js";

window.addEventListener("load", () => {
  const injector = new Injector(window);
  const dom = injector.getInstance(Dom);
  const body = document.body;
  body.innerHTML = "";
  const root = dom.spawnController(body, RootController);
});
