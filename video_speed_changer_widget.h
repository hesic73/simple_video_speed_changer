#ifndef _VIDEO_SPEED_CHANGER_WIDGET_H
#define _VIDEO_SPEED_CHANGER_WIDGET_H

#include <QWidget>
#include <QProcess>
#include <QStringList> // For forward declaration if needed, or for VIDEO_EXTENSIONS_LIST if kept here

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

// It's common to declare constants like this in the .cpp if they are only used there,
// or in the .hpp (e.g., in a namespace or as static const member) if needed by users of the header.
// For this case, VIDEO_EXTENSIONS_LIST is used in slots implemented in .cpp, so it can be in .cpp.
// However, if isValidVideoFile were to be public and used outside, it might be better here.
// Let's keep it in the .cpp for now via an anonymous namespace or static const.

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
    void onFfmpegProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onFfmpegReadyReadStandardOutput();
    void onFfmpegReadyReadStandardError();
    void updateProcessButtonState();
    void onOverlayEnabledChanged(bool checked);

private:
    void setupUi();
    void loadSettings();
    void saveSettings();
    void processNextVideo();
    QStringList generateAtempoFilter(double speedFactor);
    bool isValidVideoFile(const QString &filePath);
    void setControlsEnabled(bool enabled);

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

    QPushButton *processVideosButton;
    QProgressBar *progressBar;
    QPlainTextEdit *logOutputArea;

    // State Variables
    QSet<QString> videoFilePaths;
    QStringList filesToProcess;
    int totalFilesToProcess = 0;
    int filesProcessedCount = 0;

    QProcess *ffmpegProcess;
    QString currentInputFile;
    QString currentOutputFile;
    QString defaultFfmpegPath = "ffmpeg";
    QString defaultFontPath;
};

#endif // _VIDEO_SPEED_CHANGER_WIDGET_H