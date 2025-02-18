#include "Classifier.h"

#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

#include "Utils/NoWarningCV.hpp"
#include "Utils/Ranges.hpp"
#include "VisionUtils.hpp"

MAA_VISION_NS_BEGIN

Classifier::ResultsVec Classifier::analyze() const
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

Classifier::ResultsVec Classifier::foreach_rois() const
{
    if (!cache_.empty()) {
        return { classify(cache_) };
    }

    if (param_.roi.empty()) {
        return { classify(cv::Rect(0, 0, image_.cols, image_.rows)) };
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        Result res = classify(roi);
        results.emplace_back(std::move(res));
    }

    return results;
}

Classifier::Result Classifier::classify(const cv::Rect& roi) const
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

    std::vector<float> output;
    output.resize(param_.cls_size);
    std::array<int64_t, 2> output_shape { kBatchSize, static_cast<int64_t>(param_.cls_size) };
    Ort::Value output_tensor = Ort::Value::CreateTensor<float>(memory_info, output.data(), output.size(),
                                                               output_shape.data(), output_shape.size());

    Ort::AllocatorWithDefaultOptions allocator;
    const std::string in_0 = session_->GetInputNameAllocated(0, allocator).get();
    const std::string out_0 = session_->GetOutputNameAllocated(0, allocator).get();
    const std::vector input_names { in_0.c_str() };
    const std::vector output_names { out_0.c_str() };

    Ort::RunOptions run_options;
    session_->Run(run_options, input_names.data(), &input_tensor, 1, output_names.data(), &output_tensor, 1);

    Result result;
    result.raw = std::move(output);
    result.probs = softmax(result.raw);
    result.cls_index = std::max_element(result.probs.begin(), result.probs.end()) - result.probs.begin();
    result.score = result.probs[result.cls_index];
    result.label = param_.labels[result.cls_index];
    result.box = roi;

    draw_result(result);

    return result;
}

void Classifier::draw_result(const Result& res) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(res.box);
    cv::Point pt(res.box.x + res.box.width + 5, res.box.y);

    for (size_t i = 0; i != param_.cls_size; ++i) {
        const auto color = i == res.cls_index ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255);
        std::string text =
            MAA_FMT::format("{} {}: prob {:.3f}, raw {:.3f}", i, param_.labels[i], res.probs[i], res.raw[i]);
        cv::putText(image_draw, text, pt, cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
        pt.y += 10;
    }

    if (save_draw_) {
        save_image(image_draw);
    }
}

void Classifier::filter(ResultsVec& results, const std::vector<size_t>& expected) const
{
    if (expected.empty()) {
        return;
    }

    auto it = std::remove_if(results.begin(), results.end(), [&](const Result& res) {
        return std::find(expected.begin(), expected.end(), res.cls_index) == expected.end();
    });
    results.erase(it, results.end());
}

MAA_VISION_NS_END
