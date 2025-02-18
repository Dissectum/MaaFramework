#include "Actuator.h"

#include "Controller/ControllerMgr.h"
#include "Instance/InstanceStatus.h"
#include "Task/CustomAction.h"
#include "Utils/Logger.h"
#include "Vision/Comparator.h"

MAA_TASK_NS_BEGIN

Actuator::Actuator(InstanceInternalAPI* inst) : inst_(inst) {}

bool Actuator::run(const Recognizer::Result& rec_result, const TaskData& task_data)
{
    using namespace MAA_RES_NS::Action;
    LogFunc << VAR(task_data.name);

    wait_freezes(task_data.pre_wait_freezes, rec_result.box);
    sleep(task_data.pre_delay);

    switch (task_data.action_type) {
    case Type::DoNothing:
        break;
    case Type::Click:
        click(std::get<ClickParam>(task_data.action_param), rec_result.box);
        break;
    case Type::Swipe:
        swipe(std::get<SwipeParam>(task_data.action_param), rec_result.box);
        break;
    case Type::Key:
        press_key(std::get<KeyParam>(task_data.action_param));
        break;
    case Type::StartApp:
        start_app(std::get<AppParam>(task_data.action_param));
        break;
    case Type::StopApp:
        stop_app(std::get<AppParam>(task_data.action_param));
        break;
    case Type::Custom:
        custom_action(task_data.name, std::get<CustomParam>(task_data.action_param), rec_result.box, rec_result.detail);
        break;
    case Type::StopTask:
        LogInfo << "Action: StopTask";
        return false;
    default:
        LogError << "Unknown action" << VAR(static_cast<int>(task_data.action_type));
        break;
    }

    wait_freezes(task_data.post_wait_freezes, rec_result.box);
    sleep(task_data.post_delay);

    return true;
}

void Actuator::click(const MAA_RES_NS::Action::ClickParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }

    cv::Rect rect = get_target_rect(param.target, cur_box);

    controller()->click(rect);
}

void Actuator::swipe(const MAA_RES_NS::Action::SwipeParam& param, const cv::Rect& cur_box)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }

    cv::Rect begin = get_target_rect(param.begin, cur_box);
    cv::Rect end = get_target_rect(param.end, cur_box);

    controller()->swipe(begin, end, param.duration);
}

void Actuator::press_key(const MAA_RES_NS::Action::KeyParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    for (const auto& key : param.keys) {
        controller()->press_key(key);
    }
}

void Actuator::wait_freezes(const MAA_RES_NS::WaitFreezesParam& param, const cv::Rect& cur_box)
{
    if (param.time <= std::chrono::milliseconds(0)) {
        return;
    }

    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    LogFunc << "Wait freezes:" << VAR(param.time) << VAR(param.threshold) << VAR(param.method);

    cv::Rect target = get_target_rect(param.target, cur_box);

    Comparator comp;
    comp.set_param({
        .roi = { target },
        .threshold = param.threshold,
        .method = param.method,
    });

    cv::Mat pre_image = controller()->screencap();
    auto pre_time = std::chrono::steady_clock::now();

    while (!need_exit()) {
        cv::Mat cur_image = controller()->screencap();
        auto ret = comp.analyze(pre_image, cur_image);
        if (ret.empty()) {
            pre_image = cur_image;
            pre_time = std::chrono::steady_clock::now();
            continue;
        }

        if (duration_since(pre_time) > param.time) {
            break;
        }
    }
}

void Actuator::start_app(const MAA_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    if (param.package.empty()) {
        controller()->start_app();
    }
    else {
        controller()->start_app(param.package);
    }
}

void Actuator::stop_app(const MAA_RES_NS::Action::AppParam& param)
{
    if (!controller()) {
        LogError << "Controller is null";
        return;
    }
    using namespace MAA_VISION_NS;

    if (param.package.empty()) {
        controller()->stop_app();
    }
    else {
        controller()->stop_app(param.package);
    }
}

void Actuator::custom_action(const std::string& task_name, const MAA_RES_NS::Action::CustomParam& param,
                             const cv::Rect& cur_box, const json::value& cur_rec_detail)
{
    if (!inst_) {
        LogError << "Inst is null";
        return;
    }
    auto action = inst_->custom_action(param.name);
    if (!action) {
        LogError << "Custom task not found" << VAR(param.name);
        return;
    }

    action->run(task_name, param, cur_box, cur_rec_detail);
}

cv::Rect Actuator::get_target_rect(const MAA_RES_NS::Action::Target target, const cv::Rect& cur_box)
{
    using namespace MAA_RES_NS::Action;

    if (!status()) {
        LogError << "Status is null";
        return {};
    }

    cv::Rect raw {};
    switch (target.type) {
    case Target::Type::Self:
        raw = cur_box;
        break;
    case Target::Type::PreTask:
        raw = status()->get_pipeline_rec_box(std::get<std::string>(target.param));
        break;
    case Target::Type::Region:
        raw = std::get<cv::Rect>(target.param);
        break;
    default:
        LogError << "Unknown target" << VAR(static_cast<int>(target.type));
        return {};
    }

    return cv::Rect { raw.x + target.offset.x, raw.y + target.offset.y, raw.width + target.offset.width,
                      raw.height + target.offset.height };
}

void Actuator::sleep(unsigned ms) const
{
    sleep(std::chrono::milliseconds(ms));
}

void Actuator::sleep(std::chrono::milliseconds ms) const
{
    if (need_exit()) {
        return;
    }

    using namespace std::chrono_literals;

    if (ms == 0ms) {
        std::this_thread::yield();
        return;
    }

    auto interval = std::min(ms, 5000ms);

    LogDebug << "ready to sleep" << ms << VAR(interval);

    for (auto sleep_time = interval; sleep_time <= ms && !need_exit(); sleep_time += interval) {
        std::this_thread::sleep_for(interval);
    }
    if (!need_exit()) {
        std::this_thread::sleep_for(ms % interval);
    }

    LogDebug << "end of sleep" << ms << VAR(interval);
}

MAA_TASK_NS_END
