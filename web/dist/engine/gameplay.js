(function () {
  'use strict';
  const panel = document.createElement('section');
  panel.id = 'gameplay';
  panel.hidden = true;
  panel.setAttribute('aria-label', 'Village task');
  panel.innerHTML = `<div class="task-heading"><strong>Help the village</strong><button id="task-toggle" aria-expanded="true" aria-controls="task-body">Hide task</button></div><span id="wood-progress">Wood delivered: 0 / 30</span>
    <span id="food-progress">Food eaten: 0 / 10</span><div id="task-body"><div class="find-actions" role="group" aria-label="Find an object"><button data-game="4">Find tree</button><button data-game="9">Find field</button><button data-game="5">Find building</button><button data-game="6">Find villager</button></div>
    <b id="selected-name">Nothing selected</b><span id="selected-info">Tap a tree, villager or building to inspect it.</span>
    <div class="task-actions"><button data-game="2" disabled>Gather wood</button><button data-game="3" disabled>Deliver wood</button><button data-game="7" disabled>Return wood</button></div>
    <button data-game="8" hidden>See villager response</button>
    <span id="task-message" role="status" aria-live="polite"></span><small id="save-status">Deliveries save on this browser. Carried resources return on reload.</small></div>`;
  document.getElementById('stage').append(panel);
  const marker = document.createElement('div');
  marker.id = 'selection-marker'; marker.hidden = true; marker.setAttribute('aria-hidden', 'true');
  document.getElementById('stage').append(marker);
  function action(code, x = 0, y = 0) { if (Module._shade_gameplay) Module._shade_gameplay(code, x, y); }
  function showTask(show) {
    document.getElementById('task-body').hidden = !show;
    const toggle = document.getElementById('task-toggle');
    toggle.textContent = show ? 'Hide task' : 'Show task';
    toggle.setAttribute('aria-expanded', String(show));
  }
  document.getElementById('task-toggle').addEventListener('click', () => showTask(document.getElementById('task-body').hidden));
  const saveKey = 'shade-land1-village-task-v1';
  Module.shadeGameplaySave = json => {
    try { localStorage.setItem(saveKey, json); document.getElementById('save-status').textContent = 'Progress saved on this browser. Carried resources return on reload.'; }
    catch (_) { document.getElementById('save-status').textContent = 'Saving unavailable. Keep this page open to retain progress.'; }
  };
  function restore() {
    try {
      const text = localStorage.getItem(saveKey);
      if (!text) return;
      const saved = JSON.parse(text);
      if (saved.version !== 1 || !Array.isArray(saved.objects) || saved.objects.length > 10000 ||
          saved.objects.some(row => !Array.isArray(row) || row.length !== 6 || ![1,2,4,5,6].includes(row[0]) ||
            row.some(value => !Number.isFinite(value)) || ![row[1],row[5]].every(value => Number.isInteger(value) && value >= 0 && value <= 4294967295))) throw Error('Invalid save');
      const apply = (...args) => { if (!Module._shade_gameplay_restore(...args)) throw Error('Save does not match this island'); };
      apply(0,0,0,0,0,0);
      for (const row of saved.objects) apply(...row);
      apply(3,0,0,0,0,0);
      document.getElementById('save-status').textContent = 'Saved task resumed on this browser.';
    } catch (_) { document.getElementById('save-status').textContent = 'Saved task could not load. Starting a fresh task.'; }
  }
  panel.addEventListener('click', event => {
    const button = event.target.closest('[data-game]');
    if (button && !button.disabled) {
      action(Number(button.dataset.game));
      if (button.dataset.game === '8' && document.documentElement.clientWidth < 600) showTask(false);
    }
  });
  Module.shadeGameplayUpdate = state => {
    panel.hidden = false;
    const set = (id, text) => { const element = document.getElementById(id); if (element.textContent !== text) element.textContent = text; };
    set('wood-progress', `Wood delivered: ${Math.min(state.delivered, 30)} / 30${state.delivered >= 30 ? ' · Complete' : ''}${state.carrying && !state.foodHeld ? ' · Carrying 10 wood' : ''}`);
    set('food-progress', `Food eaten: ${Math.min(state.fed || 0, 10)} / 10${state.fed >= 10 ? ' · Complete' : ''}${state.carrying && state.foodHeld ? ' · Carrying 10 food' : ''}`);
    set('selected-name', ['', 'Tree', 'Village building', 'Villager', 'Crop field'][state.kind] || 'Nothing selected');
    set('selected-info', state.kind === 1 ? (state.gathered ? 'Wood already gathered from this tree.' : 'Available: 10 wood.') :
      state.kind === 2 ? `Wood: ${state.wood} · Food: ${state.food} · Residents: ${state.people}` :
      state.kind === 3 ? `Health: ${state.health} · Hunger: ${state.hunger} (lower is better)` :
      state.kind === 4 ? (state.gathered ? 'Food already harvested from this field.' : 'Available: 10 food.') : 'Tap a tree, field, villager or building to inspect it.');
    set('task-message', state.message);
    const gather = panel.querySelector('[data-game="2"]'), deliver = panel.querySelector('[data-game="3"]');
    gather.textContent = state.kind === 4 ? 'Harvest food' : 'Gather wood';
    gather.disabled = ![1,4].includes(state.kind) || state.gathered || state.carrying;
    deliver.textContent = state.carrying && state.foodHeld ? (state.kind === 3 ? 'Feed villager' : 'Deliver food') : 'Deliver wood';
    deliver.disabled = !state.carrying || (!([2,4].includes(state.kind)) && !(state.foodHeld && state.kind === 3 && state.hunger > 0));
    panel.querySelector('[data-game="7"]').textContent = !state.carrying ? 'Return resource' : state.foodHeld ? 'Return food' : 'Return wood';
    panel.querySelector('[data-game="7"]').disabled = !state.carrying;
    panel.querySelector('[data-game="8"]').hidden = !state.response;
    marker.hidden = !state.kind || state.x < 0 || state.y < 0 || state.x > 1 || state.y > 1;
    marker.style.left = `${state.x * 100}%`; marker.style.top = `${state.y * 100}%`;
  };
  // A short, single-pointer release selects. Dragging, pinching and cancelled touches do not.
  const pointers = new Map();
  let gestureCancelled = false;
  Module.canvas.addEventListener('pointerdown', event => {
    if (!pointers.size) gestureCancelled = false;
    pointers.set(event.pointerId, { x: event.clientX, y: event.clientY, time: performance.now() });
    if (pointers.size > 1 || event.button !== 0) gestureCancelled = true;
  });
  Module.canvas.addEventListener('pointermove', event => {
    const start = pointers.get(event.pointerId);
    if (start && Math.hypot(event.clientX - start.x, event.clientY - start.y) > 10) gestureCancelled = true;
  });
  Module.canvas.addEventListener('pointerup', event => {
    const start = pointers.get(event.pointerId);
    pointers.delete(event.pointerId);
    if (!start || gestureCancelled || performance.now() - start.time > 600) return;
    const rect = Module.canvas.getBoundingClientRect();
    const x=(event.clientX-rect.left)/rect.width,y=(event.clientY-rect.top)/rect.height;
    // SDL/ImGui consume queued pointer events during the engine frame. Let focus
    // settle before asking the bridge to select an object behind a native menu.
    requestAnimationFrame(()=>requestAnimationFrame(()=>action(1,x,y)));
  });
  const cancel = () => { gestureCancelled = true; pointers.clear(); };
  Module.canvas.addEventListener('pointercancel', cancel);
  Module.canvas.addEventListener('pointerleave', cancel);
  window.addEventListener('blur', cancel);
  document.addEventListener('visibilitychange', cancel);
  let started = false;
  let islandChanged = false;
  Module.shadeMapChanged = () => {
    islandChanged = true;panel.hidden = true;marker.hidden = true;
    for(const id of ['village','closeup','villager'])document.getElementById(id).disabled = true;
    document.getElementById('status').textContent = 'Island loaded · use OpenBlack menus · reload to return to the Shade task';
  };
  setInterval(() => {
    if (islandChanged || document.getElementById('status').textContent !== 'Running...' || document.hidden) return;
    if (!started) {
      started = true;
      if (Module._shade_village_view) Module._shade_village_view(1);
      restore();
    }
    action(0);
  }, 100);
})();
