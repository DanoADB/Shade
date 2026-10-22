// Scene primitives are original placeholder geometry, independent of game data.
export function createScene(cow){
 const model={textures:[],primitives:[]};let id=-1;
 function mesh(vertices,indices,color){const skin=id--;model.textures.push({id:skin,rgba:Array.from({length:256*256},()=>[...color,255]).flat()});model.primitives.push({skin,vertices,indices});}
 function faces(points,polygons,color){const vertices=[],indices=[];for(const poly of polygons){const a=points[poly[0]],b=points[poly[1]],c=points[poly[2]];const u=b.map((v,k)=>v-a[k]),v=c.map((v,k)=>v-a[k]);let n=[u[1]*v[2]-u[2]*v[1],u[2]*v[0]-u[0]*v[2],u[0]*v[1]-u[1]*v[0]],length=Math.hypot(...n);n=n.map(x=>x/length);const start=vertices.length/8;for(const i of poly)vertices.push(...points[i],0,0,...n);for(let j=1;j<poly.length-1;j++)indices.push(start,start+j,start+j+1);}mesh(vertices,indices,color);}
 function box(x,y,z,w,h,d,color){faces([[x-w/2,y,z-d/2],[x+w/2,y,z-d/2],[x+w/2,y,z+d/2],[x-w/2,y,z+d/2],[x-w/2,y+h,z-d/2],[x+w/2,y+h,z-d/2],[x+w/2,y+h,z+d/2],[x-w/2,y+h,z+d/2]],[[0,3,2,1],[4,5,6,7],[0,1,5,4],[1,2,6,5],[2,3,7,6],[3,0,4,7]].map(face=>face.reverse()),color);}
 // Water, raised island, paths, and a village square.
 box(0,-.42,0,30,.1,25,[46,91,107]);box(0,-.3,0,20,.3,16,[82,112,73]);
 box(2,.005,0,8,.025,1.15,[153,138,102]);box(5,.005,0,4,.03,7,[153,138,102]);
 if(!cow?.village)for(const [x,z] of [[4,-3],[7,-2.7],[4,3],[7,2.8]]){
 box(x,0,z,1.8,1.35,1.7,[207,184,140]);
 faces([[x-1.1,1.35,z-1],[x+1.1,1.35,z-1],[x,2.15,z-1],[x-1.1,1.35,z+1],[x+1.1,1.35,z+1],[x,2.15,z+1]],[[0,2,1],[3,4,5],[0,3,5,2],[2,5,4,1]],[147,82,59]);
 box(x,.02,z- .86,.42,.85,.04,[75,57,42]);
 }
 if(!cow?.village?.oak)for(const [x,z] of [[-7,-5],[-5,-5],[-7,4],[0,5],[8,5],[0,-6]]){box(x,0,z,.25,1.2,.25,[101,76,47]);box(x,1,z,1.2,1.4,1.2,[52,94,65]);}
 // Small people around the square make the creature's scale readable.
 if(!cow?.village)for(const [x,z] of [[4,0],[5,1],[6,-.5],[5,-1.5]]){box(x,0,z,.18,.35,.18,[166,110,73]);box(x,.35,z,.18,.18,.18,[225,187,138]);}
 if(cow?.village){
 function place(source,x,z,size,isPerson=false){
 const low=[Infinity,Infinity,Infinity],high=[-Infinity,-Infinity,-Infinity];
 for(const p of source.primitives)for(let i=0;i<p.vertices.length;i+=8)for(let k=0;k<3;k++){low[k]=Math.min(low[k],p.vertices[i+k]);high[k]=Math.max(high[k],p.vertices[i+k]);}
 const scale=size/(isPerson?high[1]-low[1]:Math.max(high[0]-low[0],high[2]-low[2])),cx=(low[0]+high[0])/2,cz=(low[2]+high[2])/2;
 model.textures.push(...source.textures);
 for(const p of source.primitives){const vertices=[...p.vertices];for(let i=0;i<vertices.length;i+=8){vertices[i]=(vertices[i]-cx)*scale+x;vertices[i+1]=(vertices[i+1]-low[1])*scale;vertices[i+2]=(vertices[i+2]-cz)*scale+z;}model.primitives.push({...p,vertices});}
 }
 const v=cow.village;
 if(v.grass){model.textures.push(v.grass);model.primitives.push({skin:v.grass.id,vertices:[-10,.002,-8,0,0,0,1,0, 10,.002,-8,12,0,0,1,0, 10,.002,8,12,10,0,1,0, -10,.002,8,0,10,0,1,0],indices:[0,2,1,0,3,2]});}
 if(v.oak){place(v.beech,-7,-5,3.1,true);place(v.oak,-5,-5,3.4,true);place(v.pine,-7,4,3.8,true);place(v.beech,0,5,3,true);place(v.oak,8,5,3.4,true);place(v.pine,0,-6,3.6,true);}
 place(v.house1,4,-3,2.6);place(v.house2,7,-2.7,2.6);place(v.house3,4,3,2.6);place(v.house1,7,2.8,2.6);
 place(v.woman,4,0,.65,true);place(v.man,5,1,.68,true);place(v.woman,6,-.5,.63,true);place(v.man,5,-1.5,.7,true);
 }
 if(cow){
 const low=[Infinity,Infinity,Infinity],high=[-Infinity,-Infinity,-Infinity];for(const p of cow.primitives)for(let i=0;i<p.vertices.length;i+=8)for(let k=0;k<3;k++){low[k]=Math.min(low[k],p.vertices[i+k]);high[k]=Math.max(high[k],p.vertices[i+k]);}
 const scale=4.2/(high[1]-low[1]),cx=(low[0]+high[0])/2,cz=(low[2]+high[2])/2;
 model.textures.push(...cow.textures);
 for(const p of cow.primitives){const vertices=[...p.vertices];for(let i=0;i<vertices.length;i+=8){vertices[i]=(vertices[i]-cx)*scale-3;vertices[i+1]=(vertices[i+1]-low[1])*scale;vertices[i+2]=(vertices[i+2]-cz)*scale;}model.primitives.push({...p,vertices,actor:true});}
 }else{
 const actorStart=model.primitives.length;
 box(-3,0,0,1.8,2.8,1.3,[173,165,184]);box(-3,2.6,.3,1.2,1.25,1.1,[194,181,196]);
 for(let i=actorStart;i<model.primitives.length;i++)model.primitives[i].actor=true;
 }
 return model;
}
