#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QGraphicsPixmapItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsPathItem>
#include <QPainterPath>
#include <QTextStream>
#include <QMovie>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>

// 调试日志
static QFile logFile;
static void logInit() {
    logFile.setFileName("log.txt");
    logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
}
static void logMsg(const QString &msg) {
    if (logFile.isOpen()) {
        QTextStream ts(&logFile);
        ts << msg << "\n";
        ts.flush();
    }
    qDebug() << msg;
}

// 格子坐标 → 像素坐标
static inline qreal gridToPixelX(int col) { return 130.0 + col * 80.0; }
static inline qreal gridToPixelY(int row) { return 130.0 + row * 80.0; }

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    loadUnlockProgress();

    // ========== 顶部工具栏 ==========
    topBar = new QWidget(this);
    topBar->setFixedHeight(60);
    topBar->setStyleSheet("background-color: rgba(45,45,45,200); border-bottom: 2px solid #555;");

    QHBoxLayout *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(16, 0, 16, 0);

    // 金币：图标 + 数字
    goldIcon = new QLabel();
    goldIcon->setStyleSheet("background: transparent;");
    goldIcon->setPixmap(QPixmap(":/new/prefix1/images/gold_icon.png").scaled(42, 42, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    goldText = new QLabel(QString::number(gold));
    goldText->setStyleSheet("color: gold; font-size: 18px; font-weight: bold; background: transparent;");
    gold = 500;
    updateGoldDisplay();
    topLayout->addWidget(goldIcon);
    topLayout->addWidget(goldText);

    // 波数 —— 居中在地图第七列 (x=650)，与炮塔选择栏对齐
    topLayout->addStretch(6);
    waveText = new QLabel();
    waveText->setStyleSheet("color: white; font-size: 18px; font-weight: bold; background: transparent;");
    waveText->setFixedWidth(100);
    waveText->setAlignment(Qt::AlignCenter);
    topLayout->addWidget(waveText);
    topLayout->addStretch(5);

    // 倒计时（开场）
    countdownLabel = new QLabel(this);
    countdownLabel->setFixedSize(370, 370);
    countdownLabel->move((1220 - 370) / 2, (740 - 370) / 2 + 60);
    countdownLabel->setScaledContents(true);
    countdownLabel->hide();

    // 倍速键
    speedBtn = new QPushButton("2x");
    speedBtn->setFixedSize(48, 48);
    speedBtn->setStyleSheet("color: white; font-size: 14px; background: transparent; border: 1px solid #666; border-radius: 6px;");
    connect(speedBtn, &QPushButton::clicked, this, [this]() {
        doubleSpeed = !doubleSpeed;

        int sRem = spawnTimer->isActive() ? spawnTimer->remainingTime() : -1;
        int wRem = waveIntervalTimer->isActive() ? waveIntervalTimer->remainingTime() : -1;

        if (doubleSpeed) {
            speedBtn->setStyleSheet("color: #ffcc00; font-size: 14px; font-weight: bold; background: rgba(255,255,255,15); border: 1px solid #ffcc00; border-radius: 6px;");
            timerInterval = 16;
            spawnInterval = 500;
            waveInterval = 2500;
            if (sRem >= 0) { spawnTimer->stop(); spawnTimer->start(qMax(1, sRem / 2)); }
            if (wRem >= 0) { waveIntervalTimer->stop(); waveIntervalTimer->start(qMax(1, wRem / 2)); }
        } else {
            speedBtn->setStyleSheet("color: white; font-size: 14px; background: transparent; border: 1px solid #666; border-radius: 6px;");
            timerInterval = 32;
            spawnInterval = 1000;
            waveInterval = 5000;
            if (sRem >= 0) { spawnTimer->stop(); spawnTimer->start(sRem * 2); }
            if (wRem >= 0) { waveIntervalTimer->stop(); waveIntervalTimer->start(wRem * 2); }
        }
        if (timer->isActive()) { timer->stop(); timer->start(timerInterval); }
    });
    topLayout->addWidget(speedBtn);

    // 暂停键
    pauseBtn = new QPushButton();
    pauseBtn->setIcon(QIcon(":/new/prefix1/images/pause_icon.png"));
    pauseBtn->setIconSize(QSize(42, 42));
    pauseBtn->setFixedSize(48, 48);
    pauseBtn->setStyleSheet("background: #555; border-radius: 6px;");
    connect(pauseBtn, &QPushButton::clicked, this, &MainWindow::togglePause);
    topLayout->addWidget(pauseBtn);

    // 返回菜单按钮
    menuBtn = new QPushButton();
    menuBtn->setIcon(QIcon(":/new/prefix1/images/menu_icon.png"));
    menuBtn->setIconSize(QSize(42, 42));
    menuBtn->setFixedSize(48, 48);
    menuBtn->setStyleSheet("background: #555; border-radius: 6px;");
    menuBtn->setCursor(Qt::PointingHandCursor);
    connect(menuBtn, &QPushButton::clicked, this, &MainWindow::onMenuReturnClicked);
    topLayout->addWidget(menuBtn);

    // ========== 游戏场景（仅背景，关卡内容由 loadLevel 创建） ==========
    scene = new QGraphicsScene(this);
    scene->setSceneRect(0, 0, 1220, 740);
    scene->setItemIndexMethod(QGraphicsScene::NoIndex);
    scene->installEventFilter(this);

    view = new QGraphicsView(scene, this);
    view->setFrameStyle(0);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setRenderHint(QPainter::Antialiasing);
    view->setStyleSheet("background: transparent;");
    view->setViewportUpdateMode(QGraphicsView::BoundingRectViewportUpdate);

    // ========== 整体布局 ==========
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // 全窗口背景图 —— 左上角对齐窗口，遮挡预加载界面
    QLabel *windowBg = new QLabel(central);
    windowBg->setPixmap(QPixmap(":/new/prefix1/images/background.jpg")
        .scaled(1220, 832, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    windowBg->setGeometry(0, 0, 1220, 832);
    windowBg->lower();

    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(topBar);
    mainLayout->addWidget(view);

    // ========== 怪物列表 & 计时器 ==========
    mobList = new QList<BaseMob*>();

    timer = new QTimer(this);
    // 游戏启动后才开始计时
    connect(timer, &QTimer::timeout, this, [this]() {
        scene->advance();
        static int tickCount = 0;
        if (++tickCount <= 5) logMsg(QString("Timer tick %1, mobList size=%2").arg(tickCount).arg(mobList->size()));
        // 萝卜死了 → 游戏结束
        if (carrot && carrot->isDead() && !gameOver) {
            gameOver = true;
            timer->stop();
            spawnTimer->stop();
            waveIntervalTimer->stop();
            gameStarted = false;
            clearUpgradeHints();

            QWidget *go = new QWidget(this);
            go->setFixedSize(1220, 740);
            go->move(0, 60);
            go->setStyleSheet("background: rgba(0,0,0,160);");
            QVBoxLayout *goMain = new QVBoxLayout(go);
            goMain->setAlignment(Qt::AlignCenter);
            goMain->setContentsMargins(0, 0, 0, 40);

            QLabel *carrotLabel = new QLabel();
            carrotLabel->setPixmap(QPixmap(":/new/prefix1/images/carrot_damaged.png")
                .scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            carrotLabel->setStyleSheet("background: transparent;");
            carrotLabel->setAlignment(Qt::AlignCenter);
            goMain->addWidget(carrotLabel);

            goMain->addSpacing(10);

            QLabel *gifLabel = new QLabel();
            gifLabel->setStyleSheet("background: transparent;");
            QMovie *defeatMovie = new QMovie(":/new/prefix1/images/defeat_char.gif");
            defeatMovie->setScaledSize(QSize(420, 224));
            gifLabel->setMovie(defeatMovie);
            gifLabel->setAlignment(Qt::AlignCenter);
            defeatMovie->start();
            goMain->addWidget(gifLabel);

            QWidget *btnRow = new QWidget();
            btnRow->setAttribute(Qt::WA_TranslucentBackground, true);
            QHBoxLayout *btnLay = new QHBoxLayout(btnRow);
            btnLay->setAlignment(Qt::AlignCenter);
            btnLay->setSpacing(30);

            QPushButton *restartBtn = new QPushButton("重新开始");
            restartBtn->setFlat(true);
            restartBtn->setFixedSize(160, 50);
            restartBtn->setStyleSheet("color: white; font-size: 18px; background: transparent; border: 2px solid #e74c3c; border-radius: 10px;");
            connect(restartBtn, &QPushButton::clicked, this, [this, go]() {
                go->deleteLater();
                resetAndStart();
            });
            QPushButton *menuBtn2 = new QPushButton("返回菜单");
            menuBtn2->setFlat(true);
            menuBtn2->setFixedSize(160, 50);
            menuBtn2->setStyleSheet("color: white; font-size: 18px; background: transparent; border: 2px solid #aaa; border-radius: 10px;");
            connect(menuBtn2, &QPushButton::clicked, this, [this, go]() {
                go->deleteLater();
                clearGameObjects();
                showMenuPage();
            });

            btnLay->addWidget(restartBtn);
            btnLay->addWidget(menuBtn2);
            goMain->addWidget(btnRow);

            go->show();
            go->raise();
            pauseBtn->setEnabled(false);
            return;
        }
        // 清理已死的怪物
        for (int i = mobList->size() - 1; i >= 0; i--) {
            BaseMob *m = mobList->at(i);
            if (m->hp <= 0) {
                if (!m->reachedEnd) {
                    gold += goldPerType[m->mobType];
                    updateGoldDisplay();
                }
                scene->removeItem(m);
                delete m;
                mobList->removeAt(i);
            }
        }
        // 最后一波怪全灭且萝卜活着 → 立即胜利
        if (gameStarted && currentWaveIndex == totalWaves - 1
            && spawnIndex >= waves[currentWaveIndex].size()
            && mobList->isEmpty()
            && carrot && !carrot->isDead() && !gameOver) {
            gameOver = true;
            timer->stop();
            spawnTimer->stop();
            waveIntervalTimer->stop();
            gameStarted = false;
            clearUpgradeHints();
            unlockNextLevelIfQualified();
            QWidget *win = new QWidget(this);
            win->setFixedSize(1220, 740);
            win->move(0, 60);
            win->setStyleSheet("background: rgba(0,0,0,160);");
            QVBoxLayout *wlay = new QVBoxLayout(win);
            wlay->setAlignment(Qt::AlignCenter);
            wlay->setContentsMargins(0, 0, 0, 92);

            QLabel *wImg = new QLabel();
            wImg->setPixmap(QPixmap(":/new/prefix1/images/victory.png")
                .scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            wImg->setStyleSheet("background: transparent;");
            wImg->setAlignment(Qt::AlignCenter);
            wlay->addWidget(wImg);

            QLabel *wGif = new QLabel();
            wGif->setStyleSheet("background: transparent;");
            QMovie *victoryMovie = new QMovie(":/new/prefix1/images/victory_char.gif");
            victoryMovie->setScaledSize(QSize(366, 234));
            wGif->setMovie(victoryMovie);
            wGif->setAlignment(Qt::AlignCenter);
            victoryMovie->start();
            wlay->addWidget(wGif);

            wlay->addSpacing(10);

            QWidget *wBtnRow = new QWidget();
            wBtnRow->setAttribute(Qt::WA_TranslucentBackground, true);
            QHBoxLayout *wBtnLay = new QHBoxLayout(wBtnRow);
            wBtnLay->setAlignment(Qt::AlignCenter);
            wBtnLay->setSpacing(30);
            QPushButton *wRestartBtn = new QPushButton("重新开始");
            wRestartBtn->setFlat(true);
            wRestartBtn->setFixedSize(160, 50);
            wRestartBtn->setStyleSheet("color: white; font-size: 18px; background: transparent; border: 2px solid #2ecc71; border-radius: 10px;");
            connect(wRestartBtn, &QPushButton::clicked, this, [this, win]() {
                win->deleteLater();
                resetAndStart();
            });
            QPushButton *wMenuBtn = new QPushButton("返回菜单");
            wMenuBtn->setFlat(true);
            wMenuBtn->setFixedSize(160, 50);
            wMenuBtn->setStyleSheet("color: white; font-size: 18px; background: transparent; border: 2px solid #aaa; border-radius: 10px;");
            connect(wMenuBtn, &QPushButton::clicked, this, [this, win]() {
                win->deleteLater();
                clearGameObjects();
                showMenuPage();
            });
            wBtnLay->addWidget(wRestartBtn);
            wBtnLay->addWidget(wMenuBtn);
            wlay->addWidget(wBtnRow);

            win->show();
            win->raise();
            pauseBtn->setEnabled(false);
            return;
        }
        // 本波生完 + 场上无怪 → 5秒后下一波
        if (gameStarted && currentWaveIndex >= 0 && currentWaveIndex < totalWaves
            && spawnIndex >= waves[currentWaveIndex].size()
            && mobList->isEmpty()
            && !waveIntervalTimer->isActive()
            && !spawnTimer->isActive()) {
            waveIntervalTimer->start(waveInterval);
        }
    });

    spawnTimer = new QTimer(this);
    waveIntervalTimer = new QTimer(this);

    connect(spawnTimer, &QTimer::timeout, this, [this]() {
        spawnNextMonster();
    });

    connect(waveIntervalTimer, &QTimer::timeout, this, [this]() {
        waveIntervalTimer->stop();
        startNextWave();
    });

    // ========== 开始菜单页 ==========
    menuPage = new QWidget(this);
    menuPage->setFixedSize(1220, 832);
    menuPage->move(0, 0);
    menuPage->setStyleSheet("background: rgba(0,0,0,60);");

    QVBoxLayout *menuLayout = new QVBoxLayout(menuPage);
    menuLayout->setAlignment(Qt::AlignCenter);
    menuLayout->setContentsMargins(0, 0, 0, 0);
    menuLayout->setSpacing(40);

    menuTitle = new QLabel("保卫萝卜");
    menuTitle->setAlignment(Qt::AlignCenter);
    menuTitle->setStyleSheet(
        "font-size: 72px; font-weight: bold;"
        "color: #FF8C00;"
        "background: transparent;");
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
    shadow->setBlurRadius(12);
    shadow->setColor(QColor(0, 0, 0, 200));
    shadow->setOffset(3, 3);
    menuTitle->setGraphicsEffect(shadow);
    menuLayout->addWidget(menuTitle);

    menuLayout->addSpacing(40);

    QString btnStyle =
        "QPushButton {"
        "  color: white; font-size: 24px; font-weight: bold;"
        "  background: rgba(0, 0, 0, 160);"
        "  border: 2px solid rgba(255, 255, 255, 180);"
        "  border-radius: 14px; padding: 16px 0px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 140, 0, 180);"
        "  border-color: #FF8C00;"
        "}";
    playBtn = new QPushButton("选择关卡");
    playBtn->setFixedSize(260, 64);
    playBtn->setStyleSheet(btnStyle);
    playBtn->setCursor(Qt::PointingHandCursor);
    menuLayout->addWidget(playBtn, 0, Qt::AlignCenter);

    exitBtn = new QPushButton("退出游戏");
    exitBtn->setFixedSize(260, 64);
    exitBtn->setStyleSheet(btnStyle);
    exitBtn->setCursor(Qt::PointingHandCursor);
    menuLayout->addWidget(exitBtn, 0, Qt::AlignCenter);

    connect(playBtn, &QPushButton::clicked, this, &MainWindow::showLevelSelectPage);
    connect(exitBtn, &QPushButton::clicked, this, &QWidget::close);

    // ========== 选关页面 ==========
    levelSelectPage = new QWidget(this);
    levelSelectPage->setFixedSize(1220, 832);
    levelSelectPage->move(0, 0);
    levelSelectPage->setStyleSheet("background: rgba(0,0,0,60);");

    QVBoxLayout *lvlLayout = new QVBoxLayout(levelSelectPage);
    lvlLayout->setAlignment(Qt::AlignCenter);
    lvlLayout->setContentsMargins(0, 0, 0, 40);
    lvlLayout->setSpacing(24);

    QLabel *lvlTitle = new QLabel("选择关卡");
    lvlTitle->setAlignment(Qt::AlignCenter);
    lvlTitle->setStyleSheet(
        "font-size: 64px; font-weight: bold;"
        "color: #FF8C00;"
        "background: transparent;");
    QGraphicsDropShadowEffect *lvlShadow = new QGraphicsDropShadowEffect;
    lvlShadow->setBlurRadius(12);
    lvlShadow->setColor(QColor(0, 0, 0, 200));
    lvlShadow->setOffset(3, 3);
    lvlTitle->setGraphicsEffect(lvlShadow);
    lvlLayout->addWidget(lvlTitle);

    lvlLayout->addSpacing(30);

    // 关卡按钮：2×2 网格，加大尺寸
    QGridLayout *grid = new QGridLayout;
    grid->setSpacing(24);
    QString levelBtnStyle =
        "QPushButton {"
        "  color: white; font-size: 28px; font-weight: bold;"
        "  background: rgba(0, 0, 0, 160);"
        "  border: 2px solid rgba(255, 255, 255, 180);"
        "  border-radius: 16px; padding: 24px 0px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255, 140, 0, 180);"
        "  border-color: #FF8C00;"
        "}"
        "QPushButton[locked=\"true\"] {"
        "  color: rgba(255,255,255,120);"
        "  background: rgba(0,0,0,190);"
        "  border-color: rgba(255,255,255,80);"
        "}"
        "QPushButton[locked=\"true\"]:hover {"
        "  color: rgba(255,255,255,120);"
        "  background: rgba(0,0,0,190);"
        "  border-color: rgba(255,255,255,80);"
        "}";
    for (int i = 0; i < 6; i++) {
        QPushButton *btn = new QPushButton(QString("关卡 %1").arg(i + 1));
        btn->setFixedSize(270, 92);
        btn->setStyleSheet(levelBtnStyle);
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, i]() {
            if (i >= unlockedLevelCount) {
                QMessageBox::information(this, "关卡未解锁",
                    "需要上一关萝卜血量大于等于 8 点通关后才能解锁。");
                return;
            }
            loadLevel(i);
        });
        levelButtons.append(btn);
        grid->addWidget(btn, i / 3, i % 3);
    }
    updateLevelButtons();
    lvlLayout->addLayout(grid);
    lvlLayout->addSpacing(10);

    QPushButton *backBtn = new QPushButton("返回");
    backBtn->setFixedSize(200, 54);
    backBtn->setStyleSheet(
        "QPushButton {"
        "  color: rgba(255,255,255,200); font-size: 20px;"
        "  background: rgba(0,0,0,120);"
        "  border: 1px solid rgba(255,255,255,120);"
        "  border-radius: 12px; padding: 12px 0px;"
        "}"
        "QPushButton:hover {"
        "  background: rgba(255,255,255,30);"
        "  border-color: rgba(255,255,255,200);"
        "}");
    backBtn->setCursor(Qt::PointingHandCursor);
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::showMenuPage);
    lvlLayout->addWidget(backBtn, 0, Qt::AlignCenter);

    // 初始显示菜单页
    logInit();
    logMsg("Constructor done, showing menu");
    showMenuPage();
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ========== 关卡加载 ==========

void MainWindow::clearGameObjects()
{
    logMsg("clearGameObjects() START");
    // 先清理游戏运行时对象
    cleanupGame();

    // 清理地面格子
    for (auto *g : groundList) {
        scene->removeItem(g);
        delete g;
    }
    groundList.clear();

    clearRouteConnectors();

    // 清理萝卜
    if (carrot) {
        scene->removeItem(carrot);
        delete carrot;
        carrot = nullptr;
    }
    BaseMob::carrot = nullptr;

    // 清理侵入点图标
    for (auto *icon : spawnIconList) {
        scene->removeItem(icon);
        delete icon;
    }
    spawnIconList.clear();

    // 清理炮塔选择栏
    for (auto *item : towerBarItems) {
        scene->removeItem(item);
        delete item;
    }
    towerBarItems.clear();
    towerCards.clear();
    availableTowers.clear();

    // 清理侵入点数据
    spawnPoints.clear();
    waves.clear();
    logMsg("clearGameObjects() END");
}

void MainWindow::loadLevel(int idx)
{
    logMsg(QString("loadLevel(%1) START").arg(idx));
    clearGameObjects();
    currentLevel = idx;

    // 读取地图文件
    QString path = QString(":/new/prefix1/maps/map%1.txt").arg(idx);
    logMsg(QString("loadLevel: opening %1").arg(path));
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        // 文件打不开时用默认配置
        logMsg(QString("loadLevel: FAILED to open %1, using default").arg(path));
        qWarning() << "Cannot open" << path;
        QList<QList<int>> grid;
        for (int i = 0; i < 7; i++) {
            QList<int> rowVals;
            for (int j = 0; j < 13; j++) {
                int val = i != 3 ? 1 : 0;
                rowVals.append(val);
                ground *g = new ground(val == 0 ? 2 : 1);
                g->setPos(gridToPixelX(j), gridToPixelY(i));
                scene->addItem(g);
                groundList.append(g);
            }
            grid.append(rowVals);
        }
        carrot = new Carrot();
        carrot->setPos(gridToPixelX(12), gridToPixelY(3));
        carrot->setZValue(3);
        scene->addItem(carrot);
        BaseMob::carrot = carrot;
        SpawnPoint sp;
        sp.position = QPointF(gridToPixelX(0), gridToPixelY(3));
        Checkpoint cp;
        cp.pos = QPointF(gridToPixelX(12), gridToPixelY(3));
        cp.waitTime = 0;
        sp.checkpoints.append(cp);
        spawnPoints.append(sp);
        buildRouteConnectors();
        totalWaves = 1;
        QList<MobSpawnInfo> defaultWave;
        for (int m = 0; m < 5; m++)
            defaultWave.append({0, 1, 50});
        waves.append(defaultWave);
        updateWaveDisplay();
    } else {
        QTextStream in(&file);
        logMsg("loadLevel: file opened, parsing grid...");

        // 0. 解析金币奖励
        QString goldLine = in.readLine();
        QStringList goldParts = goldLine.split(" ", Qt::SkipEmptyParts);
        if (goldParts.size() >= 3) {
            goldPerType[1] = goldParts[0].toInt();
            goldPerType[2] = goldParts[1].toInt();
            goldPerType[3] = goldParts[2].toInt();
        }

        // 1. 解析 7×13 网格
        QList<QList<int>> grid;
        for (int row = 0; row < 7; row++) {
            QString line = in.readLine();
            QStringList cells = line.split(" ", Qt::SkipEmptyParts);
            QList<int> rowVals;
            for (int col = 0; col < 13 && col < cells.size(); col++) {
                int val = cells[col].toInt();
                rowVals.append(val);
                ground *g = new ground(val == 0 ? 2 : 1);
                g->setPos(gridToPixelX(col), gridToPixelY(row));
                scene->addItem(g);
                groundList.append(g);
            }
            grid.append(rowVals);
        }
        logMsg("loadLevel: grid done, parsing carrot...");
        // 2. 解析萝卜坐标
        QString carrotLine = in.readLine();
        QStringList carrotParts = carrotLine.split(" ", Qt::SkipEmptyParts);
        int carrotRow = carrotParts.value(0).toInt();
        int carrotCol = carrotParts.value(1).toInt();
        carrot = new Carrot();
        carrot->setPos(gridToPixelX(carrotCol), gridToPixelY(carrotRow));
        carrot->setZValue(3);
        scene->addItem(carrot);
        BaseMob::carrot = carrot;

        logMsg("loadLevel: carrot done, parsing spawn points...");
        // 3. 解析侵入点
        int spawnCount = in.readLine().toInt();
        for (int i = 0; i < spawnCount; i++) {
            QString spawnLine = in.readLine();
            QStringList parts = spawnLine.split(" ", Qt::SkipEmptyParts);
            if (parts.size() < 3) continue;
            int spawnRow = parts[0].toInt();
            int spawnCol = parts[1].toInt();
            int m = parts[2].toInt();

            SpawnPoint sp;
            sp.position = QPointF(gridToPixelX(spawnCol), gridToPixelY(spawnRow));

            for (int k = 0; k < m; k++) {
                int cr = parts[3 + k * 3].toInt();
                int cc = parts[3 + k * 3 + 1].toInt();
                double wt = parts[3 + k * 3 + 2].toDouble();
                Checkpoint cp;
                cp.pos = QPointF(gridToPixelX(cc), gridToPixelY(cr));
                cp.waitTime = wt;
                sp.checkpoints.append(cp);
            }
            spawnPoints.append(sp);

            QGraphicsPixmapItem *icon = new QGraphicsPixmapItem(
                QPixmap(":/new/prefix1/images/spawn_point.png")
                    .scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            icon->setPos(gridToPixelX(spawnCol) - 30, gridToPixelY(spawnRow) - 30);
            icon->setZValue(1);  // 地面(0) < 侵入点(1) < 敌人(2)
            scene->addItem(icon);
            spawnIconList.append(icon);
        }
        buildRouteConnectors();

        logMsg(QString("loadLevel: spawn done (%1 points), parsing waves...").arg(spawnCount));
        // 4. 解析波次
        totalWaves = in.readLine().toInt();
        for (int w = 0; w < totalWaves; w++) {
            int mobCount = in.readLine().toInt();
            QList<MobSpawnInfo> wave;
            for (int m = 0; m < mobCount; m++) {
                QString mobLine = in.readLine();
                QStringList mobParts = mobLine.split(" ", Qt::SkipEmptyParts);
                MobSpawnInfo info;
                info.spawnId = mobParts.value(0).toInt();
                info.type = mobParts.value(1).toInt();
                info.hp = mobParts.value(2).toInt();
                wave.append(info);
            }
            waves.append(wave);
        }
        file.close();
    }
    updateWaveDisplay();
    logMsg(QString("loadLevel: parsing done, waves=%1, starting countdown").arg(totalWaves));
    // 加载炮塔选择栏（关卡加载时才显示，避免菜单页露出）
    loadTowers(":/new/prefix1/towers.txt");
    buildTowerBar();
    // 隐藏选关页，开始倒计时
    levelSelectPage->hide();
    topBar->show();
    speedBtn->setEnabled(false);
    pauseBtn->setEnabled(false);
    menuBtn->setEnabled(false);

    QWidget *overlay = new QWidget(this);
    overlay->setFixedSize(1220, 740);
    overlay->move(0, 60);
    overlay->setStyleSheet("background: rgba(0,0,0,160);");
    overlay->show();
    overlay->raise();

    countdownMovie = new QMovie(":/new/prefix1/images/countdown.gif", QByteArray(), this);
    countdownLabel->setMovie(countdownMovie);
    countdownLabel->raise();
    countdownLabel->show();
    countdownMovie->start();

    QTimer::singleShot(2100, this, [this, overlay]() {
        logMsg("Countdown done, starting game!");
        countdownLabel->hide();
        overlay->hide();
        overlay->deleteLater();
        gameStarted = true;
        speedBtn->setEnabled(true);
        pauseBtn->setEnabled(true);
        menuBtn->setEnabled(true);
        waveIntervalTimer->start(waveInterval);
        timer->start(timerInterval);
    });
}

// ========== 页面切换 ==========

void MainWindow::showMenuPage()
{
    topBar->hide();
    timer->stop();
    spawnTimer->stop();
    waveIntervalTimer->stop();
    gameStarted = false;
    gameOver = false;
    levelSelectPage->hide();
    menuPage->show();
    menuPage->raise();
}

void MainWindow::showLevelSelectPage()
{
    loadUnlockProgress();
    updateLevelButtons();
    menuPage->hide();
    levelSelectPage->show();
    levelSelectPage->raise();
}

void MainWindow::startGame()
{
    // 不再直接使用，改为 showLevelSelectPage → loadLevel
    showLevelSelectPage();
}

QString MainWindow::progressFilePath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath("unlocked.txt");
}

void MainWindow::loadUnlockProgress()
{
    QFile file(progressFilePath());
    if (!file.exists()) {
        QFile sourceFile(QDir::current().filePath("unlocked.txt"));
        if (sourceFile.exists() && sourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&sourceFile);
            unlockedLevelCount = in.readLine().trimmed().toInt();
        } else {
            unlockedLevelCount = 1;
        }
        saveUnlockProgress();
    } else if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        unlockedLevelCount = in.readLine().trimmed().toInt();
    }

    unlockedLevelCount = qBound(1, unlockedLevelCount, 6);
}

void MainWindow::saveUnlockProgress() const
{
    QFile file(progressFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return;

    QTextStream out(&file);
    out << qBound(1, unlockedLevelCount, 6) << "\n";
}

void MainWindow::unlockNextLevelIfQualified()
{
    if (!carrot || carrot->hp < 8)
        return;
    if (currentLevel + 1 != unlockedLevelCount)
        return;
    if (unlockedLevelCount >= 6)
        return;

    unlockedLevelCount++;
    saveUnlockProgress();
    updateLevelButtons();
}

void MainWindow::updateLevelButtons()
{
    QIcon lockIcon(":/new/prefix1/images/lock.webp");
    for (int i = 0; i < levelButtons.size(); i++) {
        QPushButton *btn = levelButtons[i];
        bool unlocked = i < unlockedLevelCount;
        btn->setEnabled(true);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setProperty("locked", !unlocked);
        btn->style()->unpolish(btn);
        btn->style()->polish(btn);
        btn->setIcon(unlocked ? QIcon() : lockIcon);
        btn->setIconSize(QSize(38, 38));
        btn->setText(unlocked ? QString("关卡 %1").arg(i + 1)
                              : QString("关卡 %1").arg(i + 1));
    }
}

// ========== 射手菜单 ==========

void MainWindow::enterShooterMenu(Shooter* s) {
    if (shooterMenuMode) exitShooterMenu();
    shooterMenuMode = true;
    selectedShooter = s;
    clearUpgradeHints();

    const int offset = 48;
    const int radius = 25;

    QPointF pos = s->pos();

    // 攻击范围圈
    double rangeR = s->rangePx;
    QGraphicsEllipseItem *rangeCircle = new QGraphicsEllipseItem(
        -rangeR, -rangeR, rangeR * 2, rangeR * 2);
    rangeCircle->setPos(pos);
    rangeCircle->setBrush(QColor(100, 200, 255, 25));
    rangeCircle->setPen(QPen(QColor(100, 200, 255, 100), 1.5));
    rangeCircle->setZValue(20);
    rangeCircle->setData(0, -1);
    scene->addItem(rangeCircle);
    shooterCircles.append(rangeCircle);

    // 未满3级显示升级图标
    if (s->level < 3) {
        upIconItem = new QGraphicsPixmapItem(
            QPixmap(":/new/prefix1/images/upgrade_icon.png")
                .scaled(radius*2, radius*2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        upIconItem->setPos(pos.x() - radius, pos.y() - offset - radius);
        upIconItem->setZValue(23);
        upIconItem->setData(0, 0);
        scene->addItem(upIconItem);
        shooterCircles.append(upIconItem);

        upTextItem = new QGraphicsSimpleTextItem(
            QString("$%1").arg(s->getUpgradeCost()));
        upTextItem->setPos(pos.x() + radius + 4,
                           pos.y() - offset - 7);
        upTextItem->setFont(QFont("Arial", 9, QFont::Bold));
        upTextItem->setZValue(24);
        upTextItem->setData(0, 0);
        scene->addItem(upTextItem);
        shooterCircles.append(upTextItem);

        QRectF tr = upTextItem->boundingRect();
        QPainterPath upPath;
        upPath.addRoundedRect(tr.adjusted(-3, -2, 3, 2), 5, 5);
        QGraphicsPathItem *upBg = new QGraphicsPathItem(upPath);
        upBg->setPos(upTextItem->pos());
        upBg->setBrush(QColor(0, 0, 0, 160));
        upBg->setPen(Qt::NoPen);
        upBg->setZValue(22);
        upBg->setData(0, 0);
        scene->addItem(upBg);
        shooterCircles.append(upBg);
    } else {
        upIconItem = nullptr;
        upTextItem = nullptr;
    }

    updateShooterMenuAppearance();

    // 删除图标（始终正常显示）
    QPixmap delPix(":/new/prefix1/images/delete_icon.png");
    QGraphicsPixmapItem* delIcon = new QGraphicsPixmapItem(
        delPix.scaled(radius*2, radius*2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    delIcon->setPos(pos.x() - radius, pos.y() + offset - radius);
    delIcon->setData(0, 1);
    delIcon->setOpacity(1.0);
    delIcon->setZValue(23);
    scene->addItem(delIcon);
    shooterCircles.append(delIcon);

    int refund = s->getTotalCost() * 0.8;
    QGraphicsSimpleTextItem *delText = new QGraphicsSimpleTextItem(
        QString("$%1").arg(refund));
    delText->setPos(pos.x() + radius + 4,
                    pos.y() + offset - 7);
    delText->setBrush(QColor(100, 180, 255));
    delText->setFont(QFont("Arial", 9, QFont::Bold));
    delText->setZValue(24);
    delText->setData(0, 1);
    delText->setOpacity(1.0);
    scene->addItem(delText);
    shooterCircles.append(delText);

    QRectF dr = delText->boundingRect();
    QPainterPath delPath;
    delPath.addRoundedRect(dr.adjusted(-3, -2, 3, 2), 5, 5);
    QGraphicsPathItem *delBg = new QGraphicsPathItem(delPath);
    delBg->setPos(delText->pos());
    delBg->setBrush(QColor(0, 0, 0, 160));
    delBg->setPen(Qt::NoPen);
    delBg->setZValue(22);
    delBg->setData(0, 1);
    scene->addItem(delBg);
    shooterCircles.append(delBg);

    maskItem = new QGraphicsRectItem(scene->sceneRect());
    maskItem->setBrush(QBrush(QColor(0, 0, 0, 40)));
    maskItem->setZValue(19);
    scene->addItem(maskItem);
}

void MainWindow::exitShooterMenu() {
    for (auto circle : shooterCircles) {
        scene->removeItem(circle);
        delete circle;
    }
    shooterCircles.clear();
    if (maskItem) {
        scene->removeItem(maskItem);
        delete maskItem;
        maskItem = nullptr;
    }
    upIconItem = nullptr;
    upTextItem = nullptr;
    shooterMenuMode = false;
    selectedShooter = nullptr;
    updateUpgradeHints();
}

void MainWindow::loadTowers(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        availableTowers.append({1, 100});
        return;
    }
    QTextStream in(&file);
    int total = qMax(1, in.readLine().toInt());
    QStringList ids = in.readLine().split(" ", Qt::SkipEmptyParts);
    for (const QString &s : ids) {
        int id = s.toInt();
        if (id == 1) availableTowers.append({1, 100});
    }
    file.close();
}

void MainWindow::buildTowerBar() {
    const int cw = 50, ch = 68;
    const int gap = 6;
    const double barY = 15;
    int n = availableTowers.size();
    double totalW = n * cw + (n - 1) * gap;
    double startX = 610.0 - totalW / 2.0;

    for (int i = 0; i < n; i++) {
        double bx = startX + i * (cw + gap);

        // 卡片背景
        QGraphicsRectItem *card = new QGraphicsRectItem(0, 0, cw, ch);
        card->setPos(bx, barY);
        card->setBrush(QColor(35, 35, 35, 220));
        card->setPen(QPen(QColor(80, 80, 80), 1));
        card->setZValue(10);
        card->setData(0, i);
        card->setData(1, -1);
        scene->addItem(card);
        towerBarItems.append(card);
        towerCards.append(card);

        QGraphicsPixmapItem *icon = new QGraphicsPixmapItem(
            QPixmap(":/new/prefix1/images/tower_icon_1.png")
                .scaled(cw - 4, ch - 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        icon->setPos(bx + 2, barY + 2);
        icon->setZValue(11);
        scene->addItem(icon);
        towerBarItems.append(icon);

        QGraphicsRectItem *priceBar = new QGraphicsRectItem(0, 0, cw, 18);
        priceBar->setPos(bx, barY + ch - 18);
        priceBar->setBrush(QColor(0, 0, 0, 180));
        priceBar->setPen(Qt::NoPen);
        priceBar->setZValue(11);
        priceBar->setData(0, i);
        priceBar->setData(1, -1);
        scene->addItem(priceBar);
        towerBarItems.append(priceBar);

        QGraphicsSimpleTextItem *costText = new QGraphicsSimpleTextItem(
            QString("$%1").arg(availableTowers[i].cost));
        costText->setPos(bx + cw / 2.0 - costText->boundingRect().width() / 2,
                         barY + ch - 16);
        costText->setBrush(Qt::yellow);
        costText->setFont(QFont("Arial", 9, QFont::Bold));
        costText->setZValue(12);
        scene->addItem(costText);
        towerBarItems.append(costText);
    }
    updateTowerBarAppearance();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (obj == scene && event->type() == QEvent::GraphicsSceneMousePress) {
        if (gameOver || !gameStarted) return true;
        QGraphicsSceneMouseEvent* mouseEvent = static_cast<QGraphicsSceneMouseEvent*>(event);
        QPointF scenePos = mouseEvent->scenePos();

        // 射手菜单模式
        if (shooterMenuMode) {
            bool hit = false;
            for (auto *item : shooterCircles) {
                if (item->data(0).toInt() >= 0 && item->contains(item->mapFromScene(scenePos))) {
                    int action = item->data(0).toInt();
                    if (action == 0) {
                        int cost = selectedShooter->getUpgradeCost();
                        if (gold >= cost && selectedShooter->upgrade()) {
                            gold -= cost;
                            updateGoldDisplay();
                        }
                    } else if (action == 1) {
                        int refund = selectedShooter->getTotalCost() * 0.8;
                        gold += refund;
                        QList<QGraphicsItem*> under = scene->items(selectedShooter->pos());
                        for (auto *u : under) {
                            ground* g = dynamic_cast<ground*>(u);
                            if (g) { g->clearShooterPlaced(); break; }
                        }
                        scene->removeItem(selectedShooter);
                        delete selectedShooter;
                        selectedShooter = nullptr;
                        shooterMenuMode = false;
                        updateGoldDisplay();
                    }
                    hit = true;
                    break;
                }
            }
            if (hit) exitShooterMenu();
            else exitShooterMenu();
            return true;
        }

        // 检查是否点击塔选择栏
        for (auto *item : towerBarItems) {
            if (item->data(1).toInt() == -1 && item->contains(item->mapFromScene(scenePos))) {
                int idx = item->data(0).toInt();
                if (idx >= 0 && idx < availableTowers.size()
                    && gold >= availableTowers[idx].cost) {
                    selectedTower = (selectedTower == idx) ? -1 : idx;
                    updateTowerBarAppearance();
                }
                return true;
            }
        }

        // 选了塔 → 点击有效地面放置
        if (selectedTower >= 0) {
            QList<QGraphicsItem*> items = scene->items(scenePos);
            for (auto *item : items) {
                ground* g = dynamic_cast<ground*>(item);
                if (g && !g->invalid() && !g->hasShooterPlaced()) {
                    const TowerInfo &ti = availableTowers[selectedTower];
                    gold -= ti.cost;
                    updateGoldDisplay();
                    Shooter* s = new Shooter();
                    s->setPos(g->pos());
                    s->setZValue(3);
                    scene->addItem(s);
                    g->setShooterPlaced();
                    selectedTower = -1;
                    updateUpgradeHints();
                    updateTowerBarAppearance();
                    return true;
                }
            }
            selectedTower = -1;
            updateTowerBarAppearance();
            return true;
        }

        // 检查地上物件
        QList<QGraphicsItem*> items = scene->items(scenePos);
        for (auto *item : items) {
            Shooter* shooter = dynamic_cast<Shooter*>(item);
            if (shooter) {
                enterShooterMenu(shooter);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::updateGoldDisplay() {
    goldText->setText(QString::number(gold));
    updateTowerBarAppearance();
    updateShooterMenuAppearance();
    updateUpgradeHints();
}

void MainWindow::updateShooterMenuAppearance() {
    if (!shooterMenuMode || !selectedShooter || !upIconItem || !upTextItem) return;
    bool canUp = gold >= selectedShooter->getUpgradeCost();
    upIconItem->setOpacity(canUp ? 1.0 : 0.35);
    upTextItem->setBrush(canUp ? QColor(100, 180, 255) : QColor(80, 80, 80));
}

void MainWindow::clearUpgradeHints() {
    if (!scene) return;
    for (auto *item : upgradeHintItems) {
        scene->removeItem(item);
        delete item;
    }
    upgradeHintItems.clear();
}

void MainWindow::updateUpgradeHints() {
    if (!scene) return;
    clearUpgradeHints();
    if (!gameStarted || gameOver || shooterMenuMode) return;

    QList<QGraphicsItem*> all = scene->items();
    for (auto *item : all) {
        Shooter *shooter = dynamic_cast<Shooter *>(item);
        if (!shooter || shooter->level >= 3) continue;

        int cost = shooter->getUpgradeCost();
        if (cost <= 0 || gold < cost) continue;

        QPointF pos = shooter->pos();

        QGraphicsEllipseItem *halo = new QGraphicsEllipseItem(0, 0, 26, 26);
        halo->setPos(pos.x() + 18, pos.y() - 50);
        halo->setBrush(QColor(255, 220, 70, 55));
        halo->setPen(QPen(QColor(255, 215, 70, 210), 2));
        halo->setZValue(5);
        scene->addItem(halo);
        upgradeHintItems.append(halo);

        QGraphicsPixmapItem *icon = new QGraphicsPixmapItem(
            QPixmap(":/new/prefix1/images/upgrade_icon.png")
                .scaled(19, 19, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        icon->setPos(pos.x() + 21.5, pos.y() - 46.5);
        icon->setZValue(6);
        scene->addItem(icon);
        upgradeHintItems.append(icon);
    }
}

void MainWindow::updateTowerBarAppearance() {
    for (int i = 0; i < towerCards.size() && i < availableTowers.size(); i++) {
        bool canAfford = gold >= availableTowers[i].cost;
        QPen pen = (selectedTower == i) ? QPen(Qt::yellow, 3)
                                        : QPen(canAfford ? QColor(80, 80, 80) : QColor(50, 50, 50), 2);
        QGraphicsRectItem *card = static_cast<QGraphicsRectItem *>(towerCards[i]);
        card->setPen(pen);
        card->setBrush(canAfford ? QColor(40, 40, 40, 200) : QColor(25, 25, 25, 200));
    }
}

void MainWindow::clearRouteConnectors() {
    if (!scene) return;
    for (auto *item : routeConnectorItems) {
        scene->removeItem(item);
        delete item;
    }
    routeConnectorItems.clear();
}

void MainWindow::buildRouteConnectors() {
    clearRouteConnectors();

    QPixmap roadPixmap(":/new/prefix1/images/dirt_path_top.png");
    QBrush roadBrush(roadPixmap.scaled(64, 64, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    QPen edgePen(QColor(105, 73, 40, 90), 1);

    auto addSegment = [&](const QPointF &from, const QPointF &to) {
        QRectF rect;
        if (qAbs(to.x() - from.x()) >= qAbs(to.y() - from.y())) {
            qreal left = qMin(from.x(), to.x()) - 32.0;
            qreal width = qAbs(to.x() - from.x()) + 64.0;
            rect = QRectF(left, from.y() - 32.0, width, 64.0);
        } else {
            qreal top = qMin(from.y(), to.y()) - 32.0;
            qreal height = qAbs(to.y() - from.y()) + 64.0;
            rect = QRectF(from.x() - 32.0, top, 64.0, height);
        }

        QGraphicsRectItem *item = scene->addRect(rect, edgePen, roadBrush);
        item->setZValue(0.1);
        routeConnectorItems.append(item);
    };

    for (const SpawnPoint &sp : spawnPoints) {
        QPointF cur = sp.position;
        for (const Checkpoint &cp : sp.checkpoints) {
            addSegment(cur, cp.pos);
            cur = cp.pos;
        }
    }
}

void MainWindow::updateWaveDisplay() {
    int cur = qMax(0, currentWaveIndex + 1);
    waveText->setText(QString("波数 %1 / %2").arg(cur).arg(totalWaves));
}

void MainWindow::togglePause() {
    if (paused) {
        // 恢复：用缓存的剩余时间
        paused = false;
        timer->start(timerInterval);
        if (spawnRemaining >= 0)
            spawnTimer->start(spawnRemaining);
        if (waveRemaining >= 0)
            waveIntervalTimer->start(waveRemaining);
        spawnRemaining = waveRemaining = -1;
        pauseBtn->setIcon(QIcon(":/new/prefix1/images/pause_icon.png"));
    } else {
        doPause();
    }
}

void MainWindow::doPause() {
    // 缓存当前 timer 剩余时间，立即停止
    spawnRemaining = spawnTimer->isActive() ? spawnTimer->remainingTime() : -1;
    waveRemaining = waveIntervalTimer->isActive() ? waveIntervalTimer->remainingTime() : -1;
    paused = true;
    timer->stop();
    spawnTimer->stop();
    waveIntervalTimer->stop();
    pauseBtn->setIcon(QIcon(":/new/prefix1/images/play_icon.png"));
    pauseBtn->setEnabled(true);
}

void MainWindow::startNextWave() {
    if (gameOver) return;
    currentWaveIndex++;
    if (currentWaveIndex >= totalWaves) {
        countdownLabel->setText("胜利");
        countdownLabel->show();
        return;
    }
    spawnIndex = 0;
    updateWaveDisplay();
    spawnNextMonster();
}

void MainWindow::spawnNextMonster() {
    if (gameOver) return;
    if (currentWaveIndex >= totalWaves || currentWaveIndex < 0) return;
    auto &wave = waves[currentWaveIndex];
    if (spawnIndex >= wave.size()) {
        spawnTimer->stop();
        return;
    }
    MobSpawnInfo info = wave[spawnIndex];

    // 根据 spawnId 找侵入点
    if (info.spawnId < 0 || info.spawnId >= spawnPoints.size()) return;
    const SpawnPoint &sp = spawnPoints[info.spawnId];

    BaseMob *mob = nullptr;
    if (info.type == 2) {
        dog *m = new dog();
        m->hp = info.hp;
        m->maxHp = info.hp;
        mob = m;
    } else if (info.type == 3) {
        tank *m = new tank();
        m->hp = info.hp;
        m->maxHp = info.hp;
        mob = m;
    } else {
        soldier *m = new soldier();
        m->hp = info.hp;
        m->maxHp = info.hp;
        mob = m;
    }

    static int spawnLogCount = 0;
    if (++spawnLogCount <= 3) logMsg(QString("spawnNextMonster: type=%1 hp=%2 spawnId=%3 pos=(%4,%5)").arg(info.type).arg(info.hp).arg(info.spawnId).arg(sp.position.x()).arg(sp.position.y()));
    mob->setZValue(2);  // 地面(0) < 侵入点(1) < 敌人(2)
    // 出生位置 = 侵入点位置
    mob->setPos(sp.position);

    // 赋予检查点路径
    mob->checkpoints = sp.checkpoints;
    mob->cpIndex = 0;

    scene->addItem(mob);
    mobList->append(mob);
    spawnIndex++;
    if (spawnIndex < wave.size()) {
        spawnTimer->start(spawnInterval);
    }
}

void MainWindow::cleanupGame() {
    logMsg("cleanupGame() START");
    logMsg(QString("cleanupGame: mobList size=%1, groundList size=%2").arg(mobList->size()).arg(groundList.size()));
    clearUpgradeHints();
    logMsg("cleanupGame: upgrade hint cleanup done");
    // 清怪物
    for (auto *m : *mobList) {
        scene->removeItem(m);
        delete m;
    }
    mobList->clear();
    logMsg("cleanupGame: mob cleanup done");

    // 清塔和子弹（先收集到临时列表，以免在 scene 迭代中删元素）
    QList<Shooter*> shootersToDelete;
    QList<Shell*> shellsToDelete;
    logMsg("cleanupGame: calling scene->items()...");
    QList<QGraphicsItem*> all = scene->items();
    logMsg(QString("cleanupGame: scene has %1 items").arg(all.size()));
    for (auto *item : all) {
        if (Shooter *s = dynamic_cast<Shooter *>(item))
            shootersToDelete.append(s);
        else if (Shell *sh = dynamic_cast<Shell *>(item))
            shellsToDelete.append(sh);
    }
    logMsg(QString("cleanupGame: found %1 shooters, %2 shells").arg(shootersToDelete.size()).arg(shellsToDelete.size()));
    for (auto *s : shootersToDelete) {
        scene->removeItem(s);
        delete s;
    }
    for (auto *sh : shellsToDelete) {
        scene->removeItem(sh);
        delete sh;
    }
    logMsg("cleanupGame: shooter/shell cleanup done");

    // 地面复原（使用 groundList 而非 all，避免访问已删除对象的悬空指针）
    for (auto *g : groundList)
        g->clearShooterPlaced();
    logMsg("cleanupGame: ground reset done");

    // 萝卜恢复
    if (carrot) {
        carrot->hp = 10;
        carrot->setZValue(3);
        carrot->show();
        carrot->update();
    }
    logMsg("cleanupGame: carrot reset done");

    // 关闭射手菜单
    exitShooterMenu();
    logMsg("cleanupGame: exitShooterMenu done");

    // 波次归零
    currentWaveIndex = -1;
    spawnIndex = 0;
    selectedTower = -1;
    gameOver = false;
    gameStarted = false;
    paused = false;
    spawnRemaining = waveRemaining = -1;
    gold = 500;
    updateGoldDisplay();
    updateWaveDisplay();

    // 倍速复位
    doubleSpeed = false;
    timerInterval = 32;
    spawnInterval = 1000;
    waveInterval = 5000;
    speedBtn->setStyleSheet("color: white; font-size: 14px; background: transparent; border: 1px solid #666; border-radius: 6px;");

    pauseBtn->setIcon(QIcon(":/new/prefix1/images/pause_icon.png"));

    timer->stop();
    spawnTimer->stop();
    waveIntervalTimer->stop();
    logMsg("cleanupGame() END");
}

void MainWindow::resetAndStart() {
    loadLevel(currentLevel);
}

void MainWindow::onMenuReturnClicked()
{
    // 强制立即暂停，缓存剩余时间
    bool wasPausedBefore = paused;
    int savedSpawnRemaining = spawnRemaining;
    int savedWaveRemaining = waveRemaining;
    if (!paused) {
        spawnRemaining = spawnTimer->isActive() ? spawnTimer->remainingTime() : -1;
        waveRemaining = waveIntervalTimer->isActive() ? waveIntervalTimer->remainingTime() : -1;
        timer->stop();
        spawnTimer->stop();
        waveIntervalTimer->stop();
        paused = true;
        pauseBtn->setIcon(QIcon(":/new/prefix1/images/play_icon.png"));
    }
    QMessageBox::StandardButton ret = QMessageBox::question(
        this, "返回主界面", "是否要回到主界面？",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        clearGameObjects();
        showMenuPage();
    } else {
        if (!wasPausedBefore) {
            paused = false;
            timer->start(timerInterval);
            if (spawnRemaining >= 0)
                spawnTimer->start(spawnRemaining);
            if (waveRemaining >= 0)
                waveIntervalTimer->start(waveRemaining);
            spawnRemaining = waveRemaining = -1;
            pauseBtn->setIcon(QIcon(":/new/prefix1/images/pause_icon.png"));
        } else {
            // 之前已暂停，恢复原有缓存
            spawnRemaining = savedSpawnRemaining;
            waveRemaining = savedWaveRemaining;
        }
        pauseBtn->setEnabled(true);
    }
}
