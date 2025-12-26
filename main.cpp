#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/TextArea.hpp> 

#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>

#include <thread>
#include <vector>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

struct GDHSV { float h, s, v; };

// Pre-calculated data to pass between threads
struct BlockData {
    float x, y;
    ccColor3B color;
    GDHSV hsv;
};

GDHSV rgbToGdhsv(ccColor3B color) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float max = std::max({r, g, b}), min = std::min({r, g, b});
    float h, s, v = max;
    float d = max - min;
    s = max == 0 ? 0 : d / max;

    if (max == min) h = 0;
    else {
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6;
    }
    return { h * 360.0f, s, v };
}

class LogPopup : public Popup<std::string> {
protected:
    bool setup(std::string text) override {
        this->setTitle("Import Report");
        auto winSize = CCDirector::get()->getWinSize();
        auto contentSize = m_mainLayer->getContentSize();

        auto textArea = SimpleTextArea::create(text.c_str(), "chatFont.fnt", 0.6f);
        textArea->setWidth(contentSize.width - 40);
        textArea->setPosition(contentSize.width / 2, contentSize.height / 2 + 10);
        m_mainLayer->addChild(textArea);

        auto menu = CCMenu::create();
        menu->setPosition(contentSize.width / 2, 30);
        menu->addChild(CCMenuItemSpriteExtra::create(
            ButtonSprite::create("OK", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(LogPopup::onClose)
        ));
        m_mainLayer->addChild(menu);
        return true;
    }
public:
    static LogPopup* create(std::string text) {
        auto ret = new LogPopup();
        if (ret && ret->initAnchored(360.f, 280.f, text)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class ImportSettingsPopup : public Popup<> {
protected:
    TextInput* m_stepInput;
    TextInput* m_scaleInput;
    std::string m_path;

    bool setup() override {
        this->setTitle("Import Pixel Art");
        auto cx = m_mainLayer->getContentSize().width / 2;

        m_stepInput = TextInput::create(100.0f, "Step", "chatFont.fnt");
        m_stepInput->setPosition({cx, m_mainLayer->getContentSize().height - 70});
        m_stepInput->setString("2");
        m_stepInput->setCommonFilter(CommonFilter::Uint);
        m_mainLayer->addChild(m_stepInput);

        m_scaleInput = TextInput::create(100.0f, "Scale", "chatFont.fnt");
        m_scaleInput->setPosition({cx, m_mainLayer->getContentSize().height - 130});
        m_scaleInput->setString("0.1");
        m_scaleInput->setCommonFilter(CommonFilter::Float);
        m_mainLayer->addChild(m_scaleInput);

        auto menu = CCMenu::create();
        menu->setPosition({cx, 45});
        menu->addChild(CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Import", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImportSettingsPopup::onImport)
        ));
        m_mainLayer->addChild(menu);
        return true;
    }

    void onImport(CCObject*) {
        // Using safe utils to prevent exceptions
        int step = utils::numFromString<int>(m_stepInput->getString()).unwrapOr(2);
        float scale = utils::numFromString<float>(m_scaleInput->getString()).unwrapOr(0.1f);
        
        if (step < 1) step = 1;

        // Run processing in background to avoid freezing the game
        this->processImageAsync(step, scale);
        this->onClose(nullptr);
    }

    void processImageAsync(int step, float scale) {
        std::string pathCopy = m_path;
        
        std::thread([pathCopy, step, scale]() {
            int w, h, c;
            unsigned char* data = stbi_load(pathCopy.c_str(), &w, &h, &c, 4);
            
            if (!data) {
                Loader::get()->queueInMainThread([]{
                    LogPopup::create("Failed to load image data.")->show();
                });
                return;
            }

            std::vector<BlockData> blocks;
            // Reserve memory to prevent reallocation overhead
            blocks.reserve((w / step) * (h / step));

            // Standard block size is 30 units
            float pixelSize = 30.0f * scale;
            // Center the image relative to 0,0 (will offset by camera later)
            float halfW = ((w / step) * pixelSize) / 2.0f;
            float halfH = ((h / step) * pixelSize) / 2.0f;

            for (int y = 0; y < h; y += step) {
                for (int x = 0; x < w; x += step) {
                    int idx = (y * w + x) * 4;
                    if (data[idx + 3] < 128) continue; // Alpha check

                    float posX = -halfW + ((x / step) * pixelSize);
                    float posY = halfH - ((y / step) * pixelSize);

                    ccColor3B col = {data[idx], data[idx+1], data[idx+2]};
                    blocks.push_back({posX, posY, col, rgbToGdhsv(col)});
                }
            }
            stbi_image_free(data);

            // Dispatch back to main thread for object creation
            Loader::get()->queueInMainThread([blocks, scale]() {
                auto editor = LevelEditorLayer::get();
                if (!editor) return;

                auto centerPos = editor->m_objectLayer->convertToNodeSpace(CCDirector::get()->getWinSize() / 2);
                int count = 0;

                for (const auto& b : blocks) {
                    // ID 211 (Solid Square)
                    auto obj = editor->createObject(211, centerPos + ccp(b.x, b.y), false);
                    if (obj) {
                        // 1.10f scale overlap to fix gaps
                        obj->setScale(scale * 1.10f);
                        
                        // HSV Override for persistence
                        if (obj->m_baseColor) 
                            obj->m_baseColor->m_hsv = { b.hsv.h, b.hsv.s, b.hsv.v, true, true };
                        if (obj->m_detailColor) 
                            obj->m_detailColor->m_hsv = { b.hsv.h, b.hsv.s, b.hsv.v, true, true };
                        
                        obj->setChildColor(b.color);
                        editor->addToSection(obj);
                        count++;
                    }
                }

                LogPopup::create(fmt::format("Imported {} blocks successfully.", count))->show();
            });

        }).detach();
    }

public:
    static ImportSettingsPopup* create(std::string path) {
        auto ret = new ImportSettingsPopup();
        if (ret && ret->initAnchored(260.f, 240.f)) {
            ret->autorelease();
            ret->m_path = path;
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class $modify(MyEditorUI, EditorUI) {
    struct Fields { 
        EventListener<Task<Result<std::filesystem::path>>> m_pickListener; 
    };

    bool init(LevelEditorLayer* el) {
        if (!EditorUI::init(el)) return false;
        
        auto menu = CCMenu::create();
        auto btn = CCMenuItemSpriteExtra::create(
            CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png"), 
            this, 
            menu_selector(MyEditorUI::onImportClick)
        );
        btn->getNormalImage()->setScale(0.6f);
        
        auto winSize = CCDirector::get()->getWinSize();
        menu->setPosition({winSize.width - 40.f, winSize.height - 75.f});
        
        menu->addChild(btn);
        this->addChild(menu);
        return true;
    }

    void onImportClick(CCObject*) {
        auto task = file::pick(file::PickMode::OpenFile, { .filters = {{ "Images", { "*.png", "*.jpg" } }} });
        m_fields->m_pickListener.bind([this](Task<Result<std::filesystem::path>>::Event* e) {
            if (auto r = e->getValue()) {
                if (r->isOk()) {
                    ImportSettingsPopup::create(r->unwrap().string())->show();
                }
            }
        });
        m_fields->m_pickListener.setFilter(task);
    }
};