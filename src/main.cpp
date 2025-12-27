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
#include <Geode/binding/FLAlertLayer.hpp>

#include <thread>
#include <vector>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

// data stuff

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

// tut popup

// RENAMED to avoid conflict with Geode's internal TutorialPopup
class ImporterTutorialPopup : public Popup<> {
protected:
    CCLayer* m_page1 = nullptr;
    CCLayer* m_page2 = nullptr;

    bool setup() override {
        this->setTitle("Import Guide");
        auto winSize = CCDirector::get()->getWinSize(); 
        auto size = m_mainLayer->getContentSize();      

        // select block
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

        // img 1
        auto img1 = CCSprite::create("tut1.png"_spr);
        if (img1) {
            img1->setScale(0.8f); 
            img1->setPosition({size.width / 2, size.height / 2 - 10});
            m_page1->addChild(img1);
        } else {
            //text if image aint tehre
            auto err = CCLabelBMFont::create("(tut1.png missing)", "chatFont.fnt");
            err->setPosition({size.width / 2, size.height / 2});
            m_page1->addChild(err);
        }

        auto menu1 = CCMenu::create();
        menu1->setPosition({size.width / 2, 30});
        auto nextBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Next", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImporterTutorialPopup::onNext)
        );
        menu1->addChild(nextBtn);
        m_page1->addChild(menu1);


        // pg 2
        m_page2 = CCLayer::create();
        m_page2->setVisible(false); // hidden lol
        m_mainLayer->addChild(m_page2);

        auto lbl2 = CCLabelBMFont::create("Step 2: Fix Color", "bigFont.fnt");
        lbl2->setScale(0.5f);
        lbl2->setPosition({size.width / 2, size.height - 50});
        m_page2->addChild(lbl2);

        auto txt2 = CCLabelBMFont::create("Set Base Color to BLACK\nso the image isn't tinted!", "chatFont.fnt");
        txt2->setScale(0.6f);
        txt2->setPosition({size.width / 2, size.height - 75});
        m_page2->addChild(txt2);

        // img 2
        auto img2 = CCSprite::create("tut2.png"_spr);
        if (img2) {
            img2->setScale(0.8f);
            img2->setPosition({size.width / 2, size.height / 2 - 10});
            m_page2->addChild(img2);
        }

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
        if (ret && ret->initAnchored(380.f, 280.f)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

// settings popup

class ImportSettingsPopup : public Popup<std::filesystem::path>, public TextInputDelegate {
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

    bool setup(std::filesystem::path path) override {
        m_filePath = path;
        this->setTitle("Import Image Settings");

        auto winSize = m_mainLayer->getContentSize();
        float centerX = winSize.width / 2;
        float topY = winSize.height - 45;

        auto dataRes = file::readBinary(m_filePath);
        if (dataRes) {
            auto rawData = dataRes.unwrap();
            int ch;
            stbi_info_from_memory(rawData.data(), static_cast<int>(rawData.size()), &m_imageWidth, &m_imageHeight, &ch);
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

        // step
        m_mainLayer->addChild(createLabel("Step", {centerX - 90, headerY}));
        m_stepInput = TextInput::create(60.0f, "0", "chatFont.fnt");
        m_stepInput->setPosition({centerX - 90, inputY});
        m_stepInput->setString("0");
        m_stepInput->setCommonFilter(CommonFilter::Uint);
        m_stepInput->setDelegate(this);
        m_mainLayer->addChild(m_stepInput);
        
        auto stepDesc = CCLabelBMFont::create("(Skip Pixels)", "chatFont.fnt");
        stepDesc->setScale(0.4f);
        stepDesc->setColor({200, 200, 200});
        stepDesc->setPosition({centerX - 90, descY});
        m_mainLayer->addChild(stepDesc);

        // tolerance
        m_mainLayer->addChild(createLabel("Tol", {centerX, headerY}));
        m_toleranceInput = TextInput::create(60.0f, "5", "chatFont.fnt");
        m_toleranceInput->setPosition({centerX, inputY});
        m_toleranceInput->setString("5"); 
        m_toleranceInput->setCommonFilter(CommonFilter::Uint);
        m_toleranceInput->setDelegate(this);
        m_mainLayer->addChild(m_toleranceInput);

        auto tolDesc = CCLabelBMFont::create("(Color Diff)", "chatFont.fnt");
        tolDesc->setScale(0.4f);
        tolDesc->setColor({200, 200, 200});
        tolDesc->setPosition({centerX, descY});
        m_mainLayer->addChild(tolDesc);

        // scale
        m_mainLayer->addChild(createLabel("Scale", {centerX + 90, headerY}));
        m_scaleInput = TextInput::create(60.0f, "0.1", "chatFont.fnt");
        m_scaleInput->setPosition({centerX + 90, inputY});
        m_scaleInput->setString("0.1");
        m_scaleInput->setCommonFilter(CommonFilter::Float);
        m_scaleInput->setDelegate(this);
        m_mainLayer->addChild(m_scaleInput);

        auto scaleDesc = CCLabelBMFont::create("(Obj Size)", "chatFont.fnt");
        scaleDesc->setScale(0.4f);
        scaleDesc->setColor({200, 200, 200});
        scaleDesc->setPosition({centerX + 90, descY});
        m_mainLayer->addChild(scaleDesc);

        // toggles
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

        auto safeLabel = CCLabelBMFont::create("Smart Safety", "bigFont.fnt");
        safeLabel->setScale(0.35f);
        safeLabel->setPosition({centerX - 60, toggleLabelY});
        m_mainLayer->addChild(safeLabel);

        auto mergeLabel = CCLabelBMFont::create("Merge Blocks", "bigFont.fnt");
        mergeLabel->setScale(0.35f);
        mergeLabel->setPosition({centerX + 60, toggleLabelY});
        mergeLabel->setColor({150, 255, 150});
        m_mainLayer->addChild(mergeLabel);
        
        auto mergeWarn = CCLabelBMFont::create("(High count = Lag)", "chatFont.fnt");
        mergeWarn->setScale(0.35f);
        mergeWarn->setPosition({centerX + 60, toggleLabelY - 12});
        mergeWarn->setColor({255, 100, 100});
        m_mainLayer->addChild(mergeWarn);

        // buttons
        auto btnMenu = CCMenu::create();
        btnMenu->setPosition({centerX, 30});
        
        auto importBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Import", "goldFont.fnt", "GJ_button_01.png", .8f),
            this, menu_selector(ImportSettingsPopup::onImport)
        );
        btnMenu->addChild(importBtn);

        // help button
        auto helpSprite = ButtonSprite::create("?", "goldFont.fnt", "GJ_button_02.png", 0.8f);
        helpSprite->setScale(0.8f);
        auto helpBtn = CCMenuItemSpriteExtra::create(
            helpSprite,
            this, menu_selector(ImportSettingsPopup::onHelp)
        );
        helpBtn->setPosition({130, 90}); 
        btnMenu->addChild(helpBtn);

        m_mainLayer->addChild(btnMenu);
        this->updateStats();

        // first time tut show
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

    virtual void textChanged(CCTextInputNode*) override { this->updateStats(); }
    void onToggle(CCObject*) { this->updateStats(); }

    void onHelp(CCObject*) { this->showTutorialPopup(); }

    void showTutorialPopup() {
        ImporterTutorialPopup::create()->show();
    }

    int calculateSafeStep(int w, int h) {
        int maxDim = std::max(w, h);
        int step = (maxDim > 200) ? static_cast<int>(std::ceil(maxDim / 200.0f)) : 1;
        long long totalArea = static_cast<long long>(w) * h;
        while ((totalArea / (step * step)) > 10000) step++;
        return step;
    }

    void updateStats() {
        if (m_imageWidth == 0) return;
        int step = m_resizeToggle->isToggled() ? calculateSafeStep(m_imageWidth, m_imageHeight) : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        int count = (m_imageWidth / step) * (m_imageHeight / step);
        m_infoLabel->setString(fmt::format("{}x{} | Step: {}\n~{} Objects", m_imageWidth, m_imageHeight, step, count).c_str());
    }

    void onImport(CCObject*) {
        if (m_isProcessing.exchange(true)) return;
        int step = m_resizeToggle->isToggled() ? calculateSafeStep(m_imageWidth, m_imageHeight) : std::max(1, utils::numFromString<int>(m_stepInput->getString()).unwrapOr(1));
        float scale = utils::numFromString<float>(m_scaleInput->getString()).unwrapOr(0.1f);
        int tolerance = utils::numFromString<int>(m_toleranceInput->getString()).unwrapOr(5);
        this->processImageBackground(step, scale, tolerance, m_mergeToggle->isToggled());
        this->onClose(nullptr);
    }

    void processImageBackground(int step, float visualScale, int tolerance, bool shouldMerge) {
        std::filesystem::path pathCopy = m_filePath;
        std::thread([pathCopy, step, visualScale, tolerance, shouldMerge]() {
            auto dataRes = file::readBinary(pathCopy);
            if (!dataRes) return;
            auto rawData = dataRes.unwrap();
            int w, h, ch;
            unsigned char* pixels = stbi_load_from_memory(rawData.data(), (int)rawData.size(), &w, &h, &ch, 4);
            if (!pixels) return;

            std::vector<BlockData> blocks;
            float effSize = 30.0f * visualScale;
            int gW = (w + step - 1) / step, gH = (h + step - 1) / step;
            float sX = -(gW * effSize) / 2.0f, sY = (gH * effSize) / 2.0f;
            std::vector<bool> visited(gW * gH, false);

            for (int gy = 0; gy < gH; gy++) {
                for (int gx = 0; gx < gW; gx++) {
                    if (visited[gy * gW + gx]) continue;
                    int px = gx * step, py = gy * step;
                    if (px >= w || py >= h) continue;

                    int idx = (py * w + px) * 4;
                    if (pixels[idx + 3] < 200) { visited[gy * gW + gx] = true; continue; }

                    ccColor3B base = { pixels[idx], pixels[idx+1], pixels[idx+2] };
                    int spX = 1, spY = 1;

                    if (shouldMerge) {
                        while (gx + spX < gW && spX < 5) {
                            int nPx = (gx + spX) * step, nIdx = (py * w + nPx) * 4;
                            if (visited[gy * gW + gx + spX] || pixels[nIdx+3] < 200 || 
                                std::abs(pixels[nIdx]-base.r) > tolerance || 
                                std::abs(pixels[nIdx+1]-base.g) > tolerance ||
                                std::abs(pixels[nIdx+2]-base.b) > tolerance) break;
                            spX++;
                        }
                        bool canY = true;
                        while (gy + spY < gH && canY && spY < 5) {
                            for (int k = 0; k < spX; k++) {
                                int cPx = (gx + k) * step, cPy = (gy + spY) * step;
                                int cIdx = (cPy * w + cPx) * 4;
                                if (visited[(gy+spY)*gW + gx+k] || pixels[cIdx+3] < 200 ||
                                    std::abs(pixels[cIdx]-base.r) > tolerance || 
                                    std::abs(pixels[cIdx+1]-base.g) > tolerance ||
                                    std::abs(pixels[cIdx+2]-base.b) > tolerance) { canY = false; break; }
                            }
                            if (canY) spY++;
                        }
                    }

                    for (int dy = 0; dy < spY; dy++)
                        for (int dx = 0; dx < spX; dx++)
                            visited[(gy + dy) * gW + (gx + dx)] = true;

                    blocks.push_back({ sX + (gx * effSize) + (effSize * spX / 2.0f), 
                                       sY - (gy * effSize) - (effSize * spY / 2.0f), 
                                       spX, spY, visualScale, rgbToGdhsv(base), base });
                }
            }
            stbi_image_free(pixels);

            Loader::get()->queueInMainThread([blocks]() {
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
        if (ret && ret->initAnchored(360.f, 260.f, path)) { ret->autorelease(); return ret; }
        CC_SAFE_DELETE(ret); return nullptr;
    }
};

// editor ui

class $modify(MyEditorUI, EditorUI) {
    struct Fields { 
        EventListener<Task<Result<std::filesystem::path>>> m_filePickListener; 
    };

    bool init(LevelEditorLayer* editorLayer) {
        if (!EditorUI::init(editorLayer)) return false;

        if (auto menu = this->getChildByID("undo-menu")) {
            auto sprite = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
            sprite->setScale(0.6f);

            auto importBtn = CCMenuItemSpriteExtra::create(
                sprite,
                this, 
                menu_selector(MyEditorUI::onImportClick)
            );
            
            importBtn->setID("image-import-btn"_spr);
            menu->addChild(importBtn);
            menu->updateLayout(); 
        } 
        return true;
    }

    void onImportClick(CCObject*) {
        auto fileTask = file::pick(file::PickMode::OpenFile, { 
            .filters = {{ "Images", { "*.png", "*.jpg", "*.jpeg" } }} 
        });

        m_fields->m_filePickListener.bind([this](Task<Result<std::filesystem::path>>::Event* event) {
            if (auto result = event->getValue()) {
                if (result->isOk()) ImportSettingsPopup::create(result->unwrap())->show();
            }
        });
        m_fields->m_filePickListener.setFilter(fileTask);
    }
};