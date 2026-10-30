#include "OnnxObjectDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSize>

#ifdef VIDEOANALYSIS_HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace {
constexpr int kDefaultInputSize = 640;
constexpr float kDefaultConfidenceThreshold = 0.45f;
constexpr float kNmsThreshold = 0.45f;

float normalizedThreshold(float threshold)
{
    if (!std::isfinite(threshold)) {
        return kDefaultConfidenceThreshold;
    }
    return std::clamp(threshold, 0.01f, 0.99f);
}

QString eventTypeForClass(int classId, const QString &className)
{
    const QString normalizedName = className.trimmed().toLower();
    if (classId == 0 || normalizedName == QStringLiteral("person")) {
        return QStringLiteral("PersonDetected");
    }

    if (classId == 2 || classId == 3 || classId == 5 || classId == 7
        || normalizedName.contains(QStringLiteral("vehicle"))
        || normalizedName.contains(QStringLiteral("car"))
        || normalizedName.contains(QStringLiteral("bus"))
        || normalizedName.contains(QStringLiteral("truck"))
        || normalizedName.contains(QStringLiteral("motorcycle"))) {
        return QStringLiteral("VehicleDetected");
    }

    return QStringLiteral("ObjectDetected");
}

QString classNameForId(int classId)
{
    static const QVector<QString> kCocoClassNames = {
        QStringLiteral("person"),
        QStringLiteral("bicycle"),
        QStringLiteral("car"),
        QStringLiteral("motorcycle"),
        QStringLiteral("airplane"),
        QStringLiteral("bus"),
        QStringLiteral("train"),
        QStringLiteral("truck"),
        QStringLiteral("boat"),
        QStringLiteral("traffic light"),
        QStringLiteral("fire hydrant"),
        QStringLiteral("stop sign"),
        QStringLiteral("parking meter"),
        QStringLiteral("bench"),
        QStringLiteral("bird"),
        QStringLiteral("cat"),
        QStringLiteral("dog"),
        QStringLiteral("horse"),
        QStringLiteral("sheep"),
        QStringLiteral("cow"),
        QStringLiteral("elephant"),
        QStringLiteral("bear"),
        QStringLiteral("zebra"),
        QStringLiteral("giraffe"),
        QStringLiteral("backpack"),
        QStringLiteral("umbrella"),
        QStringLiteral("handbag"),
        QStringLiteral("tie"),
        QStringLiteral("suitcase"),
        QStringLiteral("frisbee"),
        QStringLiteral("skis"),
        QStringLiteral("snowboard"),
        QStringLiteral("sports ball"),
        QStringLiteral("kite"),
        QStringLiteral("baseball bat"),
        QStringLiteral("baseball glove"),
        QStringLiteral("skateboard"),
        QStringLiteral("surfboard"),
        QStringLiteral("tennis racket"),
        QStringLiteral("bottle"),
        QStringLiteral("wine glass"),
        QStringLiteral("cup"),
        QStringLiteral("fork"),
        QStringLiteral("knife"),
        QStringLiteral("spoon"),
        QStringLiteral("bowl"),
        QStringLiteral("banana"),
        QStringLiteral("apple"),
        QStringLiteral("sandwich"),
        QStringLiteral("orange"),
        QStringLiteral("broccoli"),
        QStringLiteral("carrot"),
        QStringLiteral("hot dog"),
        QStringLiteral("pizza"),
        QStringLiteral("donut"),
        QStringLiteral("cake"),
        QStringLiteral("chair"),
        QStringLiteral("couch"),
        QStringLiteral("potted plant"),
        QStringLiteral("bed"),
        QStringLiteral("dining table"),
        QStringLiteral("toilet"),
        QStringLiteral("tv"),
        QStringLiteral("laptop"),
        QStringLiteral("mouse"),
        QStringLiteral("remote"),
        QStringLiteral("keyboard"),
        QStringLiteral("cell phone"),
        QStringLiteral("microwave"),
        QStringLiteral("oven"),
        QStringLiteral("toaster"),
        QStringLiteral("sink"),
        QStringLiteral("refrigerator"),
        QStringLiteral("book"),
        QStringLiteral("clock"),
        QStringLiteral("vase"),
        QStringLiteral("scissors"),
        QStringLiteral("teddy bear"),
        QStringLiteral("hair drier"),
        QStringLiteral("toothbrush"),
    };

    if (classId >= 0 && classId < kCocoClassNames.size()) {
        return kCocoClassNames.at(classId);
    }
    return QStringLiteral("class_%1").arg(classId);
}

float intersectionOverUnion(const QRect &first, const QRect &second)
{
    const QRect intersection = first.intersected(second);
    if (intersection.isEmpty()) {
        return 0.0f;
    }

    const float intersectionArea = static_cast<float>(intersection.width() * intersection.height());
    const float firstArea = static_cast<float>(first.width() * first.height());
    const float secondArea = static_cast<float>(second.width() * second.height());
    const float unionArea = firstArea + secondArea - intersectionArea;
    return unionArea <= 0.0f ? 0.0f : intersectionArea / unionArea;
}

QVector<DetectionBox> nonMaximumSuppression(QVector<DetectionBox> detections)
{
    std::sort(detections.begin(), detections.end(), [](const DetectionBox &left, const DetectionBox &right) {
        return left.confidence > right.confidence;
    });

    QVector<DetectionBox> kept;
    QVector<bool> suppressed(detections.size(), false);
    for (int index = 0; index < detections.size(); ++index) {
        if (suppressed.at(index)) {
            continue;
        }

        const DetectionBox current = detections.at(index);
        kept.append(current);
        for (int other = index + 1; other < detections.size(); ++other) {
            if (suppressed.at(other) || detections.at(other).classId != current.classId) {
                continue;
            }

            if (intersectionOverUnion(current.box, detections.at(other).box) > kNmsThreshold) {
                suppressed[other] = true;
            }
        }
    }

    return kept;
}

#ifdef VIDEOANALYSIS_HAS_ONNXRUNTIME
std::vector<float> preprocessImage(const QImage &frame, int inputWidth, int inputHeight)
{
    const QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888)
                                .scaled(inputWidth,
                                        inputHeight,
                                        Qt::IgnoreAspectRatio,
                                        Qt::SmoothTransformation);
    const int channelSize = inputWidth * inputHeight;
    std::vector<float> tensor(static_cast<size_t>(channelSize * 3), 0.0f);

    for (int y = 0; y < inputHeight; ++y) {
        const uchar *line = rgbFrame.constScanLine(y);
        for (int x = 0; x < inputWidth; ++x) {
            const int sourceIndex = x * 3;
            const int targetIndex = y * inputWidth + x;
            tensor[static_cast<size_t>(targetIndex)] = line[sourceIndex] / 255.0f;
            tensor[static_cast<size_t>(channelSize + targetIndex)] = line[sourceIndex + 1] / 255.0f;
            tensor[static_cast<size_t>(channelSize * 2 + targetIndex)] = line[sourceIndex + 2] / 255.0f;
        }
    }

    return tensor;
}

struct TensorLayout {
    int rows = 0;
    int attributes = 0;
    bool transposed = false;
};

TensorLayout outputLayout(const std::vector<int64_t> &shape, size_t elementCount)
{
    TensorLayout layout;
    if (shape.size() == 3 && shape[0] == 1 && shape[1] > 0 && shape[2] > 0) {
        if (shape[1] <= 256 && shape[2] > shape[1]) {
            layout.attributes = static_cast<int>(shape[1]);
            layout.rows = static_cast<int>(shape[2]);
            layout.transposed = true;
        } else {
            layout.rows = static_cast<int>(shape[1]);
            layout.attributes = static_cast<int>(shape[2]);
        }
    } else if (shape.size() == 2 && shape[0] > 0 && shape[1] > 0) {
        layout.rows = static_cast<int>(shape[0]);
        layout.attributes = static_cast<int>(shape[1]);
    }

    if (layout.rows <= 0 || layout.attributes <= 0
        || static_cast<size_t>(layout.rows) * static_cast<size_t>(layout.attributes) > elementCount) {
        return {};
    }
    return layout;
}

float tensorValue(const float *data, const TensorLayout &layout, int row, int attribute)
{
    if (layout.transposed) {
        return data[static_cast<size_t>(attribute) * static_cast<size_t>(layout.rows) + static_cast<size_t>(row)];
    }
    return data[static_cast<size_t>(row) * static_cast<size_t>(layout.attributes) + static_cast<size_t>(attribute)];
}

QRect scaledBoxFromCenter(float centerX,
                          float centerY,
                          float width,
                          float height,
                          const QSize &originalSize,
                          const QSize &inputSize)
{
    if (width <= 0.0f || height <= 0.0f || originalSize.isEmpty() || inputSize.isEmpty()) {
        return {};
    }

    const bool normalized = std::max({std::fabs(centerX), std::fabs(centerY), std::fabs(width), std::fabs(height)}) <= 2.0f;
    const float scaleX = normalized ? static_cast<float>(originalSize.width())
                                    : static_cast<float>(originalSize.width()) / inputSize.width();
    const float scaleY = normalized ? static_cast<float>(originalSize.height())
                                    : static_cast<float>(originalSize.height()) / inputSize.height();

    QRect box(qRound((centerX - width / 2.0f) * scaleX),
              qRound((centerY - height / 2.0f) * scaleY),
              qRound(width * scaleX),
              qRound(height * scaleY));
    return box.intersected(QRect(QPoint(0, 0), originalSize));
}

QRect scaledBoxFromCorners(float left,
                           float top,
                           float right,
                           float bottom,
                           const QSize &originalSize,
                           const QSize &inputSize)
{
    if (right <= left || bottom <= top || originalSize.isEmpty() || inputSize.isEmpty()) {
        return {};
    }

    const bool normalized = std::max({std::fabs(left), std::fabs(top), std::fabs(right), std::fabs(bottom)}) <= 2.0f;
    const float scaleX = normalized ? static_cast<float>(originalSize.width())
                                    : static_cast<float>(originalSize.width()) / inputSize.width();
    const float scaleY = normalized ? static_cast<float>(originalSize.height())
                                    : static_cast<float>(originalSize.height()) / inputSize.height();

    QRect box(qRound(left * scaleX),
              qRound(top * scaleY),
              qRound((right - left) * scaleX),
              qRound((bottom - top) * scaleY));
    return box.intersected(QRect(QPoint(0, 0), originalSize));
}

QVector<DetectionBox> parseDetections(const float *data,
                                      size_t elementCount,
                                      const std::vector<int64_t> &shape,
                                      const QSize &originalSize,
                                      const QSize &inputSize,
                                      float threshold)
{
    const TensorLayout layout = outputLayout(shape, elementCount);
    if (layout.rows <= 0 || layout.attributes < 5) {
        return {};
    }

    QVector<DetectionBox> detections;
    for (int row = 0; row < layout.rows; ++row) {
        float confidence = 0.0f;
        int classId = 0;
        QRect box;

        if (layout.attributes == 6) {
            confidence = tensorValue(data, layout, row, 4);
            classId = qRound(tensorValue(data, layout, row, 5));
            box = scaledBoxFromCorners(tensorValue(data, layout, row, 0),
                                       tensorValue(data, layout, row, 1),
                                       tensorValue(data, layout, row, 2),
                                       tensorValue(data, layout, row, 3),
                                       originalSize,
                                       inputSize);
            if (box.isEmpty()) {
                box = scaledBoxFromCenter(tensorValue(data, layout, row, 0),
                                          tensorValue(data, layout, row, 1),
                                          tensorValue(data, layout, row, 2),
                                          tensorValue(data, layout, row, 3),
                                          originalSize,
                                          inputSize);
            }
        } else if (layout.attributes == 5) {
            confidence = tensorValue(data, layout, row, 4);
            box = scaledBoxFromCenter(tensorValue(data, layout, row, 0),
                                      tensorValue(data, layout, row, 1),
                                      tensorValue(data, layout, row, 2),
                                      tensorValue(data, layout, row, 3),
                                      originalSize,
                                      inputSize);
        } else {
            const bool hasObjectness = layout.attributes >= 85;
            const int classStart = hasObjectness ? 5 : 4;
            const float objectness = hasObjectness ? tensorValue(data, layout, row, 4) : 1.0f;
            float bestClassScore = -std::numeric_limits<float>::infinity();
            int bestClassId = -1;
            for (int attribute = classStart; attribute < layout.attributes; ++attribute) {
                const float score = tensorValue(data, layout, row, attribute);
                if (score > bestClassScore) {
                    bestClassScore = score;
                    bestClassId = attribute - classStart;
                }
            }

            confidence = objectness * bestClassScore;
            classId = bestClassId;
            box = scaledBoxFromCenter(tensorValue(data, layout, row, 0),
                                      tensorValue(data, layout, row, 1),
                                      tensorValue(data, layout, row, 2),
                                      tensorValue(data, layout, row, 3),
                                      originalSize,
                                      inputSize);
        }

        if (confidence < threshold || classId < 0 || box.isEmpty()) {
            continue;
        }

        DetectionBox detection;
        detection.box = box;
        detection.classId = classId;
        detection.className = classNameForId(classId);
        detection.eventType = eventTypeForClass(classId, detection.className);
        detection.confidence = confidence;
        detections.append(detection);
    }

    return nonMaximumSuppression(detections);
}
#endif
} // namespace

struct OnnxObjectDetector::Private {
    QString modelPath;
    QString lastError;
    float confidenceThreshold = kDefaultConfidenceThreshold;
    bool ready = false;

#ifdef VIDEOANALYSIS_HAS_ONNXRUNTIME
    Ort::Env env {ORT_LOGGING_LEVEL_WARNING, "VideoAnalysisPlatform"};
    Ort::SessionOptions sessionOptions;
    std::unique_ptr<Ort::Session> session;
    std::string inputName;
    std::string outputName;
    int inputWidth = kDefaultInputSize;
    int inputHeight = kDefaultInputSize;
#endif
};

OnnxObjectDetector::OnnxObjectDetector()
    : d(std::make_unique<Private>())
{
#ifdef VIDEOANALYSIS_HAS_ONNXRUNTIME
    d->sessionOptions.SetIntraOpNumThreads(1);
    d->sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);
#endif
}

OnnxObjectDetector::~OnnxObjectDetector() = default;

bool OnnxObjectDetector::loadModel(const QString &modelPath)
{
    const QString normalizedPath = QDir::toNativeSeparators(modelPath.trimmed());
    d->modelPath = normalizedPath;
    d->ready = false;
    d->lastError.clear();

    if (normalizedPath.isEmpty()) {
        d->lastError = QStringLiteral("ONNX 模型路径不能为空。");
        return false;
    }

    if (!QFileInfo::exists(normalizedPath)) {
        d->lastError = QStringLiteral("ONNX 模型文件不存在：%1").arg(normalizedPath);
        return false;
    }

#ifndef VIDEOANALYSIS_HAS_ONNXRUNTIME
    d->lastError = QStringLiteral("ONNX Runtime 未配置，AI 检测已降级为禁用状态。");
    return false;
#else
    try {
#ifdef Q_OS_WIN
        const std::wstring modelPathString = normalizedPath.toStdWString();
        d->session = std::make_unique<Ort::Session>(d->env, modelPathString.c_str(), d->sessionOptions);
#else
        const QByteArray modelPathBytes = normalizedPath.toLocal8Bit();
        d->session = std::make_unique<Ort::Session>(d->env, modelPathBytes.constData(), d->sessionOptions);
#endif

        Ort::AllocatorWithDefaultOptions allocator;
        Ort::AllocatedStringPtr inputName = d->session->GetInputNameAllocated(0, allocator);
        Ort::AllocatedStringPtr outputName = d->session->GetOutputNameAllocated(0, allocator);
        d->inputName = inputName.get();
        d->outputName = outputName.get();

        const Ort::TypeInfo inputTypeInfo = d->session->GetInputTypeInfo(0);
        const std::vector<int64_t> inputShape = inputTypeInfo.GetTensorTypeAndShapeInfo().GetShape();
        if (inputShape.size() >= 4) {
            if (inputShape[2] > 0) {
                d->inputHeight = static_cast<int>(inputShape[2]);
            }
            if (inputShape[3] > 0) {
                d->inputWidth = static_cast<int>(inputShape[3]);
            }
        }

        if (d->inputWidth <= 0 || d->inputHeight <= 0) {
            d->inputWidth = kDefaultInputSize;
            d->inputHeight = kDefaultInputSize;
        }

        d->ready = true;
        return true;
    } catch (const Ort::Exception &error) {
        d->session.reset();
        d->lastError = QStringLiteral("加载 ONNX 模型失败：%1").arg(QString::fromLocal8Bit(error.what()));
        return false;
    } catch (const std::exception &error) {
        d->session.reset();
        d->lastError = QStringLiteral("加载 ONNX 模型失败：%1").arg(QString::fromLocal8Bit(error.what()));
        return false;
    }
#endif
}

bool OnnxObjectDetector::isReady() const
{
    return d->ready;
}

QString OnnxObjectDetector::lastError() const
{
    return d->lastError;
}

void OnnxObjectDetector::setConfidenceThreshold(float threshold)
{
    d->confidenceThreshold = normalizedThreshold(threshold);
}

float OnnxObjectDetector::confidenceThreshold() const
{
    return d->confidenceThreshold;
}

DetectionResult OnnxObjectDetector::detect(const QImage &frame, const QString &sourceId)
{
    DetectionResult result;
    result.sourceId = sourceId;
    result.timestamp = QDateTime::currentMSecsSinceEpoch();

    if (frame.isNull()) {
        return result;
    }

#ifndef VIDEOANALYSIS_HAS_ONNXRUNTIME
    d->lastError = QStringLiteral("ONNX Runtime 未配置，AI 检测已降级为禁用状态。");
    return result;
#else
    if (!d->ready || !d->session) {
        d->lastError = QStringLiteral("ONNX 模型尚未加载。");
        return result;
    }

    try {
        const QSize inputSize(d->inputWidth, d->inputHeight);
        std::vector<float> inputTensorValues = preprocessImage(frame, d->inputWidth, d->inputHeight);
        std::array<int64_t, 4> inputShape = {1, 3, d->inputHeight, d->inputWidth};
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(memoryInfo,
                                                                 inputTensorValues.data(),
                                                                 inputTensorValues.size(),
                                                                 inputShape.data(),
                                                                 inputShape.size());

        const char *inputNames[] = {d->inputName.c_str()};
        const char *outputNames[] = {d->outputName.c_str()};
        std::vector<Ort::Value> outputTensors = d->session->Run(Ort::RunOptions {nullptr},
                                                                inputNames,
                                                                &inputTensor,
                                                                1,
                                                                outputNames,
                                                                1);
        if (outputTensors.empty() || !outputTensors.front().IsTensor()) {
            d->lastError = QStringLiteral("ONNX 推理没有返回有效张量。");
            return result;
        }

        Ort::TensorTypeAndShapeInfo outputInfo = outputTensors.front().GetTensorTypeAndShapeInfo();
        const std::vector<int64_t> outputShape = outputInfo.GetShape();
        const size_t outputCount = outputInfo.GetElementCount();
        const float *outputData = outputTensors.front().GetTensorData<float>();
        result.detections = parseDetections(outputData,
                                            outputCount,
                                            outputShape,
                                            frame.size(),
                                            inputSize,
                                            d->confidenceThreshold);
        for (const DetectionBox &detection : result.detections) {
            result.boxes.append(detection.box);
        }
        d->lastError.clear();
        return result;
    } catch (const Ort::Exception &error) {
        d->lastError = QStringLiteral("ONNX 推理失败：%1").arg(QString::fromLocal8Bit(error.what()));
        return result;
    } catch (const std::exception &error) {
        d->lastError = QStringLiteral("ONNX 推理失败：%1").arg(QString::fromLocal8Bit(error.what()));
        return result;
    }
#endif
}

void OnnxObjectDetector::reset()
{
    d->lastError.clear();
}
