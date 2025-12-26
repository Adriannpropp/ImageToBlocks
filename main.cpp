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

// --- Data Structures ---
// Added scaleX/Y to support merged blocks
struct BlockData { 
    float x, y; 
    float scaleX, scaleY; 
    ccColor3B color; 
};

// --- Popups ---
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
        if (ret && ret->initAnchored(360.f, 200.f, text)) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

class ImportSettingsPopup : public Popup<std::filesystem::path>, public TextInputDelegate {
protected:
    TextInput* m_stepInput;
    TextInput* m_scaleInput;
    CCLabelBMFont* m_infoLabel;
    CCMenuItemToggler* m_resizeToggle;
    CCMenuItemToggler* m_mergeToggle; // New Toggle
    
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
            int c;
            stbi_info_from_memory(raw.data(), raw.size(), &m_imgW, &m_imgH, &c);
        }

        m_infoLabel = CCLabelBMFont::create("Loading...", "chatFont.fnt");
        m_infoLabel->setScale(0.5f);
        m_infoLabel->setPosition({cx, winSize.height - 45});
        m_mainLayer->addChild(m_infoLabel);

        float startY = winSize.height - 80;

        // Step Input
        m_mainLayer->addChild(createLabel("Step (0 = Auto):", {cx - 60, startY}));
        m_stepInput = TextInput::create(80.0f, "0", "chatFont.fnt");
        m_stepInput->setPosition({cx - 60, startY - 25});
        m_stepInput->setString("0");
        m_stepInput->setCommonFilter(CommonFilter::Uint);
        m_stepInput->setDelegate(this);
        m_mainLayer->addChild(m_stepInput);

        // Scale Input
        m_mainLayer->addChild(createLabel("Visual Scale:", {cx + 60, startY}));
        m_scaleInput = TextInput::create(80.0f, "0.1", "chatFont.fnt");
        m_scaleInput->setPosition({cx + 60, startY - 25});
        m_scaleInput->setString("0.1");
        m_scaleInput->setCommonFilter(CommonFilter::Float);
        m_scaleInput->setDelegate(this);
        m_mainLayer->addChild(m_scaleInput);

        // --- Toggles Row ---
        auto toggleMenu = CCMenu::create();
        toggleMenu->setPosition({cx, startY - 65});
        
        // Safety Toggle
        m_resizeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_resizeToggle->toggle(true);
        m_resizeToggle->setPosition({-60, 0});
        toggleMenu->addChild(m_resizeToggle);
        
        // Merge Toggle (New)
        m_mergeToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(ImportSettingsPopup::onToggle), 0.6f);
        m_mergeToggle->toggle(true); // Default ON for optimization
        m_mergeToggle->setPosition({60, 0});
        toggleMenu->addChild(m_mergeToggle);
        
        m_mainLayer->addChild(toggleMenu);

        // Toggle Labels
        auto safeLabel = CCLabelBMFont::create("Smart Safety", "bigFont.fnt");
        safeLabel->setScale(0.35f);
        safeLabel->setPosition({cx - 60, startY - 85});
        m_mainLayer->addChild(safeLabel);

        auto mergeLabel = CCLabelBMFont::create("Merge Blocks", "bigFont.fnt");
        mergeLabel->setScale(0.35f);
        mergeLabel->setPosition({cx + 60, startY - 85});
        mergeLabel->setColor({150, 255, 150}); // Green hint
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
        int step = 1;
        if (maxDim > 200) step = std::ceil((float)maxDim / 200.0f);
        while (true) {
            long long estBlocks = ((long long)w / step) * ((long long)h / step);
            if (estBlocks <= 10000) break; // Relaxed limit because Merging will reduce count
            step++;
        }
        return step;
    }

    void updateStats() {
        if (m_imgW == 0) return;
        bool safe = m_resizeToggle->isToggled();
        int userStep = utils::numFromString<int>(m_stepInput->getString()).unwrapOr(0);
        int step = calculateSafeStep(m_imgW, m_imgH);
        
        if (!safe && userStep > 0) step = userStep;
        
        int rawCount = (m_imgW / step) * (m_imgH / step);
        std::string info = fmt::format("{}x{} | Step: {}", m_imgW, m_imgH, step);
        
        if (m_mergeToggle->isToggled()) {
            info += fmt::format("\n~{} Pixels (Merged)", rawCount);
            m_infoLabel->setColor({100, 255, 255}); // Cyan for optimized
        } else {
            info += fmt::format("\n~{} Blocks (Raw)", rawCount);
            if (rawCount > 10000 && !safe) m_infoLabel->setColor({255, 50, 50});
            else m_infoLabel->setColor({255, 255, 255});
        }
        
        m_infoLabel->setString(info.c_str());
    }

    void onImport(CCObject*) {
        if (m_isProcessing.exchange(true)) return;
        int userStep = utils::numFromString<int>(m_stepInput->getString()).unwrapOr(0);
        float scale = utils::numFromString<float>(m_scaleInput->getString()).unwrapOr(0.1f);
        bool safe = m_resizeToggle->isToggled();
        bool merge = m_mergeToggle->isToggled();
        
        if (userStep < 1) userStep = 1;

        this->processImage(userStep, scale, safe, merge);
        this->onClose(nullptr);
    }

    void processImage(int userStep, float scale, bool safe, bool merge) {
        std::filesystem::path pathCopy = m_path;

        std::thread([this, pathCopy, userStep, scale, safe, merge]() {
            auto dataRes = file::readBinary(pathCopy);
            if (!dataRes) return;
            auto raw = dataRes.unwrap();

            int w, h, c;
            unsigned char* pixels = stbi_load_from_memory(raw.data(), raw.size(), &w, &h, &c, 4);
            if (!pixels) return;

            // --- Calc Step ---
            int step = 1;
            int maxDim = std::max(w, h);
            if (safe) {
                if (maxDim > 200) step = std::ceil((float)maxDim / 200.0f);
                while (((long long)w / step) * ((long long)h / step) > 10000) step++;
            } else {
                step = (userStep < 1) ? 1 : userStep;
            }

            std::vector<BlockData> blocks;
            float blockSize = 30.0f * scale;
            
            // Grid Dimensions
            int gridW = (w + step - 1) / step;
            int gridH = (h + step - 1) / step;

            // Total world size for centering
            float totalW = gridW * blockSize;
            float totalH = gridH * blockSize;
            float startX = -totalW / 2.0f;
            float startY = totalH / 2.0f;

            // Visited array for merging
            std::vector<bool> visited(gridW * gridH, false);

            for (int gy = 0; gy < gridH; gy++) {
                for (int gx = 0; gx < gridW; gx++) {
                    if (visited[gy * gridW + gx]) continue;

                    // Get pixel color at this grid position
                    int pxX = gx * step;
                    int pxY = gy * step;
                    
                    // Safety check
                    if (pxX >= w || pxY >= h) continue;

                    int idx = (pxY * w + pxX) * 4;
                    if (pixels[idx + 3] < 200) { // Alpha Threshold
                        visited[gy * gridW + gx] = true;
                        continue;
                    }

                    ccColor3B baseColor = { pixels[idx], pixels[idx+1], pixels[idx+2] };
                    
                    int spanX = 1;
                    int spanY = 1;

                    if (merge) {
                        // GREEDY MESHING: Expand Right
                        while (gx + spanX < gridW) {
                            int nextX = (gx + spanX) * step;
                            int nextIdx = (pxY * w + nextX) * 4;
                            
                            if (visited[gy * gridW + (gx + spanX)]) break;
                            if (pixels[nextIdx+3] < 200) break;
                            
                            // Exact color match for merging
                            if (std::abs(pixels[nextIdx] - baseColor.r) > 5 ||
                                std::abs(pixels[nextIdx+1] - baseColor.g) > 5 ||
                                std::abs(pixels[nextIdx+2] - baseColor.b) > 5) break;
                                
                            spanX++;
                        }

                        // GREEDY MESHING: Expand Down
                        bool canExpandY = true;
                        while (gy + spanY < gridH && canExpandY) {
                            for (int k = 0; k < spanX; k++) {
                                int checkX = (gx + k) * step;
                                int checkY = (gy + spanY) * step;
                                int checkIdx = (checkY * w + checkX) * 4;

                                if (visited[(gy + spanY) * gridW + (gx + k)]) { canExpandY = false; break; }
                                if (pixels[checkIdx+3] < 200) { canExpandY = false; break; }
                                
                                if (std::abs(pixels[checkIdx] - baseColor.r) > 5 ||
                                    std::abs(pixels[checkIdx+1] - baseColor.g) > 5 ||
                                    std::abs(pixels[checkIdx+2] - baseColor.b) > 5) { canExpandY = false; break; }
                            }
                            if (canExpandY) spanY++;
                        }
                    }

                    // Mark merged area as visited
                    for (int dy = 0; dy < spanY; dy++) {
                        for (int dx = 0; dx < spanX; dx++) {
                            visited[(gy + dy) * gridW + (gx + dx)] = true;
                        }
                    }

                    // Calculate Center Position for the merged block
                    // Center = Start + (Index * Size) + (Span * Size / 2) - (Size / 2) -> Simpler math below
                    float finalX = startX + (gx * blockSize) + (spanX * blockSize / 2.0f); 
                    float finalY = startY - (gy * blockSize) - (spanY * blockSize / 2.0f);

                    // Store with non-uniform scale
                    blocks.push_back({ finalX, finalY, scale * spanX, scale * spanY, baseColor });
                }
            }
            stbi_image_free(pixels);

            Loader::get()->queueInMainThread([blocks, scale, step, safe, merge]() {
                auto editor = LevelEditorLayer::get();
                if (!editor) return;

                auto center = editor->m_objectLayer->convertToNodeSpace(CCDirector::get()->getWinSize() / 2);
                int count = 0;

                for (const auto& b : blocks) {
                    auto obj = editor->createObject(211, center + ccp(b.x, b.y), false);
                    if (obj) {
                        // Apply Merged Scale
                        obj->setScaleX(b.scaleX);
                        obj->setScaleY(b.scaleY);
                        
                        if (obj->m_baseColor) {
                            obj->m_baseColor->m_hsv = { 0, 0, 0, true, true }; // Reset HSV
                        }
                        
                        obj->setChildColor(b.color);
                        count++;
                    }
                }
                
                std::string msg = fmt::format("Imported {} objects.", count);
                if (merge) msg += "\n(Merged Blocks Enabled)";
                LogPopup::create(msg)->show();
            });
        }).detach();
    }

public:
    static ImportSettingsPopup* create(std::filesystem::path path) {
        auto ret = new ImportSettingsPopup();
        if (ret && ret->initAnchored(300.f, 260.f, path)) {
            ret->autorelease();
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
        if (auto menu = this->getChildByID("undo-menu")) {
            auto btn = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png"), 
                this, 
                menu_selector(MyEditorUI::onImportClick)
            );
            btn->getNormalImage()->setScale(0.6f);
            menu->addChild(btn);
            menu->updateLayout(); 
        } 
        return true;
    }

    void onImportClick(CCObject*) {
        auto task = file::pick(file::PickMode::OpenFile, { .filters = {{ "Images", { "*.png", "*.jpg", "*.jpeg" } }} });
        m_fields->m_pickListener.bind([this](Task<Result<std::filesystem::path>>::Event* e) {
            if (auto r = e->getValue()) {
                if (r->isOk()) {
                    ImportSettingsPopup::create(r->unwrap())->show();
                }
            }
        });
        m_fields->m_pickListener.setFilter(task);
    }
};