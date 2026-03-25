# MQ2Viewport

Exposes EverQuest camera position, angles, and field of view to the MacroQuest TLO system. Targets the **RoF2 EMU** client.

## Installation

1. Download `MQ2Viewport.dll` from the [latest release](../../releases/latest)
2. Copy it into your MacroQuest `plugins/` folder
3. Either load it once with `/plugin MQ2Viewport` or add it to your `macroquest.ini` to auto-load

## Usage

```
/viewport
```
Prints all values to chat.

### TLO Members

| Member | Type | Description |
|--------|------|-------------|
| `${Viewport.X}` | float | Camera position X |
| `${Viewport.Y}` | float | Camera position Y |
| `${Viewport.Z}` | float | Camera position Z |
| `${Viewport.Heading}` | float | Degrees, 0=North, clockwise |
| `${Viewport.Pitch}` | float | Degrees, positive=up |
| `${Viewport.Roll}` | float | Degrees |
| `${Viewport.FOV}` | float | Vertical field of view in degrees |
| `${Viewport.ScreenW}` | float | Render buffer width in pixels |
| `${Viewport.ScreenH}` | float | Render buffer height in pixels |
| `${Viewport}` | string | `X=... Y=... Z=... H=... P=... FOV=...` |

> **Precision note:** All values are 32-bit floats (~7 significant digits). Position is formatted to 4 decimal places and angles to 2 — requesting more via string formatting will return floating-point noise, not real data.

### World-to-Screen Projection

Index format is `worldX,worldY,worldZ` (in-game coordinates).

| Member | Type | Description |
|--------|------|-------------|
| `${Viewport.Project[x,y,z]}` | string | `"sx,sy"` screen pixel coordinates, or FALSE if behind camera |
| `${Viewport.ProjectClamped[x,y,z]}` | string | `"sx,sy"` clamped to nearest screen edge — always returns a value even when off-screen or behind camera |
| `${Viewport.ProjectX[x,y,z]}` | float | Screen X pixel, or `-1` if behind camera |
| `${Viewport.ProjectY[x,y,z]}` | float | Screen Y pixel, or `-1` if behind camera |
| `${Viewport.ProjectVisible[x,y,z]}` | bool | TRUE only if in front of camera AND within screen bounds |

### Lua

```lua
local mq = require('mq')

-- Basic camera state
print(mq.TLO.Viewport.X(), mq.TLO.Viewport.Heading(), mq.TLO.Viewport.FOV())

-- Project a world coordinate to screen
local target = mq.TLO.Target
if target() then
    local wx, wy, wz = target.X(), target.Y(), target.Z()
    if mq.TLO.Viewport.ProjectVisible(wx..','..wy..','..wz)() then
        local sx = mq.TLO.Viewport.ProjectX(wx..','..wy..','..wz)()
        local sy = mq.TLO.Viewport.ProjectY(wx..','..wy..','..wz)()
        print(string.format("Target is at screen %.0f, %.0f", sx, sy))
    else
        -- Off-screen: get the nearest edge pixel instead
        local edge = mq.TLO.Viewport.ProjectClamped(wx..','..wy..','..wz)()
        print("Target off-screen, nearest edge: "..edge)
    end
end
```

## License

GPL v3 — see [LICENSE](LICENSE) or the [GNU website](https://www.gnu.org/licenses/).

For build instructions see [BUILDING.md](BUILDING.md).
