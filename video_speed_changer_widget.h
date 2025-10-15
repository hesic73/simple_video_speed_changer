#ifndef _VIDEO_SPEED_CHANGER_WIDGET_H
#define _VIDEO_SPEED_CHANGER_WIDGET_H

#include <QWidget>
#include <QStringList>

#include "ffmpeg_processor.h" // Include the new processor header

// Forward declarations for Qt classes to minimize header includes
QT_BEGIN_NAMESPACE
class QListWidget;
class QPushButton;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QProgressBar;
class QGroupBox;
class QSpinBox;
class QPlainTextEdit;
class QDragEnterEvent;
class QMimeData;
QT_END_NAMESPACE

class VideoSpeedChangerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit VideoSpeedChangerWidget(QWidget *parent = nullptr);
    ~VideoSpeedChangerWidget() override;

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void chooseFfmpegPath();
    void chooseVideoFiles();
    void chooseOutputDirectory();
    void clearVideoList();
    void processVideos();
    void updateProcessButtonState();
    void onOverlayEnabledChanged(bool checked);

    // Slots to connect to FfmpegProcessor
    void onProcessingStarted();
    void onProcessingFinished(bool success, const QString &outputFile);
    void onLogMessage(const QString &message);
    void onErrorOccurred(const QString &message);

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void processNextVideo();
    bool isValidVideoFile(const QString &filePath);
    void setControlsEnabled(bool enabled);
    static QString cleanDoubleString(double value);

    // UI Elements
    QLineEdit *ffmpegPathEdit;
    QPushButton *chooseFfmpegPathButton;

    QListWidget *videoFilesListWidget;
    QPushButton *chooseVideoFilesButton;
    QPushButton *clearListButton;

    QDoubleSpinBox *speedFactorSpinBox;

    QLabel *outputDirLabel;
    QPushButton *chooseOutputDirButton;
    QString outputDirectory;

    QGroupBox *overlayGroupBox;
    QLineEdit *fontPathEdit;
    QPushButton *chooseFontPathButton;
    QSpinBox *fontSizeSpinBox;
    QLineEdit *fontColorEdit;
    QPushButton *chooseFontColorButton;

    QPushButton *processVideosButton;
    QProgressBar *progressBar;
    QPlainTextEdit *logOutputArea;

    // State Variables
    QSet<QString> videoFilePaths;
    QStringList filesToProcess;
    int totalFilesToProcess = 0;
    int filesProcessedCount = 0;

    FfmpegProcessor *ffmpegProcessor;
    bool isProcessing = false;

    QString defaultFfmpegPath = "ffmpeg";
    QString defaultFontPath;
    QString defaultFontColor = "white";
};

#endif // _VIDEO_SPEED_CHANGER_WIDGET_H
