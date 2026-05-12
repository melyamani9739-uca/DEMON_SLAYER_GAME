
#include "Game.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>

static constexpr int   WINDOW_W    = 800;
static constexpr int   WINDOW_H    = 600;
static constexpr float GROUND_Y    = 500.f;
static constexpr float LEVEL1_TIME = 40.f;
static constexpr float BASE_SPAWN  = 2.5f;
static constexpr float MIN_SPAWN   = 0.9f;
// Target rendered height for characters
static constexpr float CHAR_HEIGHT    = 130.f;   // Akaza rendered height
static constexpr float TANJIRO_HEIGHT = 190.f;   // must match TARGET_HEIGHT in Player.cpp

static sf::FloatRect shrink(sf::FloatRect r, float sx, float sy) {
    r.left  += r.width  * sx;  r.top    += r.height * sy;
    r.width *= (1.f-2.f*sx);   r.height *= (1.f-2.f*sy);
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::loadSounds() {
    // Using ogg files for guaranteed SFML compatibility
    auto tryLoad = [](sf::SoundBuffer& buf, const char* path) -> bool {
        if (!buf.loadFromFile(path)) {
            std::cerr << "[Sound] Cannot load: " << path << "\n";
            return false;
        }
        return true;
    };
    tryLoad(sbWin,       "asseste/sounds/win_sound.ogg");
    tryLoad(sbMoney,     "asseste/sounds/money.mp3");    // money / energie pickup only
    tryLoad(sbDemon,     "asseste/sounds/demon_sound.ogg");    // killing demons
    tryLoad(sbKillAkaza, "asseste/sounds/kill_akaza_sound.ogg"); // hitting / killing Akaza

    sndWin.setBuffer(sbWin);
    sndMoney.setBuffer(sbMoney);
    sndDemon.setBuffer(sbDemon);
    sndKillAkaza.setBuffer(sbKillAkaza);

    sndWin.setVolume(100.f);
    sndMoney.setVolume(80.f);
    sndDemon.setVolume(85.f);
    sndKillAkaza.setVolume(100.f);
}

void Game::playSound(sf::Sound& snd) {
    snd.stop();
    snd.play();
}

// ─────────────────────────────────────────────────────────────────────────────
Game::Game() {
    auto load = [](sf::Texture& t, const char* p){
        if (!t.loadFromFile(p)) std::cerr<<"[Game] Cannot load: "<<p<<"\n";
        t.setSmooth(true);
    };

    // Level 1
    load(texDemon2,   "asseste/demon5.png");
    load(texDemon1,   "asseste/demon1.png");
    load(texObstacle, "asseste/obstacle.png");
    load(texEnergie,  "asseste/energie.png");
    demon5NumFrames=4; demon5Frame=0; demon5AnimT=0.f;

    // Level 2
    load(texRock,      "asseste/rock.png");
    load(texMoney,     "asseste/money.png");
    load(texDemon2Fly, "asseste/demon2.png");

    // Boss — cropped sprites (correct height, no blank rows)
    load(texAkazaWalk,   "asseste/Akaza_walk_cropped.png");    // 669x129
    load(texAkazaAttack, "asseste/Akaza_attack_cropped.png");  // 669x132

    // Tanjiro level2 / boss attack (cropped)
    load(texTanjiroAttackL2, "asseste/tanjiro_attack_level2_cropped.png"); // 669x144

    // Finisher dead (cropped)
    load(texFinisherDead, "asseste/tanjiro_dead_cropped.png");  // 669x130

    // Backgrounds
    load(texBgL1a,     "asseste/level1_BG1.jpg");
    load(texBgL1b,     "asseste/level1_BG2.jpg");
    load(texBgL2a,     "asseste/LEVEL2_bg1.jpg");
    load(texBgL2b,     "asseste/LEVEL2_b2.jpg");
    load(texBgFinale,  "asseste/finaleBg.jpg");
    load(texBgGameOver,"asseste/back1.jpeg");

  
      if (!font.loadFromFile("Montserrat/static/Montserrat-Regular.ttf")){
         std::cout << " Failed to load font\n";
      }
           
    // charScale for Tanjiro run (140px tall) → CHAR_HEIGHT
    charScale = CHAR_HEIGHT / 140.f;

    // Boss bar
    bossBarBg.setSize({300.f,22.f});
    bossBarBg.setFillColor(sf::Color(60,0,0,200));
    bossBarBg.setOutlineColor(sf::Color(200,50,50));
    bossBarBg.setOutlineThickness(2.f);
    bossBarBg.setPosition(250.f,14.f);
    bossBarFill.setSize({300.f,22.f});
    bossBarFill.setFillColor(sf::Color(220,30,30,220));
    bossBarFill.setPosition(250.f,14.f);

    flashOverlay.setSize({(float)WINDOW_W,(float)WINDOW_H});
    flashOverlay.setFillColor(sf::Color(255,50,50,0));
    flashTimer=0.f;
    killFlashOverlay.setSize({(float)WINDOW_W,(float)WINDOW_H});
    killFlashOverlay.setFillColor(sf::Color(50,255,100,0));
    killFlashTimer=0.f;

    bg1.setTexture(texBgL1a);
    bg2.setTexture(texBgL1b);

    akazaHP=AKAZA_MAX_HP;
    victoryTextAlpha=0.f; level1CompleteTimer=0.f;
    finisherActive=false; finisherIsKill=false;
    finFrame=0; finAnimTimer=0.f; finDoneTimer=0.f; finNumFrames=0;
    finisherZPressed=false;
    winSoundPlayed=false;

    loadSounds();
    initUI(); highScore=0; moneyCount=0;
    reset();
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::switchBackground(BackgroundType t) {
    sf::Texture *a,*b;
    switch(t){
        case BG_LEVEL1: a=&texBgL1a; b=&texBgL1b; break;
        case BG_LEVEL2: a=&texBgL2a; b=&texBgL2b; break;
        case BG_BOSS:   a=&texBgL2a; b=&texBgL2b; break;
        case BG_FINALE: a=&texBgFinale; b=&texBgFinale; break;
        default:        a=&texBgL1a; b=&texBgL1b; break;
    }
    auto set=[&](sf::Sprite& s,sf::Texture& tx){
        s.setTexture(tx);
        s.setScale((float)WINDOW_W/tx.getSize().x,(float)WINDOW_H/tx.getSize().y);
    };
    set(bg1,*a); set(bg2,*b);
    bg1.setPosition(0.f,0.f);
    bg2.setPosition((float)WINDOW_W,0.f);
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::initUI() {
    auto mk=[&](sf::Text& t,int sz,sf::Color c,float x,float y){
        t.setFont(font); t.setCharacterSize(sz); t.setFillColor(c);
        t.setPosition(x,y); t.setOutlineColor(sf::Color::Black);
        t.setOutlineThickness(1.5f);
    };
    mk(txtScore,26,sf::Color::White,       10.f,10.f);
    mk(txtTimer,26,sf::Color::White,       340.f,10.f);
    mk(txtLives,26,sf::Color(255,80,80),   630.f,10.f);
    mk(txtLevel,22,sf::Color(255,220,100), 10.f,45.f);
    mk(txtBig,  56,sf::Color::White,        0.f, 0.f);
    mk(txtMoney,22,sf::Color(255,215,0),  660.f,45.f);
    txtBig.setOutlineThickness(3.f);
    txtBig.setOutlineColor(sf::Color::Black);
    for(int i=0;i<3;i++){
        heartShapes[i].setSize({22.f,22.f});
        heartShapes[i].setFillColor(sf::Color(220,40,40));
        heartShapes[i].setOutlineColor(sf::Color::Black);
        heartShapes[i].setOutlineThickness(1.f);
        heartShapes[i].setPosition(635.f+i*28.f,38.f);
    }
    txtHint.setFont(font); txtHint.setCharacterSize(17);
    txtHint.setFillColor(sf::Color(200,200,200,200));
    txtHint.setOutlineColor(sf::Color::Black);
    txtHint.setOutlineThickness(1.f);
}

void Game::updateUI() {
    std::ostringstream ss;
    ss<<"Score: "<<score; txtScore.setString(ss.str());
    ss.str(""); ss<<"$"<<moneyCount; txtMoney.setString(ss.str());
    int lv=player.getLives();
    for(int i=0;i<3;i++)
        heartShapes[i].setFillColor(i<lv?sf::Color(220,40,40):sf::Color(60,60,60,120));

    switch(state){
        case GameState::LEVEL1:
            ss.str(""); ss<<"Time: "<<std::max(0,(int)levelTimer);
            txtTimer.setString(ss.str());
            txtLevel.setString("Level 1 - Survive!");
            txtHint.setString("UP: Jump   DOWN: Duck demon   Z/X: Attack");
            break;
        case GameState::LEVEL1_COMPLETE:
            txtTimer.setString(""); txtLevel.setString("Level 1 Complete!"); txtHint.setString("");
            break;
        case GameState::LEVEL2:
            txtTimer.setString("LEVEL 2");
            txtLevel.setString("Level 2 - Dodge & Reach Akaza!");
            txtHint.setString("UP: Jump rocks   DOWN: Duck demons   Z/X: Attack");
            break;
        case GameState::BOSS_APPROACH:
            txtTimer.setString(""); txtLevel.setString("AKAZA appears!"); txtHint.setString("Get ready!");
            break;
        case GameState::BOSS_FIGHT:
            txtTimer.setString(""); txtLevel.setString("BOSS - AKAZA!");
            if(akazaHP==1)
                txtHint.setString(">>> AKAZA is weak!  Press Z to finish him! <<<");
            else
                txtHint.setString("Z/X: Attack when close!");
            break;
        default:
            txtTimer.setString(""); txtLevel.setString(""); txtHint.setString(""); break;
    }
    sf::FloatRect hb=txtHint.getLocalBounds();
    txtHint.setOrigin(hb.width*0.5f,0.f);
    txtHint.setPosition(WINDOW_W*0.5f,WINDOW_H-28.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Akaza helpers — uses cropped sprites so frameH is the real character height
// ─────────────────────────────────────────────────────────────────────────────
void Game::setAkazaTexture(sf::Texture& tex, int nf) {
    akazaSprite.setTexture(tex, true);
    int fw = (int)tex.getSize().x / nf;  // ~111
    int fh = (int)tex.getSize().y;        // ~129-132 (cropped, real height)
    akazaSprite.setTextureRect(sf::IntRect(0,0,fw,fh));
    float sc = CHAR_HEIGHT / (float)fh;   // now scales to correct visible height
    akazaSprite.setScale(-sc, sc);         // -X = faces left toward Tanjiro
    akazaSprite.setOrigin(fw*0.5f,(float)fh);
}

void Game::setupAkaza() {
    akazaHP=AKAZA_MAX_HP;
    akazaState=AkazaState::WALK;
    akazaAnimFrame=0; akazaAnimTimer=0.f;
    akazaInvTimer=0.f; akazaDeathTimer=0.f;
    playerHitCooldown=0.f; akazaSpeed=-130.f;
    akazaDead=false;
    finisherActive=false; finisherIsKill=false;
    finFrame=0; finAnimTimer=0.f; finDoneTimer=0.f;
    finisherZPressed=false;
    setAkazaTexture(texAkazaWalk, 6);
    akazaSprite.setPosition(900.f, GROUND_Y);
}

void Game::updateAkaza(float dt) {
    if (akazaDead) { akazaDeathTimer+=dt; return; }

    // ── Finisher animation ───────────────────────────────────────────────────
    if (finisherActive) {
        finAnimTimer += dt;
        if (finAnimTimer >= 0.12f) {
            finAnimTimer = 0.f;
            if (finFrame < finNumFrames-1) {
                finFrame++;
                int fw=(int)finSprite.getTexture()->getSize().x/finNumFrames;
                int fh=(int)finSprite.getTexture()->getSize().y;
                finSprite.setTextureRect(sf::IntRect(finFrame*fw,0,fw,fh));
            }
        }
        if (finFrame >= finNumFrames-1) {
            finDoneTimer += dt;
            if (finDoneTimer >= 1.5f) {
                finisherActive = false;
                if (finisherIsKill) {
                    akazaDead=true; akazaDeathTimer=0.f;
                } else {
                    state=GameState::GAMEOVER;
                    if(score>highScore) highScore=score;
                }
            }
        }
        return;
    }

    // ── Normal animation ─────────────────────────────────────────────────────
    akazaAnimTimer += dt;
    int nf = 6;
    float aspd = 0.10f;

    if (akazaAnimTimer >= aspd) {
        akazaAnimTimer = 0.f;
        akazaAnimFrame++;
        if (akazaState==AkazaState::HIT) {
            if (akazaAnimFrame>=6) {
                akazaAnimFrame=0;
                akazaState=AkazaState::ATTACK;
                setAkazaTexture(texAkazaAttack,6);
            }
        } else {
            if (akazaAnimFrame>=nf) akazaAnimFrame=0;
        }
        int fw=(int)akazaSprite.getTexture()->getSize().x/nf;
        int fh=(int)akazaSprite.getTexture()->getSize().y;
        akazaSprite.setTextureRect(sf::IntRect(akazaAnimFrame*fw,0,fw,fh));
    }

    // ── Movement ─────────────────────────────────────────────────────────────
    float ax=akazaSprite.getPosition().x;
    float px=player.getPosition().x;

    if (state==GameState::BOSS_APPROACH) {
        if (akazaState!=AkazaState::WALK) {
            akazaState=AkazaState::WALK;
            setAkazaTexture(texAkazaWalk,6);
        }
        akazaSprite.move(akazaSpeed*dt, 0.f);
    } else if (state==GameState::BOSS_FIGHT && akazaState!=AkazaState::HIT) {
        float dist = std::abs(ax - px);
        if (dist > 150.f) {
            if (akazaState!=AkazaState::WALK) {
                akazaState=AkazaState::WALK;
                akazaAnimFrame=0; akazaAnimTimer=0.f;
                setAkazaTexture(texAkazaWalk,6);
            }
        } else {
            if (akazaState==AkazaState::WALK) {
                akazaState=AkazaState::ATTACK;
                akazaAnimFrame=0; akazaAnimTimer=0.f;
                setAkazaTexture(texAkazaAttack,6);
            }
        }
        if      (ax > px+130.f) akazaSpeed=-std::abs(akazaSpeed);
        else if (ax < px+60.f)  akazaSpeed= std::abs(akazaSpeed);
        if (ax>760.f)           akazaSpeed=-std::abs(akazaSpeed);
        if (ax<80.f)            akazaSpeed= std::abs(akazaSpeed);
        akazaSprite.move(akazaSpeed*dt, 0.f);
    }

    // Always face left (toward Tanjiro)
    float sc=std::abs(akazaSprite.getScale().x);
    akazaSprite.setScale(-sc, akazaSprite.getScale().y);

    float ratio=(float)akazaHP/(float)AKAZA_MAX_HP;
    bossBarFill.setSize({300.f*ratio,22.f});
    bossBarFill.setFillColor(sf::Color(220,(sf::Uint8)(30.f+ratio*120.f),30,220));
}

void Game::renderAkaza(sf::RenderWindow& w) {
    if (!akazaDead)
        w.draw(akazaSprite);
    if (finisherActive) w.draw(finSprite);
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::spawnObstacle() {
    if (state==GameState::BOSS_FIGHT || state==GameState::BOSS_APPROACH ||
        state==GameState::LEVEL1_COMPLETE ||
        state==GameState::VICTORY || state==GameState::GAMEOVER) return;

    int r=std::rand()%10;
    if (state==GameState::LEVEL1) {
        if      (r<4) obstacles.emplace_back(GROUND_Y, texDemon2,  Obstacle::GROUND,      4, 130.f);
        else if (r<7) obstacles.emplace_back(340.f,    texDemon1,  Obstacle::AIR,         4, 100.f);
        else          obstacles.emplace_back(GROUND_Y, texEnergie, Obstacle::COLLECTIBLE, 1,  70.f);
    } else if (state==GameState::LEVEL2) {
        if      (r<4) obstacles.emplace_back(GROUND_Y, texRock,      Obstacle::ROCK,        1, 85.f);
        else if (r<7) obstacles.emplace_back(340.f,    texDemon2Fly, Obstacle::AIR,         1, 100.f);
        else          obstacles.emplace_back(GROUND_Y, texMoney,     Obstacle::COLLECTIBLE, 1,  55.f);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::reset() {
    state=GameState::LEVEL1; levelTimer=LEVEL1_TIME;
    score=0; moneyCount=0; spawnInterval=BASE_SPAWN;
    bossApproachTimer=0.f; victoryTextAlpha=0.f; level1CompleteTimer=0.f;
    demon5Frame=0; demon5AnimT=0.f;
    akazaDead=false; akazaDeathTimer=0.f; akazaInvTimer=0.f; playerHitCooldown=0.f;
    finisherActive=false;
    winSoundPlayed=false;
    // Stop any playing sounds on reset
    sndWin.stop(); sndMoney.stop(); sndDemon.stop(); sndKillAkaza.stop();
    obstacles.clear(); spawnClock.restart();
    player.reset();
    switchBackground(BG_LEVEL1); scrollSpeed=200.f;
    flashTimer=0.f; killFlashTimer=0.f;
    flashOverlay.setFillColor(sf::Color(255,50,50,0));
    killFlashOverlay.setFillColor(sf::Color(50,255,100,0));
    updateUI();
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::handleEvent(const sf::Event& e) {
    if (e.type==sf::Event::KeyPressed) {
        if ((state==GameState::GAMEOVER||state==GameState::VICTORY)
            && e.key.code==sf::Keyboard::R) reset();

        if (state==GameState::LEVEL1_COMPLETE && level1CompleteTimer>=0.8f
            && e.key.code==sf::Keyboard::Return) {
            state=GameState::LEVEL2;
            obstacles.clear(); levelTimer=0.f;
            scrollSpeed=270.f; spawnInterval=BASE_SPAWN*0.65f;
            player.reset();
            player.setUseLevel2Attack(true);
            switchBackground(BG_LEVEL2);
            playSound(sndDemon);   // level 2 start sound
        }

        if (state==GameState::BOSS_FIGHT && akazaHP==1 && !finisherActive
            && (e.key.code==sf::Keyboard::Z || e.key.code==sf::Keyboard::X)) {
            float dist = std::abs(akazaSprite.getPosition().x - player.getPosition().x);
            if (dist < 200.f) finisherZPressed = true;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::update(float dt) {
    if (dt>0.1f) dt=0.1f;

    if (state==GameState::GAMEOVER||state==GameState::VICTORY||
        state==GameState::LEVEL1_COMPLETE) {
        if (state==GameState::VICTORY) {
            victoryTextAlpha=std::min(255.f,victoryTextAlpha+dt*120.f);
            if (!winSoundPlayed) {
                winSoundPlayed=true;
                playSound(sndWin);   // win sound plays exactly once on victory
            }
        }
        if (state==GameState::LEVEL1_COMPLETE)
            level1CompleteTimer+=dt;
        updateUI(); return;
    }

    player.update(dt);

    if (state!=GameState::BOSS_FIGHT) {
        bg1.move(-scrollSpeed*dt,0.f); bg2.move(-scrollSpeed*dt,0.f);
        if (bg1.getPosition().x<=-(float)WINDOW_W) bg1.setPosition(bg2.getPosition().x+(float)WINDOW_W,0.f);
        if (bg2.getPosition().x<=-(float)WINDOW_W) bg2.setPosition(bg1.getPosition().x+(float)WINDOW_W,0.f);
    }

    if (flashTimer>0.f) {
        flashTimer-=dt;
        flashOverlay.setFillColor(sf::Color(255,50,50,(sf::Uint8)(std::max(0.f,flashTimer/0.3f)*160.f)));
    }
    if (killFlashTimer>0.f) {
        killFlashTimer-=dt;
        killFlashOverlay.setFillColor(sf::Color(50,255,100,(sf::Uint8)(std::max(0.f,killFlashTimer/0.25f)*100.f)));
    }

    // ── LEVEL 1 ───────────────────────────────────────────────────────────────
    if (state==GameState::LEVEL1) {
        levelTimer-=dt;
        demon5AnimT+=dt;
        if(demon5AnimT>=0.15f){demon5AnimT=0.f;demon5Frame=(demon5Frame+1)%demon5NumFrames;}
        score+=(int)(dt*8.f);
        spawnInterval=std::max(MIN_SPAWN, BASE_SPAWN-(LEVEL1_TIME-levelTimer)*0.04f);
        if (levelTimer<=0.f) {
            state=GameState::LEVEL1_COMPLETE;
            obstacles.clear(); levelTimer=0.f; level1CompleteTimer=0.f;
            playSound(sndMoney);   // level transition fanfare
        }
    }
    // ── LEVEL 2 ───────────────────────────────────────────────────────────────
    else if (state==GameState::LEVEL2) {
        levelTimer+=dt; score+=(int)(dt*14.f);
        spawnInterval=std::max(MIN_SPAWN*0.7f, spawnInterval-dt*0.005f);
        if (levelTimer>=20.f) {
            state=GameState::BOSS_APPROACH;
            obstacles.clear();
            scrollSpeed=0.f; bossApproachTimer=0.f;
            player.forceWalk(false);
            player.setPosition(180.f, GROUND_Y);
            player.setScale(1.f, 1.f);
            player.setUseLevel2Attack(true);
            setupAkaza();
        }
    }
    // ── BOSS APPROACH ─────────────────────────────────────────────────────────
    else if (state==GameState::BOSS_APPROACH) {
        bossApproachTimer+=dt;
        updateAkaza(dt);
        float ax=akazaSprite.getPosition().x;
        if (ax<=630.f || bossApproachTimer>=4.f) {
            state=GameState::BOSS_FIGHT;
            scrollSpeed=0.f;
            player.forceWalk(false);
            player.setPosition(180.f,GROUND_Y);
        }
    }
    // ── BOSS FIGHT ────────────────────────────────────────────────────────────
    else if (state==GameState::BOSS_FIGHT) {
        score+=(int)(dt*5.f);
        if (akazaInvTimer>0.f)     akazaInvTimer-=dt;
        if (playerHitCooldown>0.f) playerHitCooldown-=dt;

        updateAkaza(dt);

        if (finisherActive) { updateUI(); return; }

        if (akazaDead) {
            akazaDeathTimer+=dt;
            if (akazaDeathTimer>=2.f) {
                state=GameState::VICTORY;
                if(score>highScore) highScore=score;
                switchBackground(BG_FINALE); victoryTextAlpha=0.f;
            }
            updateUI(); return;
        }

        sf::FloatRect ab = akazaSprite.getGlobalBounds();
        sf::FloatRect akazaBody(ab.left+ab.width*0.25f, ab.top+ab.height*0.05f,
                                ab.width*0.50f, ab.height*0.90f);
        sf::FloatRect pb = player.getBounds();
        sf::FloatRect playerBody(pb.left+pb.width*0.18f, pb.top+pb.height*0.08f,
                                 pb.width*0.64f, pb.height*0.84f);

        bool touching = playerBody.intersects(akazaBody);

        // Finisher: Z when HP=1
        if (finisherZPressed) {
            finisherZPressed = false;
            finisherActive  = true;
            finisherIsKill  = true;
            finFrame=0; finAnimTimer=0.f; finDoneTimer=0.f;
            finNumFrames=6;
            finSprite.setTexture(texTanjiroAttackL2, true);
            {
                int fw=(int)texTanjiroAttackL2.getSize().x/6;
                int fh=(int)texTanjiroAttackL2.getSize().y;
                finSprite.setTextureRect(sf::IntRect(0,0,fw,fh));
                float sc=TANJIRO_HEIGHT/(float)fh;
                finSprite.setOrigin(fw*0.5f,(float)fh);
                finSprite.setScale(sc,sc);
                float cx=(player.getPosition().x+akazaSprite.getPosition().x)*0.5f;
                finSprite.setPosition(cx,GROUND_Y);
            }
            akazaHP=0; killFlashTimer=0.3f;
            playSound(sndKillAkaza);  // kill_akaza sound on finisher
            updateUI(); return;
        }

        if (touching) {
            // Tanjiro hits Akaza
            if (player.isAttacking() && akazaInvTimer<=0.f && akazaHP>1) {
                akazaHP--;
                score+=50; killFlashTimer=0.4f;
                akazaInvTimer=1.5f;
                akazaState=AkazaState::HIT;
                akazaAnimFrame=0; akazaAnimTimer=0.f;
                setAkazaTexture(texAkazaAttack,6);
                playSound(sndKillAkaza);  // hitting Akaza sound
            }
            // Akaza hits Tanjiro
            else if (!player.isAttacking() && akazaState==AkazaState::ATTACK
                     && playerHitCooldown<=0.f) {
                if (player.getLives()==1) {
                    finisherActive=true; finisherIsKill=false;
                    finFrame=0; finAnimTimer=0.f; finDoneTimer=0.f; finNumFrames=6;
                    finSprite.setTexture(texFinisherDead, true);
                    {
                        int fw=(int)texFinisherDead.getSize().x/6;
                        int fh=(int)texFinisherDead.getSize().y;
                        finSprite.setTextureRect(sf::IntRect(0,0,fw,fh));
                        float sc=TANJIRO_HEIGHT/(float)fh;
                        finSprite.setOrigin(fw*0.5f,(float)fh);
                        finSprite.setScale(sc,sc);
                        finSprite.setPosition(player.getPosition().x,GROUND_Y);
                    }
                    flashTimer=0.4f;
                    playerHitCooldown=999.f;
                } else {
                    player.takeHit(); flashTimer=0.4f; playerHitCooldown=1.2f;
                    if(player.isDead()){state=GameState::GAMEOVER; if(score>highScore)highScore=score;}
                }
            }
        }
    }

    // ── Spawn obstacles ───────────────────────────────────────────────────────
    if (state==GameState::LEVEL1||state==GameState::LEVEL2) {
        if (spawnClock.getElapsedTime().asSeconds()>spawnInterval) {
            spawnObstacle(); spawnClock.restart();
        }
    }

    // ── Obstacle collisions ───────────────────────────────────────────────────
    bool onAnyPlatform=false;
    for (int i=(int)obstacles.size()-1;i>=0;i--) {
        obstacles[i].update(dt);
        if (obstacles[i].isCollected()){ obstacles.erase(obstacles.begin()+i); continue; }
        if (obstacles[i].isOffScreen()){ score+=3; obstacles.erase(obstacles.begin()+i); continue; }
        Obstacle::Type t=obstacles[i].getType();

        if (t==Obstacle::PLATFORM) {
            float sy=obstacles[i].getPlatformTopY();
            float px2=obstacles[i].getPos().x, pw=obstacles[i].getBounds().width;
            float pfy=player.getPosition().y, pcx=player.getBounds().left+player.getBounds().width*0.5f;
            bool over=pcx>px2-pw*0.5f&&pcx<px2+pw*0.5f;
            bool land=player.getVelY()>=0.f&&pfy>=sy-8.f&&pfy<=sy+20.f;
            if(over&&land){ player.landOnPlatform(sy); onAnyPlatform=true;
                player.setPosition(player.getPosition().x+obstacles[i].getSpeedX()*dt,sy);
                if(obstacles[i].hasEnergiePickup()){obstacles[i].collectEnergie();score+=50;killFlashTimer=0.3f;
                    playSound(sndMoney); }}
            else if(over&&player.isOnGround()&&pfy>=sy-4.f&&pfy<=sy+4.f){
                onAnyPlatform=true; player.landOnPlatform(sy);
                player.setPosition(player.getPosition().x+obstacles[i].getSpeedX()*dt,sy);
                if(obstacles[i].hasEnergiePickup()){obstacles[i].collectEnergie();score+=50;killFlashTimer=0.3f;
                    playSound(sndMoney); }}
            continue;
        }

        sf::FloatRect pb2=shrink(player.getBounds(),0.18f,0.12f);
        sf::FloatRect ob=shrink(obstacles[i].getBounds(),0.10f,0.05f);
        if(!pb2.intersects(ob)) continue;

        if(t==Obstacle::COLLECTIBLE){
            obstacles[i].collect(); moneyCount++; score+=30; killFlashTimer=0.15f;
            playSound(sndMoney);   // money/energy pickup — separate from jump
            continue;
        }
        if(t==Obstacle::ROCK){
            if(player.getState()==Player::JUMP){
                obstacles.erase(obstacles.begin()+i); score+=8;
                playSound(sndDemon);
            } else {
                player.takeHit(); flashTimer=0.3f; obstacles.erase(obstacles.begin()+i);
                if(player.isDead()){state=GameState::GAMEOVER;if(score>highScore)highScore=score;}
            }
            continue;
        }
        if(t==Obstacle::GROUND){
            float dtopY=obstacles[i].getBounds().top+obstacles[i].getBounds().height*0.15f;
            bool stomp=(player.getState()==Player::JUMP||player.getVelY()>0.f)
                       &&player.getPosition().y<=dtopY+18.f&&player.getVelY()>=0.f;
            if(stomp){ score+=25; killFlashTimer=0.25f; player.stompBounce();
                obstacles.erase(obstacles.begin()+i); playSound(sndDemon); }
            else if(player.isAttacking()){ score+=20; killFlashTimer=0.25f;
                obstacles.erase(obstacles.begin()+i); playSound(sndDemon); }
            else{ player.takeHit(); flashTimer=0.3f; obstacles.erase(obstacles.begin()+i);
                if(player.isDead()){state=GameState::GAMEOVER;if(score>highScore)highScore=score;} }
            continue;
        }
        sf::FloatRect pb_air=shrink(player.getBounds(),0.18f,player.getIsDucking()?0.45f:0.12f);
        sf::FloatRect ob_air=shrink(obstacles[i].getBounds(),0.10f,0.10f);
        if(t==Obstacle::AIR){
            if(!pb_air.intersects(ob_air)) continue;
            if(player.getIsDucking()) continue;
            else if(player.isAttacking()){ score+=15; killFlashTimer=0.2f;
                obstacles.erase(obstacles.begin()+i); playSound(sndDemon); }
            else{ player.takeHit(); flashTimer=0.3f; obstacles.erase(obstacles.begin()+i);
                if(player.isDead()){state=GameState::GAMEOVER;if(score>highScore)highScore=score;} }
            continue;
        }
    }
    if(!onAnyPlatform&&player.isOnGround()&&player.getGroundY()!=500.f) player.setGroundY(500.f);
    if(!onAnyPlatform&&player.isOnGround()&&player.getPosition().y<499.f) player.setOnGround(false);
    updateUI();
}

// ─────────────────────────────────────────────────────────────────────────────
void Game::render(sf::RenderWindow& window) {
    window.draw(bg1);
    window.draw(bg2);

    sf::RectangleShape gnd({(float)WINDOW_W,4.f});
    gnd.setFillColor(sf::Color(40,22,8,120));
    gnd.setPosition(0.f,GROUND_Y);
    window.draw(gnd);

    for (auto& o:obstacles) o.render(window);
    if (state==GameState::BOSS_APPROACH||state==GameState::BOSS_FIGHT)
        renderAkaza(window);
    if (!finisherActive)
        player.render(window);

    window.draw(txtScore); window.draw(txtTimer);
    window.draw(txtLevel); window.draw(txtHint); window.draw(txtMoney);

    sf::Text hp; hp.setFont(font); hp.setString("HP:"); hp.setCharacterSize(20);
    hp.setFillColor(sf::Color(255,130,130)); hp.setPosition(595.f,40.f);
    window.draw(hp);
    for (auto& h:heartShapes) window.draw(h);

    if (state==GameState::BOSS_FIGHT||state==GameState::BOSS_APPROACH) {
        window.draw(bossBarBg); window.draw(bossBarFill);
        sf::Text bl; bl.setFont(font);
        bl.setString("AKAZA  "+std::to_string(akazaHP)+" / "+std::to_string(AKAZA_MAX_HP));
        bl.setCharacterSize(16); bl.setFillColor(sf::Color(255,160,160));
        bl.setOutlineColor(sf::Color::Black); bl.setOutlineThickness(1.f);
        bl.setPosition(260.f,15.f); window.draw(bl);
        for(int i=0;i<AKAZA_MAX_HP;i++){
            sf::RectangleShape hb2({22.f,22.f});
            hb2.setOutlineColor(sf::Color(200,50,50)); hb2.setOutlineThickness(2.f);
            hb2.setFillColor(i<akazaHP?sf::Color(220,30,30,230):sf::Color(40,10,10,180));
            hb2.setPosition(540.f+i*28.f,14.f); window.draw(hb2);
        }
    }

    if(flashTimer>0.f)     window.draw(flashOverlay);
    if(killFlashTimer>0.f) window.draw(killFlashOverlay);

    // ── LEVEL 1 COMPLETE ─────────────────────────────────────────────────────
    if (state==GameState::LEVEL1_COMPLETE) {
        float ft=std::min(1.f,level1CompleteTimer*2.f);
        sf::RectangleShape ov({(float)WINDOW_W,(float)WINDOW_H});
        ov.setFillColor(sf::Color(0,0,0,(sf::Uint8)(ft*190.f))); window.draw(ov);
        sf::Uint8 a=(sf::Uint8)(ft*255.f);
        txtBig.setCharacterSize(42); txtBig.setString("Great Job!");
        txtBig.setFillColor(sf::Color(255,220,80,a));
        txtBig.setOutlineColor(sf::Color(0,0,0,a)); txtBig.setOutlineThickness(3.f);
        txtBig.setOrigin(txtBig.getGlobalBounds().width*0.5f,txtBig.getGlobalBounds().height*0.5f);
        txtBig.setPosition(WINDOW_W*0.5f,WINDOW_H*0.5f-110.f); window.draw(txtBig);
        auto mkt=[&](const std::string& s,int sz,sf::Color c,float y){
            sf::Text t; t.setFont(font); t.setString(s); t.setCharacterSize(sz);
            t.setFillColor(sf::Color(c.r,c.g,c.b,a));
            t.setOutlineColor(sf::Color(0,0,0,a)); t.setOutlineThickness(2.f);
            t.setOrigin(t.getLocalBounds().width*0.5f,0.f);
            t.setPosition(WINDOW_W*0.5f,y); window.draw(t);
        };
        mkt("You have successfully completed Level 1",28,sf::Color(255,255,200),WINDOW_H*0.5f-55.f);
        mkt("Score: "+std::to_string(score)+"   Energie: "+std::to_string(moneyCount),22,sf::Color(200,230,255),WINDOW_H*0.5f-5.f);
        mkt("Tanjiro keeps running... Akaza awaits!",20,sf::Color(180,220,255),WINDOW_H*0.5f+40.f);
        if(level1CompleteTimer>=0.8f && (int)(level1CompleteTimer*2.f)%2==0)
            mkt(">> Press ENTER to continue to Level 2 <<",26,sf::Color(255,255,100),WINDOW_H*0.5f+115.f);
        return;
    }

    // ── GAME OVER ─────────────────────────────────────────────────────────────
    if (state==GameState::GAMEOVER) {
        sf::Sprite goBg;
        goBg.setTexture(texBgGameOver);
        goBg.setScale((float)WINDOW_W/texBgGameOver.getSize().x,
                      (float)WINDOW_H/texBgGameOver.getSize().y);
        window.draw(goBg);
        txtBig.setCharacterSize(64); txtBig.setString("");
        txtBig.setFillColor(sf::Color(255,50,50)); txtBig.setOutlineColor(sf::Color::Black);
        txtBig.setOutlineThickness(4.f);
        txtBig.setOrigin(txtBig.getGlobalBounds().width*0.5f,txtBig.getGlobalBounds().height*0.5f);
        txtBig.setPosition(WINDOW_W*0.5f,WINDOW_H*0.5f-60.f); window.draw(txtBig);
        sf::Text s; s.setFont(font); s.setCharacterSize(28);
        s.setFillColor(sf::Color::White); s.setOutlineColor(sf::Color::Black); s.setOutlineThickness(2.f);
        s.setString("Score: "+std::to_string(score)+"   Best: "+std::to_string(highScore));
        s.setOrigin(s.getGlobalBounds().width*0.5f,0.f);
        s.setPosition(WINDOW_W*0.5f,WINDOW_H*0.5f+130.f); window.draw(s);
        sf::Text h2; h2.setFont(font); h2.setCharacterSize(22);
        h2.setFillColor(sf::Color(220,220,220)); h2.setOutlineColor(sf::Color::Black); h2.setOutlineThickness(1.5f);
        h2.setString("R: Retry   ESC: Menu");
        h2.setOrigin(h2.getGlobalBounds().width*0.5f,0.f);
        h2.setPosition(WINDOW_W*0.5f,WINDOW_H*0.5f+165.f); window.draw(h2);
    }

    // ── VICTORY ───────────────────────────────────────────────────────────────
    if (state==GameState::VICTORY) {
        sf::Uint8 a=(sf::Uint8)std::min(255.f,victoryTextAlpha);
        sf::Sprite fb; fb.setTexture(texBgFinale);
        fb.setScale((float)WINDOW_W/texBgFinale.getSize().x,(float)WINDOW_H/texBgFinale.getSize().y);
        fb.setColor(sf::Color(255,255,255,a)); window.draw(fb);
        sf::RectangleShape top({(float)WINDOW_W,160.f});
        top.setFillColor(sf::Color(0,0,0,(sf::Uint8)(a*0.7f))); top.setPosition(0.f,0.f); window.draw(top);
        sf::RectangleShape bot({(float)WINDOW_W,130.f});
        bot.setFillColor(sf::Color(0,0,0,(sf::Uint8)(a*0.65f))); bot.setPosition(0.f,(float)WINDOW_H-130.f); window.draw(bot);
        txtBig.setCharacterSize(48); txtBig.setString("AKAZA DEFEATED!");
        txtBig.setFillColor(sf::Color(255,80,60,a));
        txtBig.setOutlineColor(sf::Color(0,0,0,a)); txtBig.setOutlineThickness(3.f);
        txtBig.setOrigin(txtBig.getGlobalBounds().width*0.5f,txtBig.getGlobalBounds().height*0.5f);
        txtBig.setPosition(WINDOW_W*0.5f,55.f); window.draw(txtBig);
        auto mk2=[&](const std::string& s,int sz,sf::Color c,float y){
            sf::Text t; t.setFont(font); t.setString(s); t.setCharacterSize(sz);
            t.setFillColor(sf::Color(c.r,c.g,c.b,a));
            t.setOutlineColor(sf::Color(0,0,0,a)); t.setOutlineThickness(2.f);
            t.setOrigin(t.getLocalBounds().width*0.5f,0.f);
            t.setPosition(WINDOW_W*0.5f,y); window.draw(t);
        };
        mk2("Tanjiro finally meets Nezuko...",26,sf::Color(255,220,180),105.f);
        mk2("Score: "+std::to_string(score)+"   $"+std::to_string(moneyCount),20,sf::Color(200,200,200),(float)WINDOW_H-42.f);
        sf::Text h3; h3.setFont(font); h3.setCharacterSize(18);
        h3.setFillColor(sf::Color(180,180,180,a)); h3.setOutlineColor(sf::Color(0,0,0,a)); h3.setOutlineThickness(1.f);
        h3.setString("R: Play Again   ESC: Menu");
        h3.setOrigin(h3.getLocalBounds().width*0.5f,0.f);
        h3.setPosition(WINDOW_W*0.5f,(float)WINDOW_H-20.f); window.draw(h3);
    }
}
