(function(){
  'use strict';if(!Module.shadeDrought)return;
  const style=document.createElement('style');style.textContent=`
  body{background:#101c19;color:#f1ead8;font-family:system-ui}header{background:#101e1be8;border-bottom:1px solid #b8a16a55}header b{font-family:Georgia,serif;letter-spacing:.05em}header button{background:#213b32;border-color:#697e62}
  #drought{position:absolute;right:18px;top:18px;width:320px;max-height:calc(100% - 36px);overflow:auto;padding:20px;background:linear-gradient(145deg,#122b26f5,#12201df5);border:1px solid #a8976255;border-radius:16px;box-shadow:0 12px 50px #0006;z-index:3}#drought h1{font:30px Georgia,serif;margin:4px 0 8px;color:#f4dfad}#drought .eyebrow{font-size:10px;letter-spacing:.2em;text-transform:uppercase;color:#c0b37d}#drought p{font-size:12px;line-height:1.5;color:#c9d3c4;margin:8px 0}#drought button{font-size:12px;padding:7px 9px;min-height:40px}#drought .actions{display:grid;grid-template-columns:1fr 1fr;gap:7px;margin:12px 0}#drought .metrics{display:grid;grid-template-columns:1fr 1fr 1fr;gap:8px;margin:12px 0}#drought label{font-size:10px;color:#c7cfbb}#drought meter{display:block;width:100%;height:9px;accent-color:#9cbc73}#drought output{display:block;color:#f0d9a1;font-size:17px;margin:3px 0}#drought .line{border-top:1px solid #b8a16a33;padding-top:10px}#drought .story{padding-left:15px;font:12px/1.5 Georgia,serif;color:#d5d9c7}#drought .story li{margin:7px 0}#drought-state{font:18px Georgia,serif;color:#eedbaa}#drought button:disabled{opacity:.38}#drought-weather{position:absolute;inset:0;pointer-events:none;z-index:1;opacity:0;background:repeating-linear-gradient(108deg,transparent 0,transparent 45px,#a8d5e533 46px,transparent 47px);background-size:150px 180px}#drought-weather.raining{opacity:.65;animation:drought-rain .6s linear infinite}@keyframes drought-rain{to{background-position:-55px 180px}}#drought-hint{position:absolute;left:20px;bottom:24px;max-width:420px;background:#10221dd9;padding:12px 16px;border-left:2px solid #c2b178;border-radius:0 8px 8px 0;color:#e4debf;font:13px/1.5 Georgia,serif;pointer-events:none;z-index:2}
  @media(max-width:700px){#drought{right:8px;top:8px;width:270px;padding:12px;max-height:60%}#drought h1{font-size:24px}#drought-hint{left:10px;bottom:130px;max-width:210px;font-size:11px}#drought .story{display:none}#drought.collapsed .body{display:none}#drought.collapsed>p,#drought.collapsed>.eyebrow{display:none}#drought.collapsed h1{font-size:20px;margin:0 0 8px}}@media(prefers-reduced-motion:reduce){#drought-weather.raining{animation:none}}
  `;document.head.append(style);
  const panel=document.createElement('section');panel.id='drought';panel.setAttribute('aria-label','The long dry season');panel.innerHTML=`<span class="eyebrow">Shade · first playable chapter</span><h1>The Long Dry</h1><p>You are unseen. The creature is their god. Influence its instincts; earn a future for this village.</p><button id="drought-collapse" aria-expanded="true">Hide details</button><div class="body"><div class="metrics" id="drought-metrics"></div><div id="drought-state" aria-live="polite">Waiting for the island…</div><p id="drought-intent"></p><div class="actions"><button data-nudge="care">Stir compassion</button><button data-nudge="explore">Kindle curiosity</button><button data-nudge="rest">Offer calm</button><button data-nudge="rain">Dream of rain</button><button data-nudge="praise">Share warmth</button><button data-nudge="discourage">Show unease</button></div><p id="drought-learning"></p><p id="drought-workers" class="line"></p><ul class="story" id="drought-story" aria-live="polite"></ul><div class="actions"><button id="drought-focus">Find creature</button><button id="drought-pause">Pause</button><button id="drought-restart">Begin again</button></div><p id="drought-result"></p></div>`;
  document.getElementById('stage').append(panel);
  const weather=document.createElement('div');weather.id='drought-weather';document.getElementById('stage').append(weather);
  const hint=document.createElement('div');hint.id='drought-hint';hint.textContent='A village beneath a silent sky. Watch what the creature chooses, then respond to the lesson it learns.';document.getElementById('stage').append(hint);
  document.querySelector('header b').textContent='Shade · The unseen';
  let model=new ShadeDrought.Drought(),last=performance.now(),started=false,paused=false,stopped=false,temple=false,focus=0;
  const $=id=>document.getElementById(id);
  const codes={idle:0,care:1,explore:2,rest:3,channel:4,rain:5};
  const names={idle:'Watching the village',care:'Carrying water to the village',explore:'Exploring the watershed',rest:'Resting',channel:'Opening a shared watercourse',rain:'Gathering the courage to call rain'};
  function draw(){
    $('drought-metrics').innerHTML=['water','crops','forest','energy','confidence','belief'].map(k=>`<label>${{water:'Village water',crops:'Crops',forest:'Forest',energy:'Energy',confidence:'Confidence',belief:'Village faith'}[k]}<output>${Math.round(model[k])}</output><meter min="0" max="100" value="${model[k]}"></meter></label>`).join('');
    $('drought-state').textContent=model.outcome?(model.outcome==='won'?'The village endures':'A lesson for another season'):names[model.phase];
    $('drought-intent').textContent=`${Math.max(0,Math.ceil(180-model.time))} seconds until the dry season ends · ${model.deliveries} gifts of water${model.spring?' · Spring found':''}${model.channel?' · Community channel flowing':''}`;
    $('drought-learning').textContent=model.urge?`The creature feels your ${model.urge==='care'?'compassion':model.urge==='explore'?'curiosity':model.urge==='rest'?'calm':'dream of rain'}. It will decide after its current action.`:'Warmth and unease teach its last completed choice. A nudge influences its next decision; it does not cancel an action.';
    $('drought-workers').textContent=model.workers?model.workers.map(w=>`${w.name}: ${w.job}`).join(' · '):'Villagers are organizing their response.';
    $('drought-story').innerHTML=model.events.map(e=>`<li>${e.text}</li>`).join('');
    $('drought-focus').disabled=!started||stopped||temple;
    $('drought-pause').disabled=!started||stopped||temple;
    $('drought-restart').disabled=!started;
    for(const button of panel.querySelectorAll('[data-nudge]')){const kind=button.dataset.nudge;button.disabled=!started||paused||stopped||temple||!!model.outcome||(['praise','discourage'].includes(kind)?(!model.last||model.feedback||model.time>model.feedbackUntil):model.cooldown>0||(kind==='rain'&&!model.canRain()));}
    $('drought-result').textContent=model.outcome?(model.outcome==='won'?'You remained unknown. They thank the creature. Begin again to teach a different kind of god.':'Keep water above 25, crops above 40 and forest above 35 at the end. Discover a spring, earn faith through two deliveries, then stir compassion to open a channel.'):'Rain needs 44 faith, 48 confidence and 45 energy. The spring plus two deliveries and compassion can create a lasting supply.';
    weather.classList.toggle('raining',model.rain>0);
    hint.textContent=model.workers.map(w=>`${w.name}: ${w.job}`).join(' · ')+(model.channel?' · Community watercourse flowing.':' · Nobody waits for your orders.');
  }
  panel.addEventListener('click',e=>{const button=e.target.closest('[data-nudge]');if(button&&!button.disabled){model.influence(button.dataset.nudge);draw();}});
  $('drought-focus').onclick=()=>{closeup=!closeup;focus=closeup?2:1;$('drought-focus').textContent=closeup?'Village view':'Find creature';};
  $('drought-pause').onclick=()=>{paused=!paused;last=performance.now();$('drought-pause').textContent=paused?'Resume':'Pause';draw();};
  $('drought-restart').onclick=()=>{if(stopped){location.reload();return;}model=new ShadeDrought.Drought();Module._shade_drought(0,0,0,1,1);paused=false;last=performance.now();$('drought-pause').textContent='Pause';draw();};
  $('drought-collapse').onclick=()=>{panel.classList.toggle('collapsed');const collapsed=panel.classList.contains('collapsed');panel.querySelector('.body').hidden=collapsed;$('drought-collapse').textContent=collapsed?'Show details':'Hide details';$('drought-collapse').setAttribute('aria-expanded',String(!collapsed));};
  document.addEventListener('keydown',e=>{if(e.key.toLowerCase()==='p'&&e.target.tagName!=='INPUT'&&e.target.tagName!=='TEXTAREA'){e.preventDefault();e.stopImmediatePropagation();if(started&&!stopped)$('drought-pause').onclick();}},true);
  Module.shadeTempleChanged=active=>{temple=active;last=performance.now();panel.hidden=active;hint.hidden=active;weather.hidden=active;};
  Module.shadeStop=reason=>{stopped=true;if(Module.mobileControls)Module.mobileControls.stop();$('touch-toggle').disabled=true;document.querySelectorAll('[data-control]').forEach(button=>button.disabled=true);$('status').textContent=reason;$('restart').hidden=false;$('restart').onclick=()=>location.reload();draw();};
  Module.shadeMapChanged=()=>Module.shadeStop('Drought paused: island changed. Reload to return to the scenario.');
  Module.canvas.addEventListener('webglcontextlost',event=>{event.preventDefault();Module.shadeStop('Graphics interrupted. Reload to return to the scenario.');if(Module._shade_pause_loop)Module._shade_pause_loop();});
  Module.canvas.addEventListener('webglcontextrestored',()=>Module.shadeStop('Graphics restored. Reload to rebuild the island.'));
  setInterval(()=>{
    const now=performance.now(),dt=(now-last)/1000;last=now;
    if(stopped||paused||temple||document.hidden||$('status').textContent!=='Running...')return;
    if(!started){const ok=Module._shade_drought&&Module._shade_drought(0,0,0,1,0);if(ok!==1){Module.shadeStop('The original cow creature could not load. See Details.');return;}started=true;}
    else model.step(Math.min(dt,1));
    const result=Module._shade_drought(codes[model.phase],model.elapsed/model.duration,model.deliveries,focus,0);focus=0;
    if(Module._shade_drought_weather)Module._shade_drought_weather(1-model.water/100,model.rain>0?1:0);
    if(Module._shade_drought_worker)model.workers.forEach((w,i)=>Module._shade_drought_worker(i,{'hauling water':0,'tending crops':1,'protecting trees':2}[w.job],w.progress));
    if(result!==1){Module.shadeStop('Scenario scene changed. Reload to begin again.');return;}draw();
  },100);
  draw();
})();
