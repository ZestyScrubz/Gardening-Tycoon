#include "MainWindow.h"
#include "CropFactory.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <cmath>
#include <algorithm>

// ============================================================================
// HexFarmWidget Implementation (Hexagonal Terrain, Trees, Stones, Tilled Soil)
// ============================================================================

HexFarmWidget::HexFarmWidget(FarmController* ctrl, QWidget* parent)
    : QWidget(parent), controller(ctrl) {
    setMinimumSize(680, 540);
    setMouseTracking(true);
}

void HexFarmWidget::setSelectedTile(int x, int y) {
    selectedX = x;
    selectedY = y;
    update();
}

double HexFarmWidget::calculateHexRadius() const {
    int cols = controller->getGridWidth();
    int rows = controller->getGridHeight();
    double availW = width() - 40.0;
    double availH = height() - 50.0;

    double rByWidth = availW / (std::sqrt(3.0) * (cols + 0.5));
    double rByHeight = availH / (1.5 * rows + 0.5);
    return std::clamp(std::min(rByWidth, rByHeight), 24.0, 48.0);
}

QPointF HexFarmWidget::hexCenter(int col, int row, double radius) const {
    int cols = controller->getGridWidth();
    int rows = controller->getGridHeight();
    double hexW = std::sqrt(3.0) * radius;
    double hexH = 1.5 * radius;

    double totalW = cols * hexW + 0.5 * hexW;
    double totalH = (rows - 1) * hexH + 2.0 * radius;

    double startX = (width() - totalW) / 2.0 + hexW / 2.0;
    double startY = (height() - totalH) / 2.0 + radius;

    double cx = startX + col * hexW + ((row % 2 == 1) ? (hexW / 2.0) : 0.0);
    double cy = startY + row * hexH;
    return QPointF(cx, cy);
}

QPolygonF HexFarmWidget::createHexPolygon(const QPointF& center, double radius) const {
    QPolygonF poly;
    for (int i = 0; i < 6; ++i) {
        double angleDeg = 60.0 * i - 30.0; // Pointy-top hexagon
        double angleRad = angleDeg * M_PI / 180.0;
        poly << QPointF(center.x() + radius * std::cos(angleRad),
                        center.y() + radius * std::sin(angleRad));
    }
    return poly;
}

void HexFarmWidget::paintEvent(QPaintEvent* /*event*/) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Rich ambient backdrop based on biome
    std::string biome = controller->getActiveBiomeName();
    QLinearGradient bgGrad(0, 0, 0, height());
    if (biome == "Desert") {
        bgGrad.setColorAt(0.0, QColor("#fef3c7"));
        bgGrad.setColorAt(1.0, QColor("#fde68a"));
    } else if (biome == "Savanna") {
        bgGrad.setColorAt(0.0, QColor("#ecfccb"));
        bgGrad.setColorAt(1.0, QColor("#d9f99d"));
    } else {
        bgGrad.setColorAt(0.0, QColor("#dcfce7"));
        bgGrad.setColorAt(1.0, QColor("#bbf7d0"));
    }
    p.fillRect(rect(), bgGrad);

    double r = calculateHexRadius();
    int rows = controller->getGridHeight();
    int cols = controller->getGridWidth();

    // First pass: Draw base hexagons back-to-front so 3D bevels and trees layer naturally
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            TileSnapshot snap = controller->getTileSnapshot(x, y);
            QPointF c = hexCenter(x, y, r);
            bool isSel = (x == selectedX && y == selectedY);
            bool isHov = (x == hoveredX && y == hoveredY);
            drawHexTile(p, snap, c, r, isSel, isHov);
        }
    }
}

void HexFarmWidget::drawHexTile(QPainter& p, const TileSnapshot& snap, const QPointF& center,
                                double radius, bool isSelected, bool isHovered) {
    double innerR = radius * 0.96;

    // 1. 3D Extruded Tile Base Shadow/Edge
    QPolygonF baseHex = createHexPolygon(center + QPointF(0, 5.0), innerR);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#3f6212"));
    p.drawPolygon(baseHex);

    // 2. Top Surface Grass / Sand Hexagon
    QPolygonF topHex = createHexPolygon(center, innerR);
    std::string biome = controller->getActiveBiomeName();

    QColor grassLight("#84cc16");
    QColor grassDark("#65a30d");
    if (biome == "Desert") {
        grassLight = QColor("#fcd34d");
        grassDark  = QColor("#d97706");
    } else if (biome == "Savanna") {
        grassLight = QColor("#a3e635");
        grassDark  = QColor("#65a30d");
    }

    // Subtlecheckerboard variation between adjacent hexes
    if ((snap.x + snap.y) % 2 == 1) {
        grassLight = grassLight.darker(106);
    }

    QLinearGradient tileGrad(center.x(), center.y() - innerR, center.x(), center.y() + innerR);
    tileGrad.setColorAt(0.0, grassLight);
    tileGrad.setColorAt(1.0, grassDark);

    p.setBrush(tileGrad);
    p.setPen(QPen(QColor("#365314"), 1.2));
    p.drawPolygon(topHex);

    // Small decorative grass tufts on empty untamed plots
    if (snap.obstacle.empty() && !snap.isTilled && !snap.hasCrop) {
        p.setPen(QPen(grassDark.darker(130), 1.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(center + QPointF(-innerR * 0.3, 4), center + QPointF(-innerR * 0.35, -3));
        p.drawLine(center + QPointF(-innerR * 0.3, 4), center + QPointF(-innerR * 0.22, -2));
        p.drawLine(center + QPointF(innerR * 0.25, 8), center + QPointF(innerR * 0.30, 2));
    }

    // 3. Tilled Soil Bed or Obstacle or Crop
    if (!snap.obstacle.empty()) {
        if (snap.obstacle == "Tree") {
            drawTree(p, center, innerR);
        } else if (snap.obstacle == "Boulder") {
            drawBoulder(p, center, innerR);
        } else if (snap.obstacle == "Pond") {
            drawPond(p, center, innerR);
        }
    } else if (snap.isTilled || snap.hasCrop) {
        drawTilledSoil(p, center, innerR, snap.moisture, snap.isMulched);
        if (snap.hasCrop) {
            drawCropSprite(p, center, innerR, snap);
        }
    }

    // 4. Hover & Active Selection Glow Ring (White + Gold like the video)
    if (isSelected) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 255, 255, 230), 4.0));
        p.drawPolygon(createHexPolygon(center, innerR * 0.93));
        p.setPen(QPen(QColor("#f59e0b"), 2.0));
        p.drawPolygon(createHexPolygon(center, innerR * 0.98));
    } else if (isHovered) {
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(QColor(255, 255, 255, 160), 2.5));
        p.drawPolygon(createHexPolygon(center, innerR * 0.94));
    }

    // 5. Subtle Moisture Badge at Bottom of Tile
    if (snap.obstacle.empty()) {
        QFont f = p.font();
        f.setPointSize(8);
        f.setBold(true);
        p.setFont(f);
        p.setPen(QColor(255, 255, 255, 220));
        QRectF textRect(center.x() - innerR * 0.7, center.y() + innerR * 0.42, innerR * 1.4, 14);
        p.drawText(textRect, Qt::AlignCenter, QString("%1%💧").arg(static_cast<int>(snap.moisture)));
    }
}

void HexFarmWidget::drawTilledSoil(QPainter& p, const QPointF& c, double r, double moisture, bool isMulched) {
    double soilR = r * 0.78;
    QPolygonF soilHex = createHexPolygon(c, soilR);

    // Soil gets darker and richer as moisture increases
    int darkness = 100 + static_cast<int>((moisture / 100.0) * 45.0);
    QColor baseSoil = QColor("#78350f").darker(darkness);
    QColor ridgeColor = QColor("#451a03").darker(darkness);

    p.setPen(QPen(QColor("#290f02"), 1.5));
    p.setBrush(baseSoil);
    p.drawPolygon(soilHex);

    // Clip to the inner soil hexagon to draw parallel ploughed furrow lines
    p.save();
    QPainterPath clipPath;
    clipPath.addPolygon(soilHex);
    p.setClipPath(clipPath);

    p.setPen(QPen(ridgeColor, 3.2, Qt::SolidLine, Qt::RoundCap));
    for (double offset = -soilR; offset <= soilR; offset += 7.5) {
        p.drawLine(QPointF(c.x() - soilR, c.y() + offset - 6),
                   QPointF(c.x() + soilR, c.y() + offset + 6));
    }

    // Golden organic mulch straw flecks if mulched (FR13)
    if (isMulched) {
        p.setPen(QPen(QColor("#facc15"), 2.0, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c + QPointF(-10, -6), c + QPointF(-4, -8));
        p.drawLine(c + QPointF(8, -4), c + QPointF(14, -1));
        p.drawLine(c + QPointF(-6, 8), c + QPointF(2, 10));
        p.drawLine(c + QPointF(6, 7), c + QPointF(11, 4));
    }
    p.restore();
}

void HexFarmWidget::drawTree(QPainter& p, const QPointF& c, double r) {
    // Drop shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 55));
    p.drawEllipse(c + QPointF(2, r * 0.28), r * 0.45, r * 0.22);

    // Tree trunk
    p.setBrush(QColor("#78350f"));
    p.setPen(QPen(QColor("#451a03"), 1.0));
    p.drawRect(QRectF(c.x() - r * 0.12, c.y() - r * 0.05, r * 0.24, r * 0.35));

    // 3 Layered Pine Foliage Tiers
    auto drawTier = [&](double yOffset, double halfW, double height, QColor fill) {
        QPolygonF tier;
        tier << QPointF(c.x(), c.y() + yOffset - height)
             << QPointF(c.x() - halfW, c.y() + yOffset)
             << QPointF(c.x() + halfW, c.y() + yOffset);
        p.setBrush(fill);
        p.setPen(QPen(QColor("#14532d"), 1.2));
        p.drawPolygon(tier);
    };

    drawTier(r * 0.08, r * 0.52, r * 0.48, QColor("#15803d"));
    drawTier(-r * 0.12, r * 0.42, r * 0.45, QColor("#16a34a"));
    drawTier(-r * 0.32, r * 0.30, r * 0.40, QColor("#22c55e"));
}

void HexFarmWidget::drawBoulder(QPainter& p, const QPointF& c, double r) {
    // Shadow
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 50));
    p.drawEllipse(c + QPointF(2, r * 0.22), r * 0.48, r * 0.22);

    // Main faceted rock polygon
    QPolygonF rock;
    rock << c + QPointF(-r * 0.42, r * 0.18)
         << c + QPointF(-r * 0.32, -r * 0.22)
         << c + QPointF(-r * 0.05, -r * 0.38)
         << c + QPointF(r * 0.30, -r * 0.20)
         << c + QPointF(r * 0.44, r * 0.16)
         << c + QPointF(r * 0.10, r * 0.30);

    p.setBrush(QColor("#94a3b8"));
    p.setPen(QPen(QColor("#334155"), 1.5));
    p.drawPolygon(rock);

    // Top-left bright facet highlight
    QPolygonF highlight;
    highlight << c + QPointF(-r * 0.32, -r * 0.22)
              << c + QPointF(-r * 0.05, -r * 0.38)
              << c + QPointF(r * 0.05, -r * 0.05)
              << c + QPointF(-r * 0.42, r * 0.18);
    p.setBrush(QColor("#cbd5e1"));
    p.drawPolygon(highlight);

    // Small secondary pebble
    p.setBrush(QColor("#64748b"));
    p.drawEllipse(c + QPointF(r * 0.25, r * 0.22), r * 0.16, r * 0.12);
}

void HexFarmWidget::drawPond(QPainter& p, const QPointF& c, double r) {
    QPolygonF waterHex = createHexPolygon(c, r * 0.72);
    QRadialGradient waterGrad(c, r * 0.72);
    waterGrad.setColorAt(0.0, QColor("#0284c7"));
    waterGrad.setColorAt(0.8, QColor("#38bdf8"));
    waterGrad.setColorAt(1.0, QColor("#bae6fd"));

    p.setBrush(waterGrad);
    p.setPen(QPen(QColor("#0369a1"), 1.8));
    p.drawPolygon(waterHex);

    // Water ripples
    p.setPen(QPen(QColor(255, 255, 255, 160), 1.5, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(c + QPointF(-10, -4), c + QPointF(4, -4));
    p.drawLine(c + QPointF(-4, 5), c + QPointF(10, 5));
}

void HexFarmWidget::drawCropSprite(QPainter& p, const QPointF& c, double r, const TileSnapshot& snap) {
    if (snap.cropStage == "Seed") {
        p.setBrush(QColor("#fde047"));
        p.setPen(QPen(QColor("#854d0e"), 1.0));
        p.drawEllipse(c + QPointF(-6, -2), 3.5, 2.5);
        p.drawEllipse(c + QPointF(5, 3), 3.5, 2.5);
        p.drawEllipse(c + QPointF(0, -6), 3.5, 2.5);
    } else if (snap.cropStage == "Sprout") {
        p.setPen(QPen(QColor("#22c55e"), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c + QPointF(0, 6), c + QPointF(0, -6));
        p.setBrush(QColor("#4ade80"));
        p.setPen(QPen(QColor("#15803d"), 1.0));
        p.drawEllipse(c + QPointF(-6, -6), 6, 3.5);
        p.drawEllipse(c + QPointF(6, -6), 6, 3.5);
    } else if (snap.cropStage == "Vegetative") {
        p.setBrush(QColor("#22c55e"));
        p.setPen(QPen(QColor("#14532d"), 1.2));
        p.drawEllipse(c + QPointF(-8, -4), 9, 5);
        p.drawEllipse(c + QPointF(8, -4), 9, 5);
        p.drawEllipse(c + QPointF(0, -10), 6, 10);
    } else if (snap.cropStage == "Mature") {
        if (snap.cropSpecies == "Carrot") {
            // Orange carrot root peeking out with leafy green top
            QPolygonF carrot;
            carrot << c + QPointF(-7, -2) << c + QPointF(7, -2) << c + QPointF(0, 14);
            p.setBrush(QColor("#f97316"));
            p.setPen(QPen(QColor("#9a3412"), 1.2));
            p.drawPolygon(carrot);

            p.setBrush(QColor("#22c55e"));
            p.setPen(QPen(QColor("#14532d"), 1.0));
            p.drawEllipse(c + QPointF(-5, -9), 4, 8);
            p.drawEllipse(c + QPointF(0, -11), 4, 9);
            p.drawEllipse(c + QPointF(5, -9), 4, 8);
        } else if (snap.cropSpecies == "Wheat") {
            // Golden wheat stalks
            p.setPen(QPen(QColor("#eab308"), 2.2));
            for (int dx : {-8, 0, 8}) {
                p.drawLine(c + QPointF(dx, 10), c + QPointF(dx, -12));
                p.setBrush(QColor("#fde047"));
                p.drawEllipse(c + QPointF(dx, -12), 4.0, 8.0);
            }
        } else {
            // Tall Corn stalks with golden cobs
            p.setPen(QPen(QColor("#16a34a"), 3.0));
            p.drawLine(c + QPointF(-5, 10), c + QPointF(-5, -14));
            p.drawLine(c + QPointF(6, 10), c + QPointF(6, -14));
            p.setBrush(QColor("#facc15"));
            p.setPen(QPen(QColor("#854d0e"), 1.0));
            p.drawEllipse(c + QPointF(-5, -5), 4.5, 8.0);
            p.drawEllipse(c + QPointF(6, -4), 4.5, 8.0);
        }
    } else if (snap.cropStage == "Rotten") {
        p.setPen(QPen(QColor("#57534e"), 2.5));
        p.drawLine(c + QPointF(-6, 8), c + QPointF(-2, -4));
        p.drawLine(c + QPointF(-2, -4), c + QPointF(6, 2));
        p.setBrush(QColor("#78716c"));
        p.drawEllipse(c + QPointF(4, 2), 5, 3);
    }

    // Floating Vitality Bar above the plant (Just like the video HUD!)
    double barW = r * 1.05;
    double barH = 6.0;
    QRectF barBg(c.x() - barW / 2.0, c.y() - r * 0.72, barW, barH);
    p.setPen(QPen(QColor("#0f172a"), 1.0));
    p.setBrush(QColor(15, 23, 42, 180));
    p.drawRoundedRect(barBg, 3, 3);

    double fillRatio = std::clamp(snap.cropVitality / 100.0, 0.0, 1.0);
    QColor vitColor = (snap.cropVitality > 60) ? QColor("#4ade80")
                      : (snap.cropVitality > 25) ? QColor("#facc15")
                                                 : QColor("#ef4444");
    QRectF barFill(barBg.x() + 1, barBg.y() + 1, (barW - 2) * fillRatio, barH - 2);
    p.setPen(Qt::NoPen);
    p.setBrush(vitColor);
    p.drawRoundedRect(barFill, 2, 2);
}

void HexFarmWidget::mousePressEvent(QMouseEvent* event) {
    double r = calculateHexRadius();
    int rows = controller->getGridHeight();
    int cols = controller->getGridWidth();

    int bestX = -1, bestY = -1;
    double bestDist = r * 0.95; // Must click inside hex radius

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            QPointF c = hexCenter(x, y, r);
            double dist = std::hypot(event->position().x() - c.x(), event->position().y() - c.y());
            if (dist < bestDist) {
                bestDist = dist;
                bestX = x;
                bestY = y;
            }
        }
    }

    if (bestX != -1 && bestY != -1) {
        selectedX = bestX;
        selectedY = bestY;
        emit tileSelected(selectedX, selectedY);
        update();
    }
}

void HexFarmWidget::mouseMoveEvent(QMouseEvent* event) {
    double r = calculateHexRadius();
    int rows = controller->getGridHeight();
    int cols = controller->getGridWidth();

    int newHovX = -1, newHovY = -1;
    double bestDist = r * 0.95;

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            QPointF c = hexCenter(x, y, r);
            double dist = std::hypot(event->position().x() - c.x(), event->position().y() - c.y());
            if (dist < bestDist) {
                bestDist = dist;
                newHovX = x;
                newHovY = y;
            }
        }
    }

    if (newHovX != hoveredX || newHovY != hoveredY) {
        hoveredX = newHovX;
        hoveredY = newHovY;
        update();
    }
}

void HexFarmWidget::leaveEvent(QEvent* /*event*/) {
    hoveredX = -1;
    hoveredY = -1;
    update();
}

// ============================================================================
// MainWindow Implementation
// ============================================================================

MainWindow::MainWindow(FarmController* ctrl, QWidget* parent)
    : QMainWindow(parent), controller(ctrl) {
    setWindowTitle("Gardening Tycoon — CS3307A (Group 120)");
    resize(1220, 840);

    // Warm Wooden & Forest Tycoon Theme (inspired by your video)
    setStyleSheet(
        "QMainWindow, QWidget { background-color: #f8fafc; color: #0f172a; font-family: -apple-system, 'Segoe UI', Roboto, sans-serif; }"
        "QGroupBox { font-weight: bold; font-size: 13px; color: #451a03; border: 2px solid #b45309; border-radius: 8px; margin-top: 10px; padding-top: 12px; background-color: #fffbeb; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; background-color: #fef3c7; border-radius: 4px; }"
        "QPushButton { background-color: #fef3c7; color: #451a03; border: 2px solid #b45309; border-radius: 6px; padding: 6px 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: #fde68a; }"
        "QPushButton:disabled { background-color: #f1f5f9; color: #94a3b8; border-color: #cbd5e1; }"
        "QComboBox { background-color: #ffffff; color: #0f172a; border: 2px solid #b45309; border-radius: 5px; padding: 5px; font-weight: bold; }"
        "QTabWidget::pane { border: 2px solid #b45309; border-radius: 8px; background: #fffbeb; }"
        "QTabBar::tab { background: #fde68a; color: #78350f; padding: 9px 16px; margin-right: 4px; border-top-left-radius: 6px; border-top-right-radius: 6px; font-weight: bold; border: 1px solid #b45309; }"
        "QTabBar::tab:selected { background: #b45309; color: #ffffff; }"
        "QTableWidget, QListWidget { background-color: #ffffff; color: #0f172a; border: 1px solid #cbd5e1; font-size: 13px; }"
        "QHeaderView::section { background-color: #fef3c7; color: #451a03; font-weight: bold; padding: 6px; border: 1px solid #d97706; }"
        );

    setupUi();

    connect(controller, &FarmController::tileChanged, this, &MainWindow::onTileUpdated);
    connect(controller, &FarmController::stateRefreshed, this, &MainWindow::refreshAllViews);
    connect(controller, &FarmController::statusNotification, this, &MainWindow::showNotification);

    refreshAllViews();
    updateInspectorPanel();
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);

    // Top Wooden Tycoon HUD Bar
    QWidget* hudCard = new QWidget();
    hudCard->setStyleSheet("background-color: #78350f; color: #ffffff; border: 2px solid #451a03; border-radius: 8px; padding: 6px;");
    QHBoxLayout* hudLayout = new QHBoxLayout(hudCard);

    lblTreasury = new QLabel();
    lblTreasury->setStyleSheet("font-size: 17px; font-weight: bold; color: #fde047; background: transparent;");

    lblDayWeather = new QLabel();
    lblDayWeather->setStyleSheet("font-size: 14px; font-weight: bold; color: #fef3c7; background: transparent;");

    btnFrontStepTick = new QPushButton("⏭ Advance Day (+1 Tick)");
    btnFrontStepTick->setStyleSheet(
        "QPushButton { background-color: #2563eb; color: #ffffff; border: 2px solid #93c5fd; font-size: 13px; font-weight: bold; padding: 7px 14px; border-radius: 6px; }"
        "QPushButton:hover { background-color: #1d4ed8; }");
    connect(btnFrontStepTick, &QPushButton::clicked, controller, &FarmController::advanceSimulationTick);

    btnFrontAutoSim = new QPushButton("▶ Auto-Run: OFF");
    btnFrontAutoSim->setCheckable(true);
    btnFrontAutoSim->setStyleSheet(
        "QPushButton { background-color: #16a34a; color: #ffffff; border: 2px solid #86efac; font-size: 13px; font-weight: bold; padding: 7px 14px; border-radius: 6px; }"
        "QPushButton:checked { background-color: #d97706; border-color: #fde047; }");
    connect(btnFrontAutoSim, &QPushButton::toggled, this, [this](bool checked) {
        controller->toggleAutoRun(checked, 2000);
    });

    hudLayout->addWidget(lblTreasury);
    hudLayout->addSpacing(20);
    hudLayout->addWidget(lblDayWeather);
    hudLayout->addStretch();
    hudLayout->addWidget(btnFrontStepTick);
    hudLayout->addWidget(btnFrontAutoSim);
    mainLayout->addWidget(hudCard);

    // Status / Error Notification Banner (DP9)
    lblStatusBanner = new QLabel("Welcome to your Gardening Empire! Click any hexagonal plot to inspect, clear, till, or plant.");
    lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #dcfce7; color: #14532d; font-weight: bold; border: 1px solid #86efac; border-radius: 5px;");
    mainLayout->addWidget(lblStatusBanner);

    // Tabs
    QTabWidget* tabs = new QTabWidget(this);
    tabs->addTab(createFarmCanvasTab(), "1. Hexagonal Farm World");
    tabs->addTab(createMarketDashboardTab(), "2. Market & Exchange");
    tabs->addTab(createTownStoreTab(), "3. Town Supply Store");
    tabs->addTab(createSimulationMonitorTab(), "4. Simulation & Persistence HUD");
    tabs->addTab(createPlantIndexTab(), "5. Plant Almanac & Environment Guide");

    mainLayout->addWidget(tabs);
    setCentralWidget(central);
}

QWidget* MainWindow::createFarmCanvasTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    // Left Column: Directional Biome Navigation + Hexagonal QPainter Canvas
    QVBoxLayout* leftCol = new QVBoxLayout();

    QGroupBox* navBox = new QGroupBox("Explore Regional Hex Sectors & Biomes (Use Arrows to Travel)");
    QHBoxLayout* navLayout = new QHBoxLayout(navBox);

    btnNavWest  = new QPushButton("◀ West");
    btnNavNorth = new QPushButton("▲ North");
    btnNavSouth = new QPushButton("▼ South");
    btnNavEast  = new QPushButton("East ▶");

    connect(btnNavWest,  &QPushButton::clicked, this, [this]() { controller->navigateRegion(-1, 0); });
    connect(btnNavEast,  &QPushButton::clicked, this, [this]() { controller->navigateRegion(1, 0); });
    connect(btnNavNorth, &QPushButton::clicked, this, [this]() { controller->navigateRegion(0, -1); });
    connect(btnNavSouth, &QPushButton::clicked, this, [this]() { controller->navigateRegion(0, 1); });

    lblRegionBanner = new QLabel();
    lblRegionBanner->setAlignment(Qt::AlignCenter);

    navLayout->addWidget(btnNavWest);
    navLayout->addWidget(btnNavNorth);
    navLayout->addWidget(lblRegionBanner, 1);
    navLayout->addWidget(btnNavSouth);
    navLayout->addWidget(btnNavEast);
    leftCol->addWidget(navBox);

    // Custom Hexagonal Canvas Widget
    hexCanvas = new HexFarmWidget(controller, this);
    connect(hexCanvas, &HexFarmWidget::tileSelected, this, &MainWindow::onTileClicked);
    leftCol->addWidget(hexCanvas, 1);

    layout->addLayout(leftCol, 3);

    // Right Column: Plot Inspector & Wooden Tool Dock
    QGroupBox* actionGroup = new QGroupBox("Selected Hex Plot Inspector & Tools");
    QVBoxLayout* actLayout = new QVBoxLayout(actionGroup);

    lblInspector = new QLabel("Select a hex plot...");
    lblInspector->setWordWrap(true);
    lblInspector->setStyleSheet("font-size: 13px; padding: 10px; background: #ffffff; color: #0f172a; border: 2px solid #d97706; border-radius: 6px;");
    actLayout->addWidget(lblInspector);

    // UC1: Clear Obstacle
    QHBoxLayout* clearRow = new QHBoxLayout();
    comboClearTool = new QComboBox();
    comboClearTool->addItems({"Axe", "Pickaxe", "Shovel"});
    QPushButton* btnClear = new QPushButton("🪓 Clear Obstacle ($15)");
    connect(btnClear, &QPushButton::clicked, this, [this]() {
        controller->clearTileObstacle(selectedX, selectedY, comboClearTool->currentText().toStdString());
    });
    clearRow->addWidget(comboClearTool);
    clearRow->addWidget(btnClear);
    actLayout->addLayout(clearRow);

    // FR3: Till with Hoe
    QPushButton* btnTill = new QPushButton("⛏ Till Hex Soil with Hoe (FR3)");
    connect(btnTill, &QPushButton::clicked, this, [this]() {
        controller->tillTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnTill);

    // UC3: Plant Seed
    QHBoxLayout* plantRow = new QHBoxLayout();
    comboSeedSelect = new QComboBox();
    comboSeedSelect->addItems({"Carrot", "Wheat", "Corn"});
    QPushButton* btnPlant = new QPushButton("🌱 Plant Seed (UC3)");
    btnPlant->setStyleSheet("background-color: #dcfce7; color: #14532d; border: 2px solid #16a34a;");
    connect(btnPlant, &QPushButton::clicked, this, [this]() {
        controller->plantSeedOnTile(selectedX, selectedY, comboSeedSelect->currentText().toStdString());
    });
    plantRow->addWidget(comboSeedSelect);
    plantRow->addWidget(btnPlant);
    actLayout->addLayout(plantRow);

    // Water & Mulch (FR13)
    QPushButton* btnWater = new QPushButton("💧 Water Plot (+25% Hydration)");
    btnWater->setStyleSheet("background-color: #e0f2fe; color: #0c4a6e; border: 2px solid #0284c7;");
    connect(btnWater, &QPushButton::clicked, this, [this]() {
        controller->waterTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnWater);

    QPushButton* btnMulch = new QPushButton("🌿 Apply Organic Mulch (-50% Evap, FR13)");
    connect(btnMulch, &QPushButton::clicked, this, [this]() {
        controller->applyMulchToTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnMulch);

    // UC4: Harvest Mature Crop
    QPushButton* btnHarvest = new QPushButton("🌾 Harvest Mature Crop (FR6 / UC4)");
    btnHarvest->setStyleSheet("font-weight: bold; background-color: #fef08a; color: #713f12; border: 2px solid #eab308; padding: 8px;");
    connect(btnHarvest, &QPushButton::clicked, this, [this]() {
        controller->harvestTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnHarvest);

    QPushButton* btnClearRot = new QPushButton("🗑 Remove Rotten Crop (FR12)");
    btnClearRot->setStyleSheet("background-color: #fee2e2; color: #7f1d1d; border: 2px solid #ef4444;");
    connect(btnClearRot, &QPushButton::clicked, this, [this]() {
        controller->clearRottenOnTile(selectedX, selectedY);
    });
    actLayout->addWidget(btnClearRot);

    actLayout->addStretch();
    layout->addWidget(actionGroup, 1);
    return tab;
}

QWidget* MainWindow::createMarketDashboardTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* stratGroup = new QGroupBox("Runtime Market Pricing Strategy (FR8 / DP2)");
    QHBoxLayout* stratLayout = new QHBoxLayout(stratGroup);
    lblActivePricing = new QLabel();
    comboStrategy = new QComboBox();
    comboStrategy->addItems({"Standard", "Threshold", "Festival"});
    QPushButton* btnSwapStrategy = new QPushButton("Swap Pricing Policy");
    connect(btnSwapStrategy, &QPushButton::clicked, this, [this]() {
        controller->changePricingStrategy(comboStrategy->currentText().toStdString());
    });
    stratLayout->addWidget(lblActivePricing);
    stratLayout->addStretch();
    stratLayout->addWidget(new QLabel("Select Strategy:"));
    stratLayout->addWidget(comboStrategy);
    stratLayout->addWidget(btnSwapStrategy);
    layout->addWidget(stratGroup);

    lblSaturationInfo = new QLabel();
    lblSaturationInfo->setStyleSheet("padding: 8px; font-weight: bold; background-color: #fef3c7; border-radius: 4px;");
    layout->addWidget(lblSaturationInfo);

    tableMarketBatches = new QTableWidget(0, 4);
    tableMarketBatches->setHorizontalHeaderLabels({"Crop Species", "Quantity", "Harvest Vitality", "Dynamic Market Quote ($)"});
    tableMarketBatches->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableMarketBatches->setSelectionBehavior(QAbstractItemView::SelectRows);
    layout->addWidget(tableMarketBatches);

    QPushButton* btnSellSelected = new QPushButton("💰 Sell Selected Harvest Batch at Market (UC5)");
    btnSellSelected->setStyleSheet("font-size: 14px; font-weight: bold; padding: 10px; background-color: #fef08a; color: #713f12; border: 2px solid #eab308;");
    connect(btnSellSelected, &QPushButton::clicked, this, [this]() {
        int row = tableMarketBatches->currentRow();
        controller->sellHarvestBatch(row);
    });
    layout->addWidget(btnSellSelected);

    return tab;
}

QWidget* MainWindow::createTownStoreTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    lblInventorySummary = new QLabel();
    lblInventorySummary->setStyleSheet("font-size: 14px; padding: 10px; background: #eff6ff; color: #1e3a8a; border: 1px solid #bfdbfe; border-radius: 5px;");
    layout->addWidget(lblInventorySummary);

    QGroupBox* toolsBox = new QGroupBox("Purchase Clearing Tools (FR1 / FR10 Solvency Checked)");
    QHBoxLayout* toolsLayout = new QHBoxLayout(toolsBox);

    QPushButton* btnBuyAxe = new QPushButton("🪓 Buy Clearing Axe ($40.00) — Clears Trees");
    connect(btnBuyAxe, &QPushButton::clicked, this, [this]() { controller->buyTool("Axe", 40.0); });

    QPushButton* btnBuyPick = new QPushButton("⛏ Buy Pickaxe ($50.00) — Clears Boulders");
    connect(btnBuyPick, &QPushButton::clicked, this, [this]() { controller->buyTool("Pickaxe", 50.0); });

    QPushButton* btnBuyShovel = new QPushButton("🪏 Buy Shovel ($35.00) — Clears Ponds");
    connect(btnBuyShovel, &QPushButton::clicked, this, [this]() { controller->buyTool("Shovel", 35.0); });

    toolsLayout->addWidget(btnBuyAxe);
    toolsLayout->addWidget(btnBuyPick);
    toolsLayout->addWidget(btnBuyShovel);
    layout->addWidget(toolsBox);

    QGroupBox* seedsBox = new QGroupBox("Purchase Seed Packets & Soil Amendments (UC6 / FR13)");
    QGridLayout* seedsLayout = new QGridLayout(seedsBox);

    QPushButton* btnBuyCarrot = new QPushButton("🥕 Buy 3x Carrot Seeds ($36.00) — Moisture Lover");
    connect(btnBuyCarrot, &QPushButton::clicked, this, [this]() { controller->buySeed("Carrot", 3); });

    QPushButton* btnBuyWheat = new QPushButton("🌾 Buy 3x Wheat Seeds ($30.00) — Sunlight Lover");
    connect(btnBuyWheat, &QPushButton::clicked, this, [this]() { controller->buySeed("Wheat", 3); });

    QPushButton* btnBuyCorn = new QPushButton("🌽 Buy 3x Corn Seeds ($60.00) — High Value Solar");
    connect(btnBuyCorn, &QPushButton::clicked, this, [this]() { controller->buySeed("Corn", 3); });

    QPushButton* btnBuyMulch = new QPushButton("🌿 Buy 3x Organic Mulch ($25.00) — Cuts Evaporation 50%");
    connect(btnBuyMulch, &QPushButton::clicked, this, [this]() { controller->buyMulch(3, 25.0); });

    seedsLayout->addWidget(btnBuyCarrot, 0, 0);
    seedsLayout->addWidget(btnBuyWheat, 0, 1);
    seedsLayout->addWidget(btnBuyCorn, 1, 0);
    seedsLayout->addWidget(btnBuyMulch, 1, 1);
    layout->addWidget(seedsBox);

    layout->addStretch();
    return tab;
}

QWidget* MainWindow::createSimulationMonitorTab() {
    QWidget* tab = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(tab);

    QGroupBox* simBox = new QGroupBox("Simulation Engine & Biome Controls (UC2 / FR2 / FR11)");
    QHBoxLayout* simLayout = new QHBoxLayout(simBox);

    btnAutoSim = new QPushButton("▶ Start Continuous Simulation (Auto-Tick)");
    btnAutoSim->setCheckable(true);
    btnAutoSim->setStyleSheet("font-weight: bold; padding: 8px; background-color: #dbeafe; color: #1e3a8a;");
    connect(btnAutoSim, &QPushButton::toggled, this, [this](bool checked) {
        controller->toggleAutoRun(checked, 2000);
    });

    QPushButton* btnStepTick = new QPushButton("⏭ Manual Step (+1 Tick)");
    connect(btnStepTick, &QPushButton::clicked, controller, &FarmController::advanceSimulationTick);

    comboBiome = new QComboBox();
    comboBiome->addItems({"Plains", "Desert", "Savanna"});
    QPushButton* btnSetBiome = new QPushButton("Override Current Sector Biome (FR2)");
    connect(btnSetBiome, &QPushButton::clicked, this, [this]() {
        controller->changeBiome(comboBiome->currentText().toStdString());
    });

    simLayout->addWidget(btnAutoSim);
    simLayout->addWidget(btnStepTick);
    simLayout->addStretch();
    simLayout->addWidget(new QLabel("Override Biome:"));
    simLayout->addWidget(comboBiome);
    simLayout->addWidget(btnSetBiome);
    layout->addWidget(simBox);

    QGroupBox* saveBox = new QGroupBox("Session Persistence & Fault Tolerance (UC7 / DP8 / NFR3)");
    QHBoxLayout* saveLayout = new QHBoxLayout(saveBox);

    QPushButton* btnSave = new QPushButton("💾 Save Game to JSON (savegame.json)");
    connect(btnSave, &QPushButton::clicked, this, [this]() {
        controller->saveSession("savegame.json");
    });

    QPushButton* btnLoad = new QPushButton("📂 Load Game from JSON (savegame.json)");
    connect(btnLoad, &QPushButton::clicked, this, [this]() {
        controller->loadSession("savegame.json");
    });

    QPushButton* btnTestCorrupt = new QPushButton("⚠ Simulate Corrupt Save File Load (DP9 #5 / NFR3)");
    btnTestCorrupt->setStyleSheet("background-color: #fee2e2; color: #991b1b; border: 1px solid #f87171;");
    connect(btnTestCorrupt, &QPushButton::clicked, this, [this]() {
        controller->injectCorruptSaveTest("corrupt_save.json");
    });

    saveLayout->addWidget(btnSave);
    saveLayout->addWidget(btnLoad);
    saveLayout->addWidget(btnTestCorrupt);
    layout->addWidget(saveBox);

    layout->addWidget(new QLabel("Simulation & Weather Event Log:"));
    listSimulationLog = new QListWidget();
    layout->addWidget(listSimulationLog);

    return tab;
}

QWidget* MainWindow::createPlantIndexTab() {
    QWidget* tab = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(tab);

    QGroupBox* listGroup = new QGroupBox("Crop Species Index (Click a Plant)");
    QVBoxLayout* listLayout = new QVBoxLayout(listGroup);

    listPlantIndex = new QListWidget();
    listPlantIndex->setStyleSheet(
        "QListWidget { font-size: 15px; font-weight: bold; padding: 6px; }"
        "QListWidget::item { padding: 14px; border-bottom: 1px solid #e2e8f0; border-radius: 6px; margin-bottom: 4px; }"
        "QListWidget::item:selected { background-color: #b45309; color: #ffffff; }"
        "QListWidget::item:hover { background-color: #fef3c7; color: #78350f; }"
        );

    QListWidgetItem* itemCarrot = new QListWidgetItem("🥕 Carrot (Moisture Crop)");
    itemCarrot->setData(Qt::UserRole, "Carrot");
    listPlantIndex->addItem(itemCarrot);

    QListWidgetItem* itemWheat = new QListWidgetItem("🌾 Wheat (Balanced Solar Crop)");
    itemWheat->setData(Qt::UserRole, "Wheat");
    listPlantIndex->addItem(itemWheat);

    QListWidgetItem* itemCorn = new QListWidgetItem("🌽 Corn (High-Solar Cash Crop)");
    itemCorn->setData(Qt::UserRole, "Corn");
    listPlantIndex->addItem(itemCorn);

    listLayout->addWidget(listPlantIndex);

    QLabel* biomeRef = new QLabel(
        "<b>Regional Biome Baselines (FR2):</b><br>"
        "• <b>Plains:</b> 6.0h Solar | 1.0x Evap (-8% H2O/day)<br>"
        "• <b>Savanna:</b> 8.0h Solar | 1.4x Evap (-11.2% H2O/day)<br>"
        "• <b>Desert:</b> 9.5h Solar | 1.8x Evap (-14.4% H2O/day)"
        );
    biomeRef->setStyleSheet("background-color: #fef3c7; padding: 10px; border: 1px solid #d97706; border-radius: 6px; font-size: 12px;");
    listLayout->addWidget(biomeRef);

    layout->addWidget(listGroup, 1);

    QGroupBox* detailGroup = new QGroupBox("Optimal Environment & Cultivation Guide");
    QVBoxLayout* detailLayout = new QVBoxLayout(detailGroup);

    lblPlantGuideDetails = new QLabel();
    lblPlantGuideDetails->setWordWrap(true);
    lblPlantGuideDetails->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    lblPlantGuideDetails->setStyleSheet(
        "background-color: #ffffff; color: #0f172a; padding: 18px; border: 2px solid #d97706; border-radius: 8px; font-size: 14px;"
        );

    detailLayout->addWidget(lblPlantGuideDetails);
    layout->addWidget(detailGroup, 2);

    connect(listPlantIndex, &QListWidget::currentItemChanged, this, [this](QListWidgetItem* current, QListWidgetItem*) {
        if (current) {
            updatePlantIndexDetails(current->data(Qt::UserRole).toString());
        }
    });

    listPlantIndex->setCurrentRow(0);
    return tab;
}

void MainWindow::updatePlantIndexDetails(const QString& species) {
    QString html;
    if (species == "Carrot") {
        html =
            "<h2 style='color: #c2410c; margin-top: 0;'>🥕 Carrot — Moisture-Dependent Root Crop</h2>"
            "<p>Carrots rely on <b>MoistureGrowthPolicy</b>. They thrive in cool, well-hydrated soil and do not require intense sunlight.</p><hr>"
            "<h3 style='color: #15803d;'>🌍 Best Environment & Biome</h3>"
            "<ul>"
            "<li><b>Ideal Biome:</b> <b>Plains Biome</b> (6.0 baseline solar hours, gentle 1.0x evaporation rate).</li>"
            "<li><b>Best Weather:</b> <b>Sunny</b> (6.6h solar) or <b>Rainstorm</b> (4.2h solar + automatic +25% rain hydration).</li>"
            "</ul>"
            "<h3 style='color: #1d4ed8;'>💧 Hydration & Sunlight Thresholds</h3>"
            "<ul>"
            "<li><b>Optimal Soil Moisture (Vitality +6.0 / tick):</b> <b>40% to 85% H2O</b></li>"
            "<li><b>Minimum Moisture to Grow (+35% progress / tick):</b> At least <b>35% H2O</b></li>"
            "<li><b>Minimum Sunlight Required:</b> At least <b>4.0 Solar Hours</b></li>"
            "<li><b>Danger / Stress Zones:</b> Below <b>15% H2O</b> or above <b>92% H2O</b> causes <b>-15.0 Vitality decay</b>!</li>"
            "</ul>";
    } else if (species == "Wheat") {
        html =
            "<h2 style='color: #a16207; margin-top: 0;'>🌾 Wheat — Balanced Sunlight Grain</h2>"
            "<p>Wheat uses <b>SunlightGrowthPolicy</b> with a moderate 5.5-hour solar threshold and grows rapidly (+40% progress/tick).</p><hr>"
            "<h3 style='color: #15803d;'>🌍 Best Environment & Biome</h3>"
            "<ul>"
            "<li><b>Ideal Biomes:</b> <b>Savanna Biome</b> (8.0 baseline solar hours) or <b>Plains Biome</b> (on Sunny days).</li>"
            "</ul>"
            "<h3 style='color: #1d4ed8;'>💧 Hydration & Sunlight Thresholds</h3>"
            "<ul>"
            "<li><b>Optimal Soil Moisture (Vitality +8.0 / tick):</b> <b>25% to 70% H2O</b></li>"
            "<li><b>Minimum Moisture to Grow (+40% progress / tick):</b> At least <b>20% H2O</b></li>"
            "<li><b>Minimum Sunlight Required:</b> At least <b>5.5 Solar Hours</b></li>"
            "</ul>";
    } else if (species == "Corn") {
        html =
            "<h2 style='color: #ca8a04; margin-top: 0;'>🌽 Corn — High-Solar Cash Crop</h2>"
            "<p>Corn is governed by a strict <b>SunlightGrowthPolicy</b> requiring at least <b>7.0 Solar Hours</b> per day.</p><hr>"
            "<h3 style='color: #15803d;'>🌍 Best Environment & Biome</h3>"
            "<ul>"
            "<li><b>Ideal Biomes:</b> <b>Savanna Biome</b> (8.0h base solar) and <b>Desert Biome</b> (9.5h base solar).</li>"
            "<li><b>Why Plains Fails for Corn:</b> Plains only has 6.0 base solar hours (6.6h on Sunny days), which is below Corn's 7.0h threshold!</li>"
            "</ul>"
            "<h3 style='color: #1d4ed8;'>💧 Hydration & Sunlight Thresholds</h3>"
            "<ul>"
            "<li><b>Optimal Soil Moisture (Vitality +8.0 / tick):</b> <b>25% to 70% H2O</b></li>"
            "<li><b>Minimum Sunlight Required:</b> At least <b>7.0 Solar Hours</b></li>"
            "</ul>";
    }
    lblPlantGuideDetails->setText(html);
}

void MainWindow::onTileClicked(int x, int y) {
    selectedX = x;
    selectedY = y;
    hexCanvas->setSelectedTile(x, y);
    updateInspectorPanel();
}

void MainWindow::onTileUpdated(const TileSnapshot& snap) {
    hexCanvas->update();
    if (snap.x == selectedX && snap.y == selectedY) {
        updateInspectorPanel();
    }
}

void MainWindow::updateInspectorPanel() {
    TileSnapshot s = controller->getTileSnapshot(selectedX, selectedY);
    QString info = QString(
                       "<div style='font-size: 14px; font-weight: bold; color: #b45309; margin-bottom: 4px;'>"
                       "★ Selected Hex: (%1, %2) | Sector (%3, %4)</div>")
                       .arg(s.x).arg(s.y).arg(controller->getRegionX()).arg(controller->getRegionY());

    info += QString("<b>Regional Biome:</b> %1<br>").arg(QString::fromStdString(controller->getActiveBiomeName()));
    info += QString("<b>Obstacle:</b> %1<br>").arg(s.obstacle.empty() ? "None (Cleared)" : QString::fromStdString(s.obstacle));
    info += QString("<b>Tilled Status:</b> %1<br>").arg(s.isTilled ? "Yes (Furrowed Soil)" : "No (Untilled Grass)");
    info += QString("<b>Soil Moisture:</b> %1%<br>").arg(s.moisture, 0, 'f', 1);
    info += QString("<b>Organic Mulch:</b> %1<br>").arg(s.isMulched ? "Installed (-50% Evap)" : "None");

    if (s.hasCrop) {
        info += QString("<hr><b>Planted Crop:</b> %1<br>").arg(QString::fromStdString(s.cropSpecies));
        info += QString("<b>Lifecycle Stage:</b> %1<br>").arg(QString::fromStdString(s.cropStage));
        info += QString("<b>Vitality Score:</b> %1 / 100.0").arg(s.cropVitality, 0, 'f', 1);
    } else {
        info += "<hr><i>No crop currently planted on this hex.</i>";
    }
    lblInspector->setText(info);
}

void MainWindow::refreshAllViews() {
    lblTreasury->setText(QString("💰 Treasury: $%1").arg(controller->getBalance(), 0, 'f', 2));
    lblDayWeather->setText(QString("📅 Day %1   |   🌍 Sector (%2,%3): %4   |   ⛅ Weather: %5")
                               .arg(controller->getCurrentDay())
                               .arg(controller->getRegionX())
                               .arg(controller->getRegionY())
                               .arg(QString::fromStdString(controller->getActiveBiomeName()))
                               .arg(QString::fromStdString(controller->getCurrentWeather())));

    bool running = controller->isAutoRunning();
    btnFrontAutoSim->blockSignals(true);
    btnFrontAutoSim->setChecked(running);
    btnFrontAutoSim->setText(running ? "⏸ Auto-Run: ON (Pause)" : "▶ Auto-Run: OFF");
    btnFrontAutoSim->blockSignals(false);

    btnAutoSim->blockSignals(true);
    btnAutoSim->setChecked(running);
    btnAutoSim->setText(running ? "⏸ Pause Continuous Simulation" : "▶ Start Continuous Simulation (Auto-Tick)");
    btnAutoSim->blockSignals(false);

    std::string bName = controller->getActiveBiomeName();
    QString badgeStyle = "background-color: #dcfce7; color: #14532d; border: 1px solid #86efac;";
    if (bName == "Desert") {
        badgeStyle = "background-color: #fef08a; color: #713f12; border: 1px solid #fde047;";
    } else if (bName == "Savanna") {
        badgeStyle = "background-color: #ffedd5; color: #7c2d12; border: 1px solid #fdba74;";
    }

    lblRegionBanner->setText(QString("Hex Sector (%1, %2) — Active Biome: %3")
                                 .arg(controller->getRegionX())
                                 .arg(controller->getRegionY())
                                 .arg(QString::fromStdString(bName)));
    lblRegionBanner->setStyleSheet(QString("font-size: 13px; font-weight: bold; padding: 6px; border-radius: 5px; %1").arg(badgeStyle));

    btnNavWest->setEnabled(controller->getRegionX() > 0);
    btnNavEast->setEnabled(controller->getRegionX() < controller->getWorldWidth() - 1);
    btnNavNorth->setEnabled(controller->getRegionY() > 0);
    btnNavSouth->setEnabled(controller->getRegionY() < controller->getWorldHeight() - 1);

    hexCanvas->update();
    updateInspectorPanel();

    const Market& mkt = controller->getMarket();
    lblActivePricing->setText(QString("<b>Active Pricing Strategy:</b> %1").arg(QString::fromStdString(mkt.getStrategyName())));
    lblSaturationInfo->setText(QString("Market Saturation Volumes — Carrot: %1 | Wheat: %2 | Corn: %3")
                                   .arg(mkt.getSaturation("Carrot"))
                                   .arg(mkt.getSaturation("Wheat"))
                                   .arg(mkt.getSaturation("Corn")));

    const auto& batches = controller->getInventory().getBatches();
    tableMarketBatches->setRowCount(static_cast<int>(batches.size()));
    for (size_t i = 0; i < batches.size(); ++i) {
        tableMarketBatches->setItem(i, 0, new QTableWidgetItem(QString::fromStdString(batches[i].getSpecies())));
        tableMarketBatches->setItem(i, 1, new QTableWidgetItem(QString::number(batches[i].getQuantity())));
        tableMarketBatches->setItem(i, 2, new QTableWidgetItem(QString("%1%").arg(batches[i].getVitality(), 0, 'f', 1)));
        double quote = mkt.quotePrice(batches[i]);
        tableMarketBatches->setItem(i, 3, new QTableWidgetItem(QString("$%1").arg(quote, 0, 'f', 2)));
    }

    const Inventory& inv = controller->getInventory();
    QString toolsList;
    for (const auto& t : inv.getTools()) toolsList += QString::fromStdString(t) + "  ";
    lblInventorySummary->setText(
        QString("<b>Owned Tools:</b> %1 | <b>Organic Mulch:</b> %2<br>"
                "<b>Seed Stock:</b> Carrot: %3 | Wheat: %4 | Corn: %5")
            .arg(toolsList)
            .arg(inv.getMulchCount())
            .arg(inv.getSeedCount("Carrot"))
            .arg(inv.getSeedCount("Wheat"))
            .arg(inv.getSeedCount("Corn"))
        );
}

void MainWindow::showNotification(const QString& msg, bool isError) {
    lblStatusBanner->setText(msg);
    if (isError) {
        lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #fee2e2; color: #991b1b; font-weight: bold; border: 1px solid #f87171; border-radius: 5px;");
    } else {
        lblStatusBanner->setStyleSheet("padding: 8px 12px; background-color: #dcfce7; color: #14532d; font-weight: bold; border: 1px solid #86efac; border-radius: 5px;");
    }
    listSimulationLog->insertItem(0, QString("[Day %1] %2").arg(controller->getCurrentDay()).arg(msg));
}