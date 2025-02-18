#include "Detector.h"

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Utils/NoWarningCV.hpp"
#include "Utils/Ranges.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Detector::ResultsVec Detector::analyze() const
{
    LogFunc << name_;

    if (!session_) {
        LogError << "OrtSession not loaded";
        return {};
    }
    if (param_.cls_size == 0) {
        LogError << "cls_size == 0";
        return {};
    }
    if (param_.cls_size != param_.labels.size()) {
        LogError << "cls_size != labels.size()" << VAR(param_.cls_size) << VAR(param_.labels.size());
        return {};
    }

    auto start_time = std::chrono::steady_clock::now();
    ResultsVec results = foreach_rois();
    auto costs = duration_since(start_time);
    LogDebug << name_ << "Raw:" << VAR(results) << VAR(costs);

    const auto& expected = param_.expected;
    filter(results, expected);

    costs = duration_since(start_time);
    LogDebug << name_ << "Filter:" << VAR(results) << VAR(expected) << VAR(costs);

    return results;
}

Detector::ResultsVec Detector::foreach_rois() const
{
    if (!cache_.empty()) {
        return { detect(cache_) };
    }

    if (param_.roi.empty()) {
        return { detect(cv::Rect(0, 0, image_.cols, image_.rows)) };
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        ResultsVec res = detect(roi);
        results.insert(results.end(), std::make_move_iterator(res.begin()), std::make_move_iterator(res.end()));
    }

    return results;
}

Detector::ResultsVec Detector::detect(const cv::Rect& roi) const
{
    if (!session_) {
        LogError << "OrtSession not loaded";
        return {};
    }

    cv::Mat image = image_with_roi(roi);
    std::vector<float> input = image_to_tensor(image);

    // TODO: GPU
    auto memory_info = Ort::MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
    constexpr int64_t kBatchSize = 1;
    std::array<int64_t, 4> input_shape { kBatchSize, image.channels(), image.cols, image.rows };

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(memory_info, input.data(), input.size(),
                                                              input_shape.data(), input_shape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_0 = session_->GetInputNameAllocated(0, allocator).get();
    const std::string out_0 = session_->GetOutputNameAllocated(0, allocator).get();
    const std::vector input_names { in_0.c_str() };
    const std::vector output_names { out_0.c_str() };

    Ort::RunOptions run_options;
    auto output_tensor = session_->Run(run_options, input_names.data(), &input_tensor, input_names.size(),
                                       output_names.data(), output_names.size());

    const float* raw_output = output_tensor[0].GetTensorData<float>();
    // output_shape is { 1, 5, 8400 }
    std::vector<int64_t> output_shape = output_tensor[0].GetTensorTypeAndShapeInfo().GetShape();

    // yolov8 的 onnx 输出和前面的 v5, v7 等似乎不太一样，目前网上 yolov8 的 demo 较少，文档也没找到
    // 这里的输出解析是我跟着数据推测的：
    // center_x0, center_x1, ..... center_x8399
    // center_y0, center_y1, ..... center_y8399
    // w0, w1, ..... w8399
    // h0, h1, ..... h8399
    // conf0, conf1, ..... conf8399
    // 如果后面要做多分类，可能得再看下怎么改（我也不知道shape会变成啥样）
    std::vector<std::vector<float>> output(output_shape[1]);
    for (int64_t i = 0; i < output_shape[1]; i++) {
        output[i] = std::vector<float>(raw_output + i * output_shape[2], raw_output + (i + 1) * output_shape[2]);
    }

    ResultsVec raw_results;
    const auto& conf_vec = output.back();
    for (size_t i = 0; i < conf_vec.size(); ++i) {
        float score = conf_vec[i];
        constexpr float kThreshold = 0.3f;
        if (score < kThreshold) {
            continue;
        }

        int center_x = static_cast<int>(output[0][i]);
        int center_y = static_cast<int>(output[1][i]);
        int w = static_cast<int>(output[2][i]);
        int h = static_cast<int>(output[3][i]);

        int x = center_x - w / 2;
        int y = center_y - h / 2;
        cv::Rect box { x, y, w, h };

        Result res;
        // TODO: cls_index 可能不对，随手写的，没测过，需要看下 onnx 输出的 shape
        res.cls_index = static_cast<int>(output[4][i]);
        res.label = param_.labels[res.cls_index];
        res.box = box;
        res.score = score;

        raw_results.emplace_back(std::move(res));
    }

    auto nms_results = NMS(std::move(raw_results));
    draw_result(roi, nms_results);

    return nms_results;
}

void Detector::filter(ResultsVec& results, const std::vector<size_t>& expected) const
{
    if (expected.empty()) {
        return;
    }

    auto it = std::remove_if(results.begin(), results.end(), [&](const Result& res) {
        return std::find(expected.begin(), expected.end(), res.cls_index) == expected.end();
    });
    results.erase(it, results.end());
}

void Detector::draw_result(const cv::Rect& roi, const ResultsVec& results) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);

    for (const Result& res : results) {
        const cv::Rect& my_box = res.box;

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, my_box, color, 1);
        std::string flag = MAA_FMT::format("{} {} {:.3f}: [{}, {}, {}, {}]", res.cls_index, res.label, res.score,
                                           my_box.x, my_box.y, my_box.width, my_box.height);
        cv::putText(image_draw, flag, cv::Point(my_box.x, my_box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    if (save_draw_) {
        save_image(image_draw);
    }
}

MAA_VISION_NS_END
