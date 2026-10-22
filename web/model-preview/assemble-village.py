import sys,json
from pathlib import Path
sys.path.insert(0,str(Path('.local/python-libs').resolve()))
from PIL import Image
root=Path('.local/village-models');village={}
for name,idx in [('house1',74),('house2',75),('house3',76),('woman',443),('man',446),('beech',571),('oak',583),('pine',590)]:
 m=json.loads((root/f'{idx}.json').read_text())
 for k,p in enumerate(m['primitives']):
  if p['skin']==4294967295:
   p['skin']=-(idx*100+k);m['textures'].append({'id':p['skin'],'width':1,'height':1,'rgba':p['color'][:3]+[255]})
 for id in set(p['skin'] for p in m['primitives'] if p['skin']>=0):
  image=Image.open(root/f'{id}.dds').convert('RGBA');m['textures'].append({'id':id,'width':image.width,'height':image.height,'rgba':list(image.tobytes())})
 village[name]=m
image=Image.open('.local/terrain-textures/1.ppm').convert('RGBA')
village['grass']={'id':-900000,'width':image.width,'height':image.height,'rgba':list(image.tobytes())}
path=Path('.local/cow.json');m=json.loads(path.read_text());m['village']=village;path.write_text(json.dumps(m,separators=(',',':')))
