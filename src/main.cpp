#include "NameTags.hpp"

// ── Utility: build/update the world-space canvas on a VRRig ─────────────────

static UnityEngine::GameObject* CreateTextObject(
        UnityEngine::GameObject* parent,
        const std::string& name,
        float yOffset,
        float fontSize)
{
    auto* go = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr(name));
    go->get_transform()->SetParent(parent->get_transform(), false);
    go->get_transform()->set_localPosition({0, yOffset, 0});
    go->get_transform()->set_localScale({0.003f, 0.003f, 0.003f});

    auto* canvas = go->AddComponent<UnityEngine::Canvas*>();
    canvas->set_renderMode(UnityEngine::RenderMode::WorldSpace);

    auto* textGo = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr(name + "_Text"));
    textGo->get_transform()->SetParent(go->get_transform(), false);

    auto* text = textGo->AddComponent<UnityEngine::UI::Text*>();
    text->set_text(il2cpp_utils::newcsstr(""));
    text->set_fontSize((int)fontSize);
    text->set_alignment(UnityEngine::TextAnchor::MiddleCenter);
    text->set_color({1,1,1,1});

    // Use a bold built-in font for readability
    auto* font = UnityEngine::Resources::GetBuiltinResource<UnityEngine::Font*>(
        il2cpp_utils::newcsstr("Arial.ttf"));
    if (font) text->set_font(font);

    return go;
}

static void EnsureNametag(void* rigPtr, UnityEngine::GameObject* rigGo) {
    if (nametagMap.count(rigPtr)) return;

    NametagData data;

    // Root anchor sits 0.35 m above head origin
    auto* anchor = UnityEngine::GameObject::New_ctor(il2cpp_utils::newcsstr("NametagAnchor"));
    anchor->get_transform()->SetParent(rigGo->get_transform(), false);
    anchor->get_transform()->set_localPosition({0, 0.35f, 0});

    // Username + platform on top line (larger)
    auto* nameGo = CreateTextObject(anchor, "NameLine", 0.06f, 28);
    data.nameText = nameGo
        ->get_transform()->Find(il2cpp_utils::newcsstr("NameLine_Text"))
        ->get_gameObject()
        ->GetComponent<UnityEngine::UI::Text*>();

    // Stats line below (smaller)
    auto* statsGo = CreateTextObject(anchor, "StatsLine", -0.02f, 22);
    data.statsText = statsGo
        ->get_transform()->Find(il2cpp_utils::newcsstr("StatsLine_Text"))
        ->get_gameObject()
        ->GetComponent<UnityEngine::UI::Text*>();

    data.canvasObj = anchor;
    nametagMap[rigPtr] = data;
}

// ── Rich-text helpers ────────────────────────────────────────────────────────

static std::string ColorHex(UnityEngine::Color c) {
    char buf[10];
    snprintf(buf, sizeof(buf), "#%02X%02X%02X",
        (int)(c.r * 255), (int)(c.g * 255), (int)(c.b * 255));
    return std::string(buf);
}

static std::string Colored(const std::string& text, UnityEngine::Color c) {
    return "<color=" + ColorHex(c) + ">" + text + "</color>";
}

static std::string PlatformTag(Platform p) {
    switch (p) {
        case Platform::Quest: return Colored("[Q]", colorForPlatform(Platform::Quest));
        case Platform::Steam: return Colored("[S]", colorForPlatform(Platform::Steam));
        default:              return Colored("[?]", {0.7f,0.7f,0.7f,1});
    }
}

// ── Refresh-rate detection ───────────────────────────────────────────────────

static int GetTargetRefreshRate() {
    // XRDevice.refreshRate gives the headset's configured refresh rate
    float hz = UnityEngine::XR::XRDevice::get_refreshRate();
    if (hz > 0) return (int)hz;
    return UnityEngine::Application::get_targetFrameRate();
}

// ── Rolling FPS tracker ──────────────────────────────────────────────────────

static float  s_fpsAccum  = 0;
static int    s_fpsFrames = 0;
static int    s_lastFps   = 0;
static float  s_fpsTimer  = 0;

static void TickFps(float dt) {
    s_fpsAccum  += 1.0f / dt;
    s_fpsFrames += 1;
    s_fpsTimer  += dt;
    if (s_fpsTimer >= 0.5f) {  // update every 500 ms
        s_lastFps   = (s_fpsFrames > 0) ? (int)(s_fpsAccum / s_fpsFrames) : 0;
        s_fpsAccum  = 0;
        s_fpsFrames = 0;
        s_fpsTimer  = 0;
    }
}

// ── Hooks ────────────────────────────────────────────────────────────────────

// Hook: VRRig::LateUpdate — runs every frame per player rig
MAKE_HOOK_MATCH(VRRig_LateUpdate, &GorillaTag::VRRig::LateUpdate, void,
                GorillaTag::VRRig* self)
{
    VRRig_LateUpdate(self);

    // Skip local player's own rig
    if (self->get_isOfflineVRRig() || self->get_isMyPlayer()) return;

    void* key = (void*)self;
    auto* go  = self->get_gameObject();
    EnsureNametag(key, go);

    auto& data = nametagMap[key];

    // ── Gather data from Photon player ──────────────────────────────────────
    auto* photonPlayer = self->get_PhotonView()
        ? self->get_PhotonView()->get_Owner()
        : nullptr;

    if (photonPlayer) {
        // Username
        auto* nameCs = photonPlayer->get_NickName();
        data.username = nameCs ? to_utf8(csstrtostr(nameCs)) : "???";

        // Ping
        data.lastPingMs = (long long)photonPlayer->get_RoundTripTime();

        // Platform detection via custom properties set by GT itself
        // Gorilla Tag stores "platform" in custom room properties; 
        // common values: "oculus" (Quest), "steam"
        auto* props = photonPlayer->get_CustomProperties();
        if (props) {
            auto platObj = il2cpp_utils::GetPropertyValue(props,
                il2cpp_utils::newcsstr("platform"));
            if (platObj.has_value()) {
                std::string platStr = to_utf8(csstrtostr(
                    (Il2CppString*)*platObj));
                if (platStr == "oculus" || platStr == "quest")
                    data.platform = Platform::Quest;
                else if (platStr == "steam")
                    data.platform = Platform::Steam;
                else
                    data.platform = Platform::Unknown;
            }
        }
    }

    data.lastFps = s_lastFps;
    data.lastHz  = GetTargetRefreshRate();

    // ── Build display strings ────────────────────────────────────────────────

    // Name line:  [Q] PlayerName
    std::string nameLine =
        PlatformTag(data.platform) + " " +
        Colored(data.username, {1.0f, 1.0f, 1.0f, 1.0f});

    // Stats line: Ping: 42ms  FPS: 90  Hz: 120
    std::string statsLine =
        Colored("Ping: ", {0.8f,0.8f,0.8f,1}) +
        Colored(std::to_string(data.lastPingMs) + "ms", colorForPing(data.lastPingMs)) +
        Colored("  FPS: ", {0.8f,0.8f,0.8f,1}) +
        Colored(std::to_string(data.lastFps), colorForFps(data.lastFps)) +
        Colored("  Hz: ", {0.8f,0.8f,0.8f,1}) +
        Colored(std::to_string(data.lastHz), colorForHz(data.lastHz));

    data.nameText->set_text(il2cpp_utils::newcsstr(nameLine));
    data.statsText->set_text(il2cpp_utils::newcsstr(statsLine));
    data.statsText->set_supportRichText(true);
    data.nameText->set_supportRichText(true);

    // ── Billboard: always face the local camera ──────────────────────────────
    auto* cam = UnityEngine::Camera::get_main();
    if (cam && data.canvasObj) {
        auto camPos = cam->get_transform()->get_position();
        auto tagPos = data.canvasObj->get_transform()->get_position();
        auto dir    = camPos - tagPos;
        dir.y = 0;  // keep upright
        if (dir.sqrMagnitude() > 0.0001f) {
            data.canvasObj->get_transform()->set_rotation(
                UnityEngine::Quaternion::LookRotation(dir));
        }
    }
}

// Hook: Application::Update — track FPS via deltaTime
MAKE_HOOK_MATCH(App_Update, &UnityEngine::MonoBehaviour::Update, void,
                UnityEngine::MonoBehaviour* self)
{
    App_Update(self);
    TickFps(UnityEngine::Time::get_deltaTime());
}

// Hook: VRRig::OnDestroy — clean up when a player leaves
MAKE_HOOK_MATCH(VRRig_OnDestroy, &GorillaTag::VRRig::OnDestroy, void,
                GorillaTag::VRRig* self)
{
    void* key = (void*)self;
    if (nametagMap.count(key)) {
        auto& data = nametagMap[key];
        if (data.canvasObj)
            UnityEngine::Object::Destroy(data.canvasObj);
        nametagMap.erase(key);
    }
    VRRig_OnDestroy(self);
}

// ── Mod entry point ──────────────────────────────────────────────────────────

extern "C" void setup(ModInfo& info) {
    info.id      = "NameTags";
    info.version = "1.0.0";
    modInfo      = info;
    getLogger().info("NameTags mod setup complete.");
}

extern "C" void load() {
    il2cpp_functions::Init();

    getLogger().info("Installing NameTags hooks...");

    INSTALL_HOOK(getLogger(), VRRig_LateUpdate);
    INSTALL_HOOK(getLogger(), VRRig_OnDestroy);
    // Note: App_Update hook targets a generic MonoBehaviour Update;
    // for production you may want a dedicated coroutine instead.
    INSTALL_HOOK(getLogger(), App_Update);

    getLogger().info("NameTags hooks installed successfully.");
}
