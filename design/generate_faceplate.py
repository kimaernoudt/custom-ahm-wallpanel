import math
W,H=170,90
KX=[25,65,105,145]
knobs=list(zip(KX,['INPUT','TRAP','SAS','TOOG'])); KY=30; RR=15
btns=list(zip(KX,['START','IPAD','DJ','EXTRA'])); BY=68
holes=[(6,6),(W-6,6),(6,H-6),(W-6,H-6)]
def font(sz,w=600,anchor='middle'): return f'font-family="Inter, DejaVu Sans, Arial" font-size="{sz}" font-weight="{w}" text-anchor="{anchor}" letter-spacing="0.6"'
def ang(i): return math.radians(i*30-90)
c=[f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}mm" height="{H}mm" viewBox="0 0 {W} {H}">',
 '<!-- RED = cut, BLUE = engrave/print. Units: mm. -->',
 f'<g fill="none" stroke="#ff0000" stroke-width="0.1"><rect x="0" y="0" width="{W}" height="{H}" rx="4"/>']
for x,y in holes: c.append(f'<circle cx="{x}" cy="{y}" r="1.7"/>')
for x,_ in knobs:
    c.append(f'<circle cx="{x}" cy="{KY}" r="3.6"/>')
    for i in range(12): c.append(f'<circle cx="{x+RR*math.cos(ang(i)):.2f}" cy="{KY+RR*math.sin(ang(i)):.2f}" r="1.2"/>')
for x,_ in btns: c.append(f'<circle cx="{x}" cy="{BY}" r="6.1"/>')
c.append('</g><g fill="#0000ff">')
for x,l in knobs: c.append(f'<text x="{x}" y="{KY+25}" {font(4)}>{l}</text>')
for x,l in btns: c.append(f'<text x="{x}" y="{BY+11.5}" {font(3.2)}>{l}</text>')
c.append(f'<rect x="12" y="{KY+29.5}" width="146" height="0.3"/></g></svg>')
open('faceplate-cut.svg','w').write('\n'.join(c))
lvl={'TRAP':8,'SAS':6,'TOOG':12,'INPUT':7}; muted={'TOOG'}
p=[f'<svg xmlns="http://www.w3.org/2000/svg" width="1110" height="620" viewBox="-6.67 -6.67 {W+13.33} {H+13.33}">',
'''<defs><radialGradient id="kn" cx="40%" cy="35%" r="70%"><stop offset="0" stop-color="#5a5d62"/><stop offset="0.6" stop-color="#2b2d31"/><stop offset="1" stop-color="#141518"/></radialGradient>
<linearGradient id="pl" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#1d1e21"/><stop offset="1" stop-color="#121315"/></linearGradient>
<filter id="gl" x="-1" y="-1" width="3" height="3"><feGaussianBlur stdDeviation="0.6"/></filter>
<filter id="sh" x="-0.3" y="-0.3" width="1.6" height="1.6"><feDropShadow dx="0.4" dy="0.8" stdDeviation="0.8" flood-opacity="0.7"/></filter></defs>''',
f'<rect x="-6.67" y="-6.67" width="{W+13.33}" height="{H+13.33}" fill="#d9d6d0"/>',
f'<rect x="0" y="0" width="{W}" height="{H}" rx="4" fill="url(#pl)" filter="url(#sh)"/>',
f'<rect x="0.3" y="0.3" width="{W-0.6}" height="{H-0.6}" rx="3.8" fill="none" stroke="#2c2e32" stroke-width="0.4"/>']
for x,y in holes: p.append(f'<circle cx="{x}" cy="{y}" r="1.7" fill="#0b0b0c"/><circle cx="{x}" cy="{y}" r="1.1" fill="#3a3c40"/>')
for x,l in knobs:
    n=lvl[l]; col='#ff3b30' if l in muted else '#00e0d0'
    for i in range(12):
        cx,cy=x+RR*math.cos(ang(i)),KY+RR*math.sin(ang(i)); on=i<n
        if on: p.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="1.6" fill="{col}" opacity="0.55" filter="url(#gl)"/>')
        p.append(f'<circle cx="{cx:.2f}" cy="{cy:.2f}" r="1.2" fill="{col if on else "#2a2c30"}"/>')
    p.append(f'<circle cx="{x}" cy="{KY}" r="10" fill="url(#kn)" filter="url(#sh)"/>')
    p.append(f'<circle cx="{x}" cy="{KY}" r="8.6" fill="none" stroke="#45484d" stroke-width="0.3"/>')
    p.append(f'<line x1="{x}" y1="{KY-9}" x2="{x}" y2="{KY-6}" stroke="#e8e8e8" stroke-width="0.7" stroke-linecap="round"/>')
    p.append(f'<text x="{x}" y="{KY+25}" {font(4)} fill="#e6e6e6">{l}</text>')
    if l in muted: p.append(f'<text x="{x}" y="{KY+19.8}" {font(2.4)} fill="#ff3b30">MUTE</text>')
p.append(f'<rect x="12" y="{KY+29.5}" width="146" height="0.3" fill="#3a3c40"/>')
for x,l in btns:
    on=(l=='IPAD')
    p.append(f'<circle cx="{x}" cy="{BY}" r="6.1" fill="#0b0b0c"/>')
    if on: p.append(f'<circle cx="{x}" cy="{BY}" r="5.6" fill="#00e0d0" opacity="0.5" filter="url(#gl)"/>')
    p.append(f'<circle cx="{x}" cy="{BY}" r="5.4" fill="#26282c" filter="url(#sh)"/>')
    p.append(f'<circle cx="{x}" cy="{BY}" r="4.2" fill="none" stroke="{"#00e0d0" if on else "#3d4045"}" stroke-width="0.6"/>')
    p.append(f'<text x="{x}" y="{BY+11.5}" {font(3.2)} fill="{"#00e0d0" if on else "#bdbdbd"}">{l}</text>')
p.append(f'<text x="{W-10}" y="{H-2.5}" {font(2.2,500,"end")} fill="#55585d">CAFE · AHM-16</text></svg>')
open('faceplate-preview.svg','w').write('\n'.join(p))
