# Soldering, from zero

You only have to make **three solder joints** for this whole project: the servo's three wires onto the board's **5V**, **GND**, and **D1** pads. If you've never held a soldering iron, this is a friendly first project — three joints, lots of room, nothing fragile to cook. This guide teaches the actual skill along the way.

## The one idea to understand first

Solder is a metal glue that melts at a low temperature (~180–230 °C). You don't "paint" it on. Instead:

> **You heat the two metal parts you want to join, and then touch the solder to the *parts* — not to the iron. The hot metal melts the solder and it flows into the joint.**

That's the whole skill. Beginners fail by melting a blob on the iron tip and dabbing it onto cold metal — that makes a weak "cold joint" that looks like a dull grey blob and falls off later. A good joint is **shiny and smooth**, hugging the wire and pad like a tiny volcano.

## What you need

**Essential:**
- **Soldering iron**, ideally temperature-controlled — set it to **~330 °C / 620 °F**. A cheap fixed-temp iron works too.
- **Rosin-core solder.** 60/40 leaded is the easiest to learn on; lead-free works but flows a little stickier. 0.6–0.8 mm diameter.
- **A stand** for the hot iron, and a **damp sponge or brass wool** to wipe the tip.
- **Wire strippers** and **flush cutters** (or small scissors).
- Something to **hold the board** — a "helping hands" tool, a vise, or even tape. You want both your hands free.

**Nice to have:**
- A **multimeter** (for the continuity test at the end — cheap and worth it).
- **Flux** (a pen or paste) — makes solder flow dramatically better and covers for sloppy technique.
- **Heat-shrink tubing** or electrical tape for strain relief.
- **Safety glasses** — solder can occasionally spit.

## Set up

1. Tape or clamp the board down with the pads you're soldering facing up and accessible.
2. Plug in the iron and let it reach temperature (1–2 min).
3. **Tin the tip:** wipe the hot tip on the damp sponge, then melt a little solder onto it so it's shiny silver. This helps heat flow. Re-tin whenever the tip looks dull.
4. Work in a ventilated spot. The wisp of smoke is flux burning off — don't breathe it; point a small fan to push it aside.

## Practice (5 minutes, optional but recommended)

Before touching the board, melt solder onto a scrap wire a few times. Feel the rhythm: **touch iron to metal → count "one-one-thousand, two-one-thousand" → feed solder into the joint → it flows → pull solder away → pull iron away.** About 2–3 seconds of heat per joint. That's it.

## Which approach: wires direct, or headers?

- **Plan A — solder wires straight to the pads (recommended here).** Compact, permanent, three joints, done. Best for a thing that lives on a wall.
- **Plan B — solder the included male header pins, then plug the servo in with female "Dupont" jumper wires.** Removable and beginner-forgiving, but bulkier. If you choose this, you solder the header *pins* into the board's holes instead of wires — same technique, described at the bottom.

The rest of this assumes **Plan A**.

## Find the three pads

**Trust the silkscreen** — the labels are printed right on the board. With the USB-C port pointing up:

- **5V** and **GND** are the **top two pads on the right edge**.
- **D1** is the **second pad down on the left edge**.

(Power on one side, signal on the other — convenient, because it keeps the joints far apart so you can't accidentally bridge them.)

## Step by step

**1. Prep the servo leads.** The MG90S has three wires: **brown = GND**, **red = V+**, **orange = signal (PWM)**. Either cut off the plastic plug and use the bare leads, or cut three short extension wires. Strip ~3–4 mm of insulation off each end.

**2. Tin the wire ends.** Heat a stripped wire end with the iron and feed a little solder into the strands until they're coated and silver. Now they're stiff and ready. Do all three.

**3. Tin the pads.** Touch the iron to a pad for ~2 seconds, feed a tiny bit of solder so the pad wears a small shiny bump. Do the 5V, GND, and D1 pads. (Flux helps here if you have it.)

**4. Join wire to pad.** Hold the tinned wire end against its pad, then press the iron against **both** for ~2–3 seconds. The two pre-tinned bits melt together and fuse. Remove the iron, and **hold the wire still for a couple seconds while it solidifies** — wiggling now causes a cold joint. Match them up:

| Servo wire   | Pad   |
|--------------|-------|
| Brown (GND)  | GND   |
| Red (V+)     | 5V    |
| Orange (PWM) | D1    |

**5. (Optional) Add the capacitor.** If you're using the 470–1000 µF cap, solder its **negative leg (the side with the stripe)** to GND and its **positive leg** to 5V. Getting the polarity backwards on an electrolytic cap is the one thing that can make it pop, so double-check the stripe.

**6. Strain relief.** The solder joint is strong but brittle — a tug on the wire can crack it or rip the pad off the board. Slip heat-shrink over the joints, or put a dab of hot glue / a wrap of tape so any pulling is taken up by the insulation, not the joint.

## Inspect and test — before you plug anything in

1. **Look:** each joint should be **shiny, smooth, and concave** (volcano-shaped), wetting both the wire and the pad. Dull, cracked, or ball-shaped = reheat it (add a touch of fresh solder/flux and melt it again until it flows).
2. **Bridges:** make sure no stray solder connects two neighboring pads. The dangerous one is anything connecting **5V to GND** — that's a short circuit. The orange/D1 wire is off on the other edge by itself, which is exactly why.
3. **Multimeter (if you have one):** set it to continuity (beeps when connected).
   - Touch each servo wire and its pad → should beep (good joint).
   - Touch **5V and GND** → should **not** beep. If it beeps, you have a short — find and remove the bridge before powering on.
4. **Smoke test:** plug in USB-C. Nothing should get hot or smell. The board's LED should light. Then flash the sketch and try "Hey Siri."

## If something's off

- **Solder won't stick, just balls up and rolls off** → the metal isn't hot enough, or it's dirty/oxidized. Heat the *part* longer, re-tin your iron tip, add flux.
- **Joint looks dull, grainy, or cracked** → cold joint (moved while cooling, or too little heat). Reheat until it flows shiny, hold still while it sets.
- **Pad lifted off the board** → too much heat for too long. The pad is delicate; aim for 2–3 seconds. You can usually solder to the wire stub or the next exposed bit of trace, but go gentler next time.
- **Iron tip turns black and won't melt solder** → it oxidized. Wipe on the sponge and re-tin while hot.

## Plan B notes (headers + jumpers)

If you went with header pins: push the strip of male pins into the row of holes (long side down into the board), rest the board on the pins so they stay put, then solder **each pin to its hole** from the top — iron on the pin *and* the ring of the hole, feed solder until it fills the ring with a shiny cone. Then connect the servo with female Dupont jumpers to the 5V, GND, and D1 pins. Same brown→GND, red→5V, orange→D1.

## Safety

- The iron tip is **~330 °C**. It does not look hot. Assume every part of the metal end is. Return it to the stand every single time you set it down.
- Solder fumes (flux smoke) aren't great to breathe — ventilate.
- Leaded solder: don't eat/touch your face while working, and **wash your hands** after.
- Safety glasses — flux pockets can spit a tiny bit of molten solder.
- Unplug the iron when you're done. They don't all have auto-off.

That's it — three good joints and you've got a robot finger. Back to the [README](README.md) for flashing and calibration.
