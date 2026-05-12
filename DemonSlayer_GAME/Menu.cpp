
#include "Menu.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <sstream>

static constexpr float W = 800.f;
static constexpr float H = 600.f;

static const sf::Color COL_GOLD       {255, 215,  60, 255};
static const sf::Color COL_BTN_FILL   {220, 185,  50, 240};
static const sf::Color COL_BTN_BORDER {255, 230,  90, 255};
static const sf::Color COL_BTN_TXT    { 15,   8,   0, 255};
static const sf::Color COL_BTN_IDLE   {255, 215,  60, 255};
static const sf::Color COL_WHITE      {255, 255, 255, 255};
static const sf::Color COL_GREY       {170, 170, 170, 255};

static float randf(float lo, float hi) {
    return lo + (hi - lo) * (std::rand() / (float)RAND_MAX);
}

sf::Color Menu::lerpColor(sf::Color a, sf::Color b, float t) const {
    t = std::max(0.f, std::min(1.f, t));
    return sf::Color(
        (sf::Uint8)(a.r+(b.r-a.r)*t),(sf::Uint8)(a.g+(b.g-a.g)*t),
        (sf::Uint8)(a.b+(b.b-a.b)*t),(sf::Uint8)(a.a+(b.a-a.a)*t));
}

Menu::Menu()
    : selectedIdx(0), hoverAnim(0.f), hoverTarget(0.f),
      screen(MenuScreen::MAIN),
      sfxEnabled(true), musicEnabled(true), volume(80.f),
      time(0.f), titlePulse(0.f)
{
    if (!font.loadFromFile("asseste/font.ttf"))
        if (!font.loadFromFile("Montserrat/static/Montserrat-Regular.ttf"))
            std::cerr << "[Menu] Font load failed\n";

    // New menu background (WhatsApp image)
    if (!bgTex.loadFromFile("asseste/back2.jpeg"))
        bgTex.loadFromFile("asseste/back2.jpeg");   // fallback
    bgTex.setSmooth(true);
    bgSprite.setTexture(bgTex);
    bgSprite.setScale(W/(float)bgTex.getSize().x, H/(float)bgTex.getSize().y);
    bgSprite.setPosition(0.f, 0.f);

    // Dark overlay — lighter so the new bg shows through nicely
    overlay.setSize({W, H});
    overlay.setFillColor(sf::Color(0, 0, 0, 80));

    // Logo top-right
    if (!logoTex.loadFromFile("asseste/logo1.png"))
        std::cerr << "[Menu] Logo load failed\n";
    logoTex.setSmooth(true);
    logoSprite.setTexture(logoTex);
    float logoW = 200.f;
    float logoScale = logoW / (float)logoTex.getSize().x;
    logoSprite.setScale(logoScale, logoScale);
    logoSprite.setPosition(W - logoW - 15.f, 8.f);

    // Ember particles
    particles.reserve(60);
    for (int i = 0; i < 45; i++) spawnParticle();

    // Controls text
    controlsText.setFont(font);
    controlsText.setCharacterSize(19);
    controlsText.setFillColor(COL_WHITE);
    controlsText.setLineSpacing(1.5f);
    controlsText.setString(
        "CONTROLS\n\n"
        "  UP Arrow    Jump\n"
        "  DOWN Arrow  Duck (dodge demon)\n"
        "  Z  /  X     Attack\n"
        "  ESC         Menu\n"
        "  R           Retry\n\n"
        "TIPS\n\n"
        "  Level 1: Stomp or attack demons\n"
        "  Level 1: Duck to dodge crow\n"
        "  Level 2: Jump rocks, duck demons\n"
        "  Boss: Attack Akaza to damage!\n"
        "  Boss: Z finisher when Akaza is weak!"
    );
    controlsText.setPosition(W*0.5f+10.f, 100.f);

    backHint.setFont(font);
    backHint.setCharacterSize(17);
    backHint.setFillColor(COL_GREY);
    backHint.setString("Press ESC or ENTER to go back");
    sf::FloatRect bh = backHint.getLocalBounds();
    backHint.setOrigin(bh.width*0.5f, 0.f);
    backHint.setPosition(W*0.5f, H-46.f);

    buildMainItems();
}

void Menu::spawnParticle() {
    Particle p;
    float r = randf(1.5f, 4.5f);
    p.shape.setRadius(r); p.shape.setOrigin(r,r);
    p.shape.setPosition(randf(0.f,W), H+r*2.f);
    p.vel = {randf(-12.f,12.f), randf(-55.f,-95.f)};
    p.life = randf(0.f, 1.f);
    sf::Uint8 rv=(sf::Uint8)randf(210,255), gv=(sf::Uint8)randf(60,140);
    p.shape.setFillColor(sf::Color(rv,gv,10,180));
    particles.push_back(p);
}

void Menu::updateParticles(float dt) {
    for (auto& p : particles) {
        p.life += dt*0.22f;
        if (p.life>1.f) {
            p.life=0.f;
            float r=randf(1.5f,4.5f); p.shape.setRadius(r); p.shape.setOrigin(r,r);
            p.shape.setPosition(randf(0.f,W), H+6.f);
            p.vel={randf(-12.f,12.f), randf(-55.f,-95.f)};
            sf::Uint8 rv=(sf::Uint8)randf(210,255), gv=(sf::Uint8)randf(60,140);
            p.shape.setFillColor(sf::Color(rv,gv,10,180));
            continue;
        }
        p.shape.move(p.vel.x*dt, p.vel.y*dt);
        float alpha=(p.life<0.18f)?p.life/0.18f:(p.life>0.75f)?(1.f-p.life)/0.25f:1.f;
        sf::Color c=p.shape.getFillColor(); c.a=(sf::Uint8)(alpha*190.f);
        p.shape.setFillColor(c);
    }
    while ((int)particles.size()<50) spawnParticle();
}

void Menu::buildMainItems() {
    items.clear(); screen=MenuScreen::MAIN;
    const char* labels[]={"  Play","  Settings","  Controls","  Quit"};
    float btnW=210.f, btnH=44.f;
    float leftX  = 28.f;
    float rightX = W - 28.f - btnW;
    float startY = H*0.52f, stepY = 60.f;
    for (int i=0;i<4;i++) {
        float bx = (i % 2 == 0) ? leftX : rightX;
        float by = startY + (i / 2) * stepY;
        MenuItem item;
        item.label.setFont(font);
        item.label.setString(labels[i]);
        item.label.setCharacterSize(22);
        item.label.setOutlineColor(sf::Color::Black);
        item.label.setOutlineThickness(1.5f);
        item.label.setPosition(bx+10.f, by+10.f);
        item.enabled=true;
        items.push_back(item);
    }
    selectedIdx=0; hoverAnim=0.f; hoverTarget=0.f;
}

void Menu::buildSettingsItems() {
    items.clear(); screen=MenuScreen::SETTINGS;
    const char* labels[]={"Music","Sound FX","Volume","Back"};
    float btnW=210.f, btnH=44.f;
    float btnX = W*0.5f - btnW*0.5f;
    float startY = H*0.52f, stepY = 60.f;
    for (int i=0;i<4;i++) {
        MenuItem item;
        item.label.setFont(font);
        item.label.setString(labels[i]);
        item.label.setCharacterSize(26);
        item.label.setOutlineColor(sf::Color::Black);
        item.label.setOutlineThickness(1.5f);
        item.label.setPosition(btnX+12.f, startY+i*stepY+12.f);
        item.enabled=true;
        items.push_back(item);
    }
    selectedIdx=0; hoverAnim=0.f; hoverTarget=0.f;
}

MenuAction Menu::handleEvent(const sf::Event& e) {
    if (screen==MenuScreen::CONTROLS) {
        if (e.type==sf::Event::KeyPressed || e.type==sf::Event::MouseButtonPressed)
            buildMainItems();
        return MenuAction::NONE;
    }

    float btnW=210.f, btnH=44.f;
    float leftX  = 28.f;
    float rightX = W - 28.f - btnW;
    float startY = H*0.52f, stepY = 60.f;

    // For main menu: btn 0,2 go left column; 1,3 go right column
    // For settings: all centered
    auto getBtnX = [&](int idx) -> float {
        if (screen == MenuScreen::MAIN) return (idx % 2 == 0) ? leftX : rightX;
        return W*0.5f - btnW*0.5f;
    };
    auto getBtnY = [&](int idx) -> float {
        if (screen == MenuScreen::MAIN) return startY + (idx / 2) * stepY;
        return startY + idx * stepY;
    };

    if (e.type==sf::Event::KeyPressed) {
        if (e.key.code==sf::Keyboard::Up) {
            do { selectedIdx--; if(selectedIdx<0) selectedIdx=(int)items.size()-1; }
            while (!items[selectedIdx].enabled);
            hoverTarget=(float)selectedIdx;
        }
        if (e.key.code==sf::Keyboard::Down) {
            do { selectedIdx++; if(selectedIdx>=(int)items.size()) selectedIdx=0; }
            while (!items[selectedIdx].enabled);
            hoverTarget=(float)selectedIdx;
        }
        if (e.key.code==sf::Keyboard::Escape)
            if (screen==MenuScreen::SETTINGS) { buildMainItems(); return MenuAction::BACK; }
        if (e.key.code==sf::Keyboard::Return) return activateSelected();
        if (screen==MenuScreen::SETTINGS && selectedIdx==2) {
            if (e.key.code==sf::Keyboard::Left)  volume=std::max(0.f,volume-10.f);
            if (e.key.code==sf::Keyboard::Right) volume=std::min(100.f,volume+10.f);
        }
    }

    // Mouse hover
    if (e.type==sf::Event::MouseMoved) {
        float mx=(float)e.mouseMove.x, my=(float)e.mouseMove.y;
        for (int i=0;i<(int)items.size();i++) {
            if (items[i].enabled &&
                sf::FloatRect(getBtnX(i),getBtnY(i),btnW,btnH).contains(mx,my))
            { selectedIdx=i; hoverTarget=(float)i; }
        }
    }

    // Mouse click — fully clickable buttons
    if (e.type==sf::Event::MouseButtonPressed && e.mouseButton.button==sf::Mouse::Left) {
        float mx=(float)e.mouseButton.x, my=(float)e.mouseButton.y;
        for (int i=0;i<(int)items.size();i++) {
            if (items[i].enabled &&
                sf::FloatRect(getBtnX(i),getBtnY(i),btnW,btnH).contains(mx,my))
            { selectedIdx=i; hoverTarget=(float)i; return activateSelected(); }
        }
    }
    return MenuAction::NONE;
}

MenuAction Menu::activateSelected() {
    if (screen==MenuScreen::MAIN) {
        switch(selectedIdx) {
            case 0: return MenuAction::PLAY;
            case 1: buildSettingsItems(); return MenuAction::NONE;
            case 2: screen=MenuScreen::CONTROLS; return MenuAction::NONE;
            case 3: return MenuAction::QUIT;
        }
    }
    if (screen==MenuScreen::SETTINGS) {
        switch(selectedIdx) {
            case 0: musicEnabled=!musicEnabled; return MenuAction::NONE;
            case 1: sfxEnabled=!sfxEnabled; return MenuAction::NONE;
            case 2: return MenuAction::NONE;
            case 3: buildMainItems(); return MenuAction::BACK;
        }
    }
    return MenuAction::NONE;
}

void Menu::update(float dt) {
    time+=dt; titlePulse+=dt; updateParticles(dt);
    hoverAnim += (hoverTarget-hoverAnim)*std::min(1.f,dt*14.f);
    if (screen==MenuScreen::SETTINGS && items.size()>=4) {
        items[0].label.setString(std::string("  Music      ")+(musicEnabled?"ON":"OFF"));
        items[1].label.setString(std::string("  Sound FX  ")+(sfxEnabled?"ON":"OFF"));
        std::ostringstream ss; ss<<"  Volume    "<<(int)volume<<"%";
        items[2].label.setString(ss.str());
    }
}

void Menu::draw(sf::RenderWindow& window) {
    // 1. Full-screen new background
    window.draw(bgSprite);
    window.draw(overlay);

    // 2. Ember particles
    for (auto& p : particles) window.draw(p.shape);

    // 3. Logo top-right
    window.draw(logoSprite);

    // 4. Controls screen
    if (screen==MenuScreen::CONTROLS) {
        sf::RectangleShape cp({W-40.f, H-80.f});
        cp.setFillColor(sf::Color(5,2,15,215));
        cp.setOutlineColor(sf::Color(180,140,40,180));
        cp.setOutlineThickness(2.f);
        cp.setPosition(20.f, 40.f);
        window.draw(cp);
        window.draw(controlsText);
        window.draw(backHint);
        return;
    }

    // 5. Title text with pulse effect
    {
        float pulse = 0.94f + 0.06f * std::sin(titlePulse * 2.5f);
        sf::Text title;
        title.setFont(font);
        title.setString("");
        title.setCharacterSize((int)(44 * pulse));
        title.setFillColor(sf::Color(255, 80, 40, 240));
        title.setOutlineColor(sf::Color(0,0,0,200));
        title.setOutlineThickness(3.5f);
        sf::FloatRect tb = title.getLocalBounds();
        title.setOrigin(tb.width*0.5f, tb.height*0.5f);
        title.setPosition(W*0.5f, H*0.22f);
        window.draw(title);

        sf::Text sub;
        sub.setFont(font);
        sub.setString("");
        sub.setCharacterSize(22);
        sub.setFillColor(sf::Color(255,215,60,200));
        sub.setOutlineColor(sf::Color(0,0,0,180));
        sub.setOutlineThickness(2.f);
        sf::FloatRect sb2 = sub.getLocalBounds();
        sub.setOrigin(sb2.width*0.5f, 0.f);
        sub.setPosition(W*0.5f, H*0.22f + 34.f);
        window.draw(sub);
    }

    // 6. Buttons — two columns (left/right), smaller
    float btnW=210.f, btnH=44.f;
    float leftX  = 28.f;
    float rightX = W - 28.f - btnW;
    float startY = H*0.52f, stepY = 60.f;

    auto getBtnX2 = [&](int idx) -> float {
        if (screen == MenuScreen::MAIN) return (idx % 2 == 0) ? leftX : rightX;
        return W*0.5f - btnW*0.5f;
    };
    auto getBtnY2 = [&](int idx) -> float {
        if (screen == MenuScreen::MAIN) return startY + (idx / 2) * stepY;
        return startY + idx * stepY;
    };

    for (int i=0;i<(int)items.size();i++) {
        bool sel=(i==selectedIdx);
        float bx = getBtnX2(i);
        float y  = getBtnY2(i);

        // Button shadow
        sf::RectangleShape shadow({btnW+4.f, btnH+4.f});
        shadow.setFillColor(sf::Color(0,0,0,100));
        shadow.setPosition(bx+4.f, y+4.f);
        window.draw(shadow);

        // Button body
        sf::RectangleShape btn({btnW, btnH});
        if (sel) {
            btn.setFillColor(COL_BTN_FILL);
            btn.setOutlineColor(COL_BTN_BORDER);
            btn.setOutlineThickness(3.f);
            items[i].label.setFillColor(COL_BTN_TXT);
        } else {
            btn.setFillColor(sf::Color(12,6,28,195));
            btn.setOutlineColor(sf::Color(180,140,40,180));
            btn.setOutlineThickness(1.5f);
            items[i].label.setFillColor(COL_BTN_IDLE);
        }
        btn.setPosition(bx, y);
        window.draw(btn);

        // Hover glow bar on left side
        if (sel) {
            sf::RectangleShape glow({6.f, btnH});
            glow.setFillColor(sf::Color(255,100,30,230));
            glow.setPosition(bx, y);
            window.draw(glow);
        }

        // Reposition label to match new button position
        items[i].label.setPosition(bx+10.f, y+10.f);
        window.draw(items[i].label);
    }

    // 7. Volume bar (settings only)
    if (screen==MenuScreen::SETTINGS) {
        float sbx = W*0.5f - btnW*0.5f;
        float bx=sbx+20.f, by=startY+2*stepY+56.f;
        sf::RectangleShape bg({btnW-40.f,10.f});
        bg.setFillColor(sf::Color(30,15,5,200));
        bg.setOutlineColor(COL_GREY); bg.setOutlineThickness(1.f);
        bg.setPosition(bx,by); window.draw(bg);
        sf::RectangleShape fill({(btnW-40.f)*(volume/100.f),10.f});
        fill.setFillColor(sf::Color(200,30,20,255));
        fill.setPosition(bx,by); window.draw(fill);
    }

    // 8. Bottom hint bar
    sf::RectangleShape hb({W, 30.f});
    hb.setFillColor(sf::Color(0,0,0,170));
    hb.setPosition(0.f,H-30.f); window.draw(hb);
    sf::Text hint; hint.setFont(font); hint.setCharacterSize(15);
    hint.setFillColor(COL_GREY);
    hint.setString("UP/DOWN: Navigate     ENTER: Select     Mouse: Click buttons");
    sf::FloatRect hbr=hint.getLocalBounds();
    hint.setOrigin(hbr.width*0.5f,0.f);
    hint.setPosition(W*0.5f,H-24.f);
    window.draw(hint);
}
