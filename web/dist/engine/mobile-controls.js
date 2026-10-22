// Hold/release handling is shared by touch, mouse and keyboard-operated controls.
function createHoldController(send, timers = globalThis) {
  const pointers = new Map(), counts = new Map(), repeats = new Map();
  function start(id, action) {
    if (pointers.has(id)) return;
    pointers.set(id, action);
    const count = counts.get(action) || 0;
    counts.set(action, count + 1);
    if (count) return;
    send(action, 1);
    if (action >= 8) repeats.set(action, timers.setInterval(() => send(action, 1), 120));
  }
  function stop(id) {
    if (!pointers.has(id)) return;
    const action = pointers.get(id);
    pointers.delete(id);
    const count = counts.get(action) - 1;
    if (count) { counts.set(action, count); return; }
    counts.delete(action);
    if (repeats.has(action)) { timers.clearInterval(repeats.get(action)); repeats.delete(action); }
    if (action < 8) send(action, 0);
  }
  function reset() { for (const id of Array.from(pointers.keys())) stop(id); }
  return { start, stop, reset };
}
if (typeof module !== 'undefined') module.exports = { createHoldController };
if (typeof document !== 'undefined') (() => {
  const panel = document.getElementById('touch-controls');
  const toggle = document.getElementById('touch-toggle');
  const stage = document.getElementById('stage');
  const controller = createHoldController((action, pressed) => {
    if (Module._shade_control) Module._shade_control(action, pressed);
  });
  let ready = false, resizing = 0;
  function resize() {
    if (!ready || resizing) return;
    resizing = requestAnimationFrame(() => {
      resizing = 0;
      const box = stage.getBoundingClientRect(), scale = Math.min(1, 1280 / Math.max(box.width, box.height));
      if (Module._shade_resize) Module._shade_resize(Math.max(1, Math.round(box.width * scale)), Math.max(1, Math.round(box.height * scale)));
    });
  }
  function showControls(show) {
    controller.reset();
    panel.hidden = !show;
    toggle.setAttribute('aria-pressed', String(show));
  }
  toggle.onclick = () => showControls(panel.hidden);
  showControls(matchMedia('(pointer: coarse)').matches);
  for (const button of panel.querySelectorAll('[data-control]')) {
    button.disabled = true;
    const action = Number(button.dataset.control);
    button.addEventListener('pointerdown', event => {
      if (!ready || (event.pointerType === 'mouse' && event.button !== 0)) return;
      event.preventDefault(); event.stopPropagation();
      button.setPointerCapture(event.pointerId);
      controller.start(event.pointerId, action);
      button.classList.add('held');
    });
    for (const type of ['pointerup', 'pointercancel', 'lostpointercapture']) button.addEventListener(type, event => {
      controller.stop(event.pointerId); button.classList.remove('held');
    });
    button.addEventListener('keydown', event => {
      if (ready && !event.repeat && (event.key === ' ' || event.key === 'Enter')) {
        event.preventDefault(); event.stopPropagation(); controller.start('key-' + action, action); button.classList.add('held');
      }
    });
    button.addEventListener('keyup', event => {
      if (event.key === ' ' || event.key === 'Enter') { event.preventDefault(); event.stopPropagation(); controller.stop('key-' + action); button.classList.remove('held'); }
    });
    button.addEventListener('blur', () => { controller.stop('key-' + action); button.classList.remove('held'); });
  }
  function reset() { controller.reset(); panel.querySelectorAll('.held').forEach(button => button.classList.remove('held')); }
  window.addEventListener('blur', reset);
  document.addEventListener('visibilitychange', () => { if (document.hidden) reset(); });
  Module.mobileControls = {
    ready() { ready = true; panel.querySelectorAll('button').forEach(button => button.disabled = false); resize(); },
    stop: reset,
  };
  new ResizeObserver(resize).observe(stage);
  window.addEventListener('orientationchange', () => { reset(); resize(); });
  const fullscreen = document.getElementById('fullscreen');
  fullscreen.hidden = !document.documentElement.requestFullscreen;
  fullscreen.onclick = () => document.documentElement.requestFullscreen().catch(() => {});
})();
