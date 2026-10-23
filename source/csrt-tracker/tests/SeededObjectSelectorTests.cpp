#include <QtTest>

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include "tracking/SeededObjectSelector.hpp"

class SeededObjectSelectorTests final : public QObject
{
    Q_OBJECT
private slots:
    void selectsTightObjectContainingClick();
    void rejectsFlatBackgroundClick();
    void rejectsEdgeEnclosedRoadPatch();
    void rejectsGeneratedRoadFixtureClick();
    void selectsGeneratedCarFixtureClick();
};

void SeededObjectSelectorTests::selectsTightObjectContainingClick()
{
    cv::Mat image(180, 320, CV_8UC3, cv::Scalar(170, 170, 170));
    const cv::Rect truth(132, 76, 28, 16);
    cv::rectangle(image, truth, cv::Scalar(30, 70, 210), cv::FILLED);
    cv::rectangle(image, truth, cv::Scalar(10, 20, 40), 1);

    SeededObjectSelector selector;
    const auto result = selector.select(image, {146, 84});

    QVERIFY(result.has_value());
    QVERIFY(result->confidence >= 0.50);
    QVERIFY(intersectionOverUnion(result->box, {132, 76, 28, 16}) >= 0.55);
}

void SeededObjectSelectorTests::rejectsFlatBackgroundClick()
{
    cv::Mat sky(180, 320, CV_8UC3, cv::Scalar(210, 190, 160));
    SeededObjectSelector selector;
    QVERIFY(!selector.select(sky, {160, 90}).has_value());
}

void SeededObjectSelectorTests::rejectsEdgeEnclosedRoadPatch()
{
    cv::Mat road(240, 400, CV_8UC3, cv::Scalar(118, 122, 126));
    cv::RNG noise(0x70ad);
    cv::Mat variation(road.size(), road.type());
    noise.fill(variation, cv::RNG::NORMAL, cv::Scalar::all(0), cv::Scalar::all(4));
    cv::add(road, variation, road);

    // Painted seams and shadows can enclose a same-colour piece of road.
    // It has edges, but no distinct object appearance inside the boundary.
    cv::rectangle(road, cv::Rect(140, 70, 120, 92), cv::Scalar(76, 79, 82), 3);

    SeededObjectSelector selector;
    QVERIFY(!selector.select(road, {200, 116}).has_value());
}

void SeededObjectSelectorTests::rejectsGeneratedRoadFixtureClick()
{
    const QString fixture = QFINDTESTDATA("fixtures/road-click.png");
    QVERIFY(!fixture.isEmpty());
    const cv::Mat road = cv::imread(fixture.toStdString(), cv::IMREAD_COLOR);
    QVERIFY(!road.empty());

    SeededObjectSelector selector;
    QVERIFY(!selector.select(road, {160, 160}).has_value());
}

void SeededObjectSelectorTests::selectsGeneratedCarFixtureClick()
{
    const QString fixture = QFINDTESTDATA("fixtures/car-click.png");
    QVERIFY(!fixture.isEmpty());
    const cv::Mat frame = cv::imread(fixture.toStdString(), cv::IMREAD_COLOR);
    QVERIFY(!frame.empty());

    SeededObjectSelector selector;
    const auto result = selector.select(frame, {160, 160});
    QVERIFY(result.has_value());
    QVERIFY(result->box.x <= 160 && result->box.x + result->box.width > 160);
    QVERIFY(result->box.y <= 160 && result->box.y + result->box.height > 160);
    QVERIFY(result->box.width < 140 && result->box.height < 100);
}

QTEST_MAIN(SeededObjectSelectorTests)
#include "SeededObjectSelectorTests.moc"
