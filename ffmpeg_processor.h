#ifndef FFMPEG_PROCESSOR_H
#define FFMPEG_PROCESSOR_H

#include <QObject>
#include <QStringList>
#include <QProcess>

class FfmpegProcessor : public QObject
{
    Q_OBJECT

public:
    explicit FfmpegProcessor(QObject *parent = nullptr);
    ~FfmpegProcessor();

    struct ProcessingParameters
    {
        QString inputFile;
        QString outputFile;
        double speedFactor = 1.0;
        bool overlayEnabled = false;
        QString fontPath;
        int fontSize = 0;
        QString fontColor;
    };

    void setFfmpegPath(const QString &path);
    void startProcessing(const ProcessingParameters &params);
    void cancelProcessing();

signals:
    void processingStarted();
    void processingFinished(bool success, const QString &outputFile);
    void progressUpdated(int value);
    void logMessage(const QString &message);
    void errorOccurred(const QString &message);

private slots:
    void onFfmpegFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFfmpegReadyReadStandardOutput();
    void onFfmpegReadyReadStandardError();

private:
    QStringList generateAtempoFilter(double speedFactor);
    QString cleanDoubleString(double value);

    QProcess *ffmpegProcess = nullptr;
    QString ffmpegPath;
    ProcessingParameters currentParams;
};

#endif // FFMPEG_PROCESSOR_H