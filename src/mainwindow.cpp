#include "mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QStatusBar>

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>


MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    setupCharts();

    connect(&m_tCurrent, &QTimer::timeout, this, &MainWindow::requestCurrent);
    connect(&m_tRaw, &QTimer::timeout, this, &MainWindow::refreshRaw);
    connect(&m_tHourly, &QTimer::timeout, this, &MainWindow::refreshHourlyAll);
    connect(&m_tDaily, &QTimer::timeout, this, &MainWindow::refreshDailyAll);

    connect(m_updateBtn, &QPushButton::clicked, this, &MainWindow::refreshAll);
    connect(m_rawPeriodBox, &QComboBox::currentIndexChanged, this, &MainWindow::refreshRaw);

    m_tCurrent.start(2000);
    m_tRaw.start(5000);
    m_tHourly.start(15000);
    m_tDaily.start(30000);

    refreshAll();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* root = new QVBoxLayout(central);

    auto* top = new QHBoxLayout();

    auto* serverBox = new QGroupBox("Server", central);
    auto* sLay = new QFormLayout(serverBox);
    m_urlEdit = new QLineEdit("http://127.0.0.1:8080", serverBox);
    sLay->addRow("Base URL:", m_urlEdit);
    top->addWidget(serverBox, 2);

    auto* curBox = new QGroupBox("Current", central);
    auto* cLay = new QFormLayout(curBox);
    m_currentVal = new QLabel("—", curBox);
    m_currentTs  = new QLabel("—", curBox);
    cLay->addRow("Value:", m_currentVal);
    cLay->addRow("Time:",  m_currentTs);
    top->addWidget(curBox, 2);

    auto* ctlBox = new QGroupBox("Controls", central);
    auto* ctlLay = new QFormLayout(ctlBox);
    m_rawPeriodBox = new QComboBox(ctlBox);
    m_rawPeriodBox->addItem("RAW 5 minutes", 300);
    m_rawPeriodBox->addItem("RAW 15 minutes", 900);
    m_rawPeriodBox->addItem("RAW 1 hour", 3600);
    m_rawPeriodBox->addItem("RAW 24 hours", 86400);
    m_rawPeriodBox->addItem("RAW 7 days", 7LL*86400);
    m_rawPeriodBox->setCurrentIndex(2);

    m_updateBtn = new QPushButton("Update all", ctlBox);
    ctlLay->addRow("Raw period:", m_rawPeriodBox);
    ctlLay->addRow("", m_updateBtn);

    top->addWidget(ctlBox, 2);

    root->addLayout(top);

    // stats row
    auto* stRow = new QHBoxLayout();

    auto makeStatsBox = [&](const QString& title, QLabel*& cnt, QLabel*& mn, QLabel*& mx, QLabel*& av) {
        auto* box = new QGroupBox(title, central);
        auto* lay = new QFormLayout(box);
        cnt = new QLabel("—", box);
        mn  = new QLabel("—", box);
        mx  = new QLabel("—", box);
        av  = new QLabel("—", box);
        lay->addRow("count:", cnt);
        lay->addRow("min:", mn);
        lay->addRow("max:", mx);
        lay->addRow("avg:", av);
        return box;
    };

    stRow->addWidget(makeStatsBox("RAW stats",    m_rawCnt, m_rawMin, m_rawMax, m_rawAvg), 1);
    stRow->addWidget(makeStatsBox("HOURLY stats", m_hCnt,   m_hMin,   m_hMax,   m_hAvg),   1);
    stRow->addWidget(makeStatsBox("DAILY stats",  m_dCnt,   m_dMin,   m_dMax,   m_dAvg),   1);

    root->addLayout(stRow);

    // графики
    auto* chartsRow = new QHBoxLayout();
    m_rawChartView  = new QChartView(new QChart(), central);
    m_hourChartView = new QChartView(new QChart(), central);
    m_dayChartView  = new QChartView(new QChart(), central);

    m_rawChartView->setMinimumHeight(320);
    m_hourChartView->setMinimumHeight(320);
    m_dayChartView->setMinimumHeight(320);

    chartsRow->addWidget(m_rawChartView, 1);
    chartsRow->addWidget(m_hourChartView, 1);
    chartsRow->addWidget(m_dayChartView, 1);

    root->addLayout(chartsRow, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");
}

void MainWindow::setupCharts() {
    auto setupOne = [&](QChartView* view, const QString& title,
                        QLineSeries*& series, QDateTimeAxis*& axX, QValueAxis*& axY) {
        QChart* ch = view->chart();
        ch->setTitle(title);
        ch->legend()->setVisible(false);

        series = new QLineSeries(ch);
        ch->addSeries(series);

        axX = new QDateTimeAxis(ch);
        axX->setFormat("MM-dd HH:mm");
        axX->setTitleText("Time");
        ch->addAxis(axX, Qt::AlignBottom);
        series->attachAxis(axX);

        axY = new QValueAxis(ch);
        axY->setTitleText("°C");
        ch->addAxis(axY, Qt::AlignLeft);
        series->attachAxis(axY);
    };

    setupOne(m_rawChartView,  "RAW",   m_rawSeries,  m_rawX,  m_rawY);
    setupOne(m_hourChartView, "HOURLY",m_hourSeries, m_hourX, m_hourY);
    setupOne(m_dayChartView,  "DAILY", m_daySeries,  m_dayX,  m_dayY);
}

QString MainWindow::baseUrl() const {
    QString s = m_urlEdit->text().trimmed();
    if (s.endsWith('/')) s.chop(1);
    return s;
}

qint64 MainWindow::nowUnix() const {
    return QDateTime::currentSecsSinceEpoch();
}

qint64 MainWindow::rawPeriodSec() const {
    return m_rawPeriodBox->currentData().toLongLong();
}

qint64 MainWindow::startOfYearUnix() const {
    auto now = QDateTime::currentDateTime();
    QDate d(now.date().year(), 1, 1);
    QDateTime dt(d, QTime(0,0,0));
    return dt.toSecsSinceEpoch();
}

void MainWindow::setStatusText(const QString& s) {
    statusBar()->showMessage(s, 4000);
}

// ===== Requests =====
void MainWindow::requestCurrent() {
    QUrl url(baseUrl() + "/api/current");
    auto* rep = m_net.get(QNetworkRequest(url));
    connect(rep, &QNetworkReply::finished, this, [this, rep]{
        QByteArray body = rep->readAll();
        auto err = rep->errorString();
        bool okNet = (rep->error() == QNetworkReply::NoError);
        rep->deleteLater();

        if (!okNet) { setStatusText("current failed: " + err); return; }

        double v=0; qint64 ts=0;
        if (!parseCurrent(body, v, ts)) { setStatusText("bad JSON /api/current"); return; }

        m_currentVal->setText(QString::number(v, 'f', 3) + " °C");
        m_currentTs->setText(QDateTime::fromSecsSinceEpoch(ts).toString("yyyy-MM-dd HH:mm:ss"));
    });
}

void MainWindow::requestStats(const QString& kind, qint64 from, qint64 to,
                             QLabel* cnt, QLabel* mn, QLabel* mx, QLabel* av) {
    QUrl url(baseUrl() + "/api/stats");
    QUrlQuery q;
    q.addQueryItem("kind", kind);
    q.addQueryItem("from", QString::number(from));
    q.addQueryItem("to",   QString::number(to));
    url.setQuery(q);

    auto* rep = m_net.get(QNetworkRequest(url));
    connect(rep, &QNetworkReply::finished, this, [this, rep, cnt, mn, mx, av]{
        QByteArray body = rep->readAll();
        auto err = rep->errorString();
        bool okNet = (rep->error() == QNetworkReply::NoError);
        rep->deleteLater();

        if (!okNet) { setStatusText("stats failed: " + err); return; }

        long long c=0; double mi=0, ma=0, a=0;
        if (!parseStats(body, c, mi, ma, a)) {
            // если нет данных, оставим —
            cnt->setText("0"); mn->setText("—"); mx->setText("—"); av->setText("—");
            return;
        }
        cnt->setText(QString::number(c));
        mn->setText(QString::number(mi, 'f', 3));
        mx->setText(QString::number(ma, 'f', 3));
        av->setText(QString::number(a,  'f', 3));
    });
}

void MainWindow::requestSeries(const QString& kind, qint64 from, qint64 to, int limit,
                              QLineSeries* series, QDateTimeAxis* axX, QValueAxis* axY) {
    QUrl url(baseUrl() + "/api/series");
    QUrlQuery q;
    q.addQueryItem("kind", kind);
    q.addQueryItem("from", QString::number(from));
    q.addQueryItem("to",   QString::number(to));
    q.addQueryItem("limit",QString::number(limit));
    url.setQuery(q);

    auto* rep = m_net.get(QNetworkRequest(url));
    connect(rep, &QNetworkReply::finished, this, [this, rep, series, axX, axY, from, to]{
        QByteArray body = rep->readAll();
        auto err = rep->errorString();
        bool okNet = (rep->error() == QNetworkReply::NoError);
        rep->deleteLater();

        if (!okNet) { setStatusText("series failed: " + err); return; }

        QVector<QPointF> pts;
        if (!parseSeries(body, pts)) {
            series->clear();
            return;
        }

        series->clear();

        double ymin=0, ymax=0;
        bool inited=false;

        for (const auto& p : pts) {
            // QDateTimeAxis ожидает X в миллисекундах
            series->append(p.x() * 1000.0, p.y());
            if (!inited) { ymin=ymax=p.y(); inited=true; }
            else { ymin = std::min(ymin, p.y()); ymax = std::max(ymax, p.y()); }
        }

        axX->setRange(QDateTime::fromSecsSinceEpoch(from), QDateTime::fromSecsSinceEpoch(to));

        if (!inited) { ymin=0; ymax=1; }
        if (ymin == ymax) { ymin -= 1; ymax += 1; }
        axY->setRange(ymin, ymax);
    });
}

// ===== Refresh blocks =====
void MainWindow::refreshRaw() {
    qint64 to = nowUnix();
    qint64 from = to - rawPeriodSec();

    requestStats("raw", from, to, m_rawCnt, m_rawMin, m_rawMax, m_rawAvg);
    requestSeries("raw", from, to, 2000, m_rawSeries, m_rawX, m_rawY);
}

void MainWindow::refreshHourlyAll() {
    qint64 to = nowUnix();
    qint64 from = to - 60 * 24 * 3600;
    requestStats("hourly", from, to, m_hCnt, m_hMin, m_hMax, m_hAvg);
    requestSeries("hourly", from, to, 2000, m_hourSeries, m_hourX, m_hourY);
}

void MainWindow::refreshDailyAll() {
    qint64 to = nowUnix();
    qint64 from = startOfYearUnix();
    requestStats("daily", from, to, m_dCnt, m_dMin, m_dMax, m_dAvg);
    requestSeries("daily", from, to, 2000, m_daySeries, m_dayX, m_dayY);
}

void MainWindow::refreshAll() {
    requestCurrent();
    refreshRaw();
    refreshHourlyAll();
    refreshDailyAll();
}

// ===== JSON parsing =====
bool MainWindow::parseCurrent(const QByteArray& body, double& value, qint64& ts) {
    auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return false;
    auto o = doc.object();
    if (!o.value("ok").toBool(false)) return false;
    value = o.value("value").toDouble();
    ts = (qint64)o.value("ts").toDouble();
    return true;
}

bool MainWindow::parseStats(const QByteArray& body, long long& count, double& minv, double& maxv, double& avg) {
    auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return false;
    auto o = doc.object();
    if (!o.value("ok").toBool(false)) return false;
    count = (long long)o.value("count").toDouble();
    minv  = o.value("min").toDouble();
    maxv  = o.value("max").toDouble();
    avg   = o.value("avg").toDouble();
    return true;
}

bool MainWindow::parseSeries(const QByteArray& body, QVector<QPointF>& pts) {
    pts.clear();
    auto doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) return false;
    auto o = doc.object();
    if (!o.value("ok").toBool(false)) return false;

    auto arr = o.value("points").toArray();
    pts.reserve(arr.size());
    for (const auto& v : arr) {
        auto po = v.toObject();
        double ts = po.value("ts").toDouble();
        double val = po.value("value").toDouble();
        pts.push_back(QPointF(ts, val));
    }
    return true;
}
