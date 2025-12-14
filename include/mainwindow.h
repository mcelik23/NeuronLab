#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "neuralnetwork.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // --- Button Actions ---
    void on_btnCreate_clicked();
    void on_btnTrain_clicked();
    void on_btnTest_clicked();
    void on_btnReset_clicked();

    // --- Configuration Changes ---
    void on_cmbMode_currentIndexChanged(int index);
    void on_spinCurrentClass_valueChanged(int arg1);

private:
    Ui::MainWindow *ui;
    NeuralNetwork *network;

    // State Flags
    bool isTraining;
    bool hasTrained;

    // Internal Helpers
    void updateUIForMode();
};
#endif // MAINWINDOW_H
