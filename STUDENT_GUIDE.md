# 🎓 RGBGameEngine — Student & Beginner Friendly Guide

Welcome to **RGBGameEngine**! This guide is written so that **any student or beginner** can easily understand how a 2D game engine works on the ESP32-S3 800×480 RGB display and start creating animations, lighting, and physics games in minutes.

---

## 🧠 1. How Does the Screen Work? (The Flipbook Analogy)

Imagine drawing a cartoon on a flipbook:
1. If you draw directly on the page while your friend is looking at it, they will see your hand erasing and redrawing (this causes **flickering/tearing**).
2. Instead, **RGBGameEngine** uses **Two Pages (Double Buffering)**:
   - **Front Buffer (Page A):** The screen shows this page to the player.
   - **Draw Buffer (Page B):** You draw your background, shapes, lights, and characters here in secret!
   - **`engine.show()`:** Once you finish drawing Page B, the engine swaps the two pages in a single flash!

### Your First 10-Line Program
```cpp
#include "RGBGameEngine.h"

RGBGameEngine engine;

void setup() {
    engine.begin(); // Turn on the 800x480 screen
}

void loop() {
    engine.clear(Colors565::MidnightBlue);          // 1. Paint background dark blue
    engine.fillCircle(400, 240, 50, Colors565::Gold); // 2. Draw a golden ball in the center
    engine.drawText(310, 100, "HELLO WORLD!", Colors565::White, 3); // 3. Write text
    engine.show();                                  // 4. Flip the page to show it!
}
```

---

## 📐 2. Screen Coordinates (`X` and `Y`)

The screen is **800 pixels wide** and **480 pixels tall**:
- **`(0, 0)`** is the **Top-Left** corner.
- **`(400, 240)`** is the exact **Center** of the screen.
- **`(799, 479)`** is the **Bottom-Right** corner.
- Moving **Right** increases `X`. Moving **Down** increases `Y`.

---

## 🎨 3. Choosing Colors (`Colors565` & Color Physics)

You have **three easy ways** to pick a color:

### Way 1: Pick from 140+ Ready-Made Names (`Colors565::`)
```cpp
uint16_t c1 = Colors565::Crimson;
uint16_t c2 = Colors565::ElectricBlue;
uint16_t c3 = Colors565::Emerald;
uint16_t c4 = Colors565::SakuraPink;
```

### Way 2: Mix Your Own Red, Green, and Blue (`0` to `255`)
```cpp
Color myPurple(180, 40, 255);
engine.fillRect(100, 100, 120, 80, myPurple);
```

### Way 3: Use Real Science (Rainbow HSV, Heat Kelvin, or Light Wavelength!)
```cpp
// 1. Rainbow Hue (0 to 360 degrees)
uint16_t rainbowColor = Color::hsv565(120, 255, 255); // Pure green

// 2. Heat Temperature in Kelvin (1000K = glowing red coal, 5500K = sunlight, 10000K = blue star)
uint16_t hotMetal = Color::kelvin565(3500);

// 3. Real Light Wavelength in Nanometers (380nm Violet -> 750nm Red)
uint16_t laserColor = Color::wavelength565(650.0f); // 650nm Red Laser!
```

---

## 🔷 4. Drawing Every Shape

Every shape has a **`draw...`** version (outline only) and a **`fill...`** version (solid inside):

| What You Want to Draw | Code Example |
| :--- | :--- |
| **Single Dot (Pixel)** | `engine.drawPixel(400, 240, Colors565::White);` |
| **Line / Thick Line** | `engine.drawThickLine(50, 50, 300, 150, 6, Colors565::Gold);` |
| **Smooth Anti-Aliased Line** | `engine.drawAntiAliasedLine(50, 80, 300, 180, Colors565::Cyan);` |
| **Dashed Line** | `engine.drawDashedLine(50, 200, 350, 200, 10, 6, Colors565::White);` |
| **Rectangle** | `engine.fillRect(100, 100, 200, 120, Colors565::RoyalBlue);` |
| **Rounded Button Box** | `engine.fillRoundRect(100, 100, 200, 80, 16, Colors565::Emerald);` |
| **Circle (Ball)** | `engine.fillCircle(400, 240, 45, Colors565::Crimson);` |
| **Donut Ring** | `engine.fillRing(400, 240, 30, 50, Colors565::Gold);` |
| **Curved Health/Speed Ring** | `engine.fillArcRing(400, 240, 35, 50, 0, 240, Colors565::NeonGreen);` |
| **Pill / Capsule** | `engine.fillCapsule(100, 200, 220, 200, 25, Colors565::Turquoise);` |
| **Star (5-Pointed, 8-Pointed)**| `engine.fillStar(400, 240, 5, 60, 25, 0.0f, Colors565::Gold);` |
| **Hexagon / Octagon** | `engine.fillRegularPolygon(400, 240, 6, 50, 0.0f, Colors565::Amethyst);` |
| **Curved Bezier Line** | `engine.drawCubicBezier(50,300, 150,100, 250,400, 350,300, Colors565::Yellow, 3);` |
| **3D Rotating Cube** | `engine.drawCube3D(400, 240, 100, angleX, angleY, 0, Colors565::NeonCyan);` |

---

## 👻 5. Transparency & Glowing Blend Modes

Want to draw **see-through glass**, **ghosts**, or **glowing lasers**?
Just use `Alpha` (`0` = invisible, `128` = 50% see-through, `255` = solid):

```cpp
// Draw a 50% see-through blue glass panel
engine.fillRoundRectAlpha(200, 120, 400, 240, 20, Colors565::SkyBlue, 128);

// Draw a glowing laser circle that ADDS light to whatever is behind it!
engine.fillCircleBlend(400, 240, 60, Colors565::NeonPink, 180, BlendMode::ADDITIVE);
```

---

## 🌈 6. Smooth Gradients & 3D Shading

```cpp
// Smooth Sunset Sky (Vertical Gradient from Dark Blue to Orange)
engine.fillLinearGradientRect(0, 0, 800, 480, Color(15, 20, 60), Color(255, 110, 40), true, true);

// 3D Shaded Sphere (Radial Gradient from White highlight to Deep Blue edge)
engine.fillRadialGradientCircle(400, 240, 65, Color(255, 255, 255), Color(10, 30, 140), true);

// 3-Color Rainbow Triangle (Gouraud Shading)
engine.fillTriangleGouraud(400, 100, Colors565::Red, 250, 350, Colors565::PureGreen, 550, 350, Colors565::Blue);
```

---

## 💡 7. Dynamic Lights & Real-Time Shadows (`LightSystem2D`)

Want a lamp or flashlight that casts real shadows behind balls and walls?
1. First draw the shadows with `LightSystem2D::castCircleShadow` or `castBoxShadow`.
2. Then draw the light glow with `LightSystem2D::drawPointLight` or `drawLineLight`.

```cpp
float lightX = 200, lightY = 150;
float ballX  = 350, ballY  = 240, ballR = 40;

// 1. Cast a soft shadow behind the ball away from the light!
LightSystem2D::castCircleShadow(engine, lightX, lightY, ballX, ballY, ballR);

// 2. Draw the glowing light source (intensity = 1.2)
LightSystem2D::drawPointLight(engine, lightX, lightY, 180.0f, Colors565::Amber, 1.2f);

// 3. Draw the ball itself
engine.fillCircle(ballX, ballY, ballR, Colors565::Crimson);
```

---

## 💥 8. Shattering Effect (Glass, Stone, Wood, Sparks)

Break any object into flying pieces when it gets hit!
```cpp
ShatterSystem2D glassShatter;

// When hit: shatter at (400, 240) with radius 80, material = GLASS, intensity = 1.2
glassShatter.triggerShatter(400, 240, 80.0f, 400, 240, ShatterMaterial::GLASS, 1.2f, Colors565::ElectricBlue);

// Inside loop():
glassShatter.update(dt);
glassShatter.draw(engine);
```
You can switch `ShatterMaterial::GLASS` to `STONE`, `WOOD`, `METAL_SPARKS`, or `PIXEL_DISSOLVE`!

---

## 🛋️ 9. Squishy Cushions & Flowing Cloth (`SoftBodyCushion2D` & `VerletCloth2D`)

### Squishy Pillow / Cushion
```cpp
SoftBodyCushion2D pillow;
pillow.init(250, 280, 300, 120, Colors565::Crimson, Colors565::Gold);

// In loop(): press down at X=400 with radius 60 and depth 45px
pillow.applyPress(400, 60.0f, 45.0f);
pillow.update(dt);
pillow.draw(engine);
```

### Wind-Blown Cloth Curtain & Ninja Scarf
```cpp
VerletCloth2D curtain;
curtain.init(250, 80, 22.0f, Colors565::Crimson, Colors565::Gold);

// In loop(): push cloth with a ball at (ballX, ballY) and blow wind!
curtain.interactCircle(ballX, ballY, 40.0f);
curtain.update(dt, 150.0f, 680.0f);
curtain.draw(engine, true);
```

---

## ✨ 10. Shape Particle Emitter & Behavior Patterns (`ParticleSystem2D`)

Want particles to spawn from **any shape** (a Point, a Line, a Circle's outer edge, inside a Circle's surface, a Rectangle, a Ring, or a Triangle) and move in cool patterns?

### Step 1: Choose Where Particles Spawn From
* **`EmitRegionMode::EDGE_PERIMETER`** — Spawns along the outer border/outline of the shape.
* **`EmitRegionMode::SURFACE_AREA`** — Spawns anywhere inside the filled area of the shape.
* **`EmitRegionMode::CENTER_POINT`** — Spawns from the center.
* **`EmitRegionMode::VERTICES_ONLY`** — Spawns only from the corners!

### Step 2: Choose How Particles Behave (`ParticleBehavior`)
* **`PARALLEL`** — Straight parallel stream (like calm vertical rainfall or a laser beam).
* **`PARALLEL_WIND`** — Parallel stream pushed sideways by gusting wind!
* **`SLIGHTLY_CHAOTIC`** — Gentle swirling drift (like rising campfire embers or steam).
* **`TOTALLY_CHAOTIC`** — Wild 360° electric sparks and plasma!
* **`OUTWARD_NORMAL`** — Shoots outward perpendicular to the shape's edges.
* **`INWARD_IMPLODE`** — Sucked inward toward the shape's center (power-up charge!).
* **`VORTEX_SWIRL`** — Spins in a tornado/whirlpool around the shape.
* **`HARMONIC_WAVE`** — Flies in a sine-wave pattern.
* **`GRAVITY_BOUNCE`** — Launches like a fountain and bounces on the floor.
* **`HOMING_ATTRACTOR`** — Flies out chaotically, then curves toward a target!

```cpp
ParticleSystem2D emitter;

// Emit from the outer edge of a Circle at (400, 240) with radius 90
emitter.setCircleEmitter(400, 240, 90.0f, EmitRegionMode::EDGE_PERIMETER);
emitter.behavior = ParticleBehavior::SLIGHTLY_CHAOTIC;
emitter.renderStyle = ParticleRenderStyle::ADDITIVE_GLOW;

// In loop():
emitter.update(dt);
emitter.draw(engine);
```
