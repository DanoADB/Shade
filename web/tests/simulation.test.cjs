const fs=require('fs'),vm=require('vm'),assert=require('assert');
const elements=new Map();
const element=()=>({textContent:'',innerHTML:'',replaceChildren(){},getContext(){return new Proxy({},{get:()=>()=>{},set:()=>true})},addEventListener(){}});
const store=new Map();
const context=vm.createContext({window:{},document:{querySelector(s){if(!elements.has(s))elements.set(s,element());return elements.get(s)},querySelectorAll(){return []},createElement:element},localStorage:{setItem(k,v){store.set(k,v)},getItem(k){return store.get(k)||null}},performance:{now:()=>0},requestAnimationFrame(){},setTimeout(){},console});
vm.runInContext(fs.readFileSync(require('path').join(__dirname,'../dist/simulation.js'),'utf8'),context);
const run=s=>vm.runInContext(s,context);
run('creature.learning=9; creature.trust=81; creature.target={x:600,y:315}; saveWorld(); creature.learning=0; creature.trust=0; loadWorld();');
assert.equal(run('creature.learning'),9);assert.equal(run('creature.trust'),81);assert.equal(run('paused'),true);assert.equal(run('creature.target.x'),600);
run('const before=time; action("wait"); if(Math.abs(time-before-12)>1e-8)throw Error("wait timing");');
const before=run('JSON.stringify({spirit,food,belief,time,creature})');
store.set('shade.prototype.save.v1','{"version":1,"creature":{}}');run('loadWorld()');assert.equal(run('JSON.stringify({spirit,food,belief,time,creature})'),before);
store.set('shade.prototype.save.v1','invalid json');run('loadWorld()');assert.equal(run('JSON.stringify({spirit,food,belief,time,creature})'),before);
console.log('PASS: learned state and target round-trip; load pauses; wait advances 12 seconds; malformed saves preserve current world.');


run('document.querySelector("#reset").onclick(); paused=true; creature.target={x:600,y:315}; let startX=creature.x; action("wait"); if(creature.x<=startX)throw Error("no movement"); action("wait"); action("wait"); if(creature.learning!==1)throw Error("no help reward");');
run('document.querySelector("#reset").onclick(); const initialFood=food; const initialSpirit=spirit; action("rain"); if(food!==initialFood+27 || spirit!==initialSpirit-35)throw Error("rain costs");');
run('document.querySelector("#reset").onclick(); creature.target={x:620,y:250}; creature.x=620;creature.y=250; tick(.1); if(creature.learning!==0)throw Error("wandering incorrectly rewarded");');
console.log('PASS: 3D state snapshot, movement to village, helpful-action reward, rain resource costs, no reward for random wandering.');

run('document.querySelector("#reset").onclick(); Math.random=()=>.99; action("nudge"); if(creature.target!==null || spirit!==58 || creature.trust!==54)throw Error("refusal outcome");');
console.log('PASS: persuasion refusal remains possible and has resource/trust consequences.');
