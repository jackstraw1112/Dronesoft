#include "SetPointTargetEditDialog.h"
#include "TaskPlanningTypeConvert.h"
#include "ui_SetPointTargetEditDialog.h"

#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>

// 构造函数：初始化UI并完成控件内容与信号绑定
SetPointTargetEditDialog::SetPointTargetEditDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::SetPointTargetEditDialog)
{
    // 由uic生成的界面对象完成控件创建与布局装配
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
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
    info.targetId.clear();
    info.name = ui->cmbTargetName->currentText().trimmed();

    // 界面中文枚举值转换为内部强类型枚举，便于后续逻辑判断
    info.type = targetTypeFromChinese(ui->cmbTargetType->currentText());
    info.threatLevel = threatLevelTypeFromChinese(ui->cmbThreatLevel->currentText());

    // 坐标直接读取数值控件
    info.latitude = ui->spbLatitude->value();
    info.longitude = ui->spbLongitude->value();
    // 频段在UI上拆分为上下限，落库时合并为统一展示字符串
    info.band = QStringLiteral("%1~%2 MHz")
                        .arg(ui->spbBandLower->value(), 0, 'f', 2)
                        .arg(ui->spbBandUpper->value(), 0, 'f', 2);

    // 当前版本保留字符串字段（后续可按需要再升级为枚举）
    info.emissionPattern = ui->leEmissionPattern->text().trimmed();
    info.priority = ui->cmbPriority->currentText().trimmed();
    info.requiredPk = ui->spbRequiredPk->value();
    return info;
}

void SetPointTargetEditDialog::setTargetNameOptions(const QStringList &targetNames)
{
    // 记录当前选中项，更新候选列表后尽量恢复用户已选内容
    const QString currentText = ui->cmbTargetName->currentText().trimmed();
    ui->cmbTargetName->clear();
    ui->cmbTargetName->addItems(targetNames);
    if (!currentText.isEmpty())
    {
        // setCurrentText在可匹配时恢复原值，不匹配则保持默认行为
        ui->cmbTargetName->setCurrentText(currentText);
    }
}

void SetPointTargetEditDialog::setPointTargetInfo(const PointTargetInfo &info)
{
    // 回填名称；若名称不在选项中，先补充后再选中
    if (ui->cmbTargetName->findText(info.name) < 0)
    {
        ui->cmbTargetName->addItem(info.name);
    }
    ui->cmbTargetName->setCurrentText(info.name);

    // 强类型枚举还原为中文，保证编辑态显示与新增态一致
    ui->cmbTargetType->setCurrentText(targetTypeToChinese(info.type));
    ui->cmbThreatLevel->setCurrentText(threatLevelTypeToChinese(info.threatLevel));

    // 数值类字段直接回填
    ui->spbLatitude->setValue(info.latitude);
    ui->spbLongitude->setValue(info.longitude);

    // band采用”x~y MHz”解析；兼容大小写MHz及两端空格
    const QRegularExpression bandPattern(QStringLiteral("([\\d\\.]+)\\s*~\\s*([\\d\\.]+)\\s*MHz"),
                                         QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch bandMatch = bandPattern.match(info.band);
    if (bandMatch.hasMatch())
    {
        // 解析成功时分别回填上下限；解析失败则保留控件默认值
        ui->spbBandLower->setValue(bandMatch.captured(1).toDouble());
        ui->spbBandUpper->setValue(bandMatch.captured(2).toDouble());
    }

    ui->leEmissionPattern->setText(info.emissionPattern);
    // 兼容旧格式（"P1"）和新格式（"P1 · 优先打击"）
    if (ui->cmbPriority->findText(info.priority) >= 0)
    {
        ui->cmbPriority->setCurrentText(info.priority);
    }
    else
    {
        for (int i = 0; i < ui->cmbPriority->count(); ++i)
        {
            if (ui->cmbPriority->itemText(i).startsWith(info.priority.left(2)))
            {
                ui->cmbPriority->setCurrentIndex(i);
                break;
            }
        }
    }
    ui->spbRequiredPk->setValue(info.requiredPk);
}

void SetPointTargetEditDialog::setDialogTitle(const QString &title)
{
    // 提供给外部按“新增/编辑”场景动态设置窗口标题
    setWindowTitle(title);
}

void SetPointTargetEditDialog::onAcceptClicked()
{
    // 最小必填校验：目标名称
    if (ui->cmbTargetName->currentText().trimmed().isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("提示"), QStringLiteral("请输入或选择目标名称。"));
        return;
    }

    // 业务约束：频段下限不得高于上限
    if (ui->spbBandLower->value() > ui->spbBandUpper->value())
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

    // 目标名称支持“选择已有 + 手动输入”，兼容空列表时的首次录入场景。
    ui->cmbTargetName->setEditable(true);
    if (ui->cmbTargetName->lineEdit())
    {
        ui->cmbTargetName->lineEdit()->setPlaceholderText(QStringLiteral("请输入或选择目标名称"));
    }
    // 表单左右两列等比拉伸，窗口横向变化时两侧同步扩展。
    ui->formLy->setColumnStretch(0, 1);
    ui->formLy->setColumnStretch(1, 1);

    // 字段标签统一美化：提高层级感，区分“标签区”和“输入区”。
    const QString fieldLabelStyle = QStringLiteral(
            "QLabel { color: #00b4ff; font-size: 10px; font-weight: 600; padding: 0 0 2px 2px; }");
    ui->targetNameLbl->setStyleSheet(fieldLabelStyle);
    ui->targetTypeLbl->setStyleSheet(fieldLabelStyle);
    ui->threatLevelLbl->setStyleSheet(fieldLabelStyle);
    ui->latitudeLbl->setStyleSheet(fieldLabelStyle);
    ui->longitudeLbl->setStyleSheet(fieldLabelStyle);
    ui->bandLowerLbl->setStyleSheet(fieldLabelStyle);
    ui->bandUpperLbl->setStyleSheet(fieldLabelStyle);
    ui->emissionPatternLbl->setStyleSheet(fieldLabelStyle);
    ui->lblPriority->setStyleSheet(fieldLabelStyle);
    ui->requiredPkLbl->setStyleSheet(fieldLabelStyle);

    ui->leEmissionPattern->setPlaceholderText(QStringLiteral("例如：间歇辐射"));

    // 点目标类型：按雷达/节点/设备类型组织，供数据层枚举映射使用
    ui->cmbTargetType->addItems(QStringList()
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
    ui->cmbThreatLevel->addItems(QStringList()
                                 << QStringLiteral("高")
                                 << QStringLiteral("中")
                                 << QStringLiteral("低"));

    // 优先级：决定目标打击顺序——紧急目标优先分配兵力与航路资源
    ui->cmbPriority->addItems(QStringList()
                              << QStringLiteral("P1 · 优先打击")
                              << QStringLiteral("P2 · 正常处理")
                              << QStringLiteral("P3 · 可延后"));
}

void SetPointTargetEditDialog::initConnect()
{
    // “确定”先走校验，再决定是否accept
    connect(ui->btnConfirm, &QPushButton::clicked, this, &SetPointTargetEditDialog::onAcceptClicked);

    // “取消”直接reject，避免产生半完成数据
    connect(ui->btnCancel, &QPushButton::clicked, this, &SetPointTargetEditDialog::reject);
}
