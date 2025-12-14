#ifndef RENDERAREA_H
#define RENDERAREA_H

#include <QWidget>
#include <vector>
#include "neuralnetwork.h"

struct DataPoint {
    double x; // World Coordinate X
    double y; // World Coordinate Y
    int classID;
};

class RenderArea : public QWidget {
    Q_OBJECT
public:
    explicit RenderArea(QWidget *parent = nullptr);

    // --- Configuration ---
    void setNetwork(NeuralNetwork *net) { network = net; }
    void setRegressionMode(bool active) { isRegression = active; update(); }
    void setCurrentClass(int c) { currentClass = c; }
    void setVisualizeMode(bool active) { visualizeDecision = active; update(); }
    void setShowLines(bool active) { showNeuronLines = active; update(); }

    // --- Data Management ---
    void clearData() { data.clear(); update(); }
    const std::vector<DataPoint>& getData() const { return data; }
    double getAxisRange() const { return axisRange; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    // --- Drawing Helpers ---
    void drawGrid(QPainter &painter);
    void drawDataPoints(QPainter &painter);
    void drawNeuronLines(QPainter &painter);      // Classification boundaries
    void drawRegressionCurve(QPainter &painter);  // Curve approximation
    void drawHeatmap(QPainter &painter);          // Decision boundary regions

    // --- Coordinate Transformations ---
    QPoint toScreen(double worldX, double worldY); // Map World -> Screen pixels
    DataPoint toWorld(int screenX, int screenY);   // Map Screen pixels -> World

    QColor getClassColor(int classId);

    // Internal State
    bool isRegression;
    NeuralNetwork *network;
    std::vector<DataPoint> data;
    int currentClass;
    bool visualizeDecision;
    bool showNeuronLines;
    double axisRange;
};

#endif // RENDERAREA_H
