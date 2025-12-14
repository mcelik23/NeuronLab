#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , network(nullptr)
    , isTraining(false)
    , hasTrained(false)
{
    ui->setupUi(this);

    // Initialize RenderArea
    ui->renderArea->setNetwork(nullptr);

    // Sync UI with the default selection defined in Designer
    updateUIForMode();
}

MainWindow::~MainWindow() {
    // Memory Cleanup
    if(network) delete network;
    delete ui;
}

void MainWindow::updateUIForMode() {
    int modeIdx = ui->cmbMode->currentIndex();

    // Mode Analysis
    // 0: Single Class, 1: Single Reg, 2: Multi Class, 3: Multi Reg
    bool isMulti = (modeIdx >= 2);
    bool isRegression = (modeIdx % 2 != 0);

    // 1. Hidden Layer Configuration
    // Only enabled for Multi-Layer modes
    ui->spinHiddenLayers->setEnabled(isMulti);
    ui->spinNeurons->setEnabled(isMulti);

    // 2. Activation Function
    // Regression typically requires specific activations (Linear/Tanh), user selection disabled
    ui->cmbActivation->setEnabled(!isRegression);

    // 3. Output Layer (Class Count)
    // Fixed to 1 for Regression tasks
    ui->spinOutputLayer->setEnabled(!isRegression);

    // 4. Current Class Selection
    // No class distinction in Regression
    ui->spinCurrentClass->setEnabled(!isRegression);

    // --- UX Improvement ---
    // Auto-set sensible defaults when switching to Regression to prevent confusion
    if(isRegression) {
        ui->spinOutputLayer->setValue(1);  // Output: 1 (Y value)
        ui->spinCurrentClass->setValue(0); // Default class 0
    }
}

void MainWindow::on_cmbMode_currentIndexChanged(int index) {
    Q_UNUSED(index);
    updateUIForMode();
}

void MainWindow::on_spinCurrentClass_valueChanged(int arg1) {
    // Update drawing color based on selected class
    ui->renderArea->setCurrentClass(arg1);
}

void MainWindow::on_btnCreate_clicked() {
    // 1. Garbage Collection: Delete existing network

    ui->widgetErrorGraph->clear();

    if(network) {
         delete network;
         network = nullptr;
    }

    // 2. Instantiate New Network
    network = new NeuralNetwork();
    ui->renderArea->setNetwork(network);

    // Parse UI Configuration
    int modeIdx = ui->cmbMode->currentIndex();
    bool isMulti = (modeIdx >= 2);
    bool isRegression = (modeIdx % 2 != 0);

    int hiddenLayers = isMulti ? ui->spinHiddenLayers->value() : 0;
    int neuronsPerLayer = ui->spinNeurons->value();

    // Input Size: 1 for Regression (X -> Y), 2 for Classification (X,Y -> Class)
    int inputSize = isRegression ? 1 : 2;
    int outputSize = isRegression ? 1 : ui->spinOutputLayer->value();
    if(outputSize < 1) outputSize = 1;

    // Determine Task and Activation
    ActivationType act;
    TaskMode task;

    if (isRegression) {
        task = TaskMode::REGRESSION;
        ui->renderArea->setRegressionMode(true);
        act = ActivationType::TANH; // Preferred for regression in this context
    } else {
        task = TaskMode::CLASSIFICATION;
        ui->renderArea->setRegressionMode(false);

        QString userChoice = ui->cmbActivation->currentText();
        act = (userChoice == "TANH") ? ActivationType::TANH : ActivationType::SIGMOID;
    }

    // Initialize Network Architecture
    network->setup(inputSize, hiddenLayers, neuronsPerLayer, outputSize, act, task);

    // Update UI State
    hasTrained = false;
    ui->btnTrain->setText("Start Training");
    ui->btnTrain->setEnabled(true);
    ui->btnTest->setEnabled(false);
    ui->lblError->setText("Network Created. Ready.");
}

void MainWindow::on_btnReset_clicked() {
    isTraining = false;


    // 1. Destroy Network
    if(network) {
        delete network;
        network = nullptr;
    }

    ui->widgetErrorGraph->clear();

    // 2. Reset RenderArea
    ui->renderArea->setNetwork(nullptr);
    ui->renderArea->clearData();
    ui->renderArea->setVisualizeMode(false);
    ui->renderArea->setShowLines(false);

    // 3. Reset UI Controls
    ui->lblError->setText("Network Deleted. Create new one.");
    ui->lblEpoch->setText("Epoch: 0");
    ui->btnTrain->setEnabled(false);
    ui->btnTest->setEnabled(false);
    hasTrained = false;
    ui->btnTrain->setText("Start Training");
}

void MainWindow::on_btnTrain_clicked() {
    if(!network) return;

    // Handle Pause/Resume Logic
    if(isTraining) {
        isTraining = false;
        ui->btnTrain->setText("Resume Training");
        hasTrained = true;
        return;
    }

    // Data Validation
    auto data = ui->renderArea->getData();
    double range = ui->renderArea->getAxisRange();

    if(data.empty()) {
        ui->lblError->setText("No data points!");
        return;
    }

    // Start Training Loop
    isTraining = true;
    ui->btnTrain->setText("Stop Training");
    ui->btnTest->setEnabled(false);

    int modeIdx = ui->cmbMode->currentIndex();
    bool isRegression = (modeIdx % 2 != 0);

    // Visualization Setup
    ui->renderArea->setShowLines(true);
    ui->renderArea->setVisualizeMode(false);

    int maxEpochs = ui->spinMaxEpochs->value();
    double lr = ui->spinLR->value();
    int outputSize = ui->spinOutputLayer->value();

    int drawInterval = 1; // Update UI every epoch

    // Target Value Setup (Tanh: -1..1, Sigmoid: 0..1)
    QString actText = ui->cmbActivation->currentText();
    double targetMin = (actText == "TANH") ? -1.0 : 0.0;
    double targetMax = 1.0;

    // --- MAIN TRAINING LOOP ---
    for(int epoch = 0; epoch < maxEpochs && isTraining; epoch++) {
        double epochError = 0;

        for(const auto &p : data) {
            std::vector<double> input;
            std::vector<double> target;

            if(isRegression) {
                // Regression: Input X -> Target Y
                input.push_back(p.x / range);
                target.push_back(p.y / range);
            } else {
                // Classification: Input (X,Y) -> One-Hot Target
                input = { p.x / range, p.y / range };

                // Initialize target vector with min value (e.g., -1 or 0)
                target.resize(outputSize, targetMin);

                // Set the correct class index to max value (1.0)
                if(p.classID < outputSize) target[p.classID] = targetMax;
            }

            // Train on single sample (Stochastic Gradient Descent)
            epochError += network->train(input, target, lr);
        }

        // UI Updates (Real-time)
        if(epoch % drawInterval == 0) {
            ui->lblEpoch->setText(QString("Epoch: %1").arg(epoch));
            ui->lblError->setText(QString("Error: %1").arg(epochError));
            ui->widgetErrorGraph->addError(epochError);
            ui->renderArea->update();
            QApplication::processEvents(); // Keep UI responsive
        }
    }

    // Training Finished or Paused
    isTraining = false;
    hasTrained = true;
    ui->btnTrain->setText("Resume Training");

    // Enable Test button only for Classification
    if (isRegression) {
        ui->btnTest->setEnabled(false);
    } else {
        ui->btnTest->setEnabled(true);
    }
}

void MainWindow::on_btnTest_clicked() {
    // Switch to Visualization/Heatmap Mode
    ui->renderArea->setShowLines(false);
    ui->renderArea->setVisualizeMode(true);
}
