#ifndef PATHDISPLAYDIALOG_H
#define PATHDISPLAYDIALOG_H

#include <QDialog>
#include "TaskPlanningData.h"
#include "WindowStyle.h"
namespace Ui {
class PathDisplayDialog;
}

/**
 * @brief 路径显示对话框类
 * @details 用于显示无人机路径规划的详细信息
 */
class PathDisplayDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父窗口指针
     */
    explicit PathDisplayDialog(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~PathDisplayDialog();

    /**
        * @brief 初始化界面显示
    */
    void InitDisplay();

    /**
     * @brief 设置路径数据
     * @param path 路径规划信息
     */
    void setPathData(const PathPlanning& path);

private:
    Ui::PathDisplayDialog *ui;  // UI界面指针
};

#endif // PATHDISPLAYDIALOG_H
