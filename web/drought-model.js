// Deterministic drought rules, independent of rendering and browser timing.
(function(root,factory){if(typeof module==='object')module.exports=factory();else root.ShadeDrought=factory();})(typeof globalThis!=='undefined'?globalThis:this,()=>{
  const clamp=n=>Math.max(0,Math.min(100,n));
  class Drought {
    constructor(seed=23){this.seed=seed;this.time=0;this.water=52;this.crops=66;this.forest=73;this.energy=85;this.bond=55;this.belief=12;this.confidence=30;this.spring=false;this.channel=false;this.rain=0;this.deliveries=0;this.habits={care:1,explore:1,rest:.8};this.urge='';this.cooldown=0;this.phase='idle';this.elapsed=0;this.duration=3;this.last='';this.feedback=false;this.feedbackUntil=0;this.outcome='';this.workers=[{name:'Water gatherer',job:'hauling water',progress:0,cycles:0},{name:'Farmer',job:'tending crops',progress:0,cycles:0},{name:'Forester',job:'protecting trees',progress:0,cycles:0}];this.events=[];this.note('The village waits for its animal god. Nobody knows you are here.');}
    random(){this.seed=(1664525*this.seed+1013904223)>>>0;return this.seed/4294967296;}
    note(text){this.events.unshift({time:Math.floor(this.time),text});this.events.length=Math.min(5,this.events.length);}
    influence(kind){
      if(this.outcome)return false;
      if(kind==='praise'||kind==='discourage'){
        if(!this.last||this.feedback||this.time>this.feedbackUntil)return false;
        this.feedback=true;const good=kind==='praise';
        if(this.habits[this.last]!==undefined)this.habits[this.last]=Math.max(.15,Math.min(4,this.habits[this.last]*(good?1.35:.65)));
        this.bond=clamp(this.bond+(good?5:-3));this.note(good?'Your warmth reinforces its last choice.':'It senses your unease and questions that habit.');return true;
      }
      if(this.cooldown>0||!['care','explore','rest','rain'].includes(kind))return false;
      if(kind==='rain'&&!this.canRain())return false;
      this.urge=kind;this.cooldown=5;this.bond=clamp(this.bond-2);this.note({care:'You stir compassion. It will weigh the village’s need.',explore:'You kindle curiosity about water beyond the village.',rest:'You offer calm. It may choose to recover.',rain:'You share a dream of rain. The creature must choose to answer.'}[kind]);return true;
    }
    canRain(){return this.belief>=44&&this.confidence>=48&&this.energy>=45&&this.rain===0;}
    choose(){
      let chosen;
      if(this.energy<18)chosen='rest';
      else if(this.urge==='rain'&&this.canRain())chosen='rain';
      else if(this.spring&&!this.channel&&this.deliveries>=2&&this.belief>=28&&this.urge==='care')chosen='channel';
      else {
        const weights={care:this.habits.care*(this.water<40?2:1),explore:this.habits.explore*(this.spring?.25:1),rest:this.habits.rest*(this.energy<45?2:.4)};
        if(weights[this.urge]!==undefined)weights[this.urge]*=1+this.bond/35;
        let pick=this.random()*Object.values(weights).reduce((a,b)=>a+b,0);
        chosen=Object.keys(weights).find(k=>(pick-=weights[k])<=0)||'care';
      }
      this.urge='';this.phase=chosen;this.elapsed=0;this.duration={care:18,explore:20,rest:12,channel:26,rain:18}[chosen];
    }
    finish(){
      const act=this.phase;this.last=act;this.feedback=false;this.feedbackUntil=this.time+12;
      if(act==='care'){
        this.deliveries++;this.water=clamp(this.water+23);this.forest=clamp(this.forest-(this.spring?1:4));this.energy=clamp(this.energy-15);this.belief=clamp(this.belief+9);this.confidence=clamp(this.confidence+7);
        this.note('The creature carries water home. Villagers call it their protector.');
      }else if(act==='explore'){
        this.energy=clamp(this.energy-12);this.confidence=clamp(this.confidence+6);
        if(!this.spring){this.spring=true;this.note('It discovers an upstream spring. Compassion could turn discovery into a channel.');}
        else{this.forest=clamp(this.forest+4);this.note('It guards the spring’s trees. Villagers begin calling it keeper of the watershed.');}
      }else if(act==='rest'){this.energy=clamp(this.energy+38);this.note('The creature rests. Villagers wait; even their god has limits.');}
      else if(act==='channel'){this.channel=true;this.energy=clamp(this.energy-28);this.belief=clamp(this.belief+18);this.note('It opens a channel from the spring. Villagers maintain it and credit their animal god.');}
      else if(act==='rain'){this.rain=65;this.water=clamp(this.water+42);this.crops=clamp(this.crops+14);this.forest=clamp(this.forest+12);this.energy=clamp(this.energy-40);this.note('Rain answers the creature. Villagers now expect their god to bring it again.');}
      this.phase='idle';this.elapsed=0;this.duration=3;
    }
    step(dt){
      if(!Number.isFinite(dt)||dt<0)return;
      while(dt>0&&!this.outcome){const n=Math.min(dt,.25);dt-=n;this.time+=n;this.cooldown=Math.max(0,this.cooldown-n);this.rain=Math.max(0,this.rain-n);
        this.water=clamp(this.water+n*(-.9+(this.channel?.72:0)+(this.rain>0?.65:0)));
        this.crops=clamp(this.crops+n*(this.water<25?-.35:this.channel||this.rain>0?.16:-.045));
        this.forest=clamp(this.forest+n*(this.rain>0?.12:this.channel?.025:-.09));
        for(const worker of this.workers){
          worker.progress+=n/22;
          if(worker.progress>=1){worker.progress-=1;worker.cycles++;
            if(worker.job==='hauling water')this.water=clamp(this.water+(this.channel?6:2));
            if(worker.job==='tending crops'&&this.water>15)this.crops=clamp(this.crops+1.4);
            if(worker.job==='protecting trees')this.forest=clamp(this.forest+.9);
            worker.job=this.water<24?'hauling water':this.crops<38?'tending crops':this.forest<40?'protecting trees':{ 'Water gatherer':'hauling water',Farmer:'tending crops',Forester:'protecting trees'}[worker.name];
          }
        }
        this.elapsed+=n;if(this.elapsed>=this.duration){if(this.phase==='idle')this.choose();else this.finish();}
        if(this.crops<=5){this.outcome='lost';this.note('The harvest fails. Try another way of teaching your creature.');}
        else if(this.time>=180){this.outcome=this.water>=25&&this.crops>=40&&this.forest>=35?'won':'lost';this.note(this.outcome==='won'?(this.channel?'The village survives through shared water. Its god taught resilience.':'The village survives. Its faith rests on the creature’s continuing care.'):'The village cannot sustain its water supply. Its god needs a different lesson.');}
      }
    }
  }
  const SAVE_KEY='shade.drought.v1';
  const phases={idle:3,care:18,explore:20,rest:12,channel:26,rain:18};
  const jobs=['hauling water','tending crops','protecting trees'];
  function decodeSave(text){
    if(typeof text!=='string'||text.length>16000)throw new Error('Invalid save');
    const save=JSON.parse(text),s=save&&save.state;
    if(save.format!=='shade-drought'||save.version!==1||!s||typeof s!=='object')throw new Error('Unsupported save');
    const range=(n,min,max)=>typeof n==='number'&&Number.isFinite(n)&&n>=min&&n<=max;
    const integer=(n,min,max)=>Number.isInteger(n)&&range(n,min,max);
    const member=(v,values)=>values.includes(v);
    if(!integer(save.god,0,2)||typeof save.follow!=='boolean'||!integer(s.seed,0,4294967295)||!range(s.time,0,180.25))throw new Error('Invalid run');
    for(const k of ['water','crops','forest','energy','bond','belief','confidence'])if(!range(s[k],0,100))throw new Error('Invalid village');
    for(const k of ['spring','channel','feedback'])if(typeof s[k]!=='boolean')throw new Error('Invalid flags');
    if(s.channel&&!s.spring)throw new Error('Invalid channel');
    if(!range(s.rain,0,65)||!integer(s.deliveries,0,20)||!range(s.cooldown,0,5)||!range(s.feedbackUntil,0,193)||!member(s.phase,Object.keys(phases))||s.duration!==phases[s.phase]||!range(s.elapsed,0,s.duration)||!member(s.last,['',...Object.keys(phases).filter(p=>p!=='idle')])||!member(s.urge,['','care','explore','rest','rain'])||!member(s.outcome,['','won','lost']))throw new Error('Invalid action');
    if(!s.habits||!['care','explore','rest'].every(k=>range(s.habits[k],.15,4)))throw new Error('Invalid learning');
    const model=new Drought();
    if(!Array.isArray(s.workers)||s.workers.length!==3||!s.workers.every((w,i)=>w&&w.name===model.workers[i].name&&member(w.job,jobs)&&range(w.progress,0,1)&&integer(w.cycles,0,9)))throw new Error('Invalid workers');
    if(!Array.isArray(s.events)||s.events.length>5||!s.events.every(e=>e&&integer(e.time,0,181)&&e.time<=s.time&&typeof e.text==='string'&&e.text.length<=300))throw new Error('Invalid history');
    // Copy only known fields; saved objects cannot replace methods or prototypes.
    for(const k of Object.keys(model))if(!['habits','workers','events'].includes(k))model[k]=s[k];
    model.habits={care:s.habits.care,explore:s.habits.explore,rest:s.habits.rest};
    model.workers=s.workers.map(w=>({name:w.name,job:w.job,progress:w.progress,cycles:w.cycles}));
    model.events=s.events.map(e=>({time:e.time,text:e.text}));
    return {model,god:save.god,follow:save.follow};
  }
  function encodeSave(model,god,follow){
    const text=JSON.stringify({format:'shade-drought',version:1,god,follow,state:model});
    decodeSave(text);return text;
  }
  return {Drought,SAVE_KEY,encodeSave,decodeSave};
});
