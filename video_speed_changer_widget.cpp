#include "video_speed_changer_widget.h"

#include "ffmpeg_processor.h"
#include <QColorDialog>
#include <QDebug>
#include <QDir>
#include <QDoubleSpinBox>
#include <QDragEnterEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSpinBox>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

// Anonymous namespace for constants local to this translation unit
namespace
{
    const QStringList VIDEO_EXTENSIONS_LIST = {"*.mp4", "*.mkv", "*.avi", "*.mov", "*.wmv", "*.flv", "*.webm"};
}

VideoSpeedChangerWidget::VideoSpeedChangerWidget(QWidget *parent)
    : QWidget(parent), outputDirectory(QDir::currentPath()), ffmpegProcessor(new FfmpegProcessor(this))
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

    connect(ffmpegProcessor, &FfmpegProcessor::processingStarted, this, &VideoSpeedChangerWidget::onProcessingStarted);
    connect(ffmpegProcessor, &FfmpegProcessor::processingFinished, this, &VideoSpeedChangerWidget::onProcessingFinished);
    connect(ffmpegProcessor, &FfmpegProcessor::logMessage, this, &VideoSpeedChangerWidget::onLogMessage);
    connect(ffmpegProcessor, &FfmpegProcessor::errorOccurred, this, &VideoSpeedChangerWidget::onErrorOccurred);
}

VideoSpeedChangerWidget::~VideoSpeedChangerWidget()
{
    saveSettings();
    ffmpegProcessor->cancelProcessing();
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
    fontColorEdit = new QLineEdit(this);
    fontColorEdit->setPlaceholderText("e.g. #ffffff or white");
    fontColorEdit->setText(defaultFontColor);
    chooseFontColorButton = new QPushButton("Choose Color...", this);
    QHBoxLayout *fontColorLayout = new QHBoxLayout();
    fontColorLayout->addWidget(fontColorEdit);
    fontColorLayout->addWidget(chooseFontColorButton);
    overlayLayout->addRow("Font Color:", fontColorLayout);
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
    connect(chooseFontColorButton, &QPushButton::clicked, [this]()
            {
        QColor initialColor;
        QString currentColorText = fontColorEdit->text().trimmed();
        if (!currentColorText.isEmpty())
        {
            initialColor = QColor(currentColorText);
        }
        if (!initialColor.isValid())
        {
            initialColor = QColor(defaultFontColor);
        }
        QColor selectedColor = QColorDialog::getColor(initialColor, this, "Select Font Color");
        if (selectedColor.isValid())
        {
            fontColorEdit->setText(selectedColor.name(QColor::HexRgb));
        }
    });

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

    ffmpegProcessor->setFfmpegPath(ffmpegPathEdit->text());
    processNextVideo();
}

void VideoSpeedChangerWidget::onProcessingStarted()
{
    isProcessing = true;
    setControlsEnabled(false);
    updateProcessButtonState();
}

void VideoSpeedChangerWidget::onProcessingFinished(bool success, const QString &outputFile)
{
    isProcessing = false;
    if (success)
    {
        filesProcessedCount++;
        progressBar->setValue(filesProcessedCount);
    }
    else
    {
        QMessageBox::warning(this, "Processing Error", QString("Failed to process %1.").arg(QFileInfo(outputFile).fileName()));
    }

    processNextVideo();
}

void VideoSpeedChangerWidget::onLogMessage(const QString &message)
{
    logOutputArea->appendPlainText(message);
}

void VideoSpeedChangerWidget::onErrorOccurred(const QString &message)
{
    logOutputArea->appendPlainText("ERROR: " + message);
    QMessageBox::critical(this, "FFmpeg Error", message);
}

void VideoSpeedChangerWidget::updateProcessButtonState()
{
    bool hasFiles = videoFilesListWidget->count() > 0;
    bool outputDirSelected = !outputDirectory.isEmpty() && QDir(outputDirectory).exists();
    bool ffmpegPathOk = !ffmpegPathEdit->text().isEmpty();

    processVideosButton->setEnabled(hasFiles && outputDirSelected && ffmpegPathOk && !isProcessing);
}

void VideoSpeedChangerWidget::onOverlayEnabledChanged(bool checked)
{
    fontPathEdit->setEnabled(checked);
    chooseFontPathButton->setEnabled(checked);
    fontSizeSpinBox->setEnabled(checked);
    fontColorEdit->setEnabled(checked);
    chooseFontColorButton->setEnabled(checked);
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
    fontColorEdit->setText(settings.value("fontColor", defaultFontColor).toString());
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
    settings.setValue("fontColor", fontColorEdit->text());
}


// Helper function: remove trailing zeros and dot from a double string
QString VideoSpeedChangerWidget::cleanDoubleString(double value)
{
    QString s = QString::number(value, 'f', 2);
    s = s.replace(QRegularExpression("(\\.\\d*?[1-9])0+$"), "\\1"); // Remove unnecessary trailing zeros after decimal point
    s = s.replace(QRegularExpression("\\.0+$"), "");                // Remove .00
    if (s.endsWith('.'))
        s.chop(1);
    return s;
}

void VideoSpeedChangerWidget::processNextVideo()
{
    if (filesToProcess.isEmpty())
    {
        progressBar->setVisible(false);
        QMessageBox::information(this, "Processing Complete", QString("All %1 videos processed successfully.").arg(totalFilesToProcess));
        logOutputArea->appendPlainText("All videos processed.");
        isProcessing = false;
        setControlsEnabled(true);
        updateProcessButtonState();
        return;
    }

    QString currentInputFile = filesToProcess.takeFirst();
    QFileInfo inputFileInfo(currentInputFile);
    QString baseName = inputFileInfo.completeBaseName();
    QString extension = inputFileInfo.suffix();
    double speed = speedFactorSpinBox->value();
    QString speedStr = cleanDoubleString(speed);

    QString currentOutputFile = QDir(outputDirectory).filePath(QString("%1_x%2.%3").arg(baseName).arg(speedStr).arg(extension));

    logOutputArea->appendPlainText(QString("\nProcessing (%1/%2): %3 -> %4")
                                       .arg(filesProcessedCount + 1)
                                       .arg(totalFilesToProcess)
                                       .arg(inputFileInfo.fileName())
                                       .arg(QFileInfo(currentOutputFile).fileName()));

    FfmpegProcessor::ProcessingParameters params;
    params.inputFile = currentInputFile;
    params.outputFile = currentOutputFile;
    params.speedFactor = speed;
    params.overlayEnabled = overlayGroupBox->isChecked();
    if (params.overlayEnabled)
    {
        params.fontPath = fontPathEdit->text();
        params.fontSize = fontSizeSpinBox->value();
        params.fontColor = fontColorEdit->text();
    }

    ffmpegProcessor->startProcessing(params);
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
        fontColorEdit->setEnabled(false);
        chooseFontColorButton->setEnabled(false);
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
