#include "ffmpeg_processor.h"

#include <QFileInfo>
#include <QDebug>
#include <QRegularExpression>
#include <QColor>

FfmpegProcessor::FfmpegProcessor(QObject *parent) : QObject(parent)
{
    ffmpegProcess = new QProcess(this);
    connect(ffmpegProcess, &QProcess::finished, this, &FfmpegProcessor::onFfmpegFinished);
    connect(ffmpegProcess, &QProcess::readyReadStandardOutput, this, &FfmpegProcessor::onFfmpegReadyReadStandardOutput);
    connect(ffmpegProcess, &QProcess::readyReadStandardError, this, &FfmpegProcessor::onFfmpegReadyReadStandardError);
    connect(ffmpegProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        emit errorOccurred(QString("FFmpeg process error: %1. Error string: %2")
                               .arg(static_cast<int>(error))
                               .arg(ffmpegProcess->errorString()));
    });
}

FfmpegProcessor::~FfmpegProcessor()
{
    if (ffmpegProcess && ffmpegProcess->state() != QProcess::NotRunning)
    {
        ffmpegProcess->kill();
        ffmpegProcess->waitForFinished(1000);
    }
}

void FfmpegProcessor::setFfmpegPath(const QString &path)
{
    this->ffmpegPath = path;
}

void FfmpegProcessor::startProcessing(const ProcessingParameters &params)
{
    if (ffmpegProcess->state() != QProcess::NotRunning)
    {
        emit errorOccurred("A process is already running.");
        return;
    }

    currentParams = params;

    QStringList arguments;
    arguments << "-i" << currentParams.inputFile;

    QString videoFilterSetpts = QString("setpts=%1*PTS").arg(QString::number(1.0 / currentParams.speedFactor, 'f', 4));
    QStringList videoFilters;
    videoFilters << videoFilterSetpts;

    if (currentParams.overlayEnabled)
    {
        QFileInfo fontInfo(currentParams.fontPath);
        if (currentParams.fontPath.isEmpty() || !fontInfo.exists() || !fontInfo.isFile())
        {
            emit logMessage(QString("Warning: Font file '%1' not found or invalid. Skipping overlay.").arg(currentParams.fontPath));
        }
        else
        {
            QString text = QString("x %1").arg(cleanDoubleString(currentParams.speedFactor));
            QString escapedFontFile = currentParams.fontPath;
#ifdef Q_OS_WIN
            escapedFontFile.replace("\\", "/");
            escapedFontFile.replace(":", "\\\\:");
#endif
            QString drawTextFilter = QString("drawtext=text='%1':fontcolor=%2:fontsize=%3:x=w-tw-10:y=h-th-10:shadowcolor=black:shadowx=2:shadowy=2:fontfile='%4'")
                                         .arg(text.replace("'", "\\'"), currentParams.fontColor, QString::number(currentParams.fontSize), escapedFontFile);
            videoFilters << drawTextFilter;
        }
    }
    arguments << "-vf" << videoFilters.join(",");

    QStringList atempoAudioFilters = generateAtempoFilter(currentParams.speedFactor);
    if (!atempoAudioFilters.isEmpty())
    {
        arguments << "-af" << atempoAudioFilters.join(",");
    }

    arguments << "-y" << currentParams.outputFile;

    emit logMessage("FFmpeg command: " + ffmpegPath + " " + arguments.join(" "));
    qDebug() << "Starting ffmpeg with:" << ffmpegPath << arguments;

    ffmpegProcess->start(ffmpegPath, arguments);
    if (!ffmpegProcess->waitForStarted(5000))
    {
        emit errorOccurred("Failed to start FFmpeg. Timeout or other error.");
        return;
    }

    emit processingStarted();
}

void FfmpegProcessor::cancelProcessing()
{
    if (ffmpegProcess && ffmpegProcess->state() != QProcess::NotRunning)
    {
        ffmpegProcess->kill();
    }
}

void FfmpegProcessor::onFfmpegFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit)
    {
        emit errorOccurred(QString("FFmpeg crashed. Error: %1").arg(ffmpegProcess->errorString()));
        emit processingFinished(false, currentParams.outputFile);
    }
    else if (exitCode != 0)
    {
        emit errorOccurred(QString("FFmpeg failed (exit code %1). Error: %2").arg(exitCode).arg(ffmpegProcess->errorString()));
        emit processingFinished(false, currentParams.outputFile);
    }
    else
    {
        emit logMessage(QString("Successfully processed: %1").arg(QFileInfo(currentParams.outputFile).fileName()));
        emit processingFinished(true, currentParams.outputFile);
    }
}

void FfmpegProcessor::onFfmpegReadyReadStandardOutput()
{
    QByteArray data = ffmpegProcess->readAllStandardOutput();
    emit logMessage(QString::fromUtf8(data).trimmed());
}

void FfmpegProcessor::onFfmpegReadyReadStandardError()
{
    QByteArray data = ffmpegProcess->readAllStandardError();
    emit logMessage(QString::fromLocal8Bit(data).trimmed());
}

QStringList FfmpegProcessor::generateAtempoFilter(double speedFactor)
{
    QStringList atempoFilters;
    if (speedFactor <= 0.001)
    {
        return {"atempo=1.0"};
    }

    double currentFactor = speedFactor;
    for (int i = 0; i < 10 && (currentFactor < 0.5 || currentFactor > 2.0); ++i)
    {
        if (currentFactor < 0.5)
        {
            atempoFilters.append("atempo=0.5");
            currentFactor /= 0.5;
        }
        else
        {
            atempoFilters.append("atempo=2.0");
            currentFactor /= 2.0;
        }
    }
    if (currentFactor >= 0.01 && currentFactor <= 100.0)
    {
        atempoFilters.append(QString("atempo=%1").arg(QString::number(currentFactor, 'f', 4)));
    }
    else if (atempoFilters.isEmpty())
    {
        atempoFilters.append("atempo=1.0");
    }

    if (atempoFilters.isEmpty())
    {
        return {"atempo=1.0"};
    }
    return atempoFilters;
}

QString FfmpegProcessor::cleanDoubleString(double value)
{
    QString s = QString::number(value, 'f', 2);
    s = s.replace(QRegularExpression("(\\.\\d*?[1-9])0+$"), "\\1");
    s = s.replace(QRegularExpression("\\.0+$"), "");
    if (s.endsWith('.')) s.chop(1);
    return s;
}