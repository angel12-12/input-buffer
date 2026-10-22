# Input Buffer

Click a little early and still hit the jump.

Geometry Dash already treats a **held** jump button as "jump as soon as you can", but it forgets a click you have already released. Input Buffer keeps your click alive for a few milliseconds after you let go, so an early tap still fires when you touch the ground or an orb.

- Works with touch controls, so it runs on **Android** and iOS as well as desktop.
- Adjustable buffer window (10 to 250 ms, default 80 ms).
- Never extends your hold in **ship** or **wave**, where hold time changes the movement.
- A click made on the ground is never buffered, so normal jumps behave exactly like vanilla.

## Settings

- **Enabled**: turn the mod on or off.
- **Buffer window (ms)**: how long an early click stays alive after you release it.
- **Debug log**: writes a line to the Geode log each time a click is buffered.
