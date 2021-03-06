#include "Legend2D.h"

#include "LineSpecial2D.h"
#include "future/lib/XmlStreamReader.h"
#include "future/lib/XmlStreamWriter.h"

Legend2D::Legend2D(AxisRect2D *axisrect)
    : QCPLegend(),
      axisrect_(axisrect),
      draggingLegend_(false),
      cursorshape_(axisrect->getParentPlot2D()->cursor().shape()) {}

Legend2D::~Legend2D() {}

bool Legend2D::gethidden_legend() const { return visible(); }

QColor Legend2D::getborderstrokecolor_legend() const {
  return borderPen().color();
}

double Legend2D::getborderstrokethickness_legend() const {
  return borderPen().widthF();
}

Qt::PenStyle Legend2D::getborderstrokestyle_legend() const {
  return borderPen().style();
}

QPointF Legend2D::getposition_legend() const {
  return axisrect_->insetLayout()->insetRect(0).topLeft();
}

AxisRect2D *Legend2D::getaxisrect_legend() const { return axisrect_; }

void Legend2D::sethidden_legend(const bool status) { setVisible(status); }

void Legend2D::setborderstrokecolor_legend(const QColor &color) {
  QPen p = borderPen();
  p.setColor(color);
  setBorderPen(p);
}

void Legend2D::setborderstrokethickness_legend(const double value) {
  QPen p = borderPen();
  p.setWidthF(value);
  setBorderPen(p);
}

void Legend2D::setborderstrokestyle_legend(const Qt::PenStyle &style) {
  QPen p = borderPen();
  p.setStyle(style);
  setBorderPen(p);
}

void Legend2D::setposition_legend(QPointF origin) {
  QRectF rect = axisrect_->insetLayout()->insetRect(0);
  rect.setTopLeft(origin);
  axisrect_->insetLayout()->setInsetRect(0, rect);
}

void Legend2D::save(XmlStreamWriter *xmlwriter) {
  xmlwriter->writeStartElement("legend");
  xmlwriter->writeAttribute(
      "x",
      QString::number(axisrect_->insetLayout()->insetRect(0).topLeft().x()));
  xmlwriter->writeAttribute(
      "y",
      QString::number(axisrect_->insetLayout()->insetRect(0).topLeft().y()));
  xmlwriter->writeAttribute(
      "width", QString::number(axisrect_->insetLayout()->insetRect(0).width()));
  xmlwriter->writeAttribute(
      "height",
      QString::number(axisrect_->insetLayout()->insetRect(0).height()));

  (gethidden_legend()) ? xmlwriter->writeAttribute("visible", "false")
                       : xmlwriter->writeAttribute("visible", "true");
  xmlwriter->writeAttribute("iconwidth", QString::number(iconSize().width()));
  xmlwriter->writeAttribute("iconheight", QString::number(iconSize().height()));
  xmlwriter->writeAttribute("icontextpadding",
                            QString::number(iconTextPadding()));
  xmlwriter->writeFont(font(), textColor());
  xmlwriter->writePen(borderPen());
  xmlwriter->writeBrush(brush());
  xmlwriter->writeEndElement();
}

bool Legend2D::load(XmlStreamReader *xmlreader) {
  if (xmlreader->isStartElement() && xmlreader->name() == "legend") {
    bool ok;
    double orix = xmlreader->readAttributeDouble("x", &ok);
    if (ok) {
      double oriy = xmlreader->readAttributeDouble("y", &ok);
      if (ok) {
        double width = xmlreader->readAttributeDouble("width", &ok);
        if (ok) {
          double height = xmlreader->readAttributeDouble("height", &ok);
          if (ok) {
            QRectF rect;
            rect.setTopLeft(QPointF(orix, oriy));
            rect.setHeight(height);
            rect.setWidth(width);
            axisrect_->insetLayout()->setInsetRect(0, rect);
          }
        }
      }
    }

    bool visible = xmlreader->readAttributeBool("visible", &ok);
    (ok) ? sethidden_legend(!visible)
         : xmlreader->raiseError(
               tr("no Legend2D visible property element found"));
    int iconwidth = xmlreader->readAttributeInt("iconwidth", &ok);
    if (ok) {
      QSize size = iconSize();
      size.setWidth(iconwidth);
      setIconSize(size);
    } else
      xmlreader->raiseError(
          tr("no Legend2D icon width property element found"));
    int iconheight = xmlreader->readAttributeInt("iconheight", &ok);
    if (ok) {
      QSize size = iconSize();
      size.setHeight(iconheight);
      setIconSize(size);
    } else
      xmlreader->raiseError(
          tr("no Legend2D icon height property element found"));
    int icontextpadding = xmlreader->readAttributeInt("icontextpadding", &ok);
    (ok) ? setIconTextPadding(icontextpadding)
         : xmlreader->raiseError(
               tr("no Legend2D icon text padding property element found"));
    // font
    while (!xmlreader->atEnd()) {
      xmlreader->readNext();
      if (xmlreader->isEndElement() && xmlreader->name() == "font") break;
      // font
      if (xmlreader->isStartElement() && xmlreader->name() == "font") {
        QPair<QFont, QColor> fontpair = xmlreader->readFont(&ok);
        if (ok) {
          setFont(fontpair.first);
          setTextColor(fontpair.second);
        } else
          xmlreader->raiseWarning(tr("Legend2D font property setting error"));
      }
    }
    // pen
    while (!xmlreader->atEnd()) {
      xmlreader->readNext();
      if (xmlreader->isEndElement() && xmlreader->name() == "pen") break;
      // pen
      if (xmlreader->isStartElement() && xmlreader->name() == "pen") {
        QPen basep = xmlreader->readPen(&ok);
        if (ok)
          setBorderPen(basep);
        else
          xmlreader->raiseWarning(
              tr("Legend2D borderpen property setting error"));
      }
    }
    // brush
    while (!xmlreader->atEnd()) {
      xmlreader->readNext();
      if (xmlreader->isEndElement() && xmlreader->name() == "brush") break;
      if (xmlreader->isStartElement() && xmlreader->name() == "brush") {
        QBrush brush = xmlreader->readBrush(&ok);
        if (ok)
          setBrush(brush);
        else
          xmlreader->raiseWarning(tr("Legend2D brush property setting error"));
      }
    }
  } else  // no element
    xmlreader->raiseError(tr("no Legend2D item element found"));

  return !xmlreader->hasError();
}

void Legend2D::mousePressEvent(QMouseEvent *event, const QVariant &details) {
  Q_UNUSED(details);
  emit legendClicked();
  if (event->button() == Qt::LeftButton) {
    if (selectTest(event->pos(), false) > 0) {
      draggingLegend_ = true;
      // since insetRect is in axisRect coordinates (0..1), we transform the
      // mouse position:
      QPointF mousePoint((event->pos().x() - axisrect_->left()) /
                             static_cast<double>(axisrect_->width()),
                         (event->pos().y() - axisrect_->top()) /
                             static_cast<double>(axisrect_->height()));
      dragLegendOrigin_ =
          mousePoint - axisrect_->insetLayout()->insetRect(0).topLeft();
      cursorshape_ = axisrect_->getParentPlot2D()->cursor().shape();
      axisrect_->getParentPlot2D()->setCursor(Qt::ClosedHandCursor);
    }
  }
}

void Legend2D::mouseMoveEvent(QMouseEvent *event, const QPointF &startPos) {
  Q_UNUSED(startPos);
  // dragging legend
  if (draggingLegend_) {
    QRectF rect = axisrect_->insetLayout()->insetRect(0);
    // QRect r = rect();

    // set bounding rect for drag event
    QPoint eventpos = event->pos();
    if (event->pos().x() < axisrect_->left()) {
      eventpos.setX(axisrect_->left());
    }
    if (event->pos().x() > axisrect_->right()) {
      eventpos.setX(axisrect_->right());
    }
    if (event->pos().y() < axisrect_->top()) {
      eventpos.setY(axisrect_->top());
    }
    if (event->pos().y() > axisrect_->bottom()) {
      eventpos.setY(axisrect_->bottom());
    }

    // since insetRect is in axisRect coordinates (0..1), we transform the mouse
    // position:
    QPointF mousePoint((eventpos.x() - axisrect_->left()) /
                           static_cast<double>(axisrect_->width()),
                       (eventpos.y() - axisrect_->top()) /
                           static_cast<double>(axisrect_->height()));

    rect.moveTopLeft(mousePoint - dragLegendOrigin_);
    axisrect_->insetLayout()->setInsetRect(0, rect);
    layer()->replot();
    event->accept();
  }
}

void Legend2D::mouseReleaseEvent(QMouseEvent *event, const QPointF &startPos) {
  Q_UNUSED(startPos);
  if (event->button() == Qt::LeftButton) {
    if (draggingLegend_) {
      draggingLegend_ = false;
      axisrect_->getParentPlot2D()->setCursor(cursorshape_);
      emit legendMoved();
    }
  }
}

LegendItem2D::LegendItem2D(Legend2D *legend, QCPAbstractPlottable *plottable)
    : QCPPlottableLegendItem(legend, plottable) {
  setLayer(legend->layer());
}

LegendItem2D::~LegendItem2D() {}

void LegendItem2D::mousePressEvent(QMouseEvent *event,
                                   const QVariant &details) {
  emit legendItemClicked();
  QCPPlottableLegendItem::mousePressEvent(event, details);
}

SplineLegendItem2D::SplineLegendItem2D(Legend2D *legend, Curve2D *plottable)
    : LegendItem2D(legend, plottable),
      isspline_(false),
      curve_(plottable),
      legend_(legend) {
  isspline_ = (plottable->getcurvetype_cplot() == Curve2D::Curve2DType::Spline)
                  ? isspline_ = true
                  : isspline_ = false;
}

SplineLegendItem2D::~SplineLegendItem2D() {}

void SplineLegendItem2D::draw(QCPPainter *painter) {
  if (isspline_) {
    QPen mpen = curve_->getSplinePen();
    QBrush mbrush = curve_->getSplineBrush();
    painter->setPen(mpen);
    painter->setBrush(mbrush);
    QPainterPath path;
    QRectF icrect = QRectF(rect().topLeft(), legend_->iconSize());
    double diff = (icrect.bottomLeft().y() - icrect.topLeft().y()) / 2;
    QPointF point1 = QPointF(icrect.topLeft().x(), icrect.topLeft().y() + diff);
    QPointF point2 =
        QPointF(icrect.topRight().x(), icrect.topRight().y() + diff);
    double diff2 = (icrect.bottomRight().x() - icrect.bottomLeft().x()) / 4;
    QPointF ctrl1 = QPointF(icrect.topLeft().x() + diff2, icrect.topLeft().y());
    QPointF ctrl2 =
        QPointF(icrect.topRight().x() - diff2, icrect.bottomLeft().y());
    path.moveTo(point1);
    path.cubicTo(ctrl1, ctrl2, point2);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->drawPath(path);
  }
  LegendItem2D::draw(painter);
}

VectorLegendItem2D::VectorLegendItem2D(Legend2D *legend, Vector2D *plottable)
    : LegendItem2D(legend, plottable),
      vector_(plottable),
      legend_(legend),
      // lineitem_(new QCPItemLine(parentPlot())),
      start_(new QCPLineEnding()),
      stop_(new QCPLineEnding()) {}

VectorLegendItem2D::~VectorLegendItem2D() {
  // parentPlot()->removeItem(lineitem_);
  delete start_;
  delete stop_;
}

void VectorLegendItem2D::draw(QCPPainter *painter) {
  LegendItem2D::draw(painter);
  /*painter->setFont(getFont());
  painter->setPen(QPen(getTextColor()));
  QSizeF iconSize = mParentLegend->iconSize();
  QRectF textRect = painter->fontMetrics().boundingRect(
      0, 0, 0, static_cast<int>(iconSize.height()), Qt::TextDontClip,
      vector_->name());
  QRectF iconRect(mRect.topLeft(), iconSize);
  // if text has smaller height than icon, center text
  // vertically in icon height, else align tops
  int textHeight = qMax(static_cast<int>(textRect.height()),
                        static_cast<int>(iconSize.height()));
  painter->drawText(static_cast<int>(mRect.x() + iconSize.width() +
                                     mParentLegend->iconTextPadding()),
                    mRect.y(), static_cast<int>(textRect.width()), textHeight,
                    Qt::TextDontClip, vector_->name());
  // draw icon:
  painter->save();
  painter->setClipRect(iconRect, Qt::IntersectClip);
  painter->restore();
  // draw icon border:
  painter->setPen(getIconBorderPen());
  int halfPen = qCeil(painter->pen().widthF() * 0.5) + 1;
  // extend default clip rect so thicker pens (especially
  // during selection) are not clipped
  double diff = (iconRect.bottomLeft().y() - iconRect.topLeft().y()) / 2;
  lineitem_->start->setPixelPosition(iconRect.topLeft() + QPointF(0, diff));
  lineitem_->end->setPixelPosition(iconRect.topRight() + QPointF(0, diff));
  painter->setClipRect(
      mOuterRect.adjusted(-halfPen, -halfPen, halfPen, halfPen));
  painter->drawRect(iconRect);*/
}

void VectorLegendItem2D::setLineEndings() {
  lineitem_->setPen(vector_->pen());
  switch (vector_->getendstyle_vecplot(Vector2D::LineEndLocation::Start)) {
    case Vector2D::LineEnd::None:
      start_->setStyle(QCPLineEnding::esNone);
      break;
    case Vector2D::LineEnd::FlatArrow:
      start_->setStyle(QCPLineEnding::esFlatArrow);
      break;
    case Vector2D::LineEnd::SpikeArrow:
      start_->setStyle(QCPLineEnding::esSpikeArrow);
      break;
    case Vector2D::LineEnd::LineArrow:
      start_->setStyle(QCPLineEnding::esLineArrow);
      break;
    case Vector2D::LineEnd::Disc:
      start_->setStyle(QCPLineEnding::esDisc);
      break;
    case Vector2D::LineEnd::Square:
      start_->setStyle(QCPLineEnding::esSquare);
      break;
    case Vector2D::LineEnd::Diamond:
      start_->setStyle(QCPLineEnding::esDiamond);
      break;
    case Vector2D::LineEnd::Bar:
      start_->setStyle(QCPLineEnding::esBar);
      break;
    case Vector2D::LineEnd::HalfBar:
      start_->setStyle(QCPLineEnding::esHalfBar);
      break;
    case Vector2D::LineEnd::SkewedBar:
      start_->setStyle(QCPLineEnding::esSkewedBar);
      break;
  }
  switch (vector_->getendstyle_vecplot(Vector2D::LineEndLocation::Stop)) {
    case Vector2D::LineEnd::None:
      stop_->setStyle(QCPLineEnding::esNone);
      break;
    case Vector2D::LineEnd::FlatArrow:
      stop_->setStyle(QCPLineEnding::esFlatArrow);
      break;
    case Vector2D::LineEnd::SpikeArrow:
      stop_->setStyle(QCPLineEnding::esSpikeArrow);
      break;
    case Vector2D::LineEnd::LineArrow:
      stop_->setStyle(QCPLineEnding::esLineArrow);
      break;
    case Vector2D::LineEnd::Disc:
      stop_->setStyle(QCPLineEnding::esDisc);
      break;
    case Vector2D::LineEnd::Square:
      stop_->setStyle(QCPLineEnding::esSquare);
      break;
    case Vector2D::LineEnd::Diamond:
      stop_->setStyle(QCPLineEnding::esDiamond);
      break;
    case Vector2D::LineEnd::Bar:
      stop_->setStyle(QCPLineEnding::esBar);
      break;
    case Vector2D::LineEnd::HalfBar:
      stop_->setStyle(QCPLineEnding::esHalfBar);
      break;
    case Vector2D::LineEnd::SkewedBar:
      stop_->setStyle(QCPLineEnding::esSkewedBar);
      break;
  }
  lineitem_->setHead(*start_);
  lineitem_->setTail(*stop_);
}

PieLegendItem2D::PieLegendItem2D(Legend2D *parent, QColor color,
                                 const QString &string)
    : QCPAbstractLegendItem(parent), color_(color), string_(string) {
  setLayer(parent->layer());
  setAntialiased(false);
}

void PieLegendItem2D::draw(QCPPainter *painter) {
  painter->setFont(getFont());
  painter->setPen(QPen(getTextColor()));
  QSizeF iconSize = mParentLegend->iconSize();
  QRectF textRect = painter->fontMetrics().boundingRect(
      0, 0, 0, static_cast<int>(iconSize.height()), Qt::TextDontClip, string_);
  QRectF iconRect(mRect.topLeft(), iconSize);
  // if text has smaller height than icon, center text
  // vertically in icon height, else align tops
  int textHeight = qMax(static_cast<int>(textRect.height()),
                        static_cast<int>(iconSize.height()));
  painter->drawText(static_cast<int>(mRect.x() + iconSize.width() +
                                     mParentLegend->iconTextPadding()),
                    mRect.y(), static_cast<int>(textRect.width()), textHeight,
                    Qt::TextDontClip, string_);
  // draw icon:
  painter->save();
  painter->setClipRect(iconRect, Qt::IntersectClip);
  painter->restore();
  // draw icon border:
  painter->setPen(getIconBorderPen());
  int halfPen = qCeil(painter->pen().widthF() * 0.5) + 1;
  // extend default clip rect so thicker pens (especially
  // during selection) are not clipped
  painter->setClipRect(
      mOuterRect.adjusted(-halfPen, -halfPen, halfPen, halfPen));
  painter->setBrush(QBrush(color_));
  painter->drawRect(iconRect);
}

QSize PieLegendItem2D::minimumOuterSizeHint() const {
  QSize result(0, 0);
  QRect textRect;
  QFontMetrics fontMetrics(getFont());
  QSize iconSize = mParentLegend->iconSize();
  textRect = fontMetrics.boundingRect(0, 0, 0, iconSize.height(),
                                      Qt::TextDontClip, string_);
  result.setWidth(iconSize.width() + mParentLegend->iconTextPadding() +
                  textRect.width());
  result.setHeight(qMax(textRect.height(), iconSize.height()));
  result.rwidth() += mMargins.left() + mMargins.right();
  result.rheight() += mMargins.top() + mMargins.bottom();
  return result;
}

QPen PieLegendItem2D::getIconBorderPen() const {
  return mSelected ? mParentLegend->iconBorderPen()
                   : mParentLegend->iconBorderPen();
}

QColor PieLegendItem2D::getTextColor() const {
  return mSelected ? mSelectedTextColor : mTextColor;
}

QFont PieLegendItem2D::getFont() const {
  return mSelected ? mSelectedFont : mFont;
}
