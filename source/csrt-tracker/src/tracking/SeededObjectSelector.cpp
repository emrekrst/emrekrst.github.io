#include "tracking/SeededObjectSelector.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

#include <opencv2/imgproc.hpp>

namespace {

struct Candidate
{
    cv::Rect box;
    double confidence{};
};

cv::Rect clippedWindow(const cv::Point& centre, int radius, const cv::Size& bounds)
{
    const int left = std::max(0, centre.x - radius);
    const int top = std::max(0, centre.y - radius);
    const int right = std::min(bounds.width, centre.x + radius + 1);
    const int bottom = std::min(bounds.height, centre.y + radius + 1);
    return {left, top, right - left, bottom - top};
}

std::optional<Candidate> candidateAtScale(
    const cv::Mat& bgr,
    const cv::Point& click,
    int radius)
{
    const cv::Rect window = clippedWindow(click, radius, bgr.size());
    if (window.width < 9 || window.height < 9) {
        return std::nullopt;
    }

    const cv::Mat crop = bgr(window);
    cv::Scalar mean;
    cv::Scalar deviation;
    cv::meanStdDev(crop, mean, deviation);
    if (deviation[0] + deviation[1] + deviation[2] < 9.0) {
        return std::nullopt;
    }

    cv::Mat lab;
    cv::cvtColor(crop, lab, cv::COLOR_BGR2Lab);
    const cv::Point localClick(click.x - window.x, click.y - window.y);
    const cv::Rect seedRect = clippedWindow(localClick, 2, crop.size());
    const cv::Scalar seed = cv::mean(lab(seedRect));

    cv::Mat similar;
    cv::inRange(
        lab,
        cv::Scalar(seed[0] - 28.0, seed[1] - 22.0, seed[2] - 22.0),
        cv::Scalar(seed[0] + 28.0, seed[1] + 22.0, seed[2] + 22.0),
        similar);

    cv::Mat edges;
    cv::Canny(crop, edges, 35.0, 105.0);
    cv::dilate(edges, edges, cv::Mat(), {-1, -1}, 1);
    similar.setTo(0, edges);
    similar(seedRect).setTo(255);

    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int count = cv::connectedComponentsWithStats(similar, labels, stats, centroids, 8);
    if (count <= 1) {
        return std::nullopt;
    }

    const int label = labels.at<int>(localClick);
    if (label <= 0) {
        return std::nullopt;
    }

    const int area = stats.at<int>(label, cv::CC_STAT_AREA);
    const int cropArea = crop.rows * crop.cols;
    if (area < 10 || area > cropArea * 0.35) {
        return std::nullopt;
    }

    cv::Rect box(
        stats.at<int>(label, cv::CC_STAT_LEFT),
        stats.at<int>(label, cv::CC_STAT_TOP),
        stats.at<int>(label, cv::CC_STAT_WIDTH),
        stats.at<int>(label, cv::CC_STAT_HEIGHT));

    cv::Mat componentMask;
    cv::compare(labels, label, componentMask, cv::CMP_EQ);
    cv::Mat expanded;
    cv::Mat contextGap;
    cv::dilate(componentMask, expanded,
               cv::getStructuringElement(cv::MORPH_ELLIPSE, {33, 33}));
    cv::dilate(componentMask, contextGap,
               cv::getStructuringElement(cv::MORPH_ELLIPSE, {15, 15}));
    cv::Mat ring;
    cv::subtract(expanded, contextGap, ring);
    ring.setTo(0, edges);
    if (cv::countNonZero(ring) < 12) {
        return std::nullopt;
    }

    const cv::Scalar componentMean = cv::mean(lab, componentMask);
    const cv::Scalar ringMean = cv::mean(lab, ring);
    const double l = componentMean[0] - ringMean[0];
    const double a = componentMean[1] - ringMean[1];
    const double b = componentMean[2] - ringMean[2];
    const double colourSeparation = std::sqrt(l * l + a * a + b * b);
    if (colourSeparation < 11.0) {
        return std::nullopt;
    }

    if (box.x == 0 || box.y == 0 || box.br().x >= crop.cols || box.br().y >= crop.rows) {
        return std::nullopt;
    }

    box.x += window.x;
    box.y += window.y;
    constexpr int edgePadding = 3;
    box.x = std::max(0, box.x - edgePadding);
    box.y = std::max(0, box.y - edgePadding);
    box.width = std::min(bgr.cols - box.x, box.width + 2 * edgePadding);
    box.height = std::min(bgr.rows - box.y, box.height + 2 * edgePadding);
    if (!box.contains(click)) {
        return std::nullopt;
    }

    const double compactness = std::clamp(
        static_cast<double>(area) / static_cast<double>(box.area()), 0.0, 1.0);
    if (compactness < 0.25) {
        return std::nullopt;
    }
    const double sizeScore = std::clamp(
        1.0 - static_cast<double>(area) / static_cast<double>(cropArea * 0.35), 0.0, 1.0);
    const double separationScore = std::clamp((colourSeparation - 11.0) / 40.0, 0.0, 1.0);
    return Candidate{box,
                     0.40 + 0.30 * separationScore + 0.20 * compactness + 0.10 * sizeScore};
}

} // namespace

std::optional<SelectionResult> SeededObjectSelector::select(
    const cv::Mat& bgrFrame,
    const FramePoint& click) const
{
    if (bgrFrame.empty() || bgrFrame.type() != CV_8UC3) {
        return std::nullopt;
    }

    const cv::Point seed{
        static_cast<int>(std::lround(click.x)),
        static_cast<int>(std::lround(click.y))};
    if (seed.x < 0 || seed.y < 0 || seed.x >= bgrFrame.cols || seed.y >= bgrFrame.rows) {
        return std::nullopt;
    }

    std::optional<Candidate> best;
    for (const int radius : std::array{24, 48, 96, 160}) {
        const auto candidate = candidateAtScale(bgrFrame, seed, radius);
        if (candidate && (!best || candidate->confidence > best->confidence)) {
            best = candidate;
        }
    }

    if (!best) {
        return std::nullopt;
    }

    return SelectionResult{
        {static_cast<double>(best->box.x),
         static_cast<double>(best->box.y),
         static_cast<double>(best->box.width),
         static_cast<double>(best->box.height)},
        best->confidence};
}
