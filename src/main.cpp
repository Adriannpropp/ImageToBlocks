#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/ui/TextInput.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/async.hpp>

#include <Geode/binding/GameObject.hpp>
#include <Geode/binding/LevelEditorLayer.hpp>
#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/ButtonSprite.hpp>

#include <thread>
#include <vector>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

struct GDHSV { float h, s, v; };
struct BlockData {
    float x, y;
    int spanX, spanY;
    float visualScale;
    GDHSV hsv;
    ccColor3B color;
};

GDHSV rgbToGdhsv(ccColor3B color) {
    float r = color.r / 255.0f;
    float g = color.g / 255.0f;
    float b = color.b / 255.0f;
    float max = std::max({r, g, b}), min = std::min({r, g, b});
    float d = max - min, h = 0, s = (max > 0 ? d / max : 0), v = max;

    if (max != min) {
        if (max == r) h = (g - b) / d + (g < b ? 6 : 0);
        else if (max == g) h = (b - r) / d + 2;
        else h = (r - g) / d + 4;
        h /= 6;
    }
    return { h * 360.0f, s, v };
}

class ImporterTutorialPopup : public Popup {
protected:
    CCLayer* m_page1 = nullptr;
    CCLayer* m_page2 = nullptr;

    bool init() override {
        if (!Popup::init(380.f, 280.f)) return false;
        this->setTitle("Import Guide");
        auto size = m_mainLayer->getContentSize();

        m_page1 = CCLayer::create();
        m_mainLayer->addChild(m_page1);

        auto lbl1 = CCLabelBMFont::create("Step 1: Select Block", "bigFont.fnt");
        lbl1->setScale(0.5f);
        lbl1->setPosition({size.width / 2, size.height - 50});
        m_page1->addChild(lbl1);

        auto txt1 = CCLabelBMFont::create("Select any object you want\nto use for the pixels.", "chatFont.fnt");
        txt1->setScale(0.6f);
        txt1->setPosition({size.width / 2, size.height - 75});
        m_page1->addChild(txt1);

        auto menu1 = CCMenu::create();
        menu1->setPosition({size.width / 2, 30});
        auto nextBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Next", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImporterTutorialPopup::onNext)
        );
        menu1->addChild(nextBtn);
        m_page1->addChild(menu1);

        m_page2 = CCLayer::create();
        m_page2->setVisible(false);
        m_mainLayer->addChild(m_page2);

        auto lbl2 = CCLabelBMFont::create("Step 2: Fix Color", "bigFont.fnt");
        lbl2->setScale(0.5f);
        lbl2->setPosition({size.width / 2, size.height - 50});
        m_page2->addChild(lbl2);

        auto txt2 = CCLabelBMFont::create("Set Base Color to BLACK\nso the image isn't tinted!", "chatFont.fnt");
        txt2->setScale(0.6f);
        txt2->setPosition({size.width / 2, size.height - 75});
        m_page2->addChild(txt2);

        auto menu2 = CCMenu::create();
        menu2->setPosition({size.width / 2, 30});
        auto closeBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Close", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImporterTutorialPopup::onClose)
        );
        menu2->addChild(closeBtn);
        m_page2->addChild(menu2);

        return true;
    }

    void onNext(CCObject*) {
        m_page1->setVisible(false);
        m_page2->setVisible(true);
    }

public:
    static ImporterTutorialPopup* create() {
        auto ret = new ImporterTutorialPopup();
        if (ret && ret->init()) {
            ret->autorelease(); return ret;
        }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class ImportSettingsPopup : public Popup, public TextInputDelegate {
protected:
    TextInput* m_stepInput = nullptr;
    TextInput* m_scaleInput = nullptr;
    TextInput* m_toleranceInput = nullptr;
    CCLabelBMFont* m_infoLabel = nullptr;
    CCMenuItemToggler* m_resizeToggle = nullptr;
    CCMenuItemToggler* m_mergeToggle = nullptr;
    
    std::filesystem::path m_filePath;
    int m_imageWidth = 0;
    int m_imageHeight = 0;
    std::atomic<bool> m_isProcessing{false};

    bool init(std::filesystem::path path) {
        m_filePath = path;
        if (!Popup::init(360.f, 260.f)) return false;
        this->setTitle("Import Image Settings");

        auto winSize = m_mainLayer->getContentSize();
        float centerX = winSize.width / 2;
        float topY = winSize.height - 45;

        if (auto dataRes = file::readBinary(m_filePath)) {
            auto rawData = dataRes.unwrap();
            int ch;
            stbi_info_from_memory(rawData.data(), (int)rawData.size(), &m_imageWidth, &m_imageHeight, &ch);
        }

        m_infoLabel = CCLabelBMFont::create("Loading info...", "chatFont.fnt");
        m_infoLabel->setScale(0.5f);
        m_infoLabel->setPosition({centerX, topY});
        m_mainLayer->addChild(m_infoLabel);

        float headerY = winSize.height - 85;
        float inputY = headerY - 25;
        float descY = inputY - 25;
        float toggleY = descY - 40;
        float toggleLabelY = toggleY - 25;

        m_mainLayer->addChild(createLabel("Step", {centerX - 90, headerY}));
        m_stepInput = TextInput::create(60.0f, "0", "chatFont.fnt");
        m_stepInput->setPosition({centerX - 90, inputY});
        m_stepInput->setString("0");
        m_stepInput->setFilter("0123456789");
        m_stepInput->setDelegate(this);
        m_mainLayer->addChild(m_stepInput);

        m_mainLayer->addChild(createLabel("Tol", {centerX, headerY}));
        m_toleranceInput = TextInput::create(60.0f, "5", "chatFont.fnt");
        m_toleranceInput->setPosition({centerX, inputY});
        m_toleranceInput->setString("5");
        m_toleranceInput->setFilter("0123456789");
        m_toleranceInput->setDelegate(this);
        m_mainLayer->addChild(m_toleranceInput);

        m_mainLayer->addChild(createLabel("Scale", {centerX + 90, headerY}));
        m_scaleInput = TextInput::create(60.0f, "0.1", "chatFont.fnt");
        m_scaleInput->setPosition({centerX + 90, inputY});
        m_scaleInput->setString("0.1");
        m_scaleInput->setFilter("0123456789.");
        m_scaleInput->setDelegate(this);
        m_mainLayer->addChild(m_scaleInput);

        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({centerX, toggleY});

        m_resizeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_resizeToggle->toggle(true);
        m_resizeToggle->setPosition({-60, 0});
        toggleMenu->addChild(m_resizeToggle);

        m_mergeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_mergeToggle->toggle(true);
        m_mergeToggle->setPosition({60, 0});
        toggleMenu->addChild(m_mergeToggle);
        m_mainLayer->addChild(toggleMenu);

        m_mainLayer->addChild(createSmallLabel("Smart Safety", {centerX - 60, toggleLabelY}));
        auto mergeLabel = createSmallLabel("Merge Blocks", {centerX + 60, toggleLabelY});
        mergeLabel->setColor({150, 255, 150});
        m_mainLayer->addChild(mergeLabel);

        auto mergeWarn = createSmallLabel("(High count = Lag)", {centerX + 60, toggleLabelY - 12});
        mergeWarn->setColor({255, 100, 100});
        m_mainLayer->addChild(mergeWarn);

        auto btnMenu = CCMenu::create();
        btnMenu->setPosition({centerX, 30});
        
        auto importBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Import", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImportSettingsPopup::onImport)
        );
        btnMenu->addChild(importBtn);

        auto helpBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("?", "goldFont.fnt", "GJ_button_02.png", 0.8f),
            this, menu_selector(ImportSettingsPopup::onHelp)
        );
        helpBtn->setPosition({130, 90});
        btnMenu->addChild(helpBtn);

        m_mainLayer->addChild(btnMenu);
        this->updateStats();

        if (!Mod::get()->getSavedValue<bool>("shown-guide-v2", false)) {
            this->runAction(CCSequence::create(
                CCDelayTime::create(0.5f),
                CCCallFunc::create(this, callfunc_selector(ImportSettingsPopup::showTutorialPopup)),
                nullptr
            ));
            Mod::get()->setSavedValue("shown-guide-v2", true);
        }

        return true;
    }

    CCLabelBMFont* createLabel(const char* text, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "goldFont.fnt");
        label->setScale(0.5f);
        label->setPosition(pos);
        return label;
    }

    CCLabelBMFont* createSmallLabel(const char* text, CCPoint pos) {
        auto label = CCLabelBMFont::create(text, "chatFont.fnt");
        label->setScale(0.4f);
        label->setPosition(pos);
        return label;
    }

    void textChanged(CCTextInputNode*) override { this->updateStats(); }
    void onToggle(CCObject*) { this->updateStats(); }
    void onHelp(CCObject*) { this->showTutorialPopup(); }

    void showTutorialPopup() {
        ImporterTutorialPopup::create()->show();
    }

    int calculateSafeStep(int w, int h) {
        int maxDim = std::max(w, h);
        int step = (maxDim > 200) ? static_cast<int>(std::ceil(maxDim / 200.0f)) : 1;
        long long total = (long long)w * h;
        while ((total / (step * step)) > 10000) step++;
        return step;
    }

    void updateStats() {
        if (m_imageWidth == 0) return;
        int step = m_resizeToggle->isToggled() ? calculateSafeStep(m_imageWidth, m_imageHeight) 
                                               : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        int count = (m_imageWidth / step) * (m_imageHeight / step);
        m_infoLabel->setString(fmt::format("{}x{} | Step: {}\n~{} Objects", m_imageWidth, m_imageHeight, step, count).c_str());
    }

    void onImport(CCObject*) {
        if (m_isProcessing.exchange(true)) return;

        int step = m_resizeToggle->isToggled() ? calculateSafeStep(m_imageWidth, m_imageHeight) 
                                               : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        float scale = utils::numFromString<float>(m_scaleInput->getString()).unwrapOr(0.1f);
        int tolerance = utils::numFromString<int>(m_toleranceInput->getString()).unwrapOr(5);

        this->processImageBackground(step, scale, tolerance, m_mergeToggle->isToggled());
        this->onClose(nullptr);
    }

    void processImageBackground(int step, float visualScale, int tolerance, bool shouldMerge) {
        std::filesystem::path pathCopy = m_filePath;
        std::thread([pathCopy, step, visualScale, tolerance, shouldMerge]() mutable {
            auto dataRes = file::readBinary(pathCopy);
            if (!dataRes) return;
            auto rawData = dataRes.unwrap();

            int w, h, ch;
            unsigned char* pixels = stbi_load_from_memory(rawData.data(), (int)rawData.size(), &w, &h, &ch, 4);
            if (!pixels) return;

            std::vector<BlockData> blocks;
            float effSize = 30.0f * visualScale;
            int gW = (w + step - 1) / step;
            int gH = (h + step - 1) / step;
            float sX = -(gW * effSize) / 2.0f;
            float sY = (gH * effSize) / 2.0f;

            std::vector<bool> visited(gW * gH, false);

            for (int gy = 0; gy < gH; gy++) {
                for (int gx = 0; gx < gW; gx++) {
                    if (visited[gy * gW + gx]) continue;

                    int px = gx * step, py = gy * step;
                    if (px >= w || py >= h) continue;

                    int idx = (py * w + px) * 4;
                    if (pixels[idx + 3] < 200) {
                        visited[gy * gW + gx] = true;
                        continue;
                    }

                    ccColor3B base = {pixels[idx], pixels[idx+1], pixels[idx+2]};
                    int spX = 1, spY = 1;

                    if (shouldMerge) {
                        while (gx + spX < gW && spX < 5) {
                            int nIdx = (py * w + (gx + spX) * step) * 4;
                            if (visited[gy * gW + gx + spX] || pixels[nIdx+3] < 200 ||
                                std::abs(pixels[nIdx]-base.r) > tolerance ||
                                std::abs(pixels[nIdx+1]-base.g) > tolerance ||
                                std::abs(pixels[nIdx+2]-base.b) > tolerance) break;
                            spX++;
                        }

                        bool canY = true;
                        while (gy + spY < gH && canY && spY < 5) {
                            for (int k = 0; k < spX; k++) {
                                int cIdx = ((gy + spY) * step * w + (gx + k) * step) * 4;
                                if (visited[(gy+spY)*gW + gx+k] || pixels[cIdx+3] < 200 ||
                                    std::abs(pixels[cIdx]-base.r) > tolerance ||
                                    std::abs(pixels[cIdx+1]-base.g) > tolerance ||
                                    std::abs(pixels[cIdx+2]-base.b) > tolerance) {
                                    canY = false; break;
                                }
                            }
                            if (canY) spY++;
                        }
                    }

                    for (int dy = 0; dy < spY; dy++)
                        for (int dx = 0; dx < spX; dx++)
                            visited[(gy + dy) * gW + (gx + dx)] = true;

                    blocks.push_back({
                        sX + (gx * effSize) + (effSize * spX / 2.0f),
                        sY - (gy * effSize) - (effSize * spY / 2.0f),
                        spX, spY, visualScale, rgbToGdhsv(base), base
                    });
                }
            }
            stbi_image_free(pixels);

            Loader::get()->queueInMainThread([blocks = std::move(blocks)]() {
                auto editor = LevelEditorLayer::get();
                if (!editor) return;

                auto center = editor->m_objectLayer->convertToNodeSpace(CCDirector::get()->getWinSize() / 2);

                std::ostringstream ss;
                for (const auto& b : blocks) {
                    ss << "1,211,2," << center.x + b.x << ",3," << center.y + b.y
                       << ",41,1,67,1,43," << fmt::format("{}a{}a{}a1a1", b.hsv.h, b.hsv.s, b.hsv.v)
                       << ",128," << b.visualScale * b.spanX << ",129," << b.visualScale * b.spanY << ";";
                }

                editor->createObjectsFromString(ss.str(), true, true);
                Notification::create("Import Complete", NotificationIcon::Success)->show();
            });
        }).detach();
    }

public:
    static ImportSettingsPopup* create(std::filesystem::path path) {
        auto ret = new ImportSettingsPopup();
        if (ret && ret->init(path)) {
            ret->autorelease(); return ret;
        }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

class $modify(MyEditorUI, EditorUI) {
    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        if (auto menu = this->getChildByID("undo-menu")) {
            auto sprite = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
            sprite->setScale(0.6f);

            auto importBtn = CCMenuItemSpriteExtra::create(
                sprite, this, menu_selector(MyEditorUI::onImportClick)
            );
            
            importBtn->setID("image-import-btn"_spr);
            menu->addChild(importBtn);
            menu->updateLayout();
        }
        return true;
    }

    void onImportClick(CCObject*) {
        async::spawn(file::pick(file::PickMode::OpenFile, {
            .filters = {{ "Images", { "*.png", "*.jpg", "*.jpeg" } }}
        }), [this](Result<std::optional<std::filesystem::path>> result) {
            if (result.isOk()) {
                if (auto path = result.unwrap()) {
                    ImportSettingsPopup::create(*path)->show();
                }
            }
        });
    }
};
