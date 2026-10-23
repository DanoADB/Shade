const assert=require('node:assert/strict'),vm=require('node:vm'),fs=require('node:fs');
const {Drought}=require('../drought-model.js');
const elements=new Map();function el(){const classes=new Set();return {hidden:false,disabled:false,style:{},listeners:{},dataset:{},textContent:'',classList:{toggle(k,force){if(force===undefined? !classes.has(k):force)classes.add(k);else classes.delete(k);},contains:k=>classes.has(k)},setAttribute(){},append(){},addEventListener(k,f){this.listeners[k]=f;},querySelector(){return get('body');},querySelectorAll(){return nudges;}};}
const get=k=>{if(!elements.has(k))elements.set(k,el());return elements.get(k);};const nudges=['care','explore','rest','rain','praise','discourage'].map(k=>Object.assign(el(),{dataset:{nudge:k}}));
let now=0,tick,calls=[],resets=0,reloads=0;const Module={shadeDrought:true,canvas:el(),_shade_pause_loop(){},mobileControls:{stop(){}},_shade_drought(...args){calls.push(args);if(args[4])resets++;return 1;},_shade_drought_worker(){}};
const document={addEventListener(){},hidden:false,head:el(),querySelectorAll(){return [];},getElementById:get,querySelector(){return get('title');},createElement:el};
vm.runInNewContext(fs.readFileSync(require.resolve('../drought.js'),'utf8'),{document,Module,ShadeDrought:{Drought},performance:{now:()=>now},setInterval(fn){tick=fn;},location:{reload(){reloads++;}},Math});
assert(get('drought-focus').disabled);get('status').textContent='Loading';tick();assert.equal(calls.length,0);
get('status').textContent='Running...';tick();assert.equal(calls.length,2);assert(!get('drought-focus').disabled);
now+=1000;tick();assert.match(get('drought-intent').textContent,/179 seconds/);
get('drought-pause').onclick();now+=1000;tick();assert.match(get('drought-intent').textContent,/179 seconds/);
get('drought-pause').onclick();document.hidden=true;now+=60000;tick();document.hidden=false;now+=100;tick();assert.match(get('drought-intent').textContent,/179 seconds/,'Background time must not consume the scenario');
Module.shadeTempleChanged(true);now+=1000;tick();Module.shadeTempleChanged(false);now+=100;tick();assert.match(get('drought-intent').textContent,/179 seconds/);
get('drought-restart').onclick();assert.equal(resets,1);assert.match(get('drought-intent').textContent,/180 seconds/);
Module.shadeMapChanged();const count=calls.length;now+=1000;tick();assert.equal(calls.length,count);assert(nudges.every(b=>b.disabled));assert(get('restart').onclick);get('drought-restart').onclick();assert.equal(reloads,1);
console.log('Drought UI startup, pause, background/Citadel suspension, restart and map isolation passed.');
