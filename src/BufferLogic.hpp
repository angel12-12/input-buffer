#pragma once

// Pure input-buffer logic. It has no Geode / Geometry Dash dependencies on purpose,
// so it can be unit-tested on any machine (see tests/test_buffer_logic.cpp).
//
// How the buffer works
// --------------------
// Geometry Dash already treats a *held* jump button as "jump as soon as you can":
// hold while landing and the cube jumps, hold while touching an orb and it fires.
// What it does not do is remember a click that was already released.
//
// So instead of inventing a second input system, the buffer simply keeps the game's
// jump button "held" for a few extra milliseconds after you let go. A click made
// slightly too early is then still alive when you touch the ground or the orb.
//
// Time is measured in *level time* (the game's own clock), never in wall-clock time,
// so it behaves the same on a 60 Hz phone, a 120 Hz phone and at any TPS.

namespace inputbuffer {

class Slot {
public:
    // The real jump button went down.
    // `consumedAtPress` is true when the press was immediately useful (the player was on
    // the ground and jumped), so there is nothing to buffer for this click.
    // Returns true if a previously deferred release must be delivered to the game
    // BEFORE this press, so the game sees a clean release -> press sequence.
    bool onPress(bool consumedAtPress) {
        bool flush = m_pending;
        m_pending = false;
        m_consumed = consumedAtPress;
        return flush;
    }

    // The real jump button went up.
    // Returns true if this release must be swallowed (the buffer keeps the button "held").
    bool onRelease(double now, double windowSeconds, bool holdIsHarmless) {
        if (m_pending) {
            // Release without a matching press: deliver it so we never leave the game stuck.
            m_pending = false;
            return false;
        }
        if (windowSeconds <= 0.0 || !holdIsHarmless || m_consumed) {
            return false;
        }
        m_pending = true;
        m_deferredAt = now;
        m_releaseAt = now + windowSeconds;
        return true;
    }

    // Call once per game tick. Returns true when the deferred release must be delivered now.
    bool onTick(double now) {
        if (!m_pending) {
            return false;
        }
        // `now < m_deferredAt` means the level clock went backwards (restart / checkpoint).
        if (now >= m_releaseAt || now < m_deferredAt - kTimeEpsilon) {
            m_pending = false;
            return true;
        }
        return false;
    }

    bool pending() const { return m_pending; }

private:
    static constexpr double kTimeEpsilon = 1e-6;

    bool m_pending = false;
    // Starts as "consumed" so a release with no recorded press is never deferred.
    bool m_consumed = true;
    double m_deferredAt = 0.0;
    double m_releaseAt = 0.0;
};

} // namespace inputbuffer
