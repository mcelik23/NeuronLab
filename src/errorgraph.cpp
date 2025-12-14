#include "errorgraph.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm> // For std::max

ErrorGraph::ErrorGraph(QWidget *parent) : QWidget(parent), maxError(1.0) {
    // Set a dark background color explicitly if needed,
    // though paintEvent handles the fill.
    setBackgroundRole(QPalette::Base);
}

void ErrorGraph::addError(double err) {
    errors.push_back(err);

    // Dynamically update maximum error for vertical scaling
    if (err > maxError) maxError = err;

    // Trigger a repaint
    update();
}

void ErrorGraph::clear() {
    errors.clear();
    maxError = 1.0; // Reset scale
    update();
}

void ErrorGraph::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. Draw Background
    painter.fillRect(rect(), QColor(30, 30, 30)); // Dark Grey/Black

    // 2. Draw Grid (Optional visual aid)
    painter.setPen(QPen(QColor(60, 60, 60), 1, Qt::DotLine));
    painter.drawLine(0, height()/2, width(), height()/2);
    painter.drawLine(0, height()/4, width(), height()/4);
    painter.drawLine(0, height()*3/4, width(), height()*3/4);

    if (errors.empty()) return;

    // 3. Prepare Graph Path
    // Use QPainterPath for smoother connection of points
    QPainterPath path;

    // Scale X: Width / Total Epochs
    // Scale Y: Height / Max Error
    double xStep = (double)width() / (errors.size() > 1 ? errors.size() - 1 : 1);

    // Safety check: Avoid division by zero if maxError is 0 (unlikely)
    double effectiveMax = (maxError > 0.00001) ? maxError : 1.0;

    for (size_t i = 0; i < errors.size(); ++i) {
        double x = i * xStep;

        // Invert Y because screen coordinates start from top-left
        double y = height() - (errors[i] / effectiveMax * height());

        // Add minimal margin so the line doesn't hit the absolute floor
        y = std::min(y, (double)height() - 2);

        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }

    // 4. Draw the Graph Line
    painter.setPen(QPen(Qt::green, 2)); // Green Matrix Style, 2px width
    painter.drawPath(path);

    // 5. Draw Current Error Value (Text)
    painter.setPen(Qt::white);
    QString text = QString::asprintf("Loss: %.5f", errors.back());
    painter.drawText(5, 20, text);
}
