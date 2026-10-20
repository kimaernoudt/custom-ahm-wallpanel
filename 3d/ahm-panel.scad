// AHM cafe wall panel: faceplate + surface-mount back box
// Units: mm. Render a single part with:  openscad -D 'part="faceplate"' -o faceplate.stl ahm-panel.scad
// part = "faceplate" | "labels" | "backbox" | "buttoncap" | "assembly"
part = "assembly";
PCB = false;          // true = faceplate for the panel PCB (pcb/): light baffles instead of ring pockets

$fn = 64;

// ---- overall ----
W = 170;            // width (4 knobs at 40 mm so the 37 mm LED rings fit)
H = 90;             // height
R = 4;              // corner radius
T = 3;              // faceplate thickness

// ---- controls (x from left, y from top, like the 2D drawing) ----
KX     = [25, 65, 105, 145];
KNOBS  = ["INPUT", "TRAP", "SAS", "TOOG"];
BTNS   = ["START", "IPAD", "DJ", "EXTRA"];
KY     = 30;        // knob centre from top
BY     = 68;        // button centre from top
ENC_D  = 7.4;       // EC11 M7 bushing hole (+0.4 print clearance)
BTN_D  = 12.4;      // 12 mm panel push button (+0.4)
LED_R  = 15;        // LED radius on a 12-LED WS2812 ring (37 mm OD)
LED_D  = 2.4;       // LED window
RING_POCKET_D = 38.5;   // pocket on the back for the LED ring PCB
RING_POCKET_T = 1.2;
ENGRAVE = 0.6;      // label depth
// panel PCB version: PCB hangs on the encoder nuts, its front 6.5 mm behind the faceplate
PCB_GAP  = 6.5;     // PCB front to faceplate back (EC11E body height)
BAFFLE_D = 4.2;     // light tube round each ring LED window, stops bleed between LEDs
CAP_D = 12;         // translucent cap on a 6x6 mm, 5 mm tact switch
CAP_FLANGE_D = 15;
CAP_FLANGE_T = 1;
SW_H = 5;

// ---- screws ----
CS  = 6;            // corner screw inset
SCREW_D = 3.4;      // M3 clearance
CSK_D   = 6.6;      // countersink head

// ---- back box ----
D      = 35;        // box depth (wall to faceplate)
WALL   = 2.4;
FLOOR  = 3;         // back wall (against the room wall)
BOSS_D = 8;
INSERT_D = 4.0;     // M3 heat-set insert (use 2.8 for self-tapping M3)
INSERT_L = 6;
CABLE_D  = 20;      // Ethernet cable through the wall
KEY_BIG  = 8.5;     // keyhole head
KEY_SLOT = 4.2;     // keyhole slot (for ~4 mm wall screws)
KEY_X    = 40;      // keyholes at W/2 +/- KEY_X

function fy(y) = H - y;   // convert "from top" to model y

module rounded(w, h, t, r) {
    linear_extrude(t) offset(r) offset(-r) square([w, h]);
}

module label_text(h) {
    for (i = [0:3]) {
        translate([KX[i], fy(KY + 25), 0])
            linear_extrude(h) text(KNOBS[i], size = 4, font = "Inter:style=Bold",
                                   halign = "center", spacing = 1.12);
        translate([KX[i], fy(BY + 11.5), 0])
            linear_extrude(h) text(BTNS[i], size = 3.2, font = "Inter:style=Bold",
                                   halign = "center", spacing = 1.12);
    }
    translate([12, fy(KY + 29.5) - 0.2, 0]) cube([W - 24, 0.4, h]);
}

module led_windows(d, h, z) {
    for (i = [0:3]) for (a = [0:30:330])
        translate([KX[i] + LED_R * sin(a), fy(KY) + LED_R * cos(a), z]) cylinder(d = d, h = h);
}

module faceplate() {
    difference() {
        union() {
            rounded(W, H, T, R);
            // PCB version: tubes on the back reach down to 0.6 mm above the LEDs (1.6 mm high)
            if (PCB) led_windows(BAFFLE_D, PCB_GAP - 2.2, -(PCB_GAP - 2.2));
        }
        if (PCB) led_windows(LED_D, PCB_GAP + T, -PCB_GAP);
        for (i = [0:3]) {
            // encoder shaft
            translate([KX[i], fy(KY), -1]) cylinder(d = ENC_D, h = T + 2);
            // LED windows
            for (a = [0:30:330])
                translate([KX[i] + LED_R * sin(a), fy(KY) + LED_R * cos(a), -1])
                    cylinder(d = LED_D, h = T + 2);
            // pocket for the LED ring on the back
            if (!PCB) translate([KX[i], fy(KY), -0.01]) cylinder(d = RING_POCKET_D, h = RING_POCKET_T);
            // button
            translate([KX[i], fy(BY), -1]) cylinder(d = BTN_D, h = T + 2);
        }
        // corner screws, countersunk from the front
        for (p = [[CS, CS], [W - CS, CS], [CS, H - CS], [W - CS, H - CS]])
            translate([p[0], p[1], 0]) {
                translate([0, 0, -1]) cylinder(d = SCREW_D, h = T + 2);
                translate([0, 0, T - (CSK_D - SCREW_D) / 2]) cylinder(d1 = SCREW_D, d2 = CSK_D, h = (CSK_D - SCREW_D) / 2 + 0.01);
            }
        // engraved labels on the front face
        translate([0, 0, T - ENGRAVE]) label_text(ENGRAVE + 0.01);
    }
}

// labels as a separate body, for a white inlay on multi-material printers
module labels() { translate([0, 0, T - ENGRAVE]) label_text(ENGRAVE); }

// PCB version: translucent button cap (print in clear PETG/PLA), sits on the tact switch, flange behind the faceplate
module buttoncap() {
    stem = PCB_GAP + T + 1 - SW_H - CAP_FLANGE_T;   // stands 1 mm proud of the front
    cylinder(d = CAP_FLANGE_D, h = CAP_FLANGE_T);
    translate([0, 0, CAP_FLANGE_T]) cylinder(d = CAP_D, h = stem);
}

module keyhole() {
    translate([0, 0, -1]) {
        cylinder(d = KEY_BIG, h = FLOOR + 2);
        translate([-KEY_SLOT / 2, 0, 0]) cube([KEY_SLOT, 9, FLOOR + 2]);
        translate([0, 9, 0]) cylinder(d = KEY_SLOT, h = FLOOR + 2);
    }
}

module backbox() {
    difference() {
        union() {
            difference() {
                rounded(W, H, D, R);
                translate([WALL, WALL, FLOOR]) rounded(W - 2 * WALL, H - 2 * WALL, D, max(R - WALL, 0.5));
            }
            // screw bosses for the faceplate
            for (p = [[CS, CS], [W - CS, CS], [CS, H - CS], [W - CS, H - CS]])
                translate([p[0], p[1], 0]) cylinder(d = BOSS_D, h = D);
        }
        for (p = [[CS, CS], [W - CS, CS], [CS, H - CS], [W - CS, H - CS]])
            translate([p[0], p[1], D - INSERT_L]) cylinder(d = INSERT_D, h = INSERT_L + 1);
        // wall-mount keyholes (screw heads slide up into the slot)
        for (s = [-1, 1]) translate([W / 2 + s * KEY_X, H / 2 + 12, 0]) keyhole();
        // Ethernet cable entry from the wall
        translate([W / 2, H / 2 - 10, -1]) cylinder(d = CABLE_D, h = FLOOR + 2);
        // optional bottom cable notch for surface-run cable
        translate([W / 2 - 6, -1, FLOOR + 2]) cube([12, WALL + 2, 9]);
    }
}

// cut away everything in front of the button centre line (for the "pcbfit" section view)
module cut() difference() { children(); translate([-1, -1, -1]) cube([W + 2, fy(BY) + 1, D + 20]); }

if (part == "faceplate") faceplate();
else if (part == "labels") labels();
else if (part == "backbox") backbox();
else if (part == "buttoncap") buttoncap();
else if (part == "pcbfit") {   // section through the button row: faceplate, PCB on the encoder nuts, box
    color("#3a3d42") cut() backbox();
    color("#1c1d20") cut() translate([0, 0, D]) faceplate();
    // PCB 1.6 mm, notched corners for the bosses
    color("#1f7a3a") cut() translate([0, 0, D - PCB_GAP - 1.6]) linear_extrude(1.6) difference() {
        translate([4, 4]) square([W - 8, H - 8]);
        for (p = [[0, 0], [W - 11, 0], [0, H - 11], [W - 11, H - 11]]) translate(p) square(11);
    }
    for (i = [0:3]) {
        color("#9aa0a6") cut() translate([KX[i] - 6, fy(KY) - 6.5, D - PCB_GAP]) cube([12, 13, PCB_GAP]);   // EC11E body
        color("#c8c8c8") cut() translate([KX[i], fy(KY), D - PCB_GAP]) cylinder(d = 7, h = PCB_GAP + T + 2);
        color("#e0e0e0") cut() translate([KX[i] - 3, fy(BY) - 3, D - PCB_GAP]) cube([6, 6, SW_H]);
        color("#f2f2f2") cut() translate([KX[i], fy(BY), D - PCB_GAP + SW_H]) buttoncap();
    }
}
else {
    color("#3a3d42") backbox();
    color("#1c1d20") translate([0, 0, D]) faceplate();
    color("#e6e6e6") translate([0, 0, D]) labels();
}
