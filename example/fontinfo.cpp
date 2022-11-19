#include "fontinfo.h"
#include <QRegularExpression>
#include <QDebug>

FontInfo::FontInfo(QObject *parent) : QObject(parent)
{
    qDebug() << "FontInfo created!";
}

QString FontInfo::name() const
{
    return m_name;
}

QString FontInfo::family()
{
    if (!m_localizedFamily.isEmpty()) {
        return m_localizedFamily;
    }
    m_localizedFamily = m_family;
    if (m_useJapaneseFont && m_family.endsWith(QStringLiteral("_global"))) {
        m_localizedFamily.replace(QRegularExpression(QStringLiteral("_global$")), QStringLiteral("_jp"));
    }
    return m_localizedFamily;
}

int FontInfo::size() const
{
    return m_size;
}

void FontInfo::setName(const QString& name)
{
    qDebug() << "setName called" << name;
    if (m_name != name) {
        m_name = name;
        emit nameChanged();
    }
}

void FontInfo::setFamily(const QString& family)
{
    qDebug() << "setFamily called" << family;
    if (m_family != family) {
        m_family = family;
        m_localizedFamily.clear();
        emit familyChanged();
    }
}

void FontInfo::setSize(int size)
{
    qDebug() << "setSize called" << size;
    if (m_size != size) {
        m_size = size;
        emit sizeChanged();
    }
}

void FontInfo::setUseJapaneseFont(bool newValue)
{
    m_useJapaneseFont = newValue;
    m_localizedFamily = QString();
    emit familyChanged();
}

bool FontInfo::useJapaneseFont() const
{
    return m_useJapaneseFont;
}
