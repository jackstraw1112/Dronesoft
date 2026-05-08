#include "SetPointTargetEditDialog.h"
#include "TaskPlanningTypeConvert.h"
#include "ui_SetPointTargetEditDialog.h"

#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

// 构造函数：初始化UI并完成控件内容与信号绑定
SetPointTargetEditDialog::SetPointTargetEditDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SetPointTargetEditDialog)
{
    // 由uic生成的界面对象完成控件创建与布局装配
    ui->setupUi(this);
    // 初始化默认数据（下拉框选项、窗口标题等）
    initObject();
    // 统一绑定交互信号，保持构造函数简洁
    initConnect();
}

SetPointTargetEditDialog::~SetPointTargetEditDialog()
{
    delete ui;
}

PointTargetInfo SetPointTargetEditDialog::pointTargetInfo() const
{
    // 将界面输入收敛为业务数据对象，供外层保存/提交
    PointTargetInfo info;

    // 文本字段统一trim，避免无意义前后空格进入数据层
    info.targetId = ui->targetIdEdit->text().trimmed();
    info.name = ui->targetNameCombo->currentText().trimmed();

    // 界面中文枚举值转换为内部强类型枚举，便于后续逻辑判断
    info.type = targetTypeFromChinese(ui->targetTypeCombo->currentText());
    info.threatLevel = threatLevelTypeFromChinese(ui->threatLevelCombo->currentText());

    // 坐标、精度和毁伤概率直接读取数值控件
    info.latitude = ui->latitudeSpb->value();
    info.longitude = ui->longitudeSpb->value();
    info.cepMeters = ui->cepSpb->value();

    // 频段在UI上拆分为上下限，落库时合并为统一展示字符串
    info.band = QStringLiteral("%1~%2 MHz")
                        .arg(ui->bandLowerSpb->value(), 0, 'f', 2)
                        .arg(ui->bandUpperSpb->value(), 0, 'f', 2);

    // 当前版本保留字符串字段（后续可按需要再升级为枚举）
    info.emissionPattern = ui->emissionPatternEdit->text().trimmed();
    info.priority = ui->priorityCombo->currentText().trimmed();
    info.requiredPk = ui->requiredPkSpb->value();
    return info;
}

void SetPointTargetEditDialog::setTargetNameOptions(const QStringList &targetNames)
{
    // 记录当前选中项，更新候选列表后尽量恢复用户已选内容
    const QString currentText = ui->targetNameCombo->currentText().trimmed();
    ui->targetNameCombo->clear();
    ui->targetNameCombo->addItems(targetNames);
    if (!currentText.isEmpty())
    {
        // setCurrentText在可匹配时恢复原值，不匹配则保持默认行为
        ui->targetNameCombo->setCurrentText(currentText);
    }
}

void SetPointTargetEditDialog::setPointTargetInfo(const PointTargetInfo &info)
{
    // 回填编号与名称；若名称不在选项中，先补充后再选中
    ui->targetIdEdit->setText(info.targetId);
    if (ui->targetNameCombo->findText(info.name) < 0)
    {
        ui->targetNameCombo->addItem(info.name);
    }
    ui->targetNameCombo->setCurrentText(info.name);

    // 强类型枚举还原为中文，保证编辑态显示与新增态一致
    ui->targetTypeCombo->setCurrentText(targetTypeToChinese(info.type));
    ui->threatLevelCombo->setCurrentText(threatLevelTypeToChinese(info.threatLevel));

    // 数值类字段直接回填
    ui->latitudeSpb->setValue(info.latitude);
    ui->longitudeSpb->setValue(info.longitude);
    ui->cepSpb->setValue(info.cepMeters);

    // band采用“x~y MHz”解析；兼容大小写MHz及两端空格
    const QRegularExpression bandPattern(QStringLiteral("([\\d\\.]+)\\s*~\\s*([\\d\\.]+)\\s*MHz"),
                                         QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch bandMatch = bandPattern.match(info.band);
    if (bandMatch.hasMatch())
    {
        // 解析成功时分别回填上下限；解析失败则保留控件默认值
        ui->bandLowerSpb->setValue(bandMatch.captured(1).toDouble());
        ui->bandUpperSpb->setValue(bandMatch.captured(2).toDouble());
    }

    ui->emissionPatternEdit->setText(info.emissionPattern);
    ui->priorityCombo->setCurrentText(info.priority);
    ui->requiredPkSpb->setValue(info.requiredPk);
}

void SetPointTargetEditDialog::setDialogTitle(const QString &title)
{
    // 提供给外部按“新增/编辑”场景动态设置窗口标题
    setWindowTitle(title);
}

void SetPointTargetEditDialog::onAcceptClicked()
{
    // 最小必填校验：目标编号
    if (ui->targetIdEdit->text().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入点目标编号。"));
        return;
    }

    // 最小必填校验：目标名称
    if (ui->targetNameCombo->currentText().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请选择目标名称。"));
        return;
    }

    // 业务约束：频段下限不得高于上限
    if (ui->bandLowerSpb->value() > ui->bandUpperSpb->value())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("频段下限不能大于频段上限。"));
        return;
    }
    // 校验通过后关闭对话框并返回Accepted结果
    accept();
}

void SetPointTargetEditDialog::initObject()
{
    // 默认标题用于新增场景；编辑场景可由setDialogTitle覆盖
    setWindowTitle(QStringLiteral("新增点目标"));

    // 目标名称来自外部任务上下文，当前对话框仅负责选择，不允许自由输入
    ui->targetNameCombo->setEditable(false);

    // 点目标类型：按雷达/节点/设备类型组织，供数据层枚举映射使用
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

    // 威胁等级：用于任务排序、展示与后续策略评估
    ui->threatLevelCombo->addItems(QStringList()
                                   << QStringLiteral("高")
                                   << QStringLiteral("中")
                                   << QStringLiteral("低"));

    // 优先级：当前使用P1-P3，保持与任务规划页一致
    ui->priorityCombo->addItems(QStringList() << QStringLiteral("P1")
                                              << QStringLiteral("P2")
                                              << QStringLiteral("P3"));
}

void SetPointTargetEditDialog::initConnect()
{
    // “确定”先走校验，再决定是否accept
    connect(ui->confirmBtn, &QPushButton::clicked, this, &SetPointTargetEditDialog::onAcceptClicked);

    // “取消/关闭”均直接reject，避免产生半完成数据
    connect(ui->cancelBtn, &QPushButton::clicked, this, &SetPointTargetEditDialog::reject);
    connect(ui->closeBtn, &QPushButton::clicked, this, &SetPointTargetEditDialog::reject);
}
