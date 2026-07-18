#pragma once

#include <QList>
#include <QMainWindow>
#include <QQueue>
#include <QString>
#include <QStringList>
#include <QVector>

#include <array>

class QCheckBox;
class QCloseEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProcess;
class QPushButton;

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    enum class Action {
        CheckVersion,
        Build,
        Upload
    };

    struct FeatureSpec {
        const char *label;
        const char *macroName;
        const char *cppName;
        bool enabledByDefault;
    };

    inline static constexpr std::array<FeatureSpec, 7> kFeatureSpecs{{
        {"Autonomous", "ROBOT_FEATURE_AUTONOMOUS", "autonomous", true},
        {"Driver control", "ROBOT_FEATURE_DRIVE", "drive", true},
        {"Intake", "ROBOT_FEATURE_INTAKE", "intake", true},
        {"Lift / arm", "ROBOT_FEATURE_LIFT", "lift", true},
        {"Pneumatics", "ROBOT_FEATURE_PNEUMATICS", "pneumatics", true},
        {"Odometry", "ROBOT_FEATURE_ODOMETRY", "odometry", true},
        {"Debug logging", "ROBOT_FEATURE_DEBUG_LOGGING", "debug_logging", false},
    }};

    void buildInterface();
    void connectSignals();
    void loadSettings();
    void saveSettings() const;

    void chooseProjectDirectory();
    void chooseProsExecutable();
    void checkProsVersion();
    void buildProject();
    void uploadExistingBuild();
    void buildAndUpload();
    void cancelCurrentProcess();

    bool validateProjectDirectory() const;
    bool prepareProjectForBuild();
    bool writeFeatureHeader(QString *errorMessage) const;

    void startSequence(const QList<Action> &actions);
    void startNextAction();
    void readProcessOutput();
    void handleProcessFinished(int exitCode);
    void handleProcessError();
    void setBusy(bool busy);
    void appendCommand(const QString &program, const QStringList &arguments);

    QString prosExecutable() const;
    QString projectDirectory() const;
    QStringList buildArguments() const;
    QStringList uploadArguments() const;

    QLineEdit *projectPathEdit_ = nullptr;
    QLineEdit *prosExecutableEdit_ = nullptr;
    QLineEdit *buildArgumentsEdit_ = nullptr;
    QLineEdit *uploadArgumentsEdit_ = nullptr;

    QVector<QCheckBox *> featureCheckBoxes_;

    QPushButton *browseProjectButton_ = nullptr;
    QPushButton *browseProsButton_ = nullptr;
    QPushButton *checkProsButton_ = nullptr;
    QPushButton *buildButton_ = nullptr;
    QPushButton *uploadButton_ = nullptr;
    QPushButton *buildUploadButton_ = nullptr;
    QPushButton *cancelButton_ = nullptr;

    QLabel *statusLabel_ = nullptr;
    QPlainTextEdit *outputEdit_ = nullptr;
    QProcess *process_ = nullptr;
    QQueue<Action> actionQueue_;
    Action currentAction_ = Action::CheckVersion;
    bool busy_ = false;
};
