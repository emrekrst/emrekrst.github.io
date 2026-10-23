cmake_minimum_required(VERSION 3.24)

project(CameraTracker VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets Test)
find_package(OpenCV 4 REQUIRED COMPONENTS core imgproc imgcodecs tracking)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0>=1.20
    gstreamer-app-1.0>=1.20
    gstreamer-video-1.0>=1.20
)

include(CTest)

add_custom_target(script_workflow_tests
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/ScriptWorkflowTests.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
)
add_test(NAME script_workflow
    COMMAND bash "${CMAKE_SOURCE_DIR}/tests/ScriptWorkflowTests.sh")

add_executable(camera_config_tests
    tests/CameraConfigTests.cpp
    src/config/CameraConfig.cpp
)
target_include_directories(camera_config_tests PRIVATE src)
target_link_libraries(camera_config_tests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME camera_config COMMAND camera_config_tests)

add_executable(runtime_telemetry_tests
    tests/RuntimeTelemetryTests.cpp
    src/metrics/RuntimeTelemetry.cpp
)
target_include_directories(runtime_telemetry_tests PRIVATE src)
target_link_libraries(runtime_telemetry_tests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME runtime_telemetry COMMAND runtime_telemetry_tests)

add_executable(tracking_frame_tests
    tests/TrackingFrameTests.cpp
    src/tracking/TrackingFrame.cpp
)
target_include_directories(tracking_frame_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tracking_frame_tests PRIVATE
    Qt6::Core Qt6::Test PkgConfig::GSTREAMER ${OpenCV_LIBS}
)
add_test(NAME tracking_frame COMMAND tracking_frame_tests)

add_executable(seeded_object_selector_tests
    tests/SeededObjectSelectorTests.cpp
    src/tracking/SeededObjectSelector.cpp
)
target_include_directories(seeded_object_selector_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(seeded_object_selector_tests PRIVATE
    Qt6::Core Qt6::Test ${OpenCV_LIBS}
)
add_test(NAME seeded_object_selector COMMAND seeded_object_selector_tests)

add_executable(csrt_tracker_tests
    tests/CsrtTrackerTests.cpp
    src/tracking/CsrtTracker.cpp
    src/tracking/CsrtTrackingEngine.cpp
    src/tracking/SeededObjectSelector.cpp
)
target_include_directories(csrt_tracker_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(csrt_tracker_tests PRIVATE Qt6::Core Qt6::Test ${OpenCV_LIBS})
add_test(NAME csrt_tracker COMMAND csrt_tracker_tests)

add_executable(tracking_controller_tests
    tests/TrackingControllerTests.cpp
    src/tracking/TrackingController.cpp
    src/tracking/TrackingFrame.cpp
)
target_include_directories(tracking_controller_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(tracking_controller_tests PRIVATE
    Qt6::Core Qt6::Test PkgConfig::GSTREAMER ${OpenCV_LIBS}
)
add_test(NAME tracking_controller COMMAND tracking_controller_tests)

add_executable(video_geometry_tests
    tests/VideoGeometryTests.cpp
    src/ui/VideoGeometry.cpp
)
target_include_directories(video_geometry_tests PRIVATE src)
target_link_libraries(video_geometry_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test)
add_test(NAME video_geometry COMMAND video_geometry_tests)
set_tests_properties(video_geometry PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(placeholder_tracker_tests
    tests/PlaceholderTrackerTests.cpp
    src/tracking/PlaceholderTracker.cpp
)
target_include_directories(placeholder_tracker_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(placeholder_tracker_tests PRIVATE Qt6::Core Qt6::Test ${OpenCV_LIBS})
add_test(NAME placeholder_tracker COMMAND placeholder_tracker_tests)

add_executable(frame_mailbox_tests
    tests/FrameMailboxTests.cpp
    src/tracking/FrameMailbox.cpp
)
target_include_directories(frame_mailbox_tests PRIVATE src)
target_link_libraries(frame_mailbox_tests PRIVATE Qt6::Core Qt6::Test PkgConfig::GSTREAMER)
add_test(NAME frame_mailbox COMMAND frame_mailbox_tests)
set_tests_properties(frame_mailbox PROPERTIES
    ENVIRONMENT "GST_PLUGIN_SYSTEM_PATH_1_0=/nonexistent;GST_PLUGIN_PATH_1_0=/nonexistent"
)

add_executable(camera_pipeline_tests
    tests/CameraPipelineTests.cpp
    src/pipeline/AbstractPipeline.cpp
    src/pipeline/CameraPipeline.cpp
    src/tracking/FrameMailbox.cpp
)
target_include_directories(camera_pipeline_tests PRIVATE src)
target_link_libraries(camera_pipeline_tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Test PkgConfig::GSTREAMER)
add_test(NAME camera_pipeline COMMAND camera_pipeline_tests)
set_tests_properties(camera_pipeline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(file_pipeline_tests
    tests/FilePipelineTests.cpp
    src/pipeline/AbstractPipeline.cpp
    src/pipeline/FilePipeline.cpp
    src/tracking/FrameMailbox.cpp
)
target_include_directories(file_pipeline_tests PRIVATE src)
target_link_libraries(file_pipeline_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Test PkgConfig::GSTREAMER)
add_test(NAME file_pipeline COMMAND file_pipeline_tests)
set_tests_properties(file_pipeline PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(target_overlay_tests
    tests/TargetOverlayTests.cpp
    src/ui/TargetOverlay.cpp
    src/ui/VideoGeometry.cpp
)
target_include_directories(target_overlay_tests PRIVATE src)
target_link_libraries(target_overlay_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME target_overlay COMMAND target_overlay_tests)
set_tests_properties(target_overlay PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(video_canvas_tests
    tests/VideoCanvasTests.cpp
    src/ui/VideoCanvas.cpp
    src/ui/VideoGeometry.cpp
)
target_include_directories(video_canvas_tests PRIVATE src)
target_link_libraries(video_canvas_tests PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test)
add_test(NAME video_canvas COMMAND video_canvas_tests)
set_tests_properties(video_canvas PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

add_executable(main_window_tests
    tests/MainWindowTests.cpp
    src/metrics/RuntimeTelemetry.cpp
    src/ui/MainWindow.cpp
    src/ui/VideoCanvas.cpp
    src/ui/VideoGeometry.cpp
    src/pipeline/AbstractPipeline.cpp
    src/pipeline/CameraPipeline.cpp
    src/pipeline/FilePipeline.cpp
    src/tracking/TrackingFrame.cpp
    src/tracking/SeededObjectSelector.cpp
    src/tracking/CsrtTracker.cpp
    src/tracking/CsrtTrackingEngine.cpp
    src/tracking/TrackingController.cpp
)
target_compile_definitions(main_window_tests PRIVATE CSRT_SAMPLE_PATH="${CMAKE_SOURCE_DIR}/samples/moving-target.avi")
target_include_directories(main_window_tests PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(main_window_tests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Test
    PkgConfig::GSTREAMER ${OpenCV_LIBS}
)
add_test(NAME main_window COMMAND main_window_tests)
set_tests_properties(main_window PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

set(CAMERA_TRACKER_SOURCES
    src/main.cpp
    src/config/CameraConfig.cpp
    src/metrics/RuntimeTelemetry.cpp
    src/ui/MainWindow.cpp
    src/ui/VideoCanvas.cpp
    src/ui/VideoGeometry.cpp
    src/pipeline/AbstractPipeline.cpp
    src/pipeline/CameraPipeline.cpp
    src/pipeline/FilePipeline.cpp
    src/tracking/TrackingFrame.cpp
    src/tracking/SeededObjectSelector.cpp
    src/tracking/CsrtTracker.cpp
    src/tracking/CsrtTrackingEngine.cpp
    src/tracking/TrackingController.cpp
)

if(APPLE)
    add_executable(CameraTracker MACOSX_BUNDLE ${CAMERA_TRACKER_SOURCES})
    set_target_properties(CameraTracker PROPERTIES
        MACOSX_BUNDLE_BUNDLE_NAME "Camera Tracker"
        MACOSX_BUNDLE_GUI_IDENTIFIER "dev.cameratracker.prototype"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1.0"
        MACOSX_BUNDLE_BUNDLE_VERSION "1"
    )
else()
    add_executable(CameraTracker ${CAMERA_TRACKER_SOURCES})
endif()

target_include_directories(CameraTracker PRIVATE src ${OpenCV_INCLUDE_DIRS})
target_link_libraries(CameraTracker PRIVATE
    Qt6::Core Qt6::Gui Qt6::Widgets
    PkgConfig::GSTREAMER ${OpenCV_LIBS}
)
