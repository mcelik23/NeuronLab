#ifndef ERRORGRAPH_H
#define ERRORGRAPH_H

#include <QWidget>
#include <vector>

class ErrorGraph : public QWidget {
    Q_OBJECT
public:
    explicit ErrorGraph(QWidget *parent = nullptr);

    // Adds a new error value to the history and refreshes the graph
    void addError(double err);

    // Clears the graph history
    void clear();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    std::vector<double> errors;
    double maxError; // Caches the maximum error for scaling
};

#endif // ERRORGRAPH_H
