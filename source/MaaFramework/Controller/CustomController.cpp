#include "CustomController.h"

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "Utils/Logger.h"
#include "Utils/NoWarningCV.hpp"

MAA_CTRL_NS_BEGIN

CustomController::CustomController(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                                   MaaCallbackTransparentArg callback_arg)
    : ControllerMgr(callback, callback_arg), handle_(handle)
{}

std::string CustomController::get_uuid() const
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_uuid);

    if (!handle_ || !handle_->get_uuid) {
        LogError << "handle_ or handle_->get_uuid is nullptr";
        return {};
    }

    StringBuffer buffer;
    MaaBool ret = handle_->get_uuid(&buffer);
    if (!ret) {
        LogError << "failed to get_uuid" << VAR(ret);
        return {};
    }
    return buffer.get();
}

bool CustomController::_connect()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->connect);

    if (!handle_ || !handle_->connect) {
        LogError << "handle_ or handle_->connect is nullptr";
        return false;
    }

    return handle_->connect();
}

std::pair<int, int> CustomController::_get_resolution() const
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_resolution);
    if (!handle_ || !handle_->get_resolution) {
        LogError << "handle_ or handle_->get_resolution is nullptr";
        return {};
    }
    int width = 0, height = 0;
    handle_->get_resolution(&width, &height);
    return { width, height };
}

bool CustomController::_click(ClickParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->click) << VAR(param.x) << VAR(param.y);

    if (!handle_ || !handle_->click) {
        LogError << "handle_ or handle_->click is nullptr";
        return false;
    }

    return handle_->click(param.x, param.y);
}

bool CustomController::_swipe(SwipeParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->swipe) << VAR(param.x1) << VAR(param.x2) << VAR(param.y1)
            << VAR(param.y2) << VAR(param.duration);

    if (!handle_ || !handle_->swipe) {
        LogError << "handle_ or handle_->swipe is nullptr";
        return false;
    }
    return handle_->swipe(param.x1, param.y1, param.x2, param.y2, param.duration);
}

bool CustomController::_touch_down(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_down) << VAR(param.contact) << VAR(param.x)
            << VAR(param.y) << VAR(param.pressure);

    if (!handle_ || !handle_->touch_down) {
        LogError << "handle_ or handle_->touch_down is nullptr";
        return false;
    }

    return handle_->touch_down(param.contact, param.x, param.y, param.pressure);
}

bool CustomController::_touch_move(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_move) << VAR(param.contact) << VAR(param.x)
            << VAR(param.y) << VAR(param.pressure);

    if (!handle_ || !handle_->touch_move) {
        LogError << "handle_ or handle_->touch_move is nullptr";
        return false;
    }

    return handle_->touch_move(param.contact, param.x, param.y, param.pressure);
}

bool CustomController::_touch_up(TouchParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->touch_up) << VAR(param.contact);

    if (!handle_ || !handle_->touch_up) {
        LogError << "handle_ or handle_->touch_up is nullptr";
        return false;
    }

    return handle_->touch_up(param.contact);
}

bool CustomController::_press_key(PressKeyParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->press_key) << VAR(param.keycode);

    if (!handle_ || !handle_->press_key) {
        LogError << "handle_ or handle_->press_key is nullptr";
        return false;
    }

    return handle_->press_key(param.keycode);
}

cv::Mat CustomController::_screencap()
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->get_image);

    if (!handle_ || !handle_->get_image) {
        LogError << "handle_ or handle_->get_image is nullptr";
        return {};
    }

    ImageBuffer buffer;
    MaaBool ret = handle_->get_image(&buffer);
    if (!ret) {
        LogError << "failed to get_image" << VAR(ret);
        return {};
    }

    return buffer.get();
}

bool CustomController::_start_app(AppParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->start_app) << VAR(param.package);

    if (!handle_ || !handle_->start_app) {
        LogError << "handle_ or handle_->start_app is nullptr";
        return false;
    }

    return handle_->start_app(param.package.c_str());
}

bool CustomController::_stop_app(AppParam param)
{
    LogFunc << VAR_VOIDP(handle_) << VAR_VOIDP(handle_->stop_app) << VAR(param.package);

    if (!handle_ || !handle_->stop_app) {
        LogError << "handle_ or handle_->stop_app is nullptr";
        return false;
    }

    return handle_->stop_app(param.package.c_str());
}

MAA_CTRL_NS_END
