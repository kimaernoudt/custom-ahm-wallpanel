W,H=1600,1030
o=[]
def rect(x,y,w,h,title,fill='#f7f7f5',stroke='#222'):
    o.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="8" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')
    o.append(f'<text x="{x+w/2}" y="{y+24}" class="h">{title}</text>')
def t(x,y,s,cls='p',anchor='start',col=None):
    c=f' fill="{col}"' if col else ''
    o.append(f'<text x="{x}" y="{y}" class="{cls}" text-anchor="{anchor}"{c}>{s}</text>')
def line(pts,col='#333',w=2,dash=None):
    d=' '.join(f'{x},{y}' for x,y in pts); da=f' stroke-dasharray="{dash}"' if dash else ''
    o.append(f'<polyline points="{d}" fill="none" stroke="{col}" stroke-width="{w}"{da}/>')
def dot(x,y,col='#333'): o.append(f'<circle cx="{x}" cy="{y}" r="4" fill="{col}"/>')
def pin(x,y,label,side='r'):
    o.append(f'<circle cx="{x}" cy="{y}" r="3.5" fill="#fff" stroke="#222" stroke-width="1.5"/>')
    t(x+(-10 if side=='r' else 10),y+5,label,'p','end' if side=='r' else 'start')
def res(x,y,label,vertical=False):
    if vertical:
        o.append(f'<rect x="{x-7}" y="{y-18}" width="14" height="36" fill="#fff" stroke="#222" stroke-width="1.6"/>'); t(x+12,y+5,label,'s')
    else:
        o.append(f'<rect x="{x-18}" y="{y-7}" width="36" height="14" fill="#fff" stroke="#222" stroke-width="1.6"/>'); t(x,y-12,label,'s','middle')
RED,BLK,BLU,GRN,ORG,PUR='#d0021b','#222','#1f6feb','#1a7f37','#d4762c','#8250df'
o.append(f'<rect width="{W}" height="{H}" fill="#fff"/>')
t(30,40,'AHM wall panel: wiring','title')
t(30,64,'Olimex ESP32-POE + MCP23017 I/O expander · 4 encoders · 4 LED rings · 4 lit scene buttons','sub')
# Olimex
ox,oy,ow,oh=40,110,260,560
rect(ox,oy,ow,oh,'Olimex ESP32-POE')
t(ox+ow/2,oy+46,'(Ethernet + PoE = power)','s','middle')
P={}
rows=[('5V',RED),('3.3V',ORG),('GND',BLK),('GPIO13 (SDA)',BLU),('GPIO16 (SCL)',PUR),('GPIO36 (in)',GRN),('GPIO4',ORG),('GPIO33 (ADC)',BLU)]
for i,(n,c) in enumerate(rows):
    y=oy+100+i*52; pin(ox+ow,y,n,'r'); P[n]=(ox+ow,y)
o.append(f'<rect x="{ox+70}" y="{oy+oh-60}" width="120" height="40" rx="4" fill="#ddd" stroke="#222"/>'); t(ox+130,oy+oh-34,'RJ45 · PoE','s','middle')
# MCP23017
mx,my,mw,mh=520,190,330,520
rect(mx,my,mw,mh,'MCP23017 module')
t(mx+mw/2,my+46,'I²C address 0x20','s','middle')
L={}
lrows=['VDD','GND','SDA','SCL','INTA','A0–A2: GND','RESET: VDD']
for i,n in enumerate(lrows):
    y=my+90+i*50; pin(mx,y,n,'l'); L[n]=(mx,y)
R={}
rrows=['GPA0 · INPUT A','GPA1 · INPUT B','GPA2 · TRAP A','GPA3 · TRAP B','GPA4 · SAS A','GPA5 · SAS B','GPB0 · TOOG A','GPB1 · TOOG B','GPA6 · INPUT push','GPB2 · TRAP push','GPB3 · SAS push','GPB4 · TOOG push','GPA7 → START LED','GPB5 → IPAD LED','GPB6 → DJ LED','GPB7 → EXTRA LED']
for i,n in enumerate(rrows):
    y=my+70+i*28; pin(mx+mw,y,n,'r'); R[i]=(mx+mw,y)
# wires Olimex -> MCP
def wire(a,b,col,midx):
    line([a,(midx,a[1]),(midx,b[1]),b],col)
wire(P['3.3V'],L['VDD'],ORG,400)
wire(P['GND'],L['GND'],BLK,420)
wire(P['GPIO13 (SDA)'],L['SDA'],BLU,440)
wire(P['GPIO16 (SCL)'],L['SCL'],PUR,460)
wire(P['GPIO36 (in)'],L['INTA'],GRN,480)
# INT pull-up
res(360,P['GPIO36 (in)'][1]-40,'10k',True); line([(360,P['GPIO36 (in)'][1]-22),(360,P['GPIO36 (in)'][1])],GRN); dot(360,P['GPIO36 (in)'][1],GRN)
line([(360,P['GPIO36 (in)'][1]-58),(360,P['3.3V'][1])],ORG,2,'4 3'); dot(360,P['3.3V'][1],ORG)
# Encoders block
ex,ey=1020,110
rect(ex,ey,290,330,'4 × EC11 encoder')
for i,n in enumerate(['INPUT','TRAP','SAS','TOOG']):
    y=ey+60+i*66
    t(ex+20,y,n,'b'); t(ex+20,y+20,'A, B → MCP (see left)','s'); t(ex+20,y+38,'C (middle) + push leg → GND','s')
# connect encoder bus
line([(mx+mw+120,R[0][1]),(mx+mw+120,R[11][1])],BLU,6)
for i in range(12): line([R[i],(mx+mw+120,R[i][1])],BLU,1.5)
line([(mx+mw+120,ey+200),(ex,ey+200)],BLU,6); t(mx+mw+128,ey+192,'12 wires','s')
# button LEDs
bx,by=1020,480
rect(bx,by,290,190,'4 × 12 mm LED button (LEDs)')
t(bx+20,by+60,'LED + ← MCP GPA7, GPB5, GPB6, GPB7','s')
t(bx+20,by+82,'LED − → GND','s')
t(bx+20,by+104,'Buy the 3–6 V LED version','s')
t(bx+20,by+126,'(bare LED: add 150 Ω in series)','s')
line([(mx+mw+70,R[12][1]),(mx+mw+70,R[15][1])],RED,6)
for i in range(12,16): line([R[i],(mx+mw+70,R[i][1])],RED,1.5)
line([(mx+mw+70,R[15][1]),(mx+mw+70,by+150),(bx,by+150)],RED,6)
# LED rings
lx,ly=1330,110
rect(lx,ly,240,330,'4 × WS2812 ring')
for i,n in enumerate(['INPUT','TRAP','SAS','TOOG']):
    y=ly+70+i*62; o.append(f'<circle cx="{lx+50}" cy="{y}" r="20" fill="none" stroke="{ORG}" stroke-width="5" stroke-dasharray="4 4"/>'); t(lx+82,y-2,n,'b'); t(lx+82,y+16,'DIN ← prev DOUT' if i else 'DIN ← GPIO4 via 330 Ω','s')
    if i: line([(lx+50,y-42),(lx+50,y-20)],ORG,2)
t(lx+20,ly+318,'5V + GND to every ring','s')
# GPIO4 to rings (route along top)
g4=P['GPIO4']
res(345,g4[1],'330Ω'); line([g4,(327,g4[1])],ORG); line([(363,g4[1]),(380,g4[1]),(380,90),(lx+50,90),(lx+50,ly+50)],ORG)
# 5V to rings
p5=P['5V']; line([p5,(340,p5[1]),(340,80),(lx+200,80),(lx+200,ly)],RED)
o.append(f'<rect x="{lx+186}" y="{ly-60}" width="28" height="16" fill="#fff" stroke="#222"/>'); t(lx+180,ly-48,'1000 µF across 5V/GND','s','end')
# Button ladder
kx,ky=40,760
rect(kx,ky,1130,250,'4 × scene button: resistor ladder on GPIO33')
a=P['GPIO33 (ADC)']
line([a,(330,a[1]),(330,ky+120),(400,ky+120)],BLU)
res(430,ky+80,'10k to 3.3V',True); line([(430,ky+98),(430,ky+120)],BLU); dot(430,ky+120,BLU)
line([(400,ky+120),(1100,ky+120)],BLU)
o.append(f'<rect x="{360}" y="{ky+185}" width="40" height="20" fill="#fff" stroke="#222"/>'); t(410,ky+200,'100 nF to GND','s')
vals=[('START','0 Ω (wire)','≈0.00 V'),('IPAD','2.2k','≈0.59 V'),('DJ','4.7k','≈1.05 V'),('EXTRA','10k','≈1.65 V')]
for i,(n,r,v) in enumerate(vals):
    x=560+i*150
    dot(x,ky+120,BLU); line([(x,ky+120),(x,ky+150)],BLU)
    if i: res(x,ky+168,r,True)
    else: t(x+12,ky+172,r,'s')
    line([(x,ky+186 if i else ky+150),(x,ky+205)],BLU)
    o.append(f'<line x1="{x-12}" y1="{ky+205}" x2="{x+12}" y2="{ky+212}" stroke="#222" stroke-width="2"/>'); t(x+16,ky+212,n,'b')
    line([(x,ky+214),(x,ky+235)],BLK); o.append(f'<line x1="{x-10}" y1="{ky+235}" x2="{x+10}" y2="{ky+235}" stroke="#222" stroke-width="3"/>')
    t(x,ky+70,v,'s','middle')
t(560,ky+50,'Voltage on GPIO33 when pressed:','s')
svg=f'''<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">
<style>text{{font-family:Inter,DejaVu Sans,Arial}} .title{{font-size:28px;font-weight:700}} .sub{{font-size:15px;fill:#555}} .h{{font-size:17px;font-weight:700;text-anchor:middle}} .p{{font-size:14px}} .s{{font-size:13px;fill:#333}} .b{{font-size:15px;font-weight:700}}</style>
{''.join(o)}</svg>'''
open('wiring.svg','w').write(svg)
