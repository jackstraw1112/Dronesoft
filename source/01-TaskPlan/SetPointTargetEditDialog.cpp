#include "../TaskPlanningTypeConvert.h"
#include "SetPointTargetEditDialog.h"
#include "ui_SetPointTargetEditDialog.h"

#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

SetPointTargetEditDialog::SetPointTargetEditDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SetPointTargetEditDialog)
{
    ui->setupUi(this);
    initObject();
    initConnect();
}

SetPointTargetEditDialog::~SetPointTargetEditDialog()
{
    delete ui;
}

PointTargetInfo SetPointTargetEditDialog::pointTargetInfo() const
{
    PointTargetInfo info;
    info.targetId = ui->targetIdEdit->text().trimmed();
    info.name = ui->targetNameCombo->currentText().trimmed();
    info.type = targetTypeFromChinese(ui->targetTypeCombo->currentText());
    info.threatLevel = threatLevelTypeFromChinese(ui->threatLevelCombo->currentText());
    info.latitude = ui->latitudeSpin->value();
    info.longitude = ui->longitudeSpin->value();
    info.cepMeters = ui->cepSpin->value();
    info.band = QStringLiteral("%1~%2 MHz")
                    .arg(ui->bandLowerSpin->value(), 0, 'f', 2)
                    .arg(ui->bandUpperSpin->value(), 0, 'f', 2);
    info.emissionPattern = ui->emissionPatternEdit->text().trimmed();
    info.priority = ui->priorityCombo->currentText().trimmed();
    info.requiredPk = ui->requiredPkSpin->value();
    return info;
}

void SetPointTargetEditDialog::setTargetNameOptions(const QStringList &targetNames)
{
    const QString currentText = ui->targetNameCombo->currentText().trimmed();
    ui->targetNameCombo->clear();
    ui->targetNameCombo->addItems(targetNames);
    if (!currentText.isEmpty()) {
        ui->targetNameCombo->setCurrentText(currentText);
    }
}

void SetPointTargetEditDialog::setPointTargetInfo(const PointTargetInfo &info)
{
    ui->targetIdEdit->setText(info.targetId);
    if (ui->targetNameCombo->findText(info.name) < 0) {
        ui->targetNameCombo->addItem(info.name);
    }
    ui->targetNameCombo->setCurrentText(info.name);
    ui->targetTypeCombo->setCurrentText(targetTypeToChinese(info.type));
    ui->threatLevelCombo->setCurrentText(threatLevelTypeToChinese(info.threatLevel));
    ui->latitudeSpin->setValue(info.latitude);
    ui->longitudeSpin->setValue(info.longitude);
    ui->cepSpin->setValue(info.cepMeters);
    const QRegularExpression bandPattern(QStringLiteral("([\\d\\.]+)\\s*~\\s*([\\d\\.]+)\\s*MHz"),
                                         QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch bandMatch = bandPattern.match(info.band);
    if (bandMatch.hasMatch()) {
        ui->bandLowerSpin->setValue(bandMatch.captured(1).toDouble());
        ui->bandUpperSpin->setValue(bandMatch.captured(2).toDouble());
    }
    ui->emissionPatternEdit->setText(info.emissionPattern);
    ui->priorityCombo->setCurrentText(info.priority);
    ui->requiredPkSpin->setValue(info.requiredPk);
}

void SetPointTargetEditDialog::setDialogTitle(const QString &title)
{
    setWindowTitle(title);
}

void SetPointTargetEditDialog::onAcceptClicked()
{
    if (ui->targetIdEdit->text().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入点目标编号。"));
        return;
    }
    if (ui->targetNameCombo->currentText().trimmed().isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择目标名称。"));
        return;
    }
    if (ui->bandLowerSpin->value() > ui->bandUpperSpin->value()) {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("频段下限不能大于频段上限。"));
        return;
    }
    accept();
}

void SetPointTargetEditDialog::initObject()
{
    setWindowTitle(QStringLiteral("新增点目标"));
    ui->targetNameCombo->setEditable(false);
    ui->targetTypeCombo->addItems(QStringList()
                                  << QStringLiteral("预警雷达")
                                  << QStringLiteral("监视雷达")
                                  << QStringLiteral("跟踪雷达")
                                  << QStringLiteral("火控雷达")
                                  << QStringLiteral("测高雷达")
                                  << QStringLiteral("多功能相控阵雷达")
                                  << QStringLiteral("制导雷达")
                                  << QStringLiteral("通信/指挥台")
                                  << QStringLiteral("数传节点")
                                  << QStringLiteral("干扰设备")
                                  << QStringLiteral("导航/标信台"));
    ui->threatLevelCombo->addItems(QStringList()
                                   << QStringLiteral("高")
                                   << QStringLiteral("中")
                                   << QStringLiteral("低"));
    ui->priorityCombo->addItems(QStringList() << QStringLiteral("P1")
                                              << QStringLiteral("P2")
                                              << QStringLiteral("P3"));
}

void SetPointTargetEditDialog::initConnect()
{
    connect(ui->okBtn, &QPushButton::clicked, this, &SetPointTargetEditDialog::onAcceptClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &SetPointTargetEditDialog::reject);
    connect(ui->btn_Close, &QPushButton::clicked, this, &SetPointTargetEditDialog::reject);
}
