# 🎮 RGBGameEngine (ESP32-S3 800×480 RGB565 2D & 2.5D Engine)

**RGBGameEngine** is a hardware-synchronized, double-buffered 2D/2.5D graphics, dynamic lighting, fracture, soft-body, cloth, and particle physics game engine built for the **Waveshare ESP32-S3 800×480 RGB LCD** (with Octal PSRAM and CH422G I/O Expander).

---

## ✨ Complete Feature Matrix

| Module | Header / Files | Capabilities |
| :--- | :--- | :--- |
| **Core Display & Shapes** | [`RGBGameEngine.h`](RGBGameEngine.h), [`RGBGameEngine.cpp`](RGBGameEngine.cpp) | VSYNC tear-free double buffering, 32-bit aligned dual-pixel scanline fill, Pixel, Fast H/V Line, Thick Line, Dashed Line, Xiaolin Wu Anti-Aliased Line, Rect, RoundRect, Quad, Diamond, Polyline, Scanline Concave/Convex Polygon, Regular N-Gon, N-Pointed Star, Triangle, Gouraud Triangle, Circle, Ring, Ellipse, Arc, Pie Sector, ArcRing, Capsule, Quadratic/Cubic Bezier, Catmull-Rom Spline, Arrow, Crosshair, Depth-Sorted 3D Cube/Pyramid/Sphere |
| **Color & Light Physics** | [`Color.h`](Color.h), [`Color.cpp`](Color.cpp) | 140+ precomputed `Colors565::` constants, HSV/HSL conversion, Blackbody Kelvin temperature (`1000K–15000K`), Visible Light Wavelength (`380nm–750nm`), 4×4 Bayer ordered dithering, grayscale, sepia, invert |
| **17 Transparency & Blend Modes** | [`Color.h`](Color.h), [`PixelFormat.h`](PixelFormat.h) | Exact per-channel overflow-free `blend565` & `scaleBrightness565`, plus 17 blend modes: `SOLID`, `ALPHA`, `ADDITIVE`, `SUBTRACTIVE`, `MULTIPLY`, `SCREEN`, `OVERLAY`, `SOFT_LIGHT`, `HARD_LIGHT`, `COLOR_DODGE`, `COLOR_BURN`, `DIFFERENCE`, `EXCLUSION`, `COLOR_TINT`, `LIGHTEN`, `DARKEN`, `CHECKER_STIPPLE`, Vertical Alpha Fade & Glass Lens Refraction |
| **Gradients & Post-FX** | [`RGBGameEngine.h`](RGBGameEngine.h) | Linear (H/V/Diagonal) & Radial dithered gradients, Soft Drop Shadows, Screen Tint, Red Game-Over Glass Filter, CRT Scanlines, Vignette, Shockwave Refraction Ring, Iris Circle Wipe |
| **Sprites & Built-in Font** | [`RGBGameEngine.h`](RGBGameEngine.h) | Color-key sprite blit, H/V Flip, Alpha Sprite, Tinted Sprite, Nearest Scaling, Fixed-Point Rotation, Skewed Ground Silhouette Shadow, 5×7 Scalable ASCII Font (`drawText`, `drawTextShadow`, `drawTextOutline`) |
| **Dynamic Lights & Ray-Casted Shadows** | [`RGBLightAndShadow.h`](RGBLightAndShadow.h) | Single-pass radial `drawPointLight`, Spot/Cone light, Capsule SDF photorealistic `drawLineLight` (Laser/Lightsaber/Neon Stick), Rect Area Light; Intensity- & Distance-coupled Analytic Umbra/Penumbra Shadows (`castCircleShadow`, `castBoxShadow`); 2D Ray-Casted Visibility Fan (`drawRaytracedLight`); Recursive Jagged Lightning Bolts |
| **Physics, Fracture, Cushion & Cloth** | [`RGBPhysics2D.h`](RGBPhysics2D.h) | `RigidBody2D` & Circle Impulse Collisions, Hooke's Law `Spring2D` with live ball collisions, Contiguous Planar Fracture `ShatterSystem2D` (`GLASS`, `STONE`, `WOOD`, `METAL_SPARKS`, `PIXEL_DISSOLVE`), 33-Node Volume-Preserving `SoftBodyCushion2D` with two-way elastic ball rebound, 3D Surface-Normal Shaded `VerletCloth2D` (wind + tearable threads) & `VerletRope2D` (shaded ribbon scarf), 2-Bone `IK2D` solver, `WaterSurface2D` wave ripples, `WeatherSystem2D`, `RibbonTrail2D`, `UIWidgets` |
| **Shape Particle & Fire Physics** | [`RGBPhysics2D.h`](RGBPhysics2D.h) | `ParticleSystem2D`: Emits from any shape (`POINT`, `LINE`, `CIRCLE`, `RING`, `RECTANGLE`, `TRIANGLE`) via `SURFACE_AREA`, `EDGE_PERIMETER`, `CENTER_POINT`, or `VERTICES_ONLY` with 11 behavior patterns (`PARALLEL`, `PARALLEL_WIND`, `SLIGHTLY_CHAOTIC`, `TOTALLY_CHAOTIC`, `OUTWARD_NORMAL`, `INWARD_IMPLODE`, `VORTEX_SWIRL`, `HARMONIC_WAVE`, `GRAVITY_BOUNCE`, `HOMING_ATTRACTOR`, `REALISTIC_FIRE`) and 4-stage Blackbody thermal cooling (`FIRE_PLUME`) |

---

## 📂 Repository Structure

```text
RGBGameEngine/
├── RGBGameEngine.ino          # Interactive 9-Scene Live Showcase & Hardware Test Suite
├── RGBGameEngine.h            # Main Engine Header (includes all subsystems automatically)
├── RGBGameEngine.cpp          # Core Rasterizer, Shapes, Gradients, Sprites, Font & Post-FX
├── RGBLightAndShadow.h        # Dynamic 2D Lighting, Capsule SDF Stick, Shadows & Lightning
├── RGBPhysics2D.h             # RigidBody, Springs, Planar Shatter, Cushion, Cloth, Fire & UI
├── Color.h / Color.cpp        # 140+ RGB565 Palette, HSV/Kelvin/Wavelength & 17 Blend Modes
├── Point.h                    # Point (int16), Vec2 (float), Vec3 (3D projection) & Rect2D
├── PixelFormat.h              # PixelFormat & 17 BlendMode enums
├── PixelConverter.h / .cpp    # Color to RGB565 converter
├── waveshare_lcd_port.h / .cpp# Waveshare 800x480 RGB Panel DMA & VSYNC Callback Driver
├── esp_io_expander.h          # CH422G I/O Expander Driver Header
├── esp_io_expander_ch422g.h   # CH422G Hardware Interface Header
├── partitions.csv             # Custom 8MB Flash Partition Table (3MB App + 4MB LittleFS)
├── control.py                 # Live PC Keyboard Controller over Serial (COM8 @ 115200)
├── STUDENT_GUIDE.md           # Beginner & Student Step-by-Step Visual Tutorial
├── README.md                  # Project Overview, Quickstart & Showcase Controls
└── TECHNICAL_REFERENCE.md     # Architecture, Bitwise Math & Physics Reference Manual
```

---

## 🚀 Running the Built-In 9-Scene Showcase (`RGBGameEngine.ino`)

1. Open [`RGBGameEngine.ino`](RGBGameEngine.ino) in the **Arduino IDE**.
2. Select **ESP32S3 Dev Module** with:
   - **Flash Size:** `8MB (64Mb)`
   - **PSRAM:** `OPI PSRAM`
   - **USB CDC On Boot:** `Enabled` (or use UART0 — the sketch listens on both `Serial` and `Serial0` simultaneously).
3. Click **Upload**.
4. You can control all **9 interactive demo scenes** live using either the **Arduino Serial Monitor** (`115200` baud) or the included instant keyboard controller script [`control.py`](control.py):

```bash
pip install pyserial
python control.py COM8
```

### 🎮 Live Interactive Controls

| Key | Action |
| :--- | :--- |
| **`1` – `9`** | Lock onto **Scene 1 – Scene 9** (`3` = 17 Blend Modes Lab, `4` = Capsule Neon Stick & Ray Shadows, `5` = Contiguous Object Shatter, `6` = Bouncy Cushion & Spring Ball, `7` = Silk Curtain & Ribbon Scarf, `9` = Shape Emitter & Realistic Fire Lab) |
| **`0` / `A`** | Toggle Auto-Cycle vs. Manual Scene Lock |
| **`N` / `P`** | Next Scene / Previous Scene |
| **`W` / `S` / `F` / `D`** | Move active target / light / collider (`Up` / `Down` / `Left` / `Right`) or switch to Manual Target Mode |
| **`+` / `-`** | Increase / Decrease **Global Intensity** (`0.10` to `3.00`) across all scenes |
| **`[` / `]`** | Decrease / Increase **Simulation & Animation Speed** (`0.10x` to `3.00x`) |
| **`M`** | Cycle sub-mode (Blend Mode in Scene 3, Shatter Material in Scene 5, Cushion Stiffness in Scene 6, Wind in Scene 7, Emitter Shape in Scene 9) |
| **`B`** | Cycle Particle Behavior Pattern in Scene 9 (including `REALISTIC_FIRE`) or Trigger Cushion/Spring Impulse in Scene 6 |
| **`C`** | Cycle Color Palette / Temperature across scenes |
| **`X` or `SPACE`** | Trigger Action (Shatter Object in Scene 5, Drop Heavy Weight on Cushion in Scene 6, Slice Cloth in Scene 7, Burst Particles in Scene 9) |
| **`R`** | Reset all physics, fracture, cushion, cloth, particle, and water simulations |
| **`H` or `?`** | Print live status and command summary over Serial |
