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

#include <string>
#include <sstream> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

// --- RGB to HSV Helper ---
struct GDHSV {
    float h, s, v;
};

GDHSV rgbToGdhsv(ccColor3B color) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;

    float max = std::max({r, g, b}), min = std::min({r, g, b});
    float h, s, v = max;
    float d = max - min;
    s = max == 0 ? 0 : d / max;

    if (max == min) {
        h = 0;
    } else {
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6;
    }
    return { h * 360.0f, s, v };
}

// --- LOG POPUP ---
class LogPopup : public Popup<std::string> {
protected:
    bool setup(std::string logText) override {
        this->setTitle("Import Final Report");
        auto contentSize = m_mainLayer->getContentSize();
        auto textArea = SimpleTextArea::create(logText.c_str(), "chatFont.fnt", 0.6f);
        textArea->setWidth(contentSize.width - 40);
        textArea->setPosition(contentSize.width / 2, contentSize.height / 2 + 10);
        m_mainLayer->addChild(textArea);

        auto menu = CCMenu::create();
        menu->setPosition(contentSize.width / 2, 30);
        menu->addChild(CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Done", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(LogPopup::onClose)
        ));
        m_mainLayer->addChild(menu);
        return true;
    }
public:
    static LogPopup* create(std::string text) {
        auto ret = new LogPopup();
        if (ret && ret->initAnchored(360.0f, 280.0f, text)) {
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
        auto centerX = m_mainLayer->getContentSize().width / 2;
        m_stepInput = TextInput::create(100.0f, "Step", "chatFont.fnt");
        m_stepInput->setPosition({centerX, m_mainLayer->getContentSize().height - 70});
        m_stepInput->setString("2"); 
        m_mainLayer->addChild(m_stepInput);

        m_scaleInput = TextInput::create(100.0f, "Scale", "chatFont.fnt");
        m_scaleInput->setPosition({centerX, m_mainLayer->getContentSize().height - 130});
        m_scaleInput->setString("0.1");
        m_mainLayer->addChild(m_scaleInput);

        auto btnMenu = CCMenu::create();
        btnMenu->setPosition({centerX, 45});
        btnMenu->addChild(CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Import", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImportSettingsPopup::onImport)
        ));
        m_mainLayer->addChild(btnMenu);
        return true;
    }

    void onImport(CCObject*) {
        int step = 2; float scale = 0.1f;
        try { step = std::stoi(m_stepInput->getString()); } catch(...) {}
        try { scale = std::stof(m_scaleInput->getString()); } catch(...) {}
        this->spawnBlocks(step > 0 ? step : 1, scale);
        this->onClose(nullptr);
    }

    void spawnBlocks(int step, float scale) {
        std::stringstream ss;
        auto editor = LevelEditorLayer::get();
        if (!editor) return;

        int width, height, channels;
        unsigned char* data = stbi_load(m_path.c_str(), &width, &height, &channels, 4);
        if (!data) {
            ss << "Critical Error: Image data unreadable.\n";
            LogPopup::create(ss.str())->show();
            return;
        }

        auto winSize = CCDirector::get()->getWinSize();
        CCPoint centerPos = editor->m_objectLayer->convertToNodeSpace(winSize / 2);
        
        // Grid spacing: exactly 30 units (standard block size)
        float pixelSize = 30.0f * scale; 
        
        float startX = centerPos.x - ((width / step) * pixelSize / 2);
        float startY = centerPos.y + ((height / step) * pixelSize / 2);

        int count = 0;
        for (int y = 0; y < height; y += step) {
            for (int x = 0; x < width; x += step) {
                int idx = (y * width + x) * 4;
                if (data[idx + 3] < 128) continue;

                ccColor3B col = {data[idx], data[idx+1], data[idx+2]};
                GDHSV hsv = rgbToGdhsv(col);

                // ID 211: The Standard White Square (Massive Hitbox)
                auto obj = editor->createObject(211, {startX + (x/step * pixelSize), startY - (y/step * pixelSize)}, false);
                if (obj) {
                    // GAP FIX: 10% Overlap
                    // Visual scale is slightly larger than the grid spacing
                    obj->setScale(scale * 1.10f); 
                    
                    // The HSV Fix (Force Color)
                    if (obj->m_baseColor) {
                        obj->m_baseColor->m_hsv = { hsv.h, hsv.s, hsv.v, true, true };
                    }
                    if (obj->m_detailColor) {
                        obj->m_detailColor->m_hsv = { hsv.h, hsv.s, hsv.v, true, true };
                    }
                    
                    obj->setChildColor(col);
                    editor->addToSection(obj);
                    count++;
                }
            }
        }
        stbi_image_free(data);
        ss << "Import Success!\nPlaced " << count << " blocks (ID 211).\nMode: HSV Color + 10% Overlap.";
        LogPopup::create(ss.str())->show();
    }

public:
    static ImportSettingsPopup* create(std::string path) {
        auto ret = new ImportSettingsPopup();
        if (ret && ret->initAnchored(260.0f, 240.0f)) {
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
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;
        auto menu = CCMenu::create();
        auto btn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png"), this, menu_selector(MyEditorUI::onImportClick));
        btn->getNormalImage()->setScale(0.6f);
        menu->setPosition({CCDirector::get()->getWinSize().width - 40.0f, CCDirector::get()->getWinSize().height - 75.0f});
        menu->addChild(btn);
        this->addChild(menu);
        return true;
    }
    void onImportClick(CCObject*) {
        auto task = file::pick(file::PickMode::OpenFile, { .filters = {{ "Images", { "*.png", "*.jpg" } }} });
        m_fields->m_pickListener.bind([this](Task<Result<std::filesystem::path>>::Event* event) {
            if (auto res = event->getValue(); res && res->isOk()) {
                ImportSettingsPopup::create(res->unwrap().string())->show();
            }
        });
        m_fields->m_pickListener.setFilter(task);
    }
};