#include "MaaFramework/MaaAPI.h"

#include <meojson/json.hpp>

#include "Buffer/ImageBuffer.hpp"
#include "Buffer/StringBuffer.hpp"
#include "ControlUnit/ControlUnitAPI.h"
#include "Controller/AdbController.h"
#include "Controller/CustomController.h"
#include "Controller/CustomThriftController.h"
#include "Instance/InstanceMgr.h"
#include "Option/GlobalOptionMgr.h"
#include "Resource/ResourceMgr.h"
#include "Utils/Logger.h"
#include "Utils/Platform.h"

#pragma message("MAA_VERSION: " MAA_VERSION)

MaaStringView MaaVersion()
{
    return MAA_VERSION;
}

MaaStringBufferHandle MaaCreateStringBuffer()
{
    return new MAA_NS::StringBuffer;
}

void MaaDestroyStringBuffer(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

MaaBool MaaIsStringEmpty(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaClearString(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaStringView MaaGetString(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->data();
}

MaaSize MaaGetStringSize(MaaStringBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->size();
}

MaaBool MaaSetString(MaaStringBufferHandle handle, MaaStringView str)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(str);
    return true;
}

MaaBool MaaSetStringEx(MaaStringBufferHandle handle, MaaStringView str, MaaSize size)
{
    if (!handle || !str) {
        LogError << "handle is null";
        return false;
    }

    handle->set(std::string(str, size));
    return true;
}

MaaImageBufferHandle MaaCreateImageBuffer()
{
    return new MAA_NS::ImageBuffer;
}

void MaaDestroyImageBuffer(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return;
    }

    delete handle;
}

void* MaaGetImageRawData(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->raw_data();
}

int32_t MaaGetImageWidth(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->width();
}

int32_t MaaGetImageHeight(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->height();
}

int32_t MaaGetImageType(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->type();
}

MaaBool MaaIsImageEmpty(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return true; // means empty
    }

    return handle->empty();
}

MaaBool MaaClearImage(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return false;
    }

    handle->clear();
    return true;
}

MaaBool MaaSetImageRawData(MaaImageBufferHandle handle, MaaImageRawData data, int32_t width, int32_t height,
                           int32_t type)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img(height, width, type, data);
    if (img.empty()) {
        LogError << "img is empty" << VAR_VOIDP(data) << VAR(width) << VAR(height) << VAR(type);
        return false;
    }

    handle->set(img);
    return true;
}

uint8_t* MaaGetImageEncoded(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return handle->encoded();
}

MaaSize MaaGetImageEncodedSize(MaaImageBufferHandle handle)
{
    if (!handle) {
        LogError << "handle is null";
        return 0;
    }

    return handle->encoded_size();
}

MaaBool MaaSetImageEncoded(MaaImageBufferHandle handle, MaaImageEncodedData data, MaaSize size)
{
    if (!handle || !data) {
        LogError << "handle is null";
        return false;
    }

    cv::Mat img = cv::imdecode({ data, static_cast<int>(size) }, cv::IMREAD_COLOR);
    if (img.empty()) {
        LogError << "img is empty" << VAR_VOIDP(data) << VAR(size);
        return false;
    }

    handle->set(img);
    return true;
}

MaaBool MaaSetGlobalOption(MaaGlobalOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    return MAA_NS::GlobalOptionMgr::get_instance().set_option(key, value, val_size);
}

MaaResourceHandle MaaResourceCreate(MaaResourceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_RES_NS::ResourceMgr(callback, callback_arg);
}

void MaaResourceDestroy(MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(res);

    if (res == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete res;
}

MaaResId MaaResourcePostPath(MaaResourceHandle res, MaaStringView path)
{
    LogFunc << VAR_VOIDP(res) << VAR(path);

    if (!res) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return res->post_path(MAA_NS::path(path));
}

MaaStatus MaaResourceStatus(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->status(id);
}

MaaStatus MaaResourceWait(MaaResourceHandle res, MaaResId id)
{
    // LogFunc << VAR_VOIDP(res) << VAR(id);

    if (!res) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return res->wait(id);
}

MaaBool MaaResourceLoaded(MaaResourceHandle res)
{
    // LogFunc << VAR_VOIDP(res);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->loaded();
}

MaaBool MaaResourceSetOption(MaaResourceHandle res, MaaResOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(res) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!res) {
        LogError << "handle is null";
        return false;
    }

    return res->set_option(key, value, val_size);
}

MaaBool MaaResourceGetHash(MaaResourceHandle res, MaaStringBufferHandle buffer)
{
    if (!res || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto hash = res->get_hash();
    if (hash.empty()) {
        LogError << "hash is empty";
        return false;
    }

    buffer->set(std::move(hash));
    return true;
}

MaaControllerHandle MaaAdbControllerCreate(MaaStringView adb_path, MaaStringView address, MaaAdbControllerType type,
                                           MaaStringView config, MaaControllerCallback callback,
                                           MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(adb_path) << VAR(address) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    auto unit_mgr = MAA_CTRL_UNIT_NS::create_adb_controller_unit(adb_path, address, type, config);
    if (!unit_mgr) {
        LogError << "Failed to create controller unit";
        return nullptr;
    }

    return new MAA_CTRL_NS::AdbController(adb_path, address, std::move(unit_mgr), callback, callback_arg);
}

MaaControllerHandle MaaCustomControllerCreate(MaaCustomControllerHandle handle, MaaControllerCallback callback,
                                              MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(handle) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    if (!handle) {
        LogError << "handle is null";
        return nullptr;
    }

    return new MAA_CTRL_NS::CustomController(handle, callback, callback_arg);
}

MaaControllerHandle MaaThriftControllerCreate(MaaStringView param, MaaControllerCallback callback,
                                              MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR(param) << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

#ifdef WITH_THRIFT

    try {
        return new MAA_CTRL_NS::CustomThriftController(param, callback, callback_arg);
    }
    catch (const std::exception& e) {
        LogError << "Failed to create thrift controller: " << e.what();
        return nullptr;
    }

#else

#pragma message("The build without thrift")

    LogError << "The build without thrift";
    return nullptr;

#endif // WITH_THRIFT
}

void MaaControllerDestroy(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (ctrl == nullptr) {
        LogError << "handle is null";
        return;
    }

    ctrl->on_stop();
    delete ctrl;
}

MaaBool MaaControllerSetOption(MaaControllerHandle ctrl, MaaCtrlOption key, MaaOptionValue value,
                               MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->set_option(key, value, val_size);
}

MaaCtrlId MaaControllerPostConnection(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->post_connection();
}

MaaCtrlId MaaControllerPostClick(MaaControllerHandle ctrl, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x) << VAR(y);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_click(x, y);
}

MaaCtrlId MaaControllerPostSwipe(MaaControllerHandle ctrl, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                                 int32_t duration)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_swipe(x1, y1, x2, y2, duration);
}

MaaCtrlId MaaControllerPostPressKey(MaaControllerHandle ctrl, int32_t keycode)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(keycode);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_press_key(keycode);
}

MaaCtrlId MaaControllerPostTouchDown(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_down(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchMove(MaaControllerHandle ctrl, int32_t contact, int32_t x, int32_t y, int32_t pressure)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_move(contact, x, y, pressure);
}

MaaCtrlId MaaControllerPostTouchUp(MaaControllerHandle ctrl, int32_t contact)
{
    LogFunc << VAR_VOIDP(ctrl) << VAR(contact);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_touch_up(contact);
}

MaaCtrlId MaaControllerPostScreencap(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaInvalidId;
    }

    return ctrl->post_screencap();
}

MaaStatus MaaControllerStatus(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->status(id);
}

MaaStatus MaaControllerWait(MaaControllerHandle ctrl, MaaCtrlId id)
{
    // LogFunc << VAR_VOIDP(ctrl) << VAR(id);

    if (!ctrl) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }

    return ctrl->wait(id);
}

MaaBool MaaControllerConnected(MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(ctrl);

    if (!ctrl) {
        LogError << "handle is null";
        return false;
    }

    return ctrl->connected();
}

MaaBool MaaControllerGetImage(MaaControllerHandle ctrl, MaaImageBufferHandle buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto img = ctrl->get_image();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    buffer->set(std::move(img));
    return true;
}

MaaBool MaaControllerGetUUID(MaaControllerHandle ctrl, MaaStringBufferHandle buffer)
{
    if (!ctrl || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto uuid = ctrl->get_uuid();
    if (uuid.empty()) {
        LogError << "uuid is empty";
        return false;
    }

    buffer->set(std::move(uuid));
    return true;
}

MaaInstanceHandle MaaCreate(MaaInstanceCallback callback, MaaCallbackTransparentArg callback_arg)
{
    LogFunc << VAR_VOIDP(callback) << VAR_VOIDP(callback_arg);

    return new MAA_NS::InstanceMgr(callback, callback_arg);
}

void MaaDestroy(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (inst == nullptr) {
        LogError << "handle is null";
        return;
    }

    delete inst;
}

MaaBool MaaSetOption(MaaInstanceHandle inst, MaaInstOption key, MaaOptionValue value, MaaOptionValueSize val_size)
{
    LogFunc << VAR_VOIDP(inst) << VAR(key) << VAR_VOIDP(value) << VAR(val_size);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->set_option(key, value, val_size);
}

MaaBool MaaBindResource(MaaInstanceHandle inst, MaaResourceHandle res)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(res);

    if (!inst || !res) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_resource(res);
}

MaaBool MaaBindController(MaaInstanceHandle inst, MaaControllerHandle ctrl)
{
    LogFunc << VAR_VOIDP(inst) << VAR_VOIDP(ctrl);

    if (!inst || !ctrl) {
        LogError << "handle is null";
        return false;
    }

    return inst->bind_controller(ctrl);
}

MaaBool MaaInited(MaaInstanceHandle inst)
{
    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->inited();
}

MaaBool MaaRegisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name, MaaCustomRecognizerHandle recognizer)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(recognizer);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_recognizer(name, recognizer);
}

MaaBool MaaUnregisterCustomRecognizer(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_recognizer(name);
}

MaaBool MaaClearCustomRecognizer(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_recognizer();
    return true;
}

MaaBool MaaRegisterCustomAction(MaaInstanceHandle inst, MaaStringView name, MaaCustomActionHandle action)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name) << VAR_VOIDP(action);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->register_custom_action(name, action);
}

MaaBool MaaUnregisterCustomAction(MaaInstanceHandle inst, MaaStringView name)
{
    LogFunc << VAR_VOIDP(inst) << VAR(name);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    return inst->unregister_custom_action(name);
}

MaaBool MaaClearCustomAction(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->clear_custom_action();
    return true;
}

MaaTaskId MaaPostTask(MaaInstanceHandle inst, MaaStringView entry, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(entry) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return MaaInvalidId;
    }
    return inst->post_task(entry, param);
}

MaaBool MaaSetTaskParam(MaaInstanceHandle inst, MaaTaskId id, MaaStringView param)
{
    LogFunc << VAR_VOIDP(inst) << VAR(id) << VAR(param);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->set_task_param(id, param);
}

MaaStatus MaaTaskStatus(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_status(id);
}

MaaStatus MaaWaitTask(MaaInstanceHandle inst, MaaTaskId id)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);

    if (!inst) {
        LogError << "handle is null";
        return MaaStatus_Invalid;
    }
    return inst->task_wait(id);
}

MaaBool MaaTaskAllFinished(MaaInstanceHandle inst)
{
    // LogFunc << VAR_VOIDP(inst) << VAR(id);
    if (!inst) {
        LogError << "handle is null";
        return false;
    }
    return inst->task_all_finished();
}

MaaBool MaaStop(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return false;
    }

    inst->stop();
    return true;
}

MaaResourceHandle MaaGetResource(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }

    return inst->resource();
}

MaaControllerHandle MaaGetController(MaaInstanceHandle inst)
{
    LogFunc << VAR_VOIDP(inst);

    if (!inst) {
        LogError << "handle is null";
        return nullptr;
    }
    return inst->controller();
}

MaaBool MaaSyncContextRunTask(MaaSyncContextHandle sync_context, MaaStringView task, MaaStringView param)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(task) << VAR(param);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->run_task(task, param);
}

MaaBool MaaSyncContextRunRecognizer(MaaSyncContextHandle sync_context, MaaImageBufferHandle image, MaaStringView task,
                                    MaaStringView task_param, MaaRectHandle out_box, MaaStringBufferHandle detail_buff)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(image) << VAR(task) << VAR(task_param) << VAR(out_box)
            << VAR(detail_buff);

    if (!sync_context || !image) {
        LogError << "handle is null";
        return false;
    }

    cv::Rect cvbox {};
    std::string detail;

    bool ret = sync_context->run_recognizer(image->get(), task, task_param, cvbox, detail);

    if (out_box) {
        out_box->x = cvbox.x;
        out_box->y = cvbox.y;
        out_box->width = cvbox.width;
        out_box->height = cvbox.height;
    }
    if (detail_buff) {
        detail_buff->set(std::move(detail));
    }

    return ret;
}

MaaBool MaaSyncContextRunAction(MaaSyncContextHandle sync_context, MaaStringView task, MaaStringView task_param,
                                MaaRectHandle cur_box, MaaStringView cur_rec_detail)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(task) << VAR(task_param) << VAR(cur_box) << VAR(cur_rec_detail);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    cv::Rect cvbox {};
    if (cur_box) {
        cvbox.x = cur_box->x;
        cvbox.y = cur_box->y;
        cvbox.width = cur_box->width;
        cvbox.height = cur_box->height;
    }

    bool ret = sync_context->run_action(task, task_param, cvbox, cur_rec_detail);
    return ret;
}

MaaBool MaaSyncContextClick(MaaSyncContextHandle sync_context, int32_t x, int32_t y)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(x) << VAR(y);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->click(x, y);
}

MaaBool MaaSyncContextSwipe(MaaSyncContextHandle sync_context, int32_t x1, int32_t y1, int32_t x2, int32_t y2,
                            int32_t duration)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(x1) << VAR(y1) << VAR(x2) << VAR(y2) << VAR(duration);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->swipe(x1, y1, x2, y2, duration);
}

MaaBool MaaSyncContextPressKey(MaaSyncContextHandle sync_context, int32_t keycode)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(keycode);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->press_key(keycode);
}

MaaBool MaaSyncContextTouchDown(MaaSyncContextHandle sync_context, int32_t contact, int32_t x, int32_t y,
                                int32_t pressure)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->touch_down(contact, x, y, pressure);
}

MaaBool MaaSyncContextTouchMove(MaaSyncContextHandle sync_context, int32_t contact, int32_t x, int32_t y,
                                int32_t pressure)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(contact) << VAR(x) << VAR(y) << VAR(pressure);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->touch_move(contact, x, y, pressure);
}

MaaBool MaaSyncContextTouchUp(MaaSyncContextHandle sync_context, int32_t contact)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(contact);

    if (!sync_context) {
        LogError << "handle is null";
        return false;
    }

    return sync_context->touch_up(contact);
}

MaaBool MaaSyncContextScreencap(MaaSyncContextHandle sync_context, MaaImageBufferHandle buffer)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(buffer);

    if (!sync_context || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto img = sync_context->screencap();
    if (img.empty()) {
        LogError << "image is empty";
        return false;
    }

    buffer->set(std::move(img));
    return true;
}

MaaBool MaaSyncContextGetTaskResult(MaaSyncContextHandle sync_context, MaaStringView task, MaaStringBufferHandle buffer)
{
    LogFunc << VAR_VOIDP(sync_context) << VAR(task) << VAR(buffer);

    if (!sync_context || !buffer) {
        LogError << "handle is null";
        return false;
    }

    auto res = sync_context->task_result(task);
    if (res.empty()) {
        LogError << "res is empty";
        return false;
    }

    buffer->set(std::move(res));
    return true;
}
