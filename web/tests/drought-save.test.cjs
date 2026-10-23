const assert=require('node:assert/strict'),fs=require('node:fs'),vm=require('node:vm');
const rules=require('../drought-model.js');
const {Drought,encodeSave,decodeSave,SAVE_KEY}=rules;
// Split real runs at every tick: RNG, in-flight actions and rewards must continue identically.
const run=new Drought();let phases=new Set();
for(let i=0;i<720;i++){
  if(!run.cooldown)run.influence(run.spring?'care':'explore');
  if(run.last&&!run.feedback)run.influence('praise');
  phases.add(run.phase);
  const saved=decodeSave(encodeSave(run,i%3,true));
  assert.equal(saved.god,i%3);assert(saved.follow);
  saved.model.step(.25);run.step(.25);
  assert.deepEqual(JSON.parse(JSON.stringify(saved.model)),JSON.parse(JSON.stringify(run)),'Resume must neither replay rewards nor change the next decision');
}
assert.equal(run.outcome,'won');assert(phases.has('channel'));
for(const phase of ['care','rest','channel','rain']){
 const model=new Drought();model.time=40;model.spring=true;model.deliveries=2;model.belief=50;model.phase=phase;model.duration={care:18,rest:12,channel:26,rain:18}[phase];model.elapsed=model.duration-.125;
 model.workers.forEach(w=>{w.progress=.999;w.cycles=1;});model.habits={care:2,explore:.5,rest:1.3};
 const restored=decodeSave(encodeSave(model,2,false)).model;
 model.step(.25);restored.step(.25);assert.deepEqual(restored,model,phase+' completion is applied exactly once');
 const after=decodeSave(encodeSave(restored,2,false)).model;assert.deepEqual(after,model);
}
const valid=JSON.parse(encodeSave(new Drought(),0,false));
for(const mutate of [s=>s.version=99,s=>s.god=5,s=>s.state.seed=-1,s=>s.state.time=null,s=>s.state.habits.care=0,s=>s.state.workers[0].progress=2,s=>s.state.phase='constructor',s=>s.state.duration=0,s=>s.state.channel=true,s=>s.state.events[0].text='x'.repeat(301)]){
 const data=JSON.parse(JSON.stringify(valid));mutate(data);assert.throws(()=>decodeSave(JSON.stringify(data)));
}
for(const text of ['null','{','[]','x'.repeat(16001)])assert.throws(()=>decodeSave(text));
const polluted=JSON.parse(JSON.stringify(valid));polluted.state.step='bad';polluted.state.__proto__={polluted:true};assert.equal(typeof decodeSave(JSON.stringify(polluted)).model.step,'function');
function boot(storage,failGod=false,mask=null){
 const elements=new Map(),handlers={},windowHandlers={},calls=[],world=[],workers=[],motions=[];let tick,now=0;
 const nudges=[];const get=id=>{if(!elements.has(id)){const classes=new Set();elements.set(id,{textContent:'',value:'',disabled:false,hidden:false,style:{},dataset:{},classList:{toggle(){},contains(){return false;}},append(){},setAttribute(){},querySelector(){return get('body');},querySelectorAll(){return nudges;},addEventListener(name,fn){this[name]=fn;}});}return elements.get(id);};
 const document={hidden:false,head:get('head'),createElement(){return {...get('template')};},getElementById:get,querySelector(){return get('header');},querySelectorAll(){return [];},addEventListener(n,f){handlers[n]=f;}};
 const Module={shadeDrought:true,canvas:get('canvas'),_shade_drought(...args){calls.push(args);return 1;},_shade_drought_restore(deliveries){calls.push(['restore',deliveries]);},_shade_drought_god(god){calls.push(['god',god]);return failGod?0:1;},_shade_drought_world(...args){world.push(args);},_shade_drought_worker(...args){workers.push(args);},_shade_drought_animate(...args){motions.push(args);}};
 if(mask!==null)Module._shade_drought_routes=()=>mask;
 vm.runInNewContext(fs.readFileSync(require.resolve('../drought.js'),'utf8'),{Module,document,window:{addEventListener(n,f){windowHandlers[n]=f;}},localStorage:storage,ShadeDrought:rules,performance:{now:()=>now},setInterval(fn){tick=fn;},location:{reload(){}},Math});
 get('status').textContent='Running...';tick();
 return {get,document,handlers,windowHandlers,calls,world,workers,motions,step(ms){now+=ms;tick();}};
}
let text=null,writes=0;const storage={getItem(key){assert.equal(key,SAVE_KEY);return text;},setItem(key,value){assert.equal(key,SAVE_KEY);text=value;writes++;}};
const checkpoint=new Drought();checkpoint.time=50;checkpoint.phase='channel';checkpoint.duration=26;checkpoint.elapsed=13;checkpoint.spring=true;checkpoint.deliveries=2;checkpoint.habits.care=2;checkpoint.workers[0].progress=.9;
text=encodeSave(checkpoint,1,true);
let ui=boot(storage);assert.equal(ui.get('drought-god').value,'1');assert.match(ui.get('drought-god-status').textContent,/Rhino.*restored/);assert.equal(ui.get('drought-pause').textContent,'Resume');assert(ui.calls.some(c=>c[0]==='god'&&c[1]===1));assert(ui.calls.some(c=>c[0]===4&&c[1]===.5&&c[2]===2));assert(ui.calls.some(c=>c[0]==='restore'&&c[1]===2),'Hydration must prime visual delivery counters');assert.equal(ui.workers[0][2],.9);assert.equal(ui.motions.at(-1)[3],1);assert.equal(writes,0,'Loading must not overwrite the checkpoint');
ui.step(60000);assert.equal(writes,0);assert.match(ui.get('drought-intent').textContent,/130 seconds/);
ui.get('drought-pause').onclick();ui.step(1000);ui.get('drought-save').onclick();assert.equal(decodeSave(text).model.time,51);assert.equal(decodeSave(text).model.habits.care,2);
ui=boot(storage);assert.match(ui.get('drought-intent').textContent,/129 seconds/);assert.equal(ui.get('drought-pause').textContent,'Resume');
ui.get('drought-restart').onclick();const fresh=decodeSave(text);assert.equal(fresh.god,1);assert.equal(fresh.model.time,0);assert.equal(fresh.model.habits.care,1);assert(!fresh.model.spring);
text='{broken';ui=boot(storage);ui.step(4000);assert.equal(text,'{broken');assert.match(ui.get('drought-save-status').textContent,/kept/);ui.get('drought-save').onclick();assert.doesNotThrow(()=>decodeSave(text));
text=encodeSave(checkpoint,1,true);const prior=text;ui=boot(storage,true);assert.equal(text,prior);assert.match(ui.get('status').textContent,/Saved god could not load/);
ui=boot({getItem(){throw Error('blocked');},setItem(){throw Error('quota');}});ui.get('drought-save').onclick();assert.match(ui.get('drought-save-status').textContent,/unavailable/);
text=null;ui=boot(storage);ui.get('drought-god').value='2';ui.get('drought-god').onchange();assert.equal(decodeSave(text).god,2);ui.document.hidden=true;ui.handlers.visibilitychange();assert.doesNotThrow(()=>decodeSave(text));ui.windowHandlers.pagehide();
const unsafe=JSON.parse(encodeSave(new Drought(),0,false));unsafe.state.events[0].text='<img src=x onerror=evil()>';text=JSON.stringify(unsafe);ui=boot(storage);assert(!ui.get('drought-story').innerHTML.includes('<img'));
console.log('Drought save schema, deterministic continuation, reward boundaries, god/worker/learning restore, paused reload, reset, corrupt/blocked storage and safe history rendering passed.');

text=encodeSave(checkpoint,1,true);ui=boot(storage,false,0);assert.match(ui.get('drought-navigation').textContent,/waiting for a safe route/);ui.get('drought-pause').onclick();ui.step(1000);ui.get('drought-save').onclick();assert.equal(decodeSave(text).model.elapsed,13);assert.equal(decodeSave(text).model.workers[0].progress,.9);
console.log('Restored blocked routes freeze action and worker progress without changing the save format.');
