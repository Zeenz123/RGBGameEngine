# 🔬 RGBGameEngine — Architecture, Mathematics & Physics Reference Manual

This document contains the complete architectural, mathematical, bit-level, and algorithmic specification of **RGBGameEngine** for the ESP32-S3 (800×480 RGB565 display with Octal PSRAM). It details the internal implementation, performance characteristics, and exact API contracts of every engine subsystem.

---

## 1. Hardware & Memory Architecture

### 1.1 Display & DMA Double-Buffering Protocol
* **Resolution:** $800 \times 480 = 384,000$ pixels per frame.
* **Color Depth:** 16-bit `RGB565` ($2\text{ bytes/pixel} \implies 768,000\text{ bytes} = 750\text{ KB}$ per framebuffer).
* **Framebuffers in Octal PSRAM:**
  - `frontBuffer` (`lcd->getFrameBufferByIndex(0)`)
  - `backBuffer` (`lcd->getFrameBufferByIndex(1)`)
  - `frameBuffer` (Active CPU draw pointer; always points to the buffer **not** currently being scanned by the LCD peripheral).
* **VSYNC Synchronization (`RGBGameEngine::show()`):**
  1. Sets `refreshFinished = false`.
  2. Calls `lcd->drawBitmap(0, 0, 800, 480, (uint8_t*)frameBuffer)` to schedule a DMA buffer switch on the next vertical blanking interval (VSYNC).
  3. Yields via `vTaskDelay(1)` until the hardware ISR invokes `RGBGameEngine::refreshCallback()`, setting `refreshFinished = true`.
  4. Swaps `frameBuffer` between `frontBuffer` and `backBuffer`, guaranteeing tear-free rendering.

---

## 2. Bit-Level RGB565 Mathematics & 17 Blend Modes

### 2.1 RGB565 Bit Layout
A 16-bit `uint16_t` pixel encodes Red in bits `[15:11]`, Green in bits `[10:5]`, and Blue in bits `[4:0]`:
$$\text{RGB565} = \underbrace{R_4 R_3 R_2 R_1 R_0}_{\text{5 bits (0..31)}} \;\underbrace{G_5 G_4 G_3 G_2 G_1 G_0}_{\text{6 bits (0..63)}} \;\underbrace{B_4 B_3 B_2 B_1 B_0}_{\text{5 bits (0..31)}}$$

### 2.2 Overflow-Free Per-Channel Alpha Blending (`Color::blend565`)
Packed 32-bit unsigned subtraction (`(fg32 - bg32) * a`) suffers from cross-channel borrow corruption whenever a foreground channel is darker than its corresponding background channel ($fg_c < bg_c$). To guarantee artifact-free blending across all color combinations and shadow darkening operations, `Color::blend565(fg, bg, alpha)` performs signed per-channel interpolation in 5-bit/6-bit space:
```cpp
uint32_t a = ((uint32_t)alpha + 4) >> 3; // 0..32
int32_t fgR = (fg >> 11) & 0x1F, fgG = (fg >> 5) & 0x3F, fgB = fg & 0x1F;
int32_t bgR = (bg >> 11) & 0x1F, bgG = (bg >> 5) & 0x3F, bgB = bg & 0x1F;

uint32_t r = (uint32_t)(bgR + (((fgR - bgR) * (int32_t)a) >> 5));
uint32_t g = (uint32_t)(bgG + (((fgG - bgG) * (int32_t)a) >> 5));
uint32_t b = (uint32_t)(bgB + (((fgB - bgB) * (int32_t)a) >> 5));
return (uint16_t)((r << 11) | (g << 5) | b);
```
Inside `RGBGameEngine::drawFastHLineAlpha`, when `color == 0x0000` (shadow darkening), the loop simplifies to a single multiplicative attenuation per channel (`((((p >> 11) & 0x1F) * invA) >> 5)`), providing 32 smooth darkness levels with zero color distortion.

### 2.3 Complete 17-Mode Compositing Pipeline (`BlendMode`)
`Color::blendMode565(fg, bg, alpha, mode)` and `RGBGameEngine::drawFastHLineBlend` support 17 compositing modes:
1. `SOLID` — Direct opaque write.
2. `ALPHA` — Standard linear interpolation $\mathbf{C} = \mathbf{B} + \alpha(\mathbf{F} - \mathbf{B})$.
3. `ADDITIVE` — Saturating photopic addition $\mathbf{C} = \min(\mathbf{C}_{\max}, \mathbf{B} + \alpha \mathbf{F})$ for lasers, fire, and bloom.
4. `SUBTRACTIVE` — Saturating absorption $\mathbf{C} = \max(0, \mathbf{B} - \alpha \mathbf{F})$.
5. `MULTIPLY` — Stained-glass filter $\mathbf{M} = \frac{\mathbf{F} \cdot \mathbf{B}}{\mathbf{C}_{\max}}$ blended by $\alpha$.
6. `SCREEN` — Inverse multiply $\mathbf{S} = \mathbf{C}_{\max} - \frac{(\mathbf{C}_{\max}-\mathbf{F})(\mathbf{C}_{\max}-\mathbf{B})}{\mathbf{C}_{\max}}$.
7. `OVERLAY` — Contrast-preserving combination ($2\mathbf{B}\mathbf{F}$ if $\mathbf{B} < 0.5$, else Screen).
8. `SOFT_LIGHT` — Gentle diffused contrast lighting.
9. `HARD_LIGHT` — Intense spotlight contrast ($2\mathbf{F}\mathbf{B}$ if $\mathbf{F} < 0.5$, else Screen).
10. `COLOR_DODGE` — Brightens background by dividing by $(1 - \mathbf{F})$.
11. `COLOR_BURN` — Darkens background by dividing $(1 - \mathbf{B})$ by $\mathbf{F}$.
12. `DIFFERENCE` — Absolute channel difference $|\mathbf{F} - \mathbf{B}|$.
13. `EXCLUSION` — Low-contrast phase inversion $\mathbf{F} + \mathbf{B} - 2\mathbf{F}\mathbf{B}$.
14. `COLOR_TINT` — Preserves background luminance while tinting toward foreground chromaticity.
15. `LIGHTEN` — Per-channel maximum $\max(\mathbf{F}, \mathbf{B})$.
16. `DARKEN` — Per-channel minimum $\min(\mathbf{F}, \mathbf{B})$.
17. `CHECKER_STIPPLE` — Zero-cost $(x + y) \bmod 2$ screen-door transparency.

### 2.4 Color Physics Models
1. **Blackbody Planckian Locus (`Color::fromKelvin`):** Maps temperature $T \in [1000\text{ K}, 15000\text{ K}]$ using power-law/logarithmic curve fits of Planck's radiation law.
2. **Visible Wavelength (`Color::fromWavelength`):** Piecewise linear CIE approximation over $\lambda \in [380\text{ nm}, 750\text{ nm}]$ with photopic eye-sensitivity roll-off below $420\text{ nm}$ and above $700\text{ nm}$.
3. **4×4 Bayer Ordered Dithering (`Color::ditherRGBto565`):** Adds a spatially periodic $4 \times 4$ threshold matrix $B(x \bmod 4, y \bmod 4) \in [0..15]$ prior to RGB565 quantization to eliminate Mach banding in gradients.

---

## 3. Rasterization & 3D Projection Algorithms

| Algorithm | Method | Complexity & Implementation Notes |
| :--- | :--- | :--- |
| **32-Bit Aligned Scanline Fill** | `drawFastHLine`, `fillRect`, `clear` | Aligns pointer to 32-bit boundary and writes two 16-bit pixels per loop iteration (`uint32_t c32 = (c << 16) | c`). |
| **Midpoint Circle / Scanline Fill** | `drawCircle`, `fillCircle` | Integer decision parameter $d = 3 - 2r$; filled variant emits horizontal spans via `drawFastHLine`. |
| **Xiaolin Wu Anti-Aliased Line** | `drawAntiAliasedLine` | Fractional y-intercept coverage $f = y - \lfloor y \rfloor$, blending two vertically adjacent pixels with $\alpha_1 = 1-f$ and $\alpha_2 = f$. |
| **Concave/Convex Scanline Polygon** | `fillPolygon`, `fillPolygonAlpha` | Computes scanline edge intersections $x_i$, sorts via insertion sort, and fills spans $[x_{2k}, x_{2k+1}]$ (Even-Odd rule). |
| **Barycentric Gouraud Triangle** | `fillTriangleGouraud` | Evaluates normalized edge functions $(w_1, w_2, w_3)$ across AABB and dithers interpolated RGB888 to RGB565. |
| **Telephoto Cubic 3D Projection** | `Vec3::project`, `drawCube3D` | Uses `fov = 950.0f, viewerDist = 950.0f` to eliminate wide-angle trapezoid distortion, with back-to-front face depth sorting and directional face shading. |

---

## 4. Dynamic Lighting, Capsule SDF & Ray-Casted Shadows (`RGBLightAndShadow.h`)

### 4.1 Single-Pass Capsule SDF Glowing Stick (`drawLineLight`)
Rather than drawing discrete overlapping circles along a segment (which produces visible beading artifacts), `LightSystem2D::drawLineLight` evaluates the exact squared Euclidean distance from each pixel $\mathbf{P} = (x, y)$ in the bounding box to the line segment $\overline{\mathbf{A}\mathbf{B}}$:
$$t = \text{clamp}\left(\frac{(\mathbf{P} - \mathbf{A}) \cdot (\mathbf{B} - \mathbf{A})}{\|\mathbf{B} - \mathbf{A}\|^2},\; 0,\; 1\right), \quad d^2 = \|\mathbf{P} - (\mathbf{A} + t(\mathbf{B} - \mathbf{A}))\|^2$$
- **Inner Plasma Core ($d \le r_{\text{core}}$):** Blends pure white (`0xFFFF`) with the neon color for an incandescent core.
- **Outer Bloom Corona ($r_{\text{core}} < d \le R$):** Applies a smooth quadratic falloff $(1 - d/R)^2$ added directly to the 16-bit framebuffer in a single pass.

### 4.2 Intensity- & Distance-Coupled Shadow Volumes (`castCircleShadow`, `castBoxShadow`)
Shadow opacity dynamically couples with both `lightIntensity` and Euclidean distance $D = \|\mathbf{C} - \mathbf{L}\|$:
$$\alpha_{\text{eff}} = \text{clamp}\left(\alpha_{\text{base}} \cdot \text{clamp}(I_{\text{light}}, 0.1, 2.0) \cdot \left(1 - \frac{D}{1100}\right),\; 8,\; 245\right)$$
Umbra and penumbra tangent quads are projected analytically in $O(1)$ geometric setup and rasterized using exact black alpha attenuation.

### 4.3 2D Angular Ray-Casted Visibility (`drawRaytracedLight`)
Precomputes occluder bounding distances and casts $N$ radial rays $\theta_i = \frac{2\pi i}{N}$ from $(l_x, l_y)$ against `Occluder2D` primitives (`CIRCLE` quadratic intersection and `BOX` slab intersection), rendering a smoothradial-lit visibility fan.

---

## 5. Physics, Contiguous Fracture, Soft-Body, Cloth & Volumetric Fire (`RGBPhysics2D.h`)

### 5.1 Contiguous Planar Mesh Shattering (`ShatterSystem2D`)
Instead of spawning disconnected random polygons or ring waves, `ShatterSystem2D::triggerShatter` partitions a solid rectangular object $(W \times H)$ into an **$N_x \times N_y$ perturbed cellular grid** whose interior vertices are radially pulled toward the impact point $(x_{\text{imp}}, y_{\text{imp}})$.
- Every grid cell is split into two exact triangles sharing identical boundary vertices, guaranteeing a **100% contiguous, gap-free solid surface** at $t = 0$.
- Each shard receives a radial crack-propagation delay $\tau_k \propto \|\mathbf{c}_k - \mathbf{p}_{\text{imp}}\|$, 3D tumbling angular velocity (`tumbleSpeed` $\implies \cos(\phi)$ foreshortening + specular glint), gravity, and floor collision with friction so shards settle into a realistic debris pile on the ground.

### 5.2 33-Node Volume-Preserving Cushion & Elastic Ball Rebound (`SoftBodyCushion2D`)
Models the plush top surface as $N=33$ coupled spring-damper nodes with rest arch $\mathbf{r}_i$ and neighbor tension coupling:
$$\mathbf{a}_i = k_{\text{rest}}(\mathbf{r}_i - \mathbf{x}_i) + k_{\text{couple}}\left(\frac{\mathbf{x}_{i-1} + \mathbf{x}_{i+1}}{2} - \mathbf{x}_i\right)$$
- **Shoulder Bulge & Lateral Expansion:** Depressing central nodes raises adjacent shoulder nodes and expands the parabolic left/right bolsters (`sideBulge`) to conserve cross-sectional volume.
- **Two-Way Rigid-Body Interaction (`interactBall`):** Computes the interpolated cushion surface height under a colliding `RigidBody2D` ball, depresses the cushion nodes proportionally to impact momentum, and applies an upward Hookean + restitution impulse to launch the ball smoothly back into the air.
- **Single-Pass Scanline Fill:** Renders the entire deformed cushion interior, 3D bevel highlight, and tufted seam stitching using fast vertical/horizontal scanlines without polygon triangulation bottlenecks.

### 5.3 3D Surface-Normal Shaded Verlet Cloth & Ribbon Scarf (`VerletCloth2D`, `VerletRope2D`)
Uses fixed-timestep ($\Delta t = 0.016\text{ s}$) velocity-free Verlet integration with 4 Gauss-Seidel constraint relaxation passes per frame.
- **3D Fold Shading:** Computes horizontal and vertical surface tangents $(\Delta x_h, \Delta y_v)$ per quad to modulate RGB565 luminance across pleats and folds, complete with top curtain rod rings, golden trim, and bottom tassels.
- **Contiguous Ribbon Scarf:** `VerletRope2D` extrudes perpendicular normal widths $w_i$ at every spine node and fills contiguous dual-triangle ribbon segments with edge borders.

### 5.4 Universal Shape-Driven Particle Sampling & Volumetric Fire (`ParticleSystem2D`)
* **Uniform Shape Area vs. Perimeter Sampling (`sampleShapeSpawn`):**
  - Supports `POINT`, `LINE`, `CIRCLE`, `RING`, `RECTANGLE`, and `TRIANGLE` across `SURFACE_AREA`, `EDGE_PERIMETER`, `CENTER_POINT`, and `VERTICES_ONLY`.
* **11 Particle Behavior Patterns (`ParticleBehavior`):**
  - `PARALLEL`, `PARALLEL_WIND`, `SLIGHTLY_CHAOTIC`, `TOTALLY_CHAOTIC`, `OUTWARD_NORMAL`, `INWARD_IMPLODE`, `VORTEX_SWIRL`, `HARMONIC_WAVE`, `GRAVITY_BOUNCE`, `HOMING_ATTRACTOR`, and `REALISTIC_FIRE`.
* **Volumetric Blackbody Fire Plume (`REALISTIC_FIRE` & `FIRE_PLUME`):**
  - Combines upward thermal buoyancy ($-260\text{ px/s}^2$), Venturi aerodynamic necking toward the plume centerline, and coherent Rayleigh-Taylor sinusoidal flicker.
  - Renders 4-stage thermal cooling over normalized particle age $t \in [0, 1]$:
    1. **Core Combustion ($t < 0.22$):** Incandescent White-Blue (`0xFFFF`) $\to$ Lemon Yellow (`0xFFE6`) with additive bloom.
    2. **Main Flame Body ($0.22 \le t < 0.52$):** Lemon Yellow $\to$ Deep Orange (`0xFC60`).
    3. **Outer Flame Tips ($0.52 \le t < 0.78$):** Deep Orange $\to$ Crimson Red (`0xB800`).
    4. **Cooling Soot & Smoke ($t \ge 0.78$):** Crimson Red $\to$ Warm Charcoal Smoke (`0x2104`) with expanding alpha fade, accompanied by high-velocity crackling ember sparks (`isSpark`).
