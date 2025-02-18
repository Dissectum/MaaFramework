#include "OCRer.h"

#include <regex>

#include "Utils/ImageIo.h"
#include "Utils/Logger.h"
#include "Utils/Ranges.hpp"
#include "Utils/StringMisc.hpp"

MAA_VISION_NS_BEGIN

std::ostream& operator<<(std::ostream& os, const OCRer::Result& res)
{
    os << VAR_RAW(res.text) << VAR_RAW(res.box) << VAR_RAW(res.score);
    return os;
}

OCRer::ResultsVec OCRer::analyze() const
{
    auto start_time = std::chrono::steady_clock::now();

    ResultsVec results = foreach_rois();

    auto costs = duration_since(start_time);
    LogDebug << name_ << "Raw:" << VAR(results) << VAR(param_.model) << VAR(costs);

    const auto& expected = param_.text;
    postproc_and_filter(results, expected);

    costs = duration_since(start_time);
    LogDebug << name_ << "Proc:" << VAR(results) << VAR(expected) << VAR(param_.model) << VAR(costs);
    return results;
}

OCRer::ResultsVec OCRer::foreach_rois() const
{
    if (!cache_.empty()) {
        return { predict_only_rec(cache_) };
    }

    if (param_.roi.empty()) {
        cv::Rect roi(0, 0, image_.cols, image_.rows);
        return predict(roi);
    }

    ResultsVec results;
    for (const cv::Rect& roi : param_.roi) {
        auto cur = predict(roi);
        results.insert(results.end(), std::make_move_iterator(cur.begin()), std::make_move_iterator(cur.end()));
    }
    return results;
}

OCRer::ResultsVec OCRer::predict(const cv::Rect& roi) const
{
    return param_.only_rec ? ResultsVec { predict_only_rec(roi) } : predict_det_and_rec(roi);
}

OCRer::ResultsVec OCRer::predict_det_and_rec(const cv::Rect& roi) const
{
    if (!ocrer_) {
        LogError << "ocrer_ is null";
        return {};
    }
    auto image_roi = image_with_roi(roi);

    fastdeploy::vision::OCRResult ocr_result;
    bool ret = ocrer_->Predict(image_roi, &ocr_result);
    if (!ret) {
        LogWarn << "inferencer return false" << VAR(ocrer_) << VAR(image_) << VAR(roi) << VAR(image_roi);
        return {};
    }
    if (ocr_result.boxes.size() != ocr_result.text.size() || ocr_result.text.size() != ocr_result.rec_scores.size()) {
        LogError << "wrong ocr_result size" << VAR(ocr_result.boxes) << VAR(ocr_result.boxes.size())
                 << VAR(ocr_result.text) << VAR(ocr_result.text.size()) << VAR(ocr_result.rec_scores)
                 << VAR(ocr_result.rec_scores.size());
        return {};
    }

    ResultsVec results;

    for (size_t i = 0; i != ocr_result.text.size(); ++i) {
        // the raw_box rect like ↓
        // 0 - 1
        // 3 - 2
        const auto& raw_box = ocr_result.boxes.at(i);
        int x_collect[] = { raw_box[0], raw_box[2], raw_box[4], raw_box[6] };
        int y_collect[] = { raw_box[1], raw_box[3], raw_box[5], raw_box[7] };
        auto [left, right] = MAA_RNS::ranges::minmax(x_collect);
        auto [top, bottom] = MAA_RNS::ranges::minmax(y_collect);

        cv::Rect my_box(left + roi.x, top + roi.y, right - left, bottom - top);
        results.emplace_back(
            Result { .text = std::move(ocr_result.text.at(i)), .box = my_box, .score = ocr_result.rec_scores.at(i) });
    }

    draw_result(roi, results);

    return results;
}

OCRer::Result OCRer::predict_only_rec(const cv::Rect& roi) const
{
    if (!recer_) {
        LogError << "resource()->ocr_res().recer() is null";
        return {};
    }
    auto image_roi = image_with_roi(roi);

    std::string rec_text;
    float rec_score = 0;

    bool ret = recer_->Predict(image_roi, &rec_text, &rec_score);
    if (!ret) {
        LogWarn << "recer_ return false" << VAR(recer_) << VAR(image_) << VAR(roi) << VAR(image_roi);
        return {};
    }

    Result result { .text = std::move(rec_text), .box = roi, .score = rec_score };
    draw_result(roi, { result });

    return result;
}

void OCRer::draw_result(const cv::Rect& roi, const ResultsVec& results) const
{
    if (!debug_draw_) {
        return;
    }

    cv::Mat image_draw = draw_roi(roi);

    for (size_t i = 0; i != results.size(); ++i) {
        const cv::Rect& my_box = results.at(i).box;

        const auto color = cv::Scalar(0, 0, 255);
        cv::rectangle(image_draw, my_box, color, 1);
        std::string flag = MAA_FMT::format("{}: [{}, {}, {}, {}]", i, my_box.x, my_box.y, my_box.width, my_box.height);
        cv::putText(image_draw, flag, cv::Point(my_box.x, my_box.y - 5), cv::FONT_HERSHEY_PLAIN, 1.2, color, 1);
    }

    if (save_draw_) {
        save_image(image_draw);
    }
}

void OCRer::postproc_and_filter(ResultsVec& results, const std::vector<std::string>& expected) const
{
    for (auto iter = results.begin(); iter != results.end();) {
        auto& res = *iter;

        postproc_trim_(res);
        postproc_replace_(res);

        if (!filter_by_required(res, expected)) {
            iter = results.erase(iter);
            continue;
        }

        ++iter;
    }
}

void OCRer::postproc_trim_(Result& res) const
{
    string_trim_(res.text);
}

void OCRer::postproc_replace_(Result& res) const
{
    for (const auto& [regex, new_str] : param_.replace) {
        res.text = std::regex_replace(res.text, std::regex(regex), new_str);
    }
}

bool OCRer::filter_by_required(const Result& res, const std::vector<std::string>& expected) const
{
    if (expected.empty()) {
        return true;
    }

    for (const auto& text : expected) {
        if (std::regex_search(res.text, std::regex(text))) {
            return true;
        }
    }

    return false;
}

MAA_VISION_NS_END
