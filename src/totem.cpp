#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include "helper.hpp"

using namespace geode::prelude;

auto doTotemAnim = Mod::get()->getSettingValue<bool>("enable-totem");
auto totemCooldown = Mod::get()->getSettingValue<double>("totem-cooldown");
auto totemScale = Mod::get()->getSettingValue<double>("totem-scale");
auto totemSound = Mod::get()->getSettingValue<bool>("play-sfx");

$on_mod(Loaded){
    listenForSettingChanges("enable-totem", [](bool value) {
        doTotemAnim = value;
    });
    listenForSettingChanges("totem-cooldown", [](float value) {
        totemCooldown = value;
    });
    listenForSettingChanges("totem-scale", [](float value) {
        totemScale = value;
    });
    listenForSettingChanges("play-sfx", [](bool value) {
        totemSound = value;
    });
}

bool totemIsCooldowned = false;

void NCTotem::cooldownTotem(float dt) {
    totemIsCooldowned = false;
}

class $modify(TotemPlayLayer, PlayLayer) {
    struct Fields {
        int lastFrame = 67;
        int currentFrame = 0;
        float animTimer = 0.f;
        float frameDuration = 0.025f;
        CCSprite* totemAnimationSprite = CCSprite::createWithSpriteFrameName("mctotem_anim_0.png"_spr);
    };

    void startTotemAnim() {
        auto fields = m_fields.self();
        fields->currentFrame = 1;
        fields->animTimer = 0.f;

        std::string frameName = fmt::format("mctotem_anim_{}.png"_spr, fields->currentFrame);
        auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
        if (frame) {
            fields->totemAnimationSprite->setDisplayFrame(frame);
        }

        this->schedule(schedule_selector(TotemPlayLayer::playTotemAnim), fields->frameDuration);
    }

    void playTotemAnim(float dt) {
        auto fields = m_fields.self();

        fields->currentFrame++;
        if (fields->currentFrame > fields->lastFrame) {
            this->unschedule(schedule_selector(TotemPlayLayer::playTotemAnim));
            return;
        }

        std::string frameName = fmt::format("mctotem_anim_{}.png"_spr, fields->currentFrame);
        auto frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(frameName.c_str());
        if (frame) {
            fields->totemAnimationSprite->setDisplayFrame(frame);
        }
    }

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        auto fields = m_fields.self();
        auto winSize = CCDirector::sharedDirector()->getWinSize();

        if (doTotemAnim) {
            fields->totemAnimationSprite->setPosition({winSize.width / 2, winSize.height / 2});
            fields->totemAnimationSprite->setID("totem-animation"_spr);
            fields->totemAnimationSprite->setScale(totemScale);

            this->addChild(fields->totemAnimationSprite, 1000);
        }

        return true;
    }

    void startGame() {
        PlayLayer::startGame();

        totemIsCooldowned = false;
    }

    void resetLevel() {
        PlayLayer::resetLevel();

        totemIsCooldowned = false;
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        PlayLayer::destroyPlayer(player, obj);

        if (obj != m_anticheatSpike && !totemIsCooldowned) {
            if (m_player1->m_isDead) return;

            auto fmod = FMODAudioEngine::sharedEngine();

            if (doTotemAnim || totemSound) {
                totemIsCooldowned = true;
                this->scheduleOnce(schedule_selector(NCTotem::cooldownTotem), totemCooldown);
            }

            if (doTotemAnim) {
                TotemPlayLayer::startTotemAnim();
            }
            if (totemSound) {
                fmod->playEffect("totemUse.ogg"_spr);
            }

        }
    }
};