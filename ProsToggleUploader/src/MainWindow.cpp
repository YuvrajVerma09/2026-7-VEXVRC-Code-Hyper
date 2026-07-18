#include "MainWindow.hpp"

#include <QCheckBox>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTextCursor>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

namespace {
QString quotedForDisplay(const QString &value) {
    if (!value.contains(QLatin1Char(' ')) && !value.contains(QLatin1Char('\t'))) {
        return value;
    }

    QString escaped = value;
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QStringLiteral("\"") + escaped + QStringLiteral("\"");
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), process_(new QProcess(this)) {
    buildInterface();
    connectSignals();
    loadSettings();

    setWindowTitle(QStringLiteral("PROS Toggle Uploader"));
    resize(900, 700);
}

void MainWindow::buildInterface() {
    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);
    rootLayout->setContentsMargins(16, 16, 16, 16);
    rootLayout->setSpacing(12);

    auto *projectGroup = new QGroupBox(QStringLiteral("PROS project"), central);
    auto *projectLayout = new QGridLayout(projectGroup);

    projectPathEdit_ = new QLineEdit(projectGroup);
    projectPathEdit_->setPlaceholderText(QStringLiteral("Folder containing project.pros"));
    browseProjectButton_ = new QPushButton(QStringLiteral("Browse…"), projectGroup);

    projectLayout->addWidget(new QLabel(QStringLiteral("Project folder:"), projectGroup), 0, 0);
    projectLayout->addWidget(projectPathEdit_, 0, 1);
    projectLayout->addWidget(browseProjectButton_, 0, 2);

    auto *toolGroup = new QGroupBox(QStringLiteral("PROS command line"), central);
    auto *toolLayout = new QGridLayout(toolGroup);

    prosExecutableEdit_ = new QLineEdit(toolGroup);
    prosExecutableEdit_->setPlaceholderText(QStringLiteral("pros or the full path to the PROS executable"));
    browseProsButton_ = new QPushButton(QStringLiteral("Browse…"), toolGroup);
    checkProsButton_ = new QPushButton(QStringLiteral("Check PROS"), toolGroup);
    buildArgumentsEdit_ = new QLineEdit(toolGroup);
    uploadArgumentsEdit_ = new QLineEdit(toolGroup);

    buildArgumentsEdit_->setPlaceholderText(QStringLiteral("Optional arguments added after: pros make"));
    uploadArgumentsEdit_->setPlaceholderText(QStringLiteral("Optional arguments added after: pros upload"));

    toolLayout->addWidget(new QLabel(QStringLiteral("Executable:"), toolGroup), 0, 0);
    toolLayout->addWidget(prosExecutableEdit_, 0, 1);
    toolLayout->addWidget(browseProsButton_, 0, 2);
    toolLayout->addWidget(checkProsButton_, 0, 3);
    toolLayout->addWidget(new QLabel(QStringLiteral("Build arguments:"), toolGroup), 1, 0);
    toolLayout->addWidget(buildArgumentsEdit_, 1, 1, 1, 3);
    toolLayout->addWidget(new QLabel(QStringLiteral("Upload arguments:"), toolGroup), 2, 0);
    toolLayout->addWidget(uploadArgumentsEdit_, 2, 1, 1, 3);

    auto *featuresGroup = new QGroupBox(QStringLiteral("Robot features included in the build"), central);
    auto *featuresLayout = new QGridLayout(featuresGroup);

    for (qsizetype index = 0; index < static_cast<qsizetype>(kFeatureSpecs.size()); ++index) {
        const FeatureSpec &spec = kFeatureSpecs.at(static_cast<std::size_t>(index));
        auto *checkBox = new QCheckBox(QString::fromUtf8(spec.label), featuresGroup);
        checkBox->setChecked(spec.enabledByDefault);
        checkBox->setToolTip(QString::fromUtf8(spec.macroName));
        featureCheckBoxes_.append(checkBox);
        featuresLayout->addWidget(checkBox, static_cast<int>(index / 2), static_cast<int>(index % 2));
    }

    auto *featureHelp = new QLabel(
        QStringLiteral("Build writes <b>include/app_features.hpp</b>. Your PROS code must use the generated "
                       "macros around the code that each toggle controls."),
        featuresGroup);
    featureHelp->setWordWrap(true);
    featuresLayout->addWidget(featureHelp, 4, 0, 1, 2);

    auto *buttonLayout = new QHBoxLayout();
    buildButton_ = new QPushButton(QStringLiteral("Build"), central);
    uploadButton_ = new QPushButton(QStringLiteral("Upload Existing"), central);
    buildUploadButton_ = new QPushButton(QStringLiteral("Build + Upload"), central);
    cancelButton_ = new QPushButton(QStringLiteral("Cancel"), central);
    cancelButton_->setEnabled(false);
    buildUploadButton_->setDefault(true);

    buttonLayout->addWidget(buildButton_);
    buttonLayout->addWidget(uploadButton_);
    buttonLayout->addWidget(buildUploadButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(cancelButton_);

    statusLabel_ = new QLabel(QStringLiteral("Ready"), central);
    outputEdit_ = new QPlainTextEdit(central);
    outputEdit_->setReadOnly(true);
    outputEdit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    outputEdit_->setMaximumBlockCount(10000);
    outputEdit_->setPlaceholderText(QStringLiteral("Compiler and uploader output appears here."));

    rootLayout->addWidget(projectGroup);
    rootLayout->addWidget(toolGroup);
    rootLayout->addWidget(featuresGroup);
    rootLayout->addLayout(buttonLayout);
    rootLayout->addWidget(statusLabel_);
    rootLayout->addWidget(outputEdit_, 1);

    setCentralWidget(central);
}

void MainWindow::connectSignals() {
    connect(browseProjectButton_, &QPushButton::clicked, this, [this] { chooseProjectDirectory(); });
    connect(browseProsButton_, &QPushButton::clicked, this, [this] { chooseProsExecutable(); });
    connect(checkProsButton_, &QPushButton::clicked, this, [this] { checkProsVersion(); });
    connect(buildButton_, &QPushButton::clicked, this, [this] { buildProject(); });
    connect(uploadButton_, &QPushButton::clicked, this, [this] { uploadExistingBuild(); });
    connect(buildUploadButton_, &QPushButton::clicked, this, [this] { buildAndUpload(); });
    connect(cancelButton_, &QPushButton::clicked, this, [this] { cancelCurrentProcess(); });

    process_->setProcessChannelMode(QProcess::MergedChannels);
    connect(process_, &QProcess::readyReadStandardOutput, this, [this] { readProcessOutput(); });
    connect(process_, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus) { handleProcessFinished(exitCode); });
    connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) { handleProcessError(); });
}

void MainWindow::loadSettings() {
    QSettings settings;

    projectPathEdit_->setText(settings.value(QStringLiteral("projectDirectory")).toString());

    QString defaultPros = QStandardPaths::findExecutable(QStringLiteral("pros"));
    if (defaultPros.isEmpty()) {
        defaultPros = QStandardPaths::findExecutable(QStringLiteral("prosv5"));
    }
    if (defaultPros.isEmpty()) {
        defaultPros = QStringLiteral("pros");
    }

    prosExecutableEdit_->setText(
        settings.value(QStringLiteral("prosExecutable"), defaultPros).toString());
    buildArgumentsEdit_->setText(settings.value(QStringLiteral("buildArguments")).toString());
    uploadArgumentsEdit_->setText(settings.value(QStringLiteral("uploadArguments")).toString());

    for (qsizetype index = 0; index < featureCheckBoxes_.size(); ++index) {
        const FeatureSpec &spec = kFeatureSpecs.at(static_cast<std::size_t>(index));
        const QString key = QStringLiteral("features/") + QString::fromUtf8(spec.macroName);
        featureCheckBoxes_.at(index)->setChecked(
            settings.value(key, spec.enabledByDefault).toBool());
    }
}

void MainWindow::saveSettings() const {
    QSettings settings;
    settings.setValue(QStringLiteral("projectDirectory"), projectDirectory());
    settings.setValue(QStringLiteral("prosExecutable"), prosExecutable());
    settings.setValue(QStringLiteral("buildArguments"), buildArgumentsEdit_->text());
    settings.setValue(QStringLiteral("uploadArguments"), uploadArgumentsEdit_->text());

    for (qsizetype index = 0; index < featureCheckBoxes_.size(); ++index) {
        const FeatureSpec &spec = kFeatureSpecs.at(static_cast<std::size_t>(index));
        const QString key = QStringLiteral("features/") + QString::fromUtf8(spec.macroName);
        settings.setValue(key, featureCheckBoxes_.at(index)->isChecked());
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::chooseProjectDirectory() {
    const QString selected = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Choose a PROS project"),
        projectDirectory().isEmpty() ? QDir::homePath() : projectDirectory());

    if (!selected.isEmpty()) {
        projectPathEdit_->setText(QDir::toNativeSeparators(selected));
    }
}

void MainWindow::chooseProsExecutable() {
    const QString selected = QFileDialog::getOpenFileName(
        this,
        QStringLiteral("Choose the PROS executable"),
        QFileInfo(prosExecutable()).absolutePath(),
        QStringLiteral("All files (*)"));

    if (!selected.isEmpty()) {
        prosExecutableEdit_->setText(QDir::toNativeSeparators(selected));
    }
}

void MainWindow::checkProsVersion() {
    startSequence(QList<Action>{Action::CheckVersion});
}

void MainWindow::buildProject() {
    if (!prepareProjectForBuild()) {
        return;
    }
    startSequence(QList<Action>{Action::Build});
}

void MainWindow::uploadExistingBuild() {
    if (!validateProjectDirectory()) {
        return;
    }
    startSequence(QList<Action>{Action::Upload});
}

void MainWindow::buildAndUpload() {
    if (!prepareProjectForBuild()) {
        return;
    }
    startSequence(QList<Action>{Action::Build, Action::Upload});
}

void MainWindow::cancelCurrentProcess() {
    actionQueue_.clear();
    if (process_->state() != QProcess::NotRunning) {
        statusLabel_->setText(QStringLiteral("Cancelling…"));
        process_->terminate();
        if (!process_->waitForFinished(1500)) {
            process_->kill();
        }
    }
    setBusy(false);
    statusLabel_->setText(QStringLiteral("Cancelled"));
}

bool MainWindow::validateProjectDirectory() const {
    const QDir directory(projectDirectory());
    if (!directory.exists()) {
        QMessageBox::warning(const_cast<MainWindow *>(this),
                             QStringLiteral("Project not found"),
                             QStringLiteral("Choose an existing PROS project folder."));
        return false;
    }

    const bool hasProjectFile = QFileInfo::exists(directory.filePath(QStringLiteral("project.pros")));
    const bool hasMakefile = QFileInfo::exists(directory.filePath(QStringLiteral("Makefile")));

    if (!hasProjectFile && !hasMakefile) {
        QMessageBox::warning(
            const_cast<MainWindow *>(this),
            QStringLiteral("Not a PROS project"),
            QStringLiteral("The selected folder does not contain project.pros or a Makefile."));
        return false;
    }

    return true;
}

bool MainWindow::prepareProjectForBuild() {
    if (!validateProjectDirectory()) {
        return false;
    }

    QString errorMessage;
    if (!writeFeatureHeader(&errorMessage)) {
        QMessageBox::critical(this, QStringLiteral("Could not write feature header"), errorMessage);
        return false;
    }

    return true;
}

bool MainWindow::writeFeatureHeader(QString *errorMessage) const {
    QDir project(projectDirectory());
    if (!project.mkpath(QStringLiteral("include"))) {
        *errorMessage = QStringLiteral("Could not create the project's include folder.");
        return false;
    }

    QString contents;
    QTextStream stream(&contents);
    stream << QStringLiteral("#pragma once\n\n");
    stream << QStringLiteral("// Generated by PROS Toggle Uploader. Manual changes will be replaced.\n\n");

    for (qsizetype index = 0; index < featureCheckBoxes_.size(); ++index) {
        const FeatureSpec &spec = kFeatureSpecs.at(static_cast<std::size_t>(index));
        stream << QStringLiteral("#define ") << QString::fromUtf8(spec.macroName) << QLatin1Char(' ')
               << (featureCheckBoxes_.at(index)->isChecked() ? 1 : 0) << QLatin1Char('\n');
    }

    stream << QStringLiteral("\nnamespace robot_features {\n");
    for (const FeatureSpec &spec : kFeatureSpecs) {
        stream << QStringLiteral("inline constexpr bool ") << QString::fromUtf8(spec.cppName)
               << QStringLiteral(" = ") << QString::fromUtf8(spec.macroName)
               << QStringLiteral(" != 0;\n");
    }
    stream << QStringLiteral("} // namespace robot_features\n\n");
    stream << QStringLiteral("#if ROBOT_FEATURE_DEBUG_LOGGING\n");
    stream << QStringLiteral("#include <cstdio>\n");
    stream << QStringLiteral("#define ROBOT_DEBUG_LOG(...) std::printf(__VA_ARGS__)\n");
    stream << QStringLiteral("#else\n");
    stream << QStringLiteral("#define ROBOT_DEBUG_LOG(...) ((void)0)\n");
    stream << QStringLiteral("#endif\n");

    const QString headerPath = project.filePath(QStringLiteral("include/app_features.hpp"));
    QFile existing(headerPath);
    if (existing.open(QIODevice::ReadOnly) && existing.readAll() == contents.toUtf8()) {
        return true;
    }

    QSaveFile output(headerPath);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Text)) {
        *errorMessage = QStringLiteral("Could not open %1 for writing: %2")
                            .arg(QDir::toNativeSeparators(headerPath), output.errorString());
        return false;
    }

    if (output.write(contents.toUtf8()) < 0 || !output.commit()) {
        *errorMessage = QStringLiteral("Could not save %1: %2")
                            .arg(QDir::toNativeSeparators(headerPath), output.errorString());
        return false;
    }

    outputEdit_->appendPlainText(
        QStringLiteral("Generated %1").arg(QDir::toNativeSeparators(headerPath)));
    return true;
}

void MainWindow::startSequence(const QList<Action> &actions) {
    if (busy_) {
        return;
    }

    if (prosExecutable().isEmpty()) {
        QMessageBox::warning(this,
                             QStringLiteral("PROS executable missing"),
                             QStringLiteral("Enter 'pros' or choose the PROS executable."));
        return;
    }

    actionQueue_.clear();
    for (const Action action : actions) {
        actionQueue_.enqueue(action);
    }

    setBusy(true);
    startNextAction();
}

void MainWindow::startNextAction() {
    if (actionQueue_.isEmpty()) {
        setBusy(false);
        statusLabel_->setText(QStringLiteral("Completed successfully"));
        outputEdit_->appendPlainText(QStringLiteral("\nCompleted successfully.\n"));
        return;
    }

    currentAction_ = actionQueue_.dequeue();
    QStringList arguments;

    switch (currentAction_) {
    case Action::CheckVersion:
        statusLabel_->setText(QStringLiteral("Checking PROS…"));
        arguments << QStringLiteral("--version");
        process_->setWorkingDirectory(QDir::homePath());
        break;
    case Action::Build:
        statusLabel_->setText(QStringLiteral("Building project…"));
        arguments << QStringLiteral("make");
        arguments.append(buildArguments());
        process_->setWorkingDirectory(projectDirectory());
        break;
    case Action::Upload:
        statusLabel_->setText(QStringLiteral("Uploading to V5 Brain…"));
        arguments << QStringLiteral("upload");
        arguments.append(uploadArguments());
        process_->setWorkingDirectory(projectDirectory());
        break;
    }

    appendCommand(prosExecutable(), arguments);
    process_->start(prosExecutable(), arguments);
}

void MainWindow::readProcessOutput() {
    const QByteArray bytes = process_->readAllStandardOutput();
    if (!bytes.isEmpty()) {
        outputEdit_->moveCursor(QTextCursor::End);
        outputEdit_->insertPlainText(QString::fromLocal8Bit(bytes));
        outputEdit_->moveCursor(QTextCursor::End);
    }
}

void MainWindow::handleProcessFinished(int exitCode) {
    readProcessOutput();

    if (!busy_) {
        return;
    }

    if (exitCode != 0) {
        actionQueue_.clear();
        setBusy(false);
        statusLabel_->setText(QStringLiteral("Command failed with exit code %1").arg(exitCode));
        outputEdit_->appendPlainText(
            QStringLiteral("\nCommand failed with exit code %1.\n").arg(exitCode));
        return;
    }

    startNextAction();
}

void MainWindow::handleProcessError() {
    if (!busy_) {
        return;
    }

    const QString message = process_->errorString();
    outputEdit_->appendPlainText(QStringLiteral("\nProcess error: %1\n").arg(message));

    if (process_->state() == QProcess::NotRunning) {
        actionQueue_.clear();
        setBusy(false);
        statusLabel_->setText(QStringLiteral("Could not start PROS"));
    }
}

void MainWindow::setBusy(bool busy) {
    busy_ = busy;

    projectPathEdit_->setEnabled(!busy);
    prosExecutableEdit_->setEnabled(!busy);
    buildArgumentsEdit_->setEnabled(!busy);
    uploadArgumentsEdit_->setEnabled(!busy);
    browseProjectButton_->setEnabled(!busy);
    browseProsButton_->setEnabled(!busy);
    checkProsButton_->setEnabled(!busy);
    buildButton_->setEnabled(!busy);
    uploadButton_->setEnabled(!busy);
    buildUploadButton_->setEnabled(!busy);

    for (QCheckBox *checkBox : featureCheckBoxes_) {
        checkBox->setEnabled(!busy);
    }

    cancelButton_->setEnabled(busy);
}

void MainWindow::appendCommand(const QString &program, const QStringList &arguments) {
    QStringList displayParts;
    displayParts << quotedForDisplay(program);
    for (const QString &argument : arguments) {
        displayParts << quotedForDisplay(argument);
    }

    outputEdit_->appendPlainText(QStringLiteral("\n> ") + displayParts.join(QLatin1Char(' ')) +
                                 QLatin1Char('\n'));
}

QString MainWindow::prosExecutable() const {
    return QDir::fromNativeSeparators(prosExecutableEdit_->text().trimmed());
}

QString MainWindow::projectDirectory() const {
    return QDir::fromNativeSeparators(projectPathEdit_->text().trimmed());
}

QStringList MainWindow::buildArguments() const {
    return QProcess::splitCommand(buildArgumentsEdit_->text());
}

QStringList MainWindow::uploadArguments() const {
    return QProcess::splitCommand(uploadArgumentsEdit_->text());
}
