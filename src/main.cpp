#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>

#include "BufferLogic.hpp"

using namespace geode::prelude;

// Input Buffer
// ------------
// Keeps the game's jump button "held" for a few milliseconds after you release it, so a
// click made slightly too early still fires when you touch the ground or an orb.
// See BufferLogic.hpp for the details of the idea.
//
// Nothing here depends on a keyboard or mouse: everything goes through
// GJBaseGameLayer::handleButton, the same path touches use on Android and iOS.

namespace {
    constexpr int kJumpButton = static_cast<int>(PlayerButton::Jump);

    // Ship and wave physics depend on exactly how long the button is held, so the buffer
    // never extends a hold in those modes. Every other mode only reacts to the click itself.
    bool holdIsHarmless(PlayerObject* player) {
        return player && !player->m_isShip && !player->m_isDart;
    }

    struct ControlledPlayers {
        PlayerObject* main = nullptr;
        // Set when one input drives both players (classic dual mode).
        PlayerObject* linked = nullptr;
    };

    ControlledPlayers controlledPlayers(GJBaseGameLayer* layer, bool isPlayer1) {
        ControlledPlayers result;
        result.main = isPlayer1 ? layer->m_player1 : layer->m_player2;

        bool sharedInput = layer->m_gameState.m_isDualMode
            && layer->m_levelSettings
            && !layer->m_levelSettings->m_twoPlayerMode;
        if (sharedInput) {
            result.linked = isPlayer1 ? layer->m_player2 : layer->m_player1;
        }
        return result;
    }
}

class $modify(InputBufferLayer, GJBaseGameLayer) {
    struct Fields {
        // 0 = player 1 input, 1 = player 2 input
        inputbuffer::Slot slots[2];
    };

    void handleButton(bool down, int button, bool isPlayer1) {
        // Only the jump button is buffered (platformer left / right pass straight through).
        if (button != kJumpButton) {
            GJBaseGameLayer::handleButton(down, button, isPlayer1);
            return;
        }

        auto& slot = m_fields->slots[isPlayer1 ? 0 : 1];
        auto players = controlledPlayers(this, isPlayer1);

        if (down) {
            // If the player is already on the ground this click is used up right away.
            bool consumed = (players.main && players.main->m_isOnGround)
                || (players.linked && players.linked->m_isOnGround);

            // A click that arrives while an earlier release is still deferred: let the game
            // see the release first, otherwise it would treat the new click as a continued hold.
            if (slot.onPress(consumed)) {
                GJBaseGameLayer::handleButton(false, button, isPlayer1);
            }
            GJBaseGameLayer::handleButton(true, button, isPlayer1);
            return;
        }

        auto mod = Mod::get();
        bool enabled = mod->getSettingValue<bool>("enabled");
        double window = enabled
            ? static_cast<double>(mod->getSettingValue<int64_t>("buffer-window-ms")) / 1000.0
            : 0.0;

        bool harmless = holdIsHarmless(players.main)
            && (!players.linked || holdIsHarmless(players.linked));

        if (slot.onRelease(m_gameState.m_levelTime, window, harmless)) {
            if (mod->getSettingValue<bool>("debug-log")) {
                log::debug("[InputBuffer] release deferred by {:.0f} ms (player {})",
                    window * 1000.0, isPlayer1 ? 1 : 2);
            }
            return; // keep the game's button held for now
        }

        GJBaseGameLayer::handleButton(false, button, isPlayer1);
    }

    void processCommands(float dt, bool isHalfTick, bool isLastTick) {
        // Deliver any deferred release whose time is up, before the game runs this tick.
        double now = m_gameState.m_levelTime;
        for (int i = 0; i < 2; ++i) {
            if (m_fields->slots[i].onTick(now)) {
                GJBaseGameLayer::handleButton(false, kJumpButton, i == 0);
            }
        }
        GJBaseGameLayer::processCommands(dt, isHalfTick, isLastTick);
    }
};
