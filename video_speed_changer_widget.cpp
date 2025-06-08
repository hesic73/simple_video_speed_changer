#include "video_speed_changer_widget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>
#include <QSettings>
#include <QGroupBox>       // Included for QGroupBox definition
#include <QDragEnterEvent> // Included for event definitions
#include <QMimeData>       // Included for event definitions
#include <QFileDialog>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QLabel>

// Anonymous namespace for constants local to this translation unit
namespace
{
    const QStringList VIDEO_EXTENSIONS_LIST = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm"};
}

VideoSpeedChangerWidget::VideoSpeedChangerWidget(QWidget *parent)
    : QWidget(parent), outputDirectory(QDir::currentPath()), ffmpegProcess(nullptr)
{
#if defined(Q_OS_WIN)
    defaultFontPath = "C:/Windows/Fonts/arial.ttf";
#elif defined(Q_OS_MACOS)
    defaultFontPath = "/System/Library/Fonts/Helvetica.ttc";
    if (!QFileInfo::exists(defaultFontPath))
    {
        defaultFontPath = "/Library/Fonts/Arial.ttf";
    }
#else
    defaultFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf";
    if (!QFileInfo::exists(defaultFontPath))
    {
        defaultFontPath = "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf";
    }
    if (!QFileInfo::exists(defaultFontPath))
    {
        defaultFontPath = "";
    }
#endif

    setupUi();
    loadSettings();
    updateProcessButtonState();
    setAcceptDrops(true);
}

VideoSpeedChangerWidget::~VideoSpeedChangerWidget()
{
    saveSettings();
    if (ffmpegProcess)
    {
        if (ffmpegProcess->state() != QProcess::NotRunning)
        {
            ffmpegProcess->kill();
            ffmpegProcess->waitForFinished(1000);
        }
        delete ffmpegProcess;
        ffmpegProcess = nullptr;
    }
}

void VideoSpeedChangerWidget::setupUi()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // FFmpeg Path Section
    QGroupBox *ffmpegGroup = new QGroupBox("FFmpeg Configuration", this);
    QFormLayout *ffmpegLayout = new QFormLayout();
    ffmpegPathEdit = new QLineEdit(this);
    chooseFfmpegPathButton = new QPushButton("Browse...", this);
    QHBoxLayout *ffmpegPathLayout = new QHBoxLayout();
    ffmpegPathLayout->addWidget(ffmpegPathEdit);
    ffmpegPathLayout->addWidget(chooseFfmpegPathButton);
    ffmpegLayout->addRow("FFmpeg Path:", ffmpegPathLayout);
    ffmpegGroup->setLayout(ffmpegLayout);
    mainLayout->addWidget(ffmpegGroup);

    connect(chooseFfmpegPathButton, &QPushButton::clicked, this, &VideoSpeedChangerWidget::chooseFfmpegPath);
    connect(ffmpegPathEdit, &QLineEdit::textChanged, this, &VideoSpeedChangerWidget::updateProcessButtonState);

    // Video Files Section
    QGroupBox *videoFilesGroup = new QGroupBox("Video Files", this);
    QVBoxLayout *videoFilesLayout = new QVBoxLayout();
    videoFilesListWidget = new QListWidget(this);
    videoFilesListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    videoFilesListWidget->setAcceptDrops(true);
    QHBoxLayout *videoButtonsLayout = new QHBoxLayout();
    chooseVideoFilesButton = new QPushButton("Add Videos...", this);
    clearListButton = new QPushButton("Clear List", this);
    videoButtonsLayout->addWidget(chooseVideoFilesButton);
    videoButtonsLayout->addWidget(clearListButton);
    videoFilesLayout->addWidget(videoFilesListWidget);
    videoFilesLayout->addLayout(videoButtonsLayout);
    videoFilesGroup->setLayout(videoFilesLayout);
    mainLayout->addWidget(videoFilesGroup, 1);

    connect(chooseVideoFilesButton, &QPushButton::clicked, this, &VideoSpeedChangerWidget::chooseVideoFiles);
    connect(clearListButton, &QPushButton::clicked, this, &VideoSpeedChangerWidget::clearVideoList);
    connect(videoFilesListWidget->model(), &QAbstractItemModel::rowsInserted, this, &VideoSpeedChangerWidget::updateProcessButtonState);
    connect(videoFilesListWidget->model(), &QAbstractItemModel::rowsRemoved, this, &VideoSpeedChangerWidget::updateProcessButtonState);

    // Speed and Output Section
    QGroupBox *settingsGroup = new QGroupBox("Processing Settings", this);
    QFormLayout *settingsLayout = new QFormLayout();

    speedFactorSpinBox = new QDoubleSpinBox(this);
    speedFactorSpinBox->setRange(0.01, 100.0);
    speedFactorSpinBox->setValue(0.5);
    speedFactorSpinBox->setDecimals(2);
    speedFactorSpinBox->setSingleStep(0.1);
    settingsLayout->addRow("Speed Factor (e.g., 0.5 for half speed):", speedFactorSpinBox);

    QHBoxLayout *outputDirLayout = new QHBoxLayout();
    outputDirLabel = new QLabel("Output Directory: " + outputDirectory, this);
    outputDirLabel->setWordWrap(true);
    chooseOutputDirButton = new QPushButton("Choose...", this);
    outputDirLayout->addWidget(outputDirLabel, 1);
    outputDirLayout->addWidget(chooseOutputDirButton);
    settingsLayout->addRow(outputDirLayout);
    settingsGroup->setLayout(settingsLayout);
    mainLayout->addWidget(settingsGroup);

    connect(chooseOutputDirButton, &QPushButton::clicked, this, &VideoSpeedChangerWidget::chooseOutputDirectory);
    connect(speedFactorSpinBox, &QDoubleSpinBox::valueChanged, this, &VideoSpeedChangerWidget::updateProcessButtonState);

    // Overlay Text Section
    overlayGroupBox = new QGroupBox("Speed Overlay (Optional)", this);
    overlayGroupBox->setCheckable(true);
    overlayGroupBox->setChecked(false);
    QFormLayout *overlayLayout = new QFormLayout();
    fontPathEdit = new QLineEdit(this);
    fontPathEdit->setText(defaultFontPath);
    chooseFontPathButton = new QPushButton("Browse Font...", this);
    QHBoxLayout *fontPathLayout = new QHBoxLayout();
    fontPathLayout->addWidget(fontPathEdit);
    fontPathLayout->addWidget(chooseFontPathButton);
    overlayLayout->addRow("Font File (.ttf, .otf):", fontPathLayout);
    fontSizeSpinBox = new QSpinBox(this);
    fontSizeSpinBox->setRange(8, 200);
    fontSizeSpinBox->setValue(64);
    overlayLayout->addRow("Font Size:", fontSizeSpinBox);
    overlayGroupBox->setLayout(overlayLayout);
    mainLayout->addWidget(overlayGroupBox);

    connect(overlayGroupBox, &QGroupBox::toggled, this, &VideoSpeedChangerWidget::onOverlayEnabledChanged);
    onOverlayEnabledChanged(overlayGroupBox->isChecked());

    connect(chooseFontPathButton, &QPushButton::clicked, [this]()
            {
        QString fontFilter = "Font files (*.ttf *.otf);;All files (*)";
        QString currentFontDir = QFileInfo(fontPathEdit->text()).absolutePath();
        if (currentFontDir.isEmpty()) {
             currentFontDir = QStandardPaths::standardLocations(QStandardPaths::FontsLocation).value(0, QDir::homePath());
        }
        QString path = QFileDialog::getOpenFileName(this, "Select Font File", currentFontDir, fontFilter);
        if (!path.isEmpty()) {
            fontPathEdit->setText(path);
        } });

    // Process and Progress Section
    processVideosButton = new QPushButton("Process Videos", this);
    processVideosButton->setFixedHeight(40);
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    logOutputArea = new QPlainTextEdit(this);
    logOutputArea->setReadOnly(true);
    // logOutputArea->setMaximumHeight(150);

    mainLayout->addWidget(processVideosButton);
    mainLayout->addWidget(progressBar);
    mainLayout->addWidget(logOutputArea, 2);

    connect(processVideosButton, &QPushButton::clicked, this, &VideoSpeedChangerWidget::processVideos);

    setLayout(mainLayout);
}

void VideoSpeedChangerWidget::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
    {
        for (const QUrl &url : event->mimeData()->urls())
        {
            if (url.isLocalFile())
            {
                QString filePath = url.toLocalFile();
                if (isValidVideoFile(filePath))
                {
                    event->acceptProposedAction();
                    return;
                }
            }
        }
    }
    event->ignore();
}

void VideoSpeedChangerWidget::dropEvent(QDropEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls())
    {
        QList<QUrl> validUrls;
        for (const QUrl &url : mimeData->urls())
        {
            if (url.isLocalFile())
            {
                QString filePath = url.toLocalFile();
                if (isValidVideoFile(filePath))
                {
                    validUrls.append(url);
                }
            }
        }

        if (!validUrls.isEmpty())
        {
            for (const QUrl &url : validUrls)
            {
                QString filePath = url.toLocalFile();
                if (!videoFilePaths.contains(filePath))
                {
                    videoFilePaths.insert(filePath);
                    videoFilesListWidget->addItem(QFileInfo(filePath).fileName() + " (" + filePath + ")");
                }
            }
            updateProcessButtonState();
            event->acceptProposedAction();
        }
        else
        {
            event->ignore();
        }
    }
    else
    {
        event->ignore();
    }
}

void VideoSpeedChangerWidget::chooseFfmpegPath()
{
    QString currentPath = ffmpegPathEdit->text();
    if (currentPath.isEmpty() || currentPath == defaultFfmpegPath || !QFileInfo::exists(currentPath))
    {
        currentPath = QStandardPaths::findExecutable("ffmpeg");
    }
    QString startDir = QFileInfo(currentPath).absolutePath();
    if (startDir.isEmpty())
    {
        startDir = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
        if (startDir.isEmpty())
            startDir = QDir::homePath();
    }

    QString filePath = QFileDialog::getOpenFileName(this, "Select FFmpeg Executable",
                                                    startDir,
#ifdef Q_OS_WIN
                                                    "FFmpeg (ffmpeg.exe);;All Files (*)"
#else
                                                    "FFmpeg (ffmpeg);;All Files (*)"
#endif
    );
    if (!filePath.isEmpty())
    {
        ffmpegPathEdit->setText(filePath);
    }
}

void VideoSpeedChangerWidget::chooseVideoFiles()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(
        this,
        "Select Video Files",
        QStandardPaths::writableLocation(QStandardPaths::MoviesLocation),
        "Video Files (" + VIDEO_EXTENSIONS_LIST.join(" ") + ");;All Files (*.*)");

    if (!fileNames.isEmpty())
    {
        QList<QUrl> urls;
        for (const QString &fileName : fileNames)
        {
            urls.append(QUrl::fromLocalFile(fileName));
        }
        for (const QUrl &url : urls)
        {
            QString filePath = url.toLocalFile();
            if (!filePath.isEmpty() && !videoFilePaths.contains(filePath) && isValidVideoFile(filePath))
            {
                videoFilePaths.insert(filePath);
                videoFilesListWidget->addItem(QFileInfo(filePath).fileName() + " (" + filePath + ")");
            }
        }
        updateProcessButtonState();
    }
}

void VideoSpeedChangerWidget::chooseOutputDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Select Output Directory",
                                                    outputDirectory,
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty())
    {
        outputDirectory = dir;
        outputDirLabel->setText("Output Directory: " + outputDirectory);
        updateProcessButtonState();
    }
}

void VideoSpeedChangerWidget::clearVideoList()
{
    videoFilesListWidget->clear();
    videoFilePaths.clear();
    logOutputArea->clear();
    updateProcessButtonState();
}

void VideoSpeedChangerWidget::processVideos()
{
    if (videoFilePaths.isEmpty())
    {
        QMessageBox::warning(this, "No Videos", "Please add video files to process.");
        return;
    }
    if (outputDirectory.isEmpty() || !QDir(outputDirectory).exists())
    {
        QMessageBox::warning(this, "Output Directory Invalid", "Please select a valid output directory.");
        return;
    }
    if (ffmpegPathEdit->text().isEmpty())
    {
        QMessageBox::warning(this, "FFmpeg Path Missing", "Please specify the path to FFmpeg executable.");
        return;
    }

    QFileInfo ffmpegInfo(ffmpegPathEdit->text());
    bool ffmpegIsExecutable = ffmpegInfo.isExecutable();
#ifndef Q_OS_WIN
    if (!ffmpegIsExecutable && ffmpegPathEdit->text() == "ffmpeg")
    {
        ffmpegIsExecutable = !QStandardPaths::findExecutable("ffmpeg").isEmpty();
    }
#endif

    if (!ffmpegInfo.exists() || !ffmpegIsExecutable)
    {
        QMessageBox::warning(this, "FFmpeg Error",
                             QString("FFmpeg not found or not executable at: %1\n"
                                     "Please ensure FFmpeg is installed and the path is correct. "
                                     "If using 'ffmpeg', ensure it's in your system's PATH.")
                                 .arg(ffmpegPathEdit->text()));
        return;
    }

    QDir outDir(outputDirectory);
    if (!outDir.exists())
    {
        if (!outDir.mkpath("."))
        {
            QMessageBox::critical(this, "Output Error", "Could not create output directory: " + outputDirectory);
            return;
        }
    }

    filesToProcess = videoFilePaths.values();
    totalFilesToProcess = filesToProcess.size();
    filesProcessedCount = 0;

    logOutputArea->clear();
    logOutputArea->appendPlainText(QString("Starting batch processing of %1 videos...").arg(totalFilesToProcess));

    progressBar->setRange(0, totalFilesToProcess);
    progressBar->setValue(0);
    progressBar->setVisible(true);
    setControlsEnabled(false);

    processNextVideo();
}

void VideoSpeedChangerWidget::onFfmpegProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString currentFileNameForLog = QFileInfo(currentInputFile).fileName();
    if (!ffmpegProcess)
    {
        logOutputArea->appendPlainText(QString("FFmpeg process for %1 was unexpectedly null during finish.").arg(currentFileNameForLog));
    }
    else
    {
        if (exitStatus == QProcess::CrashExit)
        {
            logOutputArea->appendPlainText(QString("Error: FFmpeg crashed while processing %1. Error: %2").arg(currentFileNameForLog, ffmpegProcess->errorString()));
            QMessageBox::critical(this, "FFmpeg Crash", "FFmpeg crashed. Check logs for details on file: " + currentFileNameForLog);
        }
        else if (exitCode != 0)
        {
            logOutputArea->appendPlainText(QString("Error: FFmpeg failed (exit code %1) for %2. Error: %3").arg(exitCode).arg(currentFileNameForLog, ffmpegProcess->errorString()));
            QMessageBox::warning(this, "Processing Error", QString("Failed to process %1. Exit code: %2.").arg(currentFileNameForLog).arg(exitCode));
        }
        else
        {
            logOutputArea->appendPlainText(QString("Successfully processed: %1").arg(QFileInfo(currentOutputFile).fileName()));
            filesProcessedCount++;
            progressBar->setValue(filesProcessedCount);
        }

        ffmpegProcess->disconnect();
        ffmpegProcess->deleteLater();
        ffmpegProcess = nullptr;
    }

    processNextVideo();
}

void VideoSpeedChangerWidget::onFfmpegReadyReadStandardOutput()
{
    if (ffmpegProcess)
    {
        QByteArray data = ffmpegProcess->readAllStandardOutput();
        logOutputArea->appendPlainText(QString::fromUtf8(data).trimmed());
        qDebug().noquote() << "FFMPEG_STDOUT:" << QString::fromUtf8(data).trimmed();
    }
}

void VideoSpeedChangerWidget::onFfmpegReadyReadStandardError()
{
    if (ffmpegProcess)
    {
        QByteArray data = ffmpegProcess->readAllStandardError();
        logOutputArea->appendPlainText(QString::fromLocal8Bit(data).trimmed());
        qDebug().noquote() << "FFMPEG_STDERR:" << QString::fromLocal8Bit(data).trimmed();
    }
}

void VideoSpeedChangerWidget::updateProcessButtonState()
{
    bool hasFiles = videoFilesListWidget->count() > 0;
    bool outputDirSelected = !outputDirectory.isEmpty() && QDir(outputDirectory).exists();
    bool ffmpegPathOk = !ffmpegPathEdit->text().isEmpty();
    bool isProcessing = (ffmpegProcess && ffmpegProcess->state() != QProcess::NotRunning) || !filesToProcess.isEmpty();

    processVideosButton->setEnabled(hasFiles && outputDirSelected && ffmpegPathOk && !isProcessing);
}

void VideoSpeedChangerWidget::onOverlayEnabledChanged(bool checked)
{
    fontPathEdit->setEnabled(checked);
    chooseFontPathButton->setEnabled(checked);
    fontSizeSpinBox->setEnabled(checked);
}

void VideoSpeedChangerWidget::loadSettings()
{
    QSettings settings("MyCompany", "VideoSpeedChangerQt6");
    ffmpegPathEdit->setText(settings.value("ffmpegPath", defaultFfmpegPath).toString());
    outputDirectory = settings.value("outputDirectory", QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString();
    if (outputDirectory.isEmpty() || !QDir(outputDirectory).exists())
    {
        outputDirectory = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
        if (outputDirectory.isEmpty())
            outputDirectory = QDir::currentPath();
    }
    outputDirLabel->setText("Output Directory: " + outputDirectory);
    speedFactorSpinBox->setValue(settings.value("speedFactor", 0.5).toDouble());
    overlayGroupBox->setChecked(settings.value("overlayEnabled", false).toBool());
    fontPathEdit->setText(settings.value("fontPath", defaultFontPath).toString());
    fontSizeSpinBox->setValue(settings.value("fontSize", 64).toInt());
    onOverlayEnabledChanged(overlayGroupBox->isChecked());
}

void VideoSpeedChangerWidget::saveSettings()
{
    QSettings settings("MyCompany", "VideoSpeedChangerQt6");
    settings.setValue("ffmpegPath", ffmpegPathEdit->text());
    settings.setValue("outputDirectory", outputDirectory);
    settings.setValue("speedFactor", speedFactorSpinBox->value());
    settings.setValue("overlayEnabled", overlayGroupBox->isChecked());
    settings.setValue("fontPath", fontPathEdit->text());
    settings.setValue("fontSize", fontSizeSpinBox->value());
}

void VideoSpeedChangerWidget::processNextVideo()
{
    if (filesToProcess.isEmpty())
    {
        progressBar->setVisible(false);
        QMessageBox::information(this, "Processing Complete", QString("All %1 videos processed successfully.").arg(totalFilesToProcess));
        logOutputArea->appendPlainText("All videos processed.");
        setControlsEnabled(true);
        updateProcessButtonState();
        return;
    }

    currentInputFile = filesToProcess.takeFirst();
    QFileInfo inputFileInfo(currentInputFile);
    QString baseName = inputFileInfo.completeBaseName();
    QString extension = inputFileInfo.suffix();
    double speed = speedFactorSpinBox->value();

    currentOutputFile = QDir(outputDirectory).filePath(QString("%1_x%2.%3").arg(baseName).arg(QString::number(speed, 'f', 2)).arg(extension));

    if (ffmpegProcess)
    {
        if (ffmpegProcess->state() != QProcess::NotRunning)
        {
            ffmpegProcess->kill();
            ffmpegProcess->waitForFinished(500);
        }
        delete ffmpegProcess;
        ffmpegProcess = nullptr;
    }
    ffmpegProcess = new QProcess(this);
    ffmpegProcess->disconnect();

    connect(ffmpegProcess, &QProcess::finished, this, &VideoSpeedChangerWidget::onFfmpegProcessFinished);
    connect(ffmpegProcess, &QProcess::readyReadStandardOutput, this, &VideoSpeedChangerWidget::onFfmpegReadyReadStandardOutput);
    connect(ffmpegProcess, &QProcess::readyReadStandardError, this, &VideoSpeedChangerWidget::onFfmpegReadyReadStandardError);
    connect(ffmpegProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error)
            {
        logOutputArea->appendPlainText(QString("FFmpeg process error: %1 for file %2. FFmpeg error string: %3")
                                       .arg(static_cast<int>(error)) // Cast for clarity if needed by arg
                                       .arg(QFileInfo(currentInputFile).fileName())
                                       .arg(ffmpegProcess ? ffmpegProcess->errorString() : "N/A"));
        onFfmpegProcessFinished(-1, QProcess::CrashExit); });

    QStringList arguments;
    arguments << "-i" << currentInputFile;

    QString videoFilterSetpts = QString("setpts=%1*PTS").arg(QString::number(1.0 / speed, 'f', 4));
    QStringList videoFilters;
    videoFilters << videoFilterSetpts;

    if (overlayGroupBox->isChecked())
    {
        QString fontFile = fontPathEdit->text();
        QFileInfo fontInfo(fontFile);
        if (fontFile.isEmpty() || !fontInfo.exists() || !fontInfo.isFile())
        {
            logOutputArea->appendPlainText(QString("Warning: Font file '%1' not found or invalid for overlay on '%2'. Skipping overlay.").arg(fontFile, inputFileInfo.fileName()));
        }
        else
        {
            QString text = QString("x %1").arg(QString::number(speed, 'f', 2));
            int fontSize = fontSizeSpinBox->value();
            QString escapedFontFile = fontFile;
#ifdef Q_OS_WIN
            escapedFontFile.replace("\\", "/");
            escapedFontFile.replace(":", "\\\\:");
#endif
            qDebug() << "Escaped font path:" << escapedFontFile;
            QString drawTextFilter = QString("drawtext=text=%1:fontcolor=white:fontsize=%2:x=w-tw-10:y=h-th-10:shadowcolor=black:shadowx=2:shadowy=2:fontfile=\"%3\"")
                                         .arg(text.replace("'", "\\'"), QString::number(fontSize), escapedFontFile);
            videoFilters << drawTextFilter;
        }
    }
    arguments << "-vf" << videoFilters.join(",");

    QStringList atempoAudioFilters = generateAtempoFilter(speed);
    if (!atempoAudioFilters.isEmpty())
    {
        arguments << "-af" << atempoAudioFilters.join(",");
    }

    arguments << "-y" << currentOutputFile;

    logOutputArea->appendPlainText(QString("\nProcessing (%1/%2): %3 -> %4")
                                       .arg(filesProcessedCount + 1)
                                       .arg(totalFilesToProcess)
                                       .arg(inputFileInfo.fileName())
                                       .arg(QFileInfo(currentOutputFile).fileName()));
    logOutputArea->appendPlainText("FFmpeg command: " + ffmpegPathEdit->text() + " " + arguments.join(" "));
    qDebug() << "Starting ffmpeg with:" << ffmpegPathEdit->text() << arguments;

    ffmpegProcess->start(ffmpegPathEdit->text(), arguments);
    if (!ffmpegProcess->waitForStarted(5000))
    {
        logOutputArea->appendPlainText(QString("Error: Failed to start FFmpeg for %1. Timeout or other error. Process error: %2")
                                           .arg(inputFileInfo.fileName())
                                           .arg(ffmpegProcess->errorString()));
        qDebug() << "FFmpeg failed to start. Error: " << ffmpegProcess->errorString();
        onFfmpegProcessFinished(ffmpegProcess->exitCode(), QProcess::CrashExit);
        return;
    }
}

QStringList VideoSpeedChangerWidget::generateAtempoFilter(double speedFactor)
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
    { // Ensure final factor is somewhat reasonable
        atempoFilters.append(QString("atempo=%1").arg(QString::number(currentFactor, 'f', 4)));
    }
    else if (atempoFilters.isEmpty())
    {                                       // If loop didn't run, and factor is still bad
        atempoFilters.append("atempo=1.0"); // Fallback
    }

    if (atempoFilters.isEmpty())
    {
        return {"atempo=1.0"};
    }
    return atempoFilters;
}

bool VideoSpeedChangerWidget::isValidVideoFile(const QString &filePath)
{
    QFileInfo fileInfo(filePath);
    if (!fileInfo.exists() || !fileInfo.isFile())
        return false;
    QString ext = "*." + fileInfo.suffix().toLower();
    return VIDEO_EXTENSIONS_LIST.contains(ext, Qt::CaseInsensitive);
}

void VideoSpeedChangerWidget::setControlsEnabled(bool enabled)
{
    ffmpegPathEdit->setEnabled(enabled); // Also enable/disable ffmpeg path edit
    chooseFfmpegPathButton->setEnabled(enabled);
    chooseVideoFilesButton->setEnabled(enabled);
    clearListButton->setEnabled(enabled);
    chooseOutputDirButton->setEnabled(enabled);
    speedFactorSpinBox->setEnabled(enabled);
    overlayGroupBox->setEnabled(enabled);
    if (enabled)
    {
        onOverlayEnabledChanged(overlayGroupBox->isChecked()); // Restore based on checkbox
    }
    else
    { // When disabling all, ensure overlay children are also disabled
        fontPathEdit->setEnabled(false);
        chooseFontPathButton->setEnabled(false);
        fontSizeSpinBox->setEnabled(false);
    }
    // processVideosButton's state is managed by updateProcessButtonState or directly when starting/stopping
    // but it should generally follow 'enabled' unless processing has just started
    if (enabled)
    {
        updateProcessButtonState(); // Re-evaluate if process button should be enabled
    }
    else
    {
        processVideosButton->setEnabled(false);
    }
}
