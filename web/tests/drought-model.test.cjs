const assert=require('node:assert/strict');
const {Drought}=require('../drought-model.js');
const a=new Drought(),b=new Drought();a.step(40);for(let i=0;i<160;i++)b.step(.25);
for(const key of ['water','crops','forest','phase','energy','spring'])assert.equal(a[key],b[key],key+' must not depend on frame size');
const c=new Drought();assert.equal(c.influence('rain'),false);assert.equal(c.influence('care'),true);assert.equal(c.influence('explore'),false,'Nudges have a cooldown');
c.phase='care';c.elapsed=0;c.duration=1;c.step(1);assert.equal(c.deliveries,1);assert(c.belief>12);assert(c.water>52);assert(c.influence('praise'));assert(!c.influence('praise'),'Only one response per action');assert(c.habits.care>1);
const d=new Drought();d.spring=true;d.deliveries=2;d.belief=40;d.energy=100;d.urge='care';d.choose();assert.equal(d.phase,'channel');d.step(26);assert(d.channel);d.step(180);assert.equal(d.outcome,'won');
const e=new Drought();e.phase='rest';e.duration=1000;e.workers=[];e.step(180);assert.equal(e.outcome,'lost','Neglect cannot win');
const f=new Drought();f.belief=50;f.confidence=60;f.energy=70;assert(f.influence('rain'));f.choose();assert.equal(f.phase,'rain');f.step(18);assert(f.rain>0);assert(f.energy<70);
const g=new Drought();g.step(Infinity);assert.equal(g.time,0);g.outcome='won';g.step(1);assert.equal(g.time,0);
console.log('Drought timing, influence cooldown, learning, community water, failure, miracle and terminal states passed.');

const h=new Drought();h.water=10;h.phase="rest";h.duration=1000;h.step(23);assert(h.workers.every(w=>w.job==='hauling water'),'Villagers independently react to water shortage');assert(h.workers.every(w=>w.cycles===1));

const unguided=new Drought();unguided.step(180);assert.equal(unguided.outcome,'lost','Default scenario needs the player’s influence');
const guided=new Drought();for(let i=0;i<720;i++){if(guided.cooldown===0)guided.influence(guided.spring?'care':'explore');if(guided.last&&!guided.feedback)guided.influence('praise');guided.step(.25);}assert.equal(guided.outcome,'won');assert(guided.channel,'Teaching discovery and care creates a sustainable ending');

const blocked=new Drought();blocked.phase='care';blocked.duration=18;blocked.elapsed=17.9;blocked.workers.forEach(w=>w.progress=.9);
const noRoutes={care:false,explore:false,channel:false,workers:blocked.workers.map(()=>({'hauling water':false,'tending crops':false,'protecting trees':false}))};
blocked.step(1,noRoutes);assert.equal(blocked.deliveries,0);assert.equal(blocked.elapsed,17.9);assert(blocked.workers.every(w=>w.cycles===0&&w.progress===.9),'Blocked workers cannot earn rewards');
blocked.phase='idle';blocked.elapsed=0;blocked.duration=3;blocked.step(3,noRoutes);assert.equal(blocked.phase,'rest','Choose a reachable activity instead of walking through a blocked route');
const reassigned=new Drought();reassigned.step(.25,{workers:[{'hauling water':false,'tending crops':true}]});assert.equal(reassigned.workers[0].job,'tending crops');assert(reassigned.workers[0].progress>0);
console.log('Unreachable trips pause without rewards; idle actors choose safe alternative work.');
