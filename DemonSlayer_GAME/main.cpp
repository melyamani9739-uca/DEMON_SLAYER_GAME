// ─────────────────────────────────────────────────────────────────────────────
//  main.cpp  –  Demon Slayer Survival Game
//
//  App state machine:
//    MENU  →  GAME  →  MENU (ESC or game-over+R handled inside Game)
//             MENU  →  ABOUT (Controls, handled by Menu)
//             GAME  →  PAUSE  →  GAME  (TAB or P)
//
//  Delta-time is clamped to 0.1s to prevent physics explosions after a
//  window-drag pause.
//
//  Window: 800×600, vsync on, frame limit 120.
// ─────────────────────────────────────────────────────────────────────────────
#include <SFML/Graphics.hpp>
#include "Menu.h"
#include "Game.h"
#include <iostream>

// ─── App states ──────────────────────────────────────────────────────────────
enum class AppState { MENU, GAME, PAUSE };

// ─── Pause overlay helper ────────────────────────────────────────────────────
static void drawPauseScreen(sf::RenderWindow& window, const sf::Font& font) {

    sf::RectangleShape overlay({800.f, 600.f});
    overlay.setFillColor(sf::Color(0, 0, 0, 160));
    window.draw(overlay);

    sf::Text title;
    title.setFont(font);
    title.setString("PAUSED");
    title.setCharacterSize(64);
    title.setFillColor(sf::Color::White);
    title.setOutlineColor(sf::Color::Black);
    title.setOutlineThickness(3.f);
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.width * 0.5f, tb.height * 0.5f);
    title.setPosition(400.f, 240.f);
    window.draw(title);

    auto makeHint = [&](const char* s, float y) {
        sf::Text t;
        t.setFont(font);
        t.setString(s);
        t.setCharacterSize(24);
        t.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect r = t.getLocalBounds();
        t.setOrigin(r.width * 0.5f, 0.f);
        t.setPosition(400.f, y);
        window.draw(t);
    };

    makeHint("P / TAB   Resume", 320.f);
    makeHint("ESC         Main Menu", 360.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────────────────────────────────────
int main() {

    // ── Window ───────────────────────────────────────────────
    sf::RenderWindow window(
        sf::VideoMode(800, 600),
        "Demon Slayer – Survival",
        sf::Style::Close | sf::Style::Titlebar
    );
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(120);

    // ── Game objects ─────────────────────────────────────────
    Menu     menu;
    Game     game;
    AppState state = AppState::MENU;

    // Font for pause screen (load once)
    sf::Font pauseFont;
    if (!pauseFont.loadFromFile("asseste/font.ttf"))
        pauseFont.loadFromFile("Montserrat/static/Montserrat-Regular.ttf");

    sf::Clock clock;

    // ── Fade transition ──────────────────────────────────────
    sf::RectangleShape fadeRect({800.f, 600.f});
    fadeRect.setFillColor(sf::Color::Black);
    float fadeAlpha  = 255.f;   // start black, fade in
    float fadeDir    = -1.f;    // -1 = fade in, +1 = fade out
    bool  fadePending   = false;
    AppState fadeTarget = AppState::GAME;

    // ─────────────────────────────────────────────────────────
    //  Main loop
    // ─────────────────────────────────────────────────────────
    while (window.isOpen()) {

        float dt = clock.restart().asSeconds();
        if (dt > 0.1f) dt = 0.1f;

        // ── Update fade ──────────────────────────────────────
        fadeAlpha += fadeDir * dt * 600.f;   // ~0.4s full fade
        fadeAlpha  = std::max(0.f, std::min(255.f, fadeAlpha));

        if (fadePending && fadeAlpha >= 255.f) {
            fadePending = false;
            state       = fadeTarget;
            fadeDir     = -1.f;   // start fade-in on new screen
            if (state == AppState::GAME) game.reset();
        }

        // ── Events ───────────────────────────────────────────
        sf::Event event;
        while (window.pollEvent(event)) {

            if (event.type == sf::Event::Closed)
                window.close();

            // ── MENU ─────────────────────────────────────────
            if (state == AppState::MENU) {

                MenuAction action = menu.handleEvent(event);

                switch (action) {
                    case MenuAction::PLAY:
                        // Start fade-out to game
                        fadeDir     = +1.f;
                        fadePending = true;
                        fadeTarget  = AppState::GAME;
                        break;

                    case MenuAction::QUIT:
                        window.close();
                        break;

                    default:
                        break;
                }
            }

            // ── GAME ─────────────────────────────────────────
            else if (state == AppState::GAME) {

                game.handleEvent(event);

                if (event.type == sf::Event::KeyPressed) {

                    // Pause
                    if (event.key.code == sf::Keyboard::P ||
                        event.key.code == sf::Keyboard::Tab)
                    {
                        state = AppState::PAUSE;
                    }

                    // Back to menu
                    if (event.key.code == sf::Keyboard::Escape) {
                        fadeDir     = +1.f;
                        fadePending = true;
                        fadeTarget  = AppState::MENU;
                    }
                }
            }

            // ── PAUSE ────────────────────────────────────────
            else if (state == AppState::PAUSE) {

                if (event.type == sf::Event::KeyPressed) {

                    if (event.key.code == sf::Keyboard::P  ||
                        event.key.code == sf::Keyboard::Tab ||
                        event.key.code == sf::Keyboard::Enter)
                    {
                        state = AppState::GAME;
                        clock.restart();   // reset dt to avoid jump
                    }

                    if (event.key.code == sf::Keyboard::Escape) {
                        fadeDir     = +1.f;
                        fadePending = true;
                        fadeTarget  = AppState::MENU;
                    }
                }
            }
        }

        // ── Update ───────────────────────────────────────────
        if (!fadePending) {
            if (state == AppState::MENU)
                menu.update(dt);
            else if (state == AppState::GAME)
                game.update(dt);
            // PAUSE: nothing updates
        }

        // ── Render ───────────────────────────────────────────
        window.clear(sf::Color::Black);

        if (state == AppState::MENU)
            menu.draw(window);
        else if (state == AppState::GAME)
            game.render(window);
        else if (state == AppState::PAUSE)
            game.render(window);   // draw frozen game behind

        // Pause overlay on top
        if (state == AppState::PAUSE)
            drawPauseScreen(window, pauseFont);

        // Fade overlay (always last)
        if (fadeAlpha > 0.f) {
            fadeRect.setFillColor(sf::Color(0, 0, 0, (sf::Uint8)fadeAlpha));
            window.draw(fadeRect);
        }

        window.display();
    }

    return 0;
}
