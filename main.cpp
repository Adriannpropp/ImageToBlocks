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
#include <atomic>
#include <cmath>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

struct GDHSV { float h, s, v; };
struct BlockData { 
    float x, y; 
    float scaleX, scaleY; 
    GDHSV hsv; 
    ccColor3B color;
};

// Precise RGB -> HSV Converter for Absolute Injection
GDHSV rgbToGdhsv(ccColor3B color) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float max = std::max({r, g, b});
    float min = std::min({r, g, b});
    float d = max - min;
    float h = 0, s = 0, v = max;

    if (max > 0) s = d / max;
    else s = 0;

    if (max == min) h = 0;
    else {
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6;
    }
    // GD Range: Hue 0-360, Sat 0-1, Val 0-1
    return { h * 360.0f, s, v };
}

class LogPopup : public Popup<std::string> {
protected:
    bool setup(std::string text) override {
        this->setTitle("Import Report");
        auto winSize = m_mainLayer->getContentSize();
        auto textArea = SimpleTextArea::create(text.c_str(), "chatFont.fnt", 0.6f);
        textArea->setWidth(winSize.width - 40);
        textArea->setPosition(winSize.width / 2, winSize.height / 2 + 10);
        m_mainLayer->addChild(textArea);

        auto menu = CCMenu::create();
        menu->setPosition(winSize.width / 2, 30);
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
        if (ret && ret->initAnchored(360.f, 200.f, text)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class ImportSettingsPopup : public Popup<std::filesystem::path>, public TextInputDelegate {
protected:
    TextInput* m_stepInput;
    TextInput* m_scaleInput;
    CCLabelBMFont* m_infoLabel;
    CCMenuItemToggler* m_resizeToggle;
    CCMenuItemToggler* m_mergeToggle;
    std::filesystem::path m_path;
    int m_imgW = 0, m_imgH = 0;
    std::atomic<bool> m_isProcessing{false};

    bool setup(std::filesystem::path path) override {
        m_path = path;
        this->setTitle("Import Image");
        auto winSize = m_mainLayer->getContentSize();
        auto cx = winSize.width / 2;

        auto dataRes = file::readBinary(m_path);
        if (dataRes) {
            auto raw = dataRes.unwrap();
            int c; stbi_info_from_memory(raw.data(), raw.size(), &m_imgW, &m_imgH, &c);
        }

        m_infoLabel = CCLabelBMFont::create("Loading...", "chatFont.fnt");
        m_infoLabel->setScale(0.5f);
        m_infoLabel->setPosition({cx, winSize.height - 45});
        m_mainLayer->addChild(m_infoLabel);

        float startY = winSize.height - 80;
        m_mainLayer->addChild(createLabel("Step (0 = Auto):", {cx - 60, startY}));
        m_stepInput = TextInput::create(80.0f, "0", "chatFont.fnt");
        m_stepInput->setPosition({cx - 60, startY - 25});
        m_stepInput->setString("0");
        m_stepInput->setCommonFilter(CommonFilter::Uint);
        m_stepInput->setDelegate(this);
        m_mainLayer->addChild(m_stepInput);

        m_mainLayer->addChild(createLabel("Visual Scale:", {cx + 60, startY}));
        m_scaleInput = TextInput::create(80.0f, "0.1", "chatFont.fnt");
        m_scaleInput->setPosition({cx + 60, startY - 25});
        m_scaleInput->setString("0.1");
        m_scaleInput->setCommonFilter(CommonFilter::Float);
        m_scaleInput->setDelegate(this);
        m_mainLayer->addChild(m_scaleInput);

        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({cx, startY - 65});
        
        m_resizeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_resizeToggle->toggle(true);
        m_resizeToggle->setPosition({-60, 0});
        toggleMenu->addChild(m_resizeToggle);
        
        m_mergeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_mergeToggle->toggle(true); 
        m_mergeToggle->setPosition({60, 0});
        toggleMenu->addChild(m_mergeToggle);
        
        m_mainLayer->addChild(toggleMenu);

        auto safeLabel = CCLabelBMFont::create("Smart Safety", "bigFont.fnt");
        safeLabel->setScale(0.35f);
        safeLabel->setPosition({cx - 60, startY - 85});
        m_mainLayer->addChild(safeLabel);

        auto mergeLabel = CCLabelBMFont::create("Merge Blocks", "bigFont.fnt");
        mergeLabel->setScale(0.35f);
        mergeLabel->setPosition({cx + 60, startY - 85});
        mergeLabel->setColor({150, 255, 150});
        m_mainLayer->addChild(mergeLabel);

        auto menu = CCMenu::create();
        menu->setPosition({cx, 40});
        menu->addChild(CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Import", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImportSettingsPopup::onImport)
        ));
        m_mainLayer->addChild(menu);
        this->updateStats();
        return true;
    }

    CCLabelBMFont* createLabel(const char* txt, CCPoint pos) {
        auto l = CCLabelBMFont::create(txt, "goldFont.fnt");
        l->setScale(0.5f); l->setPosition(pos); return l;
    }

    virtual void textChanged(CCTextInputNode* p0) override { this->updateStats(); }
    void onToggle(CCObject*) { updateStats(); }

    int calculateSafeStep(int w, int h) {
        int maxDim = std::max(w, h);
        int step = (maxDim > 200) ? std::ceil((float)maxDim / 200.0f) : 1;
        while (((long long)w / step) * ((long long)h / step) > 10000) step++;
        return step;
    }

    void updateStats() {
        if (m_imgW == 0) return;
        bool safe = m_resizeToggle->isToggled();
        int step = safe ? calculateSafeStep(m_imgW, m_imgH) : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        int rawCount = (m_imgW / step) * (m_imgH / step);
        m_infoLabel->setString(fmt::format("{}x{} | Step: {}\n~{} Objects", m_imgW, m_imgH, step, rawCount).c_str());
    }

    void onImport(CCObject*) {
        if (m_isProcessing.exchange(true)) return;
        int step = m_resizeToggle->isToggled() ? calculateSafeStep(m_imgW, m_imgH) : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        this->processImage(step, utils::numFromString<float>(m_scaleInput->getString()).unwrapOr(0.1f), m_mergeToggle->isToggled());
        this->onClose(nullptr);
    }

    void processImage(int step, float scale, bool merge) {
        std::filesystem::path pathCopy = m_path;
        std::thread([pathCopy, step, scale, merge]() {
            auto dataRes = file::readBinary(pathCopy);
            if (!dataRes) return;
            auto raw = dataRes.unwrap();
            int w, h, c;
            unsigned char* pixels = stbi_load_from_memory(raw.data(), raw.size(), &w, &h, &c, 4);
            if (!pixels) return;

            std::vector<BlockData> blocks;
            float blockSize = 30.0f * scale;
            int gW = (w + step - 1) / step, gH = (h + step - 1) / step;
            float sX = -(gW * blockSize) / 2.0f, sY = (gH * blockSize) / 2.0f;
            std::vector<bool> visited(gW * gH, false);

            for (int gy = 0; gy < gH; gy++) {
                for (int gx = 0; gx < gW; gx++) {
                    int idx = (gy * step * w + gx * step) * 4;
                    if (visited[gy * gW + gx] || (gy*step >= h || gx*step >= w) || pixels[idx+3] < 200) { visited[gy * gW + gx] = true; continue; }

                    ccColor3B base = { pixels[idx], pixels[idx+1], pixels[idx+2] };
                    int spX = 1, spY = 1;

                    if (merge) {
                        // MERGE CAP = 5 (Safety Zone for ID 211)
                        while (gx + spX < gW && spX < 5) {
                            int nextX = (gx + spX) * step;
                            if (nextX >= w) break; 
                            
                            int checkIdx = (gy * step * w + nextX) * 4;
                            if (visited[gy * gW + (gx + spX)] || pixels[checkIdx+3] < 200 || 
                                std::abs(pixels[checkIdx]-base.r)>3 || 
                                std::abs(pixels[checkIdx+1]-base.g)>3 || 
                                std::abs(pixels[checkIdx+2]-base.b)>3) break;
                            spX++;
                        }
                        
                        bool canY = true;
                        while (gy + spY < gH && canY && spY < 5) {
                            int nextY = (gy + spY) * step;
                            if (nextY >= h) { canY = false; break; }
                            
                            for (int k = 0; k < spX; k++) {
                                int checkX = (gx + k) * step;
                                int cIdx = (nextY * w + checkX) * 4;
                                if (visited[(gy + spY) * gW + (gx + k)] || pixels[cIdx+3] < 200 || 
                                    std::abs(pixels[cIdx]-base.r)>3 || 
                                    std::abs(pixels[cIdx+1]-base.g)>3 || 
                                    std::abs(pixels[cIdx+2]-base.b)>3) { canY = false; break; }
                            }
                            if (canY) spY++;
                        }
                    }
                    for (int dy = 0; dy < spY; dy++) for (int dx = 0; dx < spX; dx++) visited[(gy + dy) * gW + (gx + dx)] = true;
                    
                    float finalX = sX + (gx * blockSize) + (spX * blockSize / 2.0f);
                    float finalY = sY - (gy * blockSize) - (spY * blockSize / 2.0f);
                    
                    // STORE BOTH: HSV for overriding black levels, RGB for editor preview
                    blocks.push_back({ finalX, finalY, scale * spX, scale * spY, rgbToGdhsv(base), base });
                }
            }
            stbi_image_free(pixels);

            Loader::get()->queueInMainThread([blocks]() {
                auto editor = LevelEditorLayer::get();
                if (!editor) return;
                auto center = editor->m_objectLayer->convertToNodeSpace(CCDirector::get()->getWinSize() / 2);
                for (const auto& b : blocks) {
                    // KEEPING ID 211 AS REQUESTED
                    auto obj = editor->createObject(211, center + ccp(b.x, b.y), false);
                    if (obj) {
                        obj->setScaleX(b.scaleX); 
                        obj->setScaleY(b.scaleY);
                        
                        // Visibility Flags
                        obj->m_isDontFade = true; 
                        obj->m_isDontEnter = true; 
                        
                        // FORCE COLOR: Inject Absolute HSV to override level background
                        // This uses brace initialization {h, s, v, absSat, absBri}
                        if (obj->m_baseColor) { 
                            obj->m_baseColor->m_hsv = { b.hsv.h, b.hsv.s, b.hsv.v, true, true };
                        }
                        if (obj->m_detailColor) {
                            obj->m_detailColor->m_hsv = { b.hsv.h, b.hsv.s, b.hsv.v, true, true };
                        }
                        
                        // Fallback tint
                        obj->setColor(b.color);
                        obj->setChildColor(b.color);
                    }
                }
            });
        }).detach();
    }

public:
    static ImportSettingsPopup* create(std::filesystem::path path) {
        auto ret = new ImportSettingsPopup();
        if (ret && ret->initAnchored(300.f, 260.f, path)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class $modify(MyEditorUI, EditorUI) {
    struct Fields { EventListener<Task<Result<std::filesystem::path>>> m_pickListener; };
    bool init(LevelEditorLayer* el) {
        if (!EditorUI::init(el)) return false;
        if (auto menu = this->getChildByID("undo-menu")) {
            auto btn = CCMenuItemSpriteExtra::create(CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png"), this, menu_selector(MyEditorUI::onImportClick));
            btn->getNormalImage()->setScale(0.6f); menu->addChild(btn); menu->updateLayout(); 
        } 
        return true;
    }
    void onImportClick(CCObject*) {
        auto task = file::pick(file::PickMode::OpenFile, { .filters = {{ "Images", { "*.png", "*.jpg", "*.jpeg" } }} });
        m_fields->m_pickListener.bind([this](Task<Result<std::filesystem::path>>::Event* e) {
            if (auto r = e->getValue()) if (r->isOk()) ImportSettingsPopup::create(r->unwrap())->show();
        });
        m_fields->m_pickListener.setFilter(task);
    }
};