#pragma once

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QTimer>


#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

class QLineEdit;
class QLabel;
class QPushButton;
class QComboBox;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    // ===== UI =====
    QLineEdit* m_urlEdit = nullptr;

    QLabel* m_currentVal = nullptr;
    QLabel* m_currentTs  = nullptr;

    // raw period
    QComboBox* m_rawPeriodBox = nullptr;
    QPushButton* m_updateBtn = nullptr;

    // stats labels
    QLabel* m_rawAvg=nullptr; QLabel* m_rawMin=nullptr; QLabel* m_rawMax=nullptr; QLabel* m_rawCnt=nullptr;
    QLabel* m_hAvg=nullptr;   QLabel* m_hMin=nullptr;   QLabel* m_hMax=nullptr;   QLabel* m_hCnt=nullptr;
    QLabel* m_dAvg=nullptr;   QLabel* m_dMin=nullptr;   QLabel* m_dMax=nullptr;   QLabel* m_dCnt=nullptr;

    // charts
    QLineSeries* m_rawSeries=nullptr;
    QLineSeries* m_hourSeries=nullptr;
    QLineSeries* m_daySeries=nullptr;

    QChartView* m_rawChartView=nullptr;
    QChartView* m_hourChartView=nullptr;
    QChartView* m_dayChartView=nullptr;

    QDateTimeAxis* m_rawX=nullptr;  QValueAxis* m_rawY=nullptr;
    QDateTimeAxis* m_hourX=nullptr; QValueAxis* m_hourY=nullptr;
    QDateTimeAxis* m_dayX=nullptr;  QValueAxis* m_dayY=nullptr;

    // ===== timers =====
    QNetworkAccessManager m_net;
    QTimer m_tCurrent;
    QTimer m_tRaw;
    QTimer m_tHourly;
    QTimer m_tDaily;

    // ===== Helpers =====
    void setupUi();
    void setupCharts();

    QString baseUrl() const;
    qint64 nowUnix() const;
    qint64 rawPeriodSec() const;
    qint64 startOfYearUnix() const;

    void requestCurrent();

    void requestStats(const QString& kind, qint64 from, qint64 to,
                      QLabel* cnt, QLabel* mn, QLabel* mx, QLabel* av);

    void requestSeries(const QString& kind, qint64 from, qint64 to, int limit,
                       QLineSeries* series,
                       QDateTimeAxis* axX,
                       QValueAxis* axY);

    void refreshRaw();
    void refreshHourlyAll();
    void refreshDailyAll();
    void refreshAll();

    void setStatusText(const QString& s);

    // JSON parse
    static bool parseCurrent(const QByteArray& body, double& value, qint64& ts);
    static bool parseStats(const QByteArray& body, long long& count, double& minv, double& maxv, double& avg);
    static bool parseSeries(const QByteArray& body, QVector<QPointF>& pts);
};
