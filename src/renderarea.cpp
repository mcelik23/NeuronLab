#include "renderarea.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>

RenderArea::RenderArea(QWidget *parent)
    : QWidget(parent), isRegression(false), network(nullptr),
      currentClass(0), visualizeDecision(false), showNeuronLines(false), axisRange(10.0)
{
    // Set background color to system base color (usually white)
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

// --- COORDINATE TRANSFORMATION HELPERS ---

QPoint RenderArea::toScreen(double worldX, double worldY) {
    int w = width();
    int h = height();
    int centerX = w / 2;
    int centerY = h / 2;

    // Map world coordinates to screen pixels relative to center
    int px = centerX + (int)(worldX / axisRange * centerX);
    int py = centerY - (int)(worldY / axisRange * centerY); // Y-axis is inverted on screen
    return QPoint(px, py);
}

DataPoint RenderArea::toWorld(int screenX, int screenY) {
    int w = width();
    int h = height();
    int centerX = w / 2;
    int centerY = h / 2;

    // Map screen pixels back to world coordinates
    double wx = (double)(screenX - centerX) / centerX * axisRange;
    double wy = -(double)(screenY - centerY) / centerY * axisRange; // Invert Y-axis back
    return {wx, wy, 0};
}

QColor RenderArea::getClassColor(int classId) {
    // Define a palette for different classes
    const QColor colors[] = { Qt::red, Qt::blue, Qt::green, Qt::yellow, Qt::cyan, Qt::magenta };
    return colors[classId % 6];
}

// --- EVENTS ---

void RenderArea::mousePressEvent(QMouseEvent *event) {
    // Convert clicked pixel to data point
    DataPoint p = toWorld(event->x(), event->y());
    p.classID = currentClass;

    data.push_back(p);
    update(); // Trigger repaint
}

void RenderArea::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. Draw Grid and Axes
    drawGrid(painter);

    // 2. Draw Background Heatmap (Visualization Mode)
    if (visualizeDecision && network) {
        drawHeatmap(painter);
    }

    // 3. Draw Network Decisions (Lines or Curves)
    if (network && (showNeuronLines || isRegression) && !visualizeDecision) {
        if (isRegression) {
            drawRegressionCurve(painter);
        } else {
            drawNeuronLines(painter);
        }
    }

    // 4. Draw User Data Points
    drawDataPoints(painter);
}

// --- DRAWING IMPLEMENTATIONS ---

void RenderArea::drawGrid(QPainter &painter) {
    int w = width();
    int h = height();
    int centerX = w / 2;
    int centerY = h / 2;

    painter.setPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));

    // Calculate pixels per logical unit
    double pixelsPerUnit = (double)centerX / axisRange;

    for(int i = 0; i <= axisRange; i++) {
        int offset = i * pixelsPerUnit;

        // Vertical Grid Lines
        painter.drawLine(centerX + offset, 0, centerX + offset, h);
        painter.drawLine(centerX - offset, 0, centerX - offset, h);

        // Horizontal Grid Lines
        painter.drawLine(0, centerY + offset, w, centerY + offset);
        painter.drawLine(0, centerY - offset, w, centerY - offset);
    }

    // Main Axes (Black and Thicker)
    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(centerX, 0, centerX, h);      // Y Axis
    painter.drawLine(0, centerY, w, centerY);      // X Axis
}

void RenderArea::drawHeatmap(QPainter &painter) {
    int w = width();
    int h = height();
    int resolution = 6; // Lower value = Higher quality but slower performance

    for (int x = 0; x < w; x += resolution) {
        for (int y = 0; y < h; y += resolution) {

            // Pixel -> World
            DataPoint p = toWorld(x, y);

            // Normalize input for the network
            std::vector<double> input = { p.x / axisRange, p.y / axisRange };
            auto output = network->predict(input);

            if(output.empty()) continue;

            QColor c;
            if (isRegression) {
                // Regression Mode: Grayscale mapping
                // Map output from [-1, 1] to [0, 255]
                double val = (output[0] + 1.0) / 2.0 * 255.0;
                if(val < 0) val = 0; if(val > 255) val = 255;
                c = QColor((int)val, (int)val, (int)val);
            } else {
                // Classification Mode: Winner-Takes-All color
                int maxIdx = 0;
                double maxVal = output[0];
                for(size_t i = 1; i < output.size(); i++) {
                    if(output[i] > maxVal) { maxVal = output[i]; maxIdx = i; }
                }
                c = getClassColor(maxIdx);
            }
            c.setAlpha(60); // Semi-transparent
            painter.fillRect(x, y, resolution, resolution, c);
        }
    }
}

void RenderArea::drawRegressionCurve(QPainter &painter) {
    QPainterPath curvePath;
    bool firstPoint = true;
    int w = width();

    // Scan X-axis pixel by pixel
    for (int x = 0; x < w; x += 2) {
        DataPoint p = toWorld(x, 0); // Y is irrelevant for input scan

        std::vector<double> input = { p.x / axisRange };
        auto output = network->predict(input);

        if (!output.empty()) {
            double worldY = output[0] * axisRange; // Scale output back to world
            QPoint screenPt = toScreen(p.x, worldY);

            if (firstPoint) {
                curvePath.moveTo(screenPt);
                firstPoint = false;
            } else {
                curvePath.lineTo(screenPt);
            }
        }
    }
    painter.setPen(QPen(Qt::blue, 3));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(curvePath);
}

void RenderArea::drawNeuronLines(QPainter &painter) {
    // Only visualize the first layer weights (Single Layer Perceptron logic)
    if (!network || network->getLayerCount() == 0) return;

    int layerSize = network->getLayerSize(0);

    for(int i = 0; i < layerSize; i++) {
        // Retrieve weights
        double w1 = network->getWeight(0, i, 0);
        double w2 = network->getWeight(0, i, 1);
        double b  = network->getBias(0, i);

        // Avoid division by zero for vertical lines
        if(std::abs(w2) < 0.0001) continue;

        // Line Equation: w1*x + w2*y + b = 0  =>  y = (-w1*x - b) / w2
        double x1_world = -axisRange;
        double y1_world = (-w1 * (x1_world / axisRange) - b) * axisRange / w2;

        double x2_world = axisRange;
        double y2_world = (-w1 * (x2_world / axisRange) - b) * axisRange / w2;

        QPoint p1 = toScreen(x1_world, y1_world);
        QPoint p2 = toScreen(x2_world, y2_world);

        QColor neuronColor = getClassColor(i);
        QPen pen(neuronColor);
        pen.setWidth(2);

        painter.setPen(pen);
        painter.drawLine(p1, p2);
    }
}

void RenderArea::drawDataPoints(QPainter &painter) {
    for (const auto &p : data) {
        painter.setBrush(getClassColor(p.classID));
        painter.setPen(Qt::black);

        QPoint center = toScreen(p.x, p.y);
        painter.drawEllipse(center, 6, 6);
    }
}
