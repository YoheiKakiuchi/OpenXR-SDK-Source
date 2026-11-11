#include <openxr/openxr.h>

#include <array>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {
volatile std::sig_atomic_t stopRequested = 0;
void HandleSignal(int) { stopRequested = 1; }

void Check(XrResult result, const char* operation) {
    if (XR_FAILED(result)) throw std::runtime_error(std::string(operation) + " failed: " + std::to_string(result));
}

class HeadlessTracker {
public:
    ~HeadlessTracker() {
        for (XrSpace space : handSpaces) {
            if (space != XR_NULL_HANDLE) xrDestroySpace(space);
        }
        if (headSpace != XR_NULL_HANDLE) xrDestroySpace(headSpace);
        if (localSpace != XR_NULL_HANDLE) xrDestroySpace(localSpace);
        if (session != XR_NULL_HANDLE) xrDestroySession(session);
        if (actionSet != XR_NULL_HANDLE) xrDestroyActionSet(actionSet);
        if (instance != XR_NULL_HANDLE) xrDestroyInstance(instance);
    }

    void Initialize() {
        uint32_t extensionCount = 0;
        Check(xrEnumerateInstanceExtensionProperties(nullptr, 0, &extensionCount, nullptr), "xrEnumerateInstanceExtensionProperties");
        std::vector<XrExtensionProperties> extensions(extensionCount, {XR_TYPE_EXTENSION_PROPERTIES});
        Check(xrEnumerateInstanceExtensionProperties(nullptr, extensionCount, &extensionCount, extensions.data()),
              "xrEnumerateInstanceExtensionProperties");
        bool supported = false;
        for (const auto& extension : extensions) {
            if (std::strcmp(extension.extensionName, XR_MND_HEADLESS_EXTENSION_NAME) == 0) supported = true;
        }
        if (!supported) throw std::runtime_error("Active OpenXR runtime does not support XR_MND_headless");

        const char* enabledExtension = XR_MND_HEADLESS_EXTENSION_NAME;
        XrInstanceCreateInfo instanceInfo{XR_TYPE_INSTANCE_CREATE_INFO};
        std::strcpy(instanceInfo.applicationInfo.applicationName, "hello_xr_headless");
        instanceInfo.applicationInfo.apiVersion = XR_API_VERSION_1_0;
        instanceInfo.enabledExtensionCount = 1;
        instanceInfo.enabledExtensionNames = &enabledExtension;
        Check(xrCreateInstance(&instanceInfo, &instance), "xrCreateInstance");

        XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};
        systemInfo.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
        Check(xrGetSystem(instance, &systemInfo, &systemId), "xrGetSystem");

        uint32_t blendCount = 0;
        Check(xrEnumerateEnvironmentBlendModes(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, 0, &blendCount, nullptr),
              "xrEnumerateEnvironmentBlendModes");
        if (blendCount == 0) throw std::runtime_error("No environment blend mode available");
        std::vector<XrEnvironmentBlendMode> blendModes(blendCount);
        Check(xrEnumerateEnvironmentBlendModes(instance, systemId, XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO, blendCount,
                                               &blendCount, blendModes.data()), "xrEnumerateEnvironmentBlendModes");
        blendMode = blendModes[0];

        XrSessionCreateInfo sessionInfo{XR_TYPE_SESSION_CREATE_INFO};
        sessionInfo.systemId = systemId;
        Check(xrCreateSession(instance, &sessionInfo, &session), "xrCreateSession");
        XrReferenceSpaceCreateInfo spaceInfo{XR_TYPE_REFERENCE_SPACE_CREATE_INFO};
        spaceInfo.poseInReferenceSpace.orientation.w = 1.0f;
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
        Check(xrCreateReferenceSpace(session, &spaceInfo, &localSpace), "xrCreateReferenceSpace(local)");
        spaceInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
        Check(xrCreateReferenceSpace(session, &spaceInfo, &headSpace), "xrCreateReferenceSpace(view)");

        XrActionSetCreateInfo setInfo{XR_TYPE_ACTION_SET_CREATE_INFO};
        std::strcpy(setInfo.actionSetName, "tracking");
        std::strcpy(setInfo.localizedActionSetName, "Tracking");
        Check(xrCreateActionSet(instance, &setInfo, &actionSet), "xrCreateActionSet");
        handPaths = {Path("/user/hand/left"), Path("/user/hand/right")};
        poseAction = CreateAction("hand_pose", "Hand pose", XR_ACTION_TYPE_POSE_INPUT);
        buttonAction = CreateAction("button", "Button", XR_ACTION_TYPE_BOOLEAN_INPUT);
        SuggestBindings("/interaction_profiles/khr/simple_controller", "/user/hand/left/input/select/click",
                        "/user/hand/right/input/select/click");
        SuggestBindings("/interaction_profiles/oculus/touch_controller", "/user/hand/left/input/x/click",
                        "/user/hand/right/input/a/click");
        SuggestBindings("/interaction_profiles/htc/vive_controller", "/user/hand/left/input/menu/click",
                        "/user/hand/right/input/menu/click");
        SuggestBindings("/interaction_profiles/valve/index_controller", "/user/hand/left/input/a/click",
                        "/user/hand/right/input/a/click");
        SuggestBindings("/interaction_profiles/microsoft/motion_controller", "/user/hand/left/input/menu/click",
                        "/user/hand/right/input/menu/click");
        for (size_t hand = 0; hand < handSpaces.size(); ++hand) {
            XrActionSpaceCreateInfo actionInfo{XR_TYPE_ACTION_SPACE_CREATE_INFO};
            actionInfo.action = poseAction;
            actionInfo.subactionPath = handPaths[hand];
            actionInfo.poseInActionSpace.orientation.w = 1.0f;
            Check(xrCreateActionSpace(session, &actionInfo, &handSpaces[hand]), "xrCreateActionSpace");
        }
        XrSessionActionSetsAttachInfo attachInfo{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
        attachInfo.countActionSets = 1;
        attachInfo.actionSets = &actionSet;
        Check(xrAttachSessionActionSets(session, &attachInfo), "xrAttachSessionActionSets");
    }

    void Run() {
        while (!stopRequested && !exiting) {
            PollEvents();
            if (exiting || stopRequested) break;
            if (running) Frame();
            else std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

private:
    XrPath Path(const char* name) {
        XrPath path = XR_NULL_PATH;
        Check(xrStringToPath(instance, name, &path), "xrStringToPath");
        return path;
    }
    XrAction CreateAction(const char* name, const char* label, XrActionType type) {
        XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
        std::strcpy(info.actionName, name);
        std::strcpy(info.localizedActionName, label);
        info.actionType = type;
        info.countSubactionPaths = static_cast<uint32_t>(handPaths.size());
        info.subactionPaths = handPaths.data();
        XrAction action = XR_NULL_HANDLE;
        Check(xrCreateAction(actionSet, &info, &action), "xrCreateAction");
        return action;
    }
    void SuggestBindings(const char* profile, const char* leftButton, const char* rightButton) {
        const std::array<XrActionSuggestedBinding, 4> bindings{{
            {poseAction, Path("/user/hand/left/input/grip/pose")},
            {poseAction, Path("/user/hand/right/input/grip/pose")},
            {buttonAction, Path(leftButton)}, {buttonAction, Path(rightButton)},
        }};
        XrInteractionProfileSuggestedBinding info{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
        info.interactionProfile = Path(profile);
        info.countSuggestedBindings = static_cast<uint32_t>(bindings.size());
        info.suggestedBindings = bindings.data();
        Check(xrSuggestInteractionProfileBindings(instance, &info), "xrSuggestInteractionProfileBindings");
    }
    void PollEvents() {
        XrEventDataBuffer event{XR_TYPE_EVENT_DATA_BUFFER};
        XrResult result;
        while ((result = xrPollEvent(instance, &event)) == XR_SUCCESS) {
            if (event.type == XR_TYPE_EVENT_DATA_INSTANCE_LOSS_PENDING) exiting = true;
            else if (event.type == XR_TYPE_EVENT_DATA_SESSION_STATE_CHANGED) {
                const auto& change = *reinterpret_cast<const XrEventDataSessionStateChanged*>(&event);
                if (change.session == session && change.state == XR_SESSION_STATE_READY) {
                    XrSessionBeginInfo info{XR_TYPE_SESSION_BEGIN_INFO};
                    info.primaryViewConfigurationType = XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO;
                    Check(xrBeginSession(session, &info), "xrBeginSession");
                    running = true;
                } else if (change.session == session && change.state == XR_SESSION_STATE_STOPPING) {
                    running = false;
                    Check(xrEndSession(session), "xrEndSession");
                } else if (change.session == session &&
                           (change.state == XR_SESSION_STATE_EXITING || change.state == XR_SESSION_STATE_LOSS_PENDING)) exiting = true;
            }
            event = {XR_TYPE_EVENT_DATA_BUFFER};
        }
        if (result != XR_EVENT_UNAVAILABLE) Check(result, "xrPollEvent");
    }
    static void PrintPose(const char* name, const XrSpaceLocation& location) {
        std::printf("%s p%s(%.3f,%.3f,%.3f) q%s(%.3f,%.3f,%.3f,%.3f)", name,
                    (location.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) ? "" : "?",
                    location.pose.position.x, location.pose.position.y, location.pose.position.z,
                    (location.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) ? "" : "?",
                    location.pose.orientation.x, location.pose.orientation.y, location.pose.orientation.z, location.pose.orientation.w);
    }
    void Frame() {
        XrFrameWaitInfo waitInfo{XR_TYPE_FRAME_WAIT_INFO};
        XrFrameState frameState{XR_TYPE_FRAME_STATE};
        Check(xrWaitFrame(session, &waitInfo, &frameState), "xrWaitFrame");
        XrFrameBeginInfo beginInfo{XR_TYPE_FRAME_BEGIN_INFO};
        Check(xrBeginFrame(session, &beginInfo), "xrBeginFrame");
        XrActiveActionSet activeSet{actionSet, XR_NULL_PATH};
        XrActionsSyncInfo syncInfo{XR_TYPE_ACTIONS_SYNC_INFO};
        syncInfo.countActiveActionSets = 1;
        syncInfo.activeActionSets = &activeSet;
        Check(xrSyncActions(session, &syncInfo), "xrSyncActions");
        XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
        Check(xrLocateSpace(headSpace, localSpace, frameState.predictedDisplayTime, &location), "xrLocateSpace(head)");
        PrintPose("head", location);
        for (size_t hand = 0; hand < handSpaces.size(); ++hand) {
            location = {XR_TYPE_SPACE_LOCATION};
            Check(xrLocateSpace(handSpaces[hand], localSpace, frameState.predictedDisplayTime, &location), "xrLocateSpace(hand)");
            std::printf(" | ");
            PrintPose(hand == 0 ? "left" : "right", location);
            XrActionStateGetInfo getInfo{XR_TYPE_ACTION_STATE_GET_INFO};
            getInfo.action = buttonAction;
            getInfo.subactionPath = handPaths[hand];
            XrActionStateBoolean state{XR_TYPE_ACTION_STATE_BOOLEAN};
            Check(xrGetActionStateBoolean(session, &getInfo, &state), "xrGetActionStateBoolean");
            std::printf(" button=%s", state.isActive ? (state.currentState ? "1" : "0") : "inactive");
        }
        std::printf("\n");
        std::fflush(stdout);
        XrFrameEndInfo endInfo{XR_TYPE_FRAME_END_INFO};
        endInfo.displayTime = frameState.predictedDisplayTime;
        endInfo.environmentBlendMode = blendMode;
        Check(xrEndFrame(session, &endInfo), "xrEndFrame");
    }
    XrInstance instance{XR_NULL_HANDLE};
    XrSystemId systemId{XR_NULL_SYSTEM_ID};
    XrSession session{XR_NULL_HANDLE};
    XrSpace localSpace{XR_NULL_HANDLE};
    XrSpace headSpace{XR_NULL_HANDLE};
    XrActionSet actionSet{XR_NULL_HANDLE};
    XrAction poseAction{XR_NULL_HANDLE};
    XrAction buttonAction{XR_NULL_HANDLE};
    std::array<XrPath, 2> handPaths{};
    std::array<XrSpace, 2> handSpaces{};
    XrEnvironmentBlendMode blendMode{XR_ENVIRONMENT_BLEND_MODE_OPAQUE};
    bool running{false};
    bool exiting{false};
};
}

int main() {
    std::signal(SIGINT, HandleSignal);
    try {
        HeadlessTracker tracker;
        tracker.Initialize();
        std::puts("Tracking started. Press Ctrl+C to exit.");
        tracker.Run();
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}