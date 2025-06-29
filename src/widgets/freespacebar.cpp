/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <utility>

#include <QtGlobal>
#include <QWidget>
#include <QList>
#include <QString>
#include <QImage>
#include <QFont>
#include <QFontMetrics>
#include <QPainter>
#include <QPainterPath>
#include <QPalette>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QTransform>
#include <QLinearGradient>

#include "utilities/strutils.h"
#include "freespacebar.h"

using namespace Qt::Literals::StringLiterals;

class QPaintEvent;

namespace {
constexpr int kBarHeight = 20;
constexpr int kBarBorderRadius = 8;
constexpr int kMarkerSpacing = 32;
constexpr int kLabelBoxSize = 12;
constexpr int kLabelBoxPadding = 4;
constexpr int kLabelSpacing = 16;

constexpr QRgb kColorBg1 = qRgb(214, 207, 200);
constexpr QRgb kColorBg2 = qRgb(234, 226, 218);
constexpr QRgb kColorAdd1 = qRgb(136, 180, 229);
constexpr QRgb kColorAdd2 = qRgb(72, 146, 229);
constexpr QRgb kColorBar1 = qRgb(250, 148, 76);
constexpr QRgb kColorBar2 = qRgb(214, 102, 24);
constexpr QRgb kColorBorder = qRgb(174, 168, 162);

}  // namespace

FreeSpaceBar::FreeSpaceBar(QWidget *parent)
    : QWidget(parent),
      free_(100),
      additional_(0),
      total_(100),
      free_text_(tr("Available")),
      additional_text_(tr("New songs")),
      exceeded_text_(tr("Exceeded by")),
      used_text_(tr("Used")) {

  setMinimumHeight(FreeSpaceBar::sizeHint().height());

}

QSize FreeSpaceBar::sizeHint() const {
  return QSize(150, kBarHeight + kLabelBoxPadding + fontMetrics().height());
}

void FreeSpaceBar::paintEvent(QPaintEvent *e) {

  Q_UNUSED(e)

  // Geometry
  QRect bar_rect(rect());
  bar_rect.setHeight(kBarHeight);

  QRect reflection_rect(bar_rect);
  reflection_rect.moveTop(reflection_rect.bottom());

  QRect labels_rect(rect());
  labels_rect.setTop(labels_rect.top() + kBarHeight + kLabelBoxPadding);

  // Draw the reflection
  // Create the reflected pixmap
  QImage reflection(reflection_rect.size(), QImage::Format_ARGB32_Premultiplied);
  reflection.fill(palette().color(QPalette::Window).rgba());
  QPainter p(&reflection);

  // Set up the transformation
  QTransform transform;
  transform.scale(1.0, -1.0);
  transform.translate(0.0, -reflection.height());
  p.setTransform(transform);

  // Draw the bar
  DrawBar(&p, QRect(QPoint(0, 0), reflection.size()));

  // Make it fade out towards the bottom
  QLinearGradient fade_gradient(reflection.rect().topLeft(), reflection.rect().bottomLeft());
  fade_gradient.setColorAt(0.0, QColor(0, 0, 0, 0));
  fade_gradient.setColorAt(1.0, QColor(0, 0, 0, 128));

  p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
  p.fillRect(reflection.rect(), fade_gradient);

  p.end();

  // Draw on the widget
  p.begin(this);
  DrawBar(&p, bar_rect);
  p.drawImage(reflection_rect, reflection);
  DrawText(&p, labels_rect);

}

void FreeSpaceBar::DrawBar(QPainter *p, const QRect r) {

  p->setRenderHint(QPainter::Antialiasing, true);

  QRect bar_rect(r);
  bar_rect.setWidth(std::max(0, static_cast<int>(static_cast<float>(bar_rect.width()) * (static_cast<float>(total_ - free_) / static_cast<float>(total_)))));

  QLinearGradient background_gradient(r.topLeft(), r.bottomLeft());
  background_gradient.setColorAt(0, kColorBg1);
  background_gradient.setColorAt(1, kColorBg2);

  QLinearGradient bar_gradient(bar_rect.topLeft(), bar_rect.bottomLeft());
  bar_gradient.setColorAt(0, kColorBar1);
  bar_gradient.setColorAt(1, kColorBar2);

  // Draw the background
  p->setPen(Qt::NoPen);
  p->setBrush(background_gradient);
  p->drawRoundedRect(r, kBarBorderRadius, kBarBorderRadius);

  // Create a path to use for clipping the bars
  QPainterPath clip_path;
  clip_path.addRoundedRect(r, kBarBorderRadius, kBarBorderRadius);
  p->setClipPath(clip_path);

  // Draw any additional space
  if (additional_ > 0) {
    QRect additional_rect(bar_rect);
    additional_rect.setLeft(bar_rect.right());
    additional_rect.setWidth(static_cast<int>(static_cast<float>(r.width()) * (static_cast<float>(qMin(free_, additional_)) / static_cast<float>(total_)) + 1.0F));

    QLinearGradient additional_gradient(additional_rect.topLeft(), additional_rect.bottomLeft());
    additional_gradient.setColorAt(0, kColorAdd1);
    additional_gradient.setColorAt(1, kColorAdd2);

    p->fillRect(additional_rect, additional_gradient);
  }

  // Draw the bar foreground
  p->fillRect(bar_rect, bar_gradient);

  // Draw a border
  p->setClipping(false);
  p->setPen(kColorBorder);
  p->setBrush(Qt::NoBrush);
  p->drawRoundedRect(r, kBarBorderRadius, kBarBorderRadius);

  // Draw marker lines over the top every few pixels
  p->setOpacity(0.35);
  p->setRenderHint(QPainter::Antialiasing, false);
  p->setPen(QPen(palette().color(QPalette::Light), 1.0));
  for (int x = r.left() + kMarkerSpacing; x < r.right(); x += kMarkerSpacing) {
    p->drawLine(x, r.top() + 2, x, r.bottom() - 2);
  }

  p->setOpacity(1.0);

}

void FreeSpaceBar::DrawText(QPainter *p, const QRect r) {

  QFont small_font(font());
  small_font.setPointSize(small_font.pointSize() - 1);
  small_font.setBold(true);
  QFontMetrics small_metrics(small_font);
  p->setFont(small_font);

  // Work out the geometry for the text
  QList<Label> labels;
  labels << Label(TextForSize(used_text_, total_ - free_), kColorBar1);
  if (additional_ > 0) {
    labels << Label(TextForSize(additional_text_, additional_), kColorAdd1);
  }
  if (free_ > additional_ || additional_ == 0) {
    labels << Label(TextForSize(free_text_, free_ - additional_), kColorBg2);
  }
  else {
    labels << Label(TextForSize(exceeded_text_, additional_ - free_), kColorBar2);
  }

  int text_width = 0;
  for (const Label &label : std::as_const(labels)) {
    text_width += kLabelBoxSize + kLabelBoxPadding + kLabelSpacing + small_metrics.horizontalAdvance(label.text);
  }

  // Draw the text
  int x = (r.width() - text_width) / 2;

  p->setRenderHint(QPainter::Antialiasing, false);
  for (const Label &label : std::as_const(labels)) {
    const bool light = palette().color(QPalette::Base).value() > 128;

    QRect box(x, r.top() + (r.height() - kLabelBoxSize) / 2, kLabelBoxSize, kLabelBoxSize);
    p->setPen(label.color.darker());
    p->setBrush(label.color);
    p->drawRect(box);

    QRect text(x + kLabelBoxSize + kLabelBoxPadding, r.top(), small_metrics.horizontalAdvance(label.text), r.height());
    p->setPen(light ? label.color.darker() : label.color);
    p->drawText(text, Qt::AlignCenter, label.text);
    x += kLabelBoxSize + kLabelBoxPadding + kLabelSpacing + small_metrics.horizontalAdvance(label.text);
  }

}

QString FreeSpaceBar::TextForSize(const QString &prefix, const quint64 size) {

  QString ret;
  if (size > 0) {
    ret = Utilities::PrettySize(size);
  }
  else {
    ret = u"0 MB"_s;
  }

  if (!prefix.isEmpty()) ret.prepend(prefix + QLatin1Char(' '));

  return ret;

}
